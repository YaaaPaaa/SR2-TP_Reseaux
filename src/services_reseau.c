/********************************************************************
 *         Simple abstraction of a network layer library            *
 *         (and some other useful tools handling timers)            *
 *                                                                  *
 * v2.4, Sept. 2024 - E. Lavinal                                    *
 * v1.7, Sept. 2022, Sept 2023 - E. Lavinal                         *
 * v1.1, Jan. 2020 - P. Torguet                                     *
 *                                                                  *
 * Département d'Informatique                                       *
 * Faculté Sciences et Ingénierie                                   *
 * Université Toulouse III - Paul Sabatier                          *
 ********************************************************************/

#include "config.h"
#include "services_reseau.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <time.h>

#define RED "\x1B[31m"
#define NRM "\x1B[0m"

#define SENDER 0
#define RECEIVER 1

// Static ports for sender and receiver
// (we could generate ports using user id if we were one a shared server)
#define SENDER_PORT 42525
#define RECEIVER_PORT 42526

#define MAX_TIMERS 32

#define MAX_PERF_ARRAY 10000

/* Internal variables */
/* ------------------ */

/* Network layer conf (loss, error...) */
// by default, init all conf params to 0
static netlib_config_t nl_conf = { 0 };

/* Socket */
static int sock;

/* Communication role (sender or receiver) */
static int my_role = 0;

/* Timers */
typedef struct my_timer_t
{
    int num_timer;
    int exp;
} my_timer_t;

static my_timer_t timers[MAX_TIMERS];
static int num_timers = 0;

/* Init state */
static int net_initialized = 0;

/* Remote host (string IPv4 addr)*/
static char remote_ipv4[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN == 16 bytes

/* Variables to plot performance */
static pthread_t perf_thid;
static int sent_packet_count = 0;
static int packet_loss_count = 0;
static int end_communication = 0;

/* Last data packet bool used for last ACK loss */
static int last_data_pkt = 0;

/* ========================================================================= */
/* ========================================================================= */

// Perf eval thread to plot sender's perf
void *perf_eval_thread(void *args) {

    struct telemetry_point {
        short packet_count;
        short loss_count;
    };
    struct telemetry_point perf_array[MAX_PERF_ARRAY];
    int number_of_points = 0;
    int sent_packet_count_prev = 0;
    int packet_loss_count_prev = 0;

    while (!end_communication) {

        usleep(nl_conf.plot_period_ms * 1000);
        perf_array[number_of_points].packet_count = sent_packet_count - sent_packet_count_prev;
        perf_array[number_of_points].loss_count = packet_loss_count - packet_loss_count_prev;
        sent_packet_count_prev = sent_packet_count;
        packet_loss_count_prev = packet_loss_count;
        number_of_points++;
    }

    FILE *perf_file = fopen("perf.txt", "wt");
    if (perf_file == NULL) {
        perror("[NET] Error creating file: ");
        exit(1);
    }
    fprintf(perf_file, "Time; Packet; Loss\n");
    for (int i=0; i<number_of_points; i++)
        fprintf(perf_file, "%d; %d; %d\n", (i + 1) * nl_conf.plot_period_ms, perf_array[i].packet_count, perf_array[i].loss_count);
    fclose(perf_file);
    printf("[NET] Performance trace written in perf.txt!\n");

    pthread_exit(NULL);
}

/* ========================================================================= */
/* ========================================================================= */

int local_port() {

    int port = (my_role==SENDER)?SENDER_PORT:RECEIVER_PORT;
    return port;
}

int remote_port() {

    int port = (my_role==SENDER)?RECEIVER_PORT:SENDER_PORT;
    return port;
}

void init_network(int role, char *remote_host) {

    // communication role
    my_role = role;
    // network layer conf params (loss, error...)
    if (my_role == SENDER) {
        conf_net_sender(&nl_conf);
        if (nl_conf.plot_period_ms != 0)
            // create a new thread for performance evaluation
            pthread_create(&perf_thid, NULL, &perf_eval_thread, NULL);
    }
    else
        conf_net_receiver(&nl_conf);
    // rand
    srand((unsigned)time(NULL));
    // socket
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket() error: ");
        exit(1);
    }
    struct sockaddr_in local_addr;
    local_addr.sin_port = htons(local_port());
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;    
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind() error: ");
        close(sock);
        exit(1);
    }
    // state
    net_initialized = 1;
    // remote host
    strcpy(remote_ipv4, remote_host);

    printf("[NET] INIT NETWORK LAYER OK (with local port %d).\n", local_port());
    printf("[NET] (using %.2f loss and %.2f error probability)\n",
            nl_conf.loss_proba, nl_conf.error_proba);

}

/******************************************************
 * Initialisation de la couche réseau en mode local.  *
 *   role : 0 émetteur, ou 1 récepteur                *
 ******************************************************/
