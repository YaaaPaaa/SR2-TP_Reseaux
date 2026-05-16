/****************************************************************
*  Lecture de variables de configuration à partir d'un fichier *
*                                                              *
* E. Lavinal - Université de Toulouse III - Paul Sabatier      *
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define MAX_LINE 80
#define MAX_PARAM_NAME 32
#define MAX_PARAM_VALUE 32

/* =========================================== */
/* ============ CONF NETWORK LAYER =========== */
/* =========================================== */

/* Configure sender's network layer */
/* -------------------------------- */
void conf_net_sender(netlib_config_t *nl_conf) {

    FILE *f = NULL;
    char line[MAX_LINE];
    char param_name[MAX_PARAM_NAME], param_value[MAX_PARAM_VALUE];

    f = fopen(CONF_FILE, "r");
    if (f == NULL) {
        perror("[Config] Problème ouverture fichier de configuration.\n");
        exit(1);
    }

    /* fgets(char[] dest, int size, FILE* stream); */
    while ( fgets(line, sizeof(line), f) ) {

        if (line[0] != '#' && line[0] != '\n') {
            // printf("line : %s", line);
            sscanf(line, "%s%s", param_name, param_value);
            // printf("\t param name %s -- param value %s\n", param_name, param_value);
            if ( !strcmp(param_name, LOSS_PROBA_S) )
                nl_conf->loss_proba = atof(param_value);
            else if ( !strcmp(param_name, ERROR_PROBA_S) )
                nl_conf->error_proba = atof(param_value);
            else if ( !strcmp(param_name, LOSS_CONNECTION_REQ) )
                nl_conf->loss_connect = atoi(param_value);
            else if ( !strcmp(param_name, LOSS_DECONNECTION) )
                nl_conf->loss_disconnect = atoi(param_value);
            else if ( !strcmp(param_name, PLOT_PERIOD_THROUGHPUT) )
                nl_conf->plot_period_ms = atoi(param_value);
        }
    }
    fclose(f);
}

/* Configure receiver's network layer */
/* ---------------------------------- */
void conf_net_receiver(netlib_config_t *nl_conf) {

    FILE *f = NULL;
    char line[MAX_LINE];
    char param_name[MAX_PARAM_NAME], param_value[MAX_PARAM_VALUE];

    f = fopen(CONF_FILE, "r");
    if (f == NULL) {
        perror("[Config] Problème ouverture fichier de configuration.\n");
        exit(1);
    }

    /* fgets(char[] dest, int size, FILE* stream); */
    while ( fgets(line, sizeof(line), f) ) {

        if (line[0] != '#' && line[0] != '\n') {
            /* printf("line : %s", line); */
            sscanf(line, "%s%s", param_name, param_value);
            if ( !strcmp(param_name, LOSS_PROBA_R) )
                nl_conf->loss_proba = atof(param_value);
            else if ( !strcmp(param_name, ERROR_PROBA_R) )
                nl_conf->error_proba = atof(param_value);
            else if ( !strcmp(param_name, LOSS_CONNECTION_REP) )
                nl_conf->loss_connect = atoi(param_value);
            else if ( !strcmp(param_name, LOSS_DECONNECTION_ACK) )
                nl_conf->loss_disconnect = atoi(param_value);
            else if ( !strcmp(param_name, LOSS_LAST_ACK) )
                nl_conf->loss_last_ack = atoi(param_value);            
        }
    }
    fclose(f);
}

/* =========================================== */
/* ========== CONF APPLICATION LAYER ========= */
/* =========================================== */

/* Configure application layer          */
/* role == 0 sender, role 1 == receiver */
/* ------------------------------------ */
void conf_app(int role, char *file_name) {

    FILE *f = NULL;
    char line[MAX_LINE];
    char param_name[MAX_PARAM_NAME], param_value[MAX_PARAM_VALUE];
    int found = 0;

    f = fopen(CONF_FILE, "r");
    if (f == NULL) {
        perror("[Config] Problème ouverture fichier de configuration.\n");
        exit(1);
    }

    /* fgets(char[] dest, int size, FILE* stream); */
    while ( !found && fgets(line, sizeof(line), f) ) {

        if (line[0] != '#' && line[0] != '\n') {
            // printf("line : %s", line);
            sscanf(line, "%s%s", param_name, param_value);
            // printf("\t param name %s -- param value %s\n", param_name, param_value);
            if ( (role == 0 && !strcmp(param_name, FILE_TO_SEND)) ||
                 (role == 1 && !strcmp(param_name, FILE_TO_RECEIVE)) ) {
                strcpy(file_name, param_value);
                found = 1;
            }
        }
    }
    fclose(f);
    if (!found) {
        perror("[Config] Problème, nom du fichier (à envoyer ou recevoir) non présent dans configuration.\n");
        exit(1);        
    }
}

/* Configure sender's application layer */
/* ------------------------------------ */
void conf_app_sender(char *file_to_send) {

    conf_app(0, file_to_send);
}

/* Configure receiver's application layer */
/* -------------------------------------- */
void conf_app_receiver(char *file_to_receive) {

    conf_app(1, file_to_receive);
}