void init_reseau(int role) {

    init_network(role, "127.0.0.1"); // TODO. check with "localhost" and inet_pton function
}

/**********************************************************************
 * Initialisation de la couche réseau en mode réparti.                *
 *   role : 0 émetteur, ou 1 récepteur                                *
 *   hote_distant : @ IP du destinataire (l'emetteur ou le récepteur) *
 **********************************************************************/
void init_reseau_mode_reparti(int role, char *hote_distant) {

    init_network(role, hote_distant);
}

/******************************************************************************
 * Attend un evenement et retourne :
 *    -1 si un paquet reçu est disponible
 *    un numero de timer si un timeout a été généré
 *****************************************************************************/
int attendre() {

    int packet_received = 0;

    if (!net_initialized) {
        printf("[NET] ERROR, can't call \"attendre\" if network not initialized!\n");
        exit(1);
    }

    // Monitor multiple file descriptors with select
    // (only one here --> the socket)
    fd_set rfds; // set of file descriptors
    int nfds = sock + 1; // highest-numbered file descriptor, plus 1

    struct timeval timeout;
    struct timeval *ptimeout;

    while (!packet_received) {
        // checking for any timeout
        for (int i=0; i<num_timers; i++) {
            if (timers[i].exp == 0) {
                // found one!
                int timer = timers[i].num_timer;
                /* removing timer from array */
                for (; i < num_timers - 1; i++) {
                    timers[i] = timers[i+1];
                }
                num_timers--;
                return timer; // return timer that has expired!
            }
        }

        // reinit select timeout
        // (man: "Consider timeout to be undefined after select() returns")
        if (num_timers > 0) {
            // if timmers are running, we block the select only 10ms
            // to see if a packet has arrived
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000; /* 10000 us = 10 ms */
            // timeout.tv_usec = 100000; /* 100000 us = 100 ms */
            ptimeout = &timeout;
        }
        else
            // no timers, block indefinitely waiting for a packet to arrive.
            ptimeout = NULL;
        
        FD_ZERO(&rfds); // clearing set of file descriptors
        FD_SET(sock, &rfds); // adding our socket fd to the set

        /* Watching reading fd set (block for max timeout) */
        int rep = select(nfds, &rfds, NULL, NULL, ptimeout);

        switch (rep) {
        case -1:
            perror("select error: ");
            close(sock);
            exit(1);
        case 0:
            /* select() timeout has expired, no packet, we update the timers */
            /* the timers are thus updated only when you call attendre() and
             * when no packet has arrived! */
            for (int i=0; i<num_timers; i++) {
                if (timers[i].exp > 0) {
                    timers[i].exp -= 10; // 10ms step
                    if (timers[i].exp < 0)
                        timers[i].exp = 0;
                }
            }
            break;
        default:
            /* select success, check if socket is ready for reading */
            if (FD_ISSET(sock, &rfds))
                packet_received = 1;
        }
    }
    // packet_received
    return -1;
}

/*******************************************************************************
 * Recoit un paquet de type paquet_t
 ******************************************************************************/
void de_reseau(paquet_t *packet) {

    int data_len;
    if (!net_initialized) {
        printf("[NET] ERROR, can't call \"de_reseau\" if network not initialized!\n");
        exit(1);
    }

    data_len = recvfrom(sock, (char *)packet, sizeof(paquet_t), 0, NULL, NULL);
    if (data_len < 0) {
        perror("recvfrom() error: ");
        close(sock);
        exit(1);
    }

    /* last data packet? */
    if (my_role == RECEIVER && packet->lg_info < MAX_INFO) {
        last_data_pkt = 1;
    }

    printf("[NET] packet received.\n");
}

/*******************************************************************************
 * Envoie un paquet de type paquet_t
 ******************************************************************************/
void vers_reseau(paquet_t *packet) {

    if (!net_initialized) {
        printf("[NET] ERROR, can't call \"vers_reseau\" if network not initialized!\n");
        exit(1);
    }

    /* loss connection request? */
    if (packet->type == CON_REQ && nl_conf.loss_connect) {
        printf("%s[NET] loss CON_REQ packet%s\n", RED, NRM);
        nl_conf.loss_connect = 0; /* discard packet only one time */
        return;
    }

    /* loss connection response? */
    if (packet->type == CON_ACCEPT && nl_conf.loss_connect) {
        printf("%s[NET] loss CON_ACCEPT packet%s\n", RED, NRM);
        nl_conf.loss_connect = 0; /* discard packet only one time */
        return;
    }

    /* loss disconnection? */
    if (packet->type == CON_CLOSE && nl_conf.loss_disconnect) {
        printf("%s[NET] loss CON_CLOSE packet%s\n", RED, NRM);
        nl_conf.loss_disconnect = 0; /* discard packet only one time */
        return;
    }

    /* loss disconnection ack? */
    if (packet->type == CON_CLOSE_ACK && nl_conf.loss_disconnect) {
        printf("%s[NET] loss CON_CLOSE_ACK packet%s\n", RED, NRM);
        nl_conf.loss_disconnect = 0; /* discard packet only one time */
        return;
    }

    /* loss last ack? */
    if (packet->type == ACK && last_data_pkt && nl_conf.loss_last_ack) {
        printf("%s[NET] loss LAST ACK%s\n", RED, NRM);
        nl_conf.loss_last_ack = 0; /* discard packet only one time */
        return;
    }

    /* loss? */
    if (rand() / (float)RAND_MAX < nl_conf.loss_proba) {
        printf("%s[NET] packet loss!%s\n", RED, NRM);
        packet_loss_count++; /* update loss count for perf eval */
        return;
    }

    // copy packet (mandatory to generate errors and
    //              be able to retransmit the original packet)
    paquet_t *new_packet;
    new_packet = malloc(sizeof(paquet_t));
    memcpy(new_packet, packet, sizeof(paquet_t));

    /* error? */
    if (rand() / (float)RAND_MAX < nl_conf.error_proba) {
        printf("%s[NET] generating error in packet!%s\n", RED, NRM);
        if (packet->lg_info > 0 && packet->lg_info <= MAX_INFO) {
            int r = rand() % packet->lg_info;
            /* ones' complement of a random data byte */
            new_packet->info[r] = ~(new_packet->info[r]);
        }
        else
            new_packet->num_seq++;
    }

    // init remote server addr
    struct sockaddr_in dst_addr;
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(remote_port()); /* htons: host to net byte order (short int) */
    inet_pton(AF_INET, remote_ipv4, &(dst_addr.sin_addr));
    // TODO. check "localhost" with inet_pton...
    int addr_len = sizeof(dst_addr);

    int data_len = sendto(
        sock,
        (char *)new_packet, sizeof(paquet_t), 0,
        (struct sockaddr *)&dst_addr, addr_len);

    if (data_len < 0) {
        perror("sendto error: ");
        close(sock);
        exit(1);
    }
    sent_packet_count++; /* update packet count for perf eval */
    printf("[NET] packet sent.\n");
    // printf("(to remote @ %s and remote port %d)\n", remote_ipv4, remote_port());
    // check if last packet and performance thread running
    if (my_role == SENDER && new_packet->lg_info < MAX_INFO && nl_conf.plot_period_ms != 0) {
        // stop perf eval thread
        end_communication = 1;
        // wait to make sure performace thread finished (and wrote perf.txt)
        pthread_join(perf_thid, NULL);        
    }
    free(new_packet);
}

/**************************************************************************
 * Demarre le timer numero n (0 <= n <= NBR_MAX_TIMERS-1) qui s'arrete    *
 * apres ms millisecondes (ms doit etre un multiple de 50)                *
 **************************************************************************/
void depart_temporisateur_num(int n, int ms) {

    if (n < 0 || n > MAX_TIMERS - 1) {
        printf("%s[NET] Timer number %d is incorrect.%s\n", RED, n, NRM);
        return;
    }

    /* lookup if timer already in use */
    for (int i=0; i < num_timers; i++)
        if (timers[i].num_timer == n) {
            printf("%s[NET] Can't start timer %d, it is already running.%s\n", RED, n, NRM);
            return;
        }

    /* ok, it's not already used */
    timers[num_timers].num_timer = n;
    timers[num_timers].exp = ms;
    num_timers++;
}

/*********************************************************
 * Vérification si le timer numéro n est en marche       *
 * Retour :  1 si le timer numéro n est en route         *
 *           0 sinon                                     *
 *********************************************************/
int test_temporisateur(int n) {

    for (int i = 0; i < num_timers; i++) {
        if (timers[i].num_timer == n)
            return 1;
    }
    return 0;
}

/****************************
 * Arrete le timer numero n *
 ****************************/
void arret_temporisateur_num(int n) {

    if (n < 0 || n > MAX_TIMERS - 1) {
        printf("%s[NET] Timer number %d is incorrect.%s\n", RED, n, NRM);
        return;
    }

    /* lookup for the timer */
    int i;
    for (i = 0; i < num_timers; i++) {
        if (timers[i].num_timer == n) {
            /* deleting timer */
            for (; i < num_timers - 1; i++) {
                timers[i] = timers[i + 1];
            }
            num_timers--;
            return;
        }
    }

    if (i == num_timers)
        printf("%s[NET] Can't stop timer %d, it is not started!%s\n", RED, n, NRM);
}

void depart_temporisateur(int ms) {

    depart_temporisateur_num(1, ms);
}

void arret_temporisateur() {

    arret_temporisateur_num(1);
}
