/*************************************************************
* proto_tdd_v3.1 -  émetteur                                 *
* TRANSFERT DE DONNEES  v3.1                                 *
*                                                            *
* Protocole v3.1 : « Go-Back-N » ARQ                         *
*                                                            *
* Y. Paluch - Univ. de Toulouse III - Paul Sabatier          *
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t tab_paquet[SEQ_NUM_SIZE]; /*[Capacité de numérotation de 16]*/
    paquet_t p_ack; /* paquet utilisé par le protocole */
    Evenement evt;

    /*[Compteur de retransmissions pour gérer le cas de perte du dernier ACK]*/
    int retransmissions = 0;

    /*[Création de la fenêtre d'anticipation]*/
    int taille_fenetre = 4; /*Taille de la fenêtre à 4 par défaut*/
    unsigned int borne_inf = 0; /*Initialisation de la borne inférieur à 0*/
    unsigned int curseur = 0; /*Initialisation du curseur à 0*/

    /*[Dans ce "if" on test si on veut modifier la taille de la fenêtre dans l'appel de la fonction]*/
    if(argc == 2) taille_fenetre = modifier_taille_fenetre(atoi(argv[1]),0);
    if (taille_fenetre == -1) return 1;
    
    init_reseau(EMISSION);
    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /*[Lecture de donnees provenant de la couche application]*/
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while (taille_msg != 0 || borne_inf != curseur) {
        /*[Si on peut écrire et qu'on a du crédit alors on envoie le paquet]*/
        if(dans_fenetre(borne_inf,curseur,taille_fenetre) && taille_msg != 0){
            /*[Construction et contrôle du paquet]*/
            for (int i=0; i<taille_msg; i++) {
                tab_paquet[curseur].info[i] = message[i];
            }
            tab_paquet[curseur].lg_info = taille_msg;
            tab_paquet[curseur].type = DATA;
            tab_paquet[curseur].num_seq = curseur;
            tab_paquet[curseur].somme_ctrl = somme_de_controle(tab_paquet[curseur]);

            /*[Remise à la couche reseau]*/
            vers_reseau(&tab_paquet[curseur]);
            
            /*[Départ du temporisateur pour le premier bit encoyé]*/
            if(borne_inf == curseur) depart_temporisateur(100);

            /*[On déplace le curseur]*/
            if (taille_msg > 0) {/*[Condtion pour éviter le cas ou le curseur s'incrémente même s'il n'y a plus de données après]*/
                curseur = inc(curseur, SEQ_NUM_SIZE);
            }

            /*[Lecture de donnees provenant de la couche application]*/
            de_application(message, &taille_msg);

        } else { /*[Sinon -> attente obligatoire]*/
            while (borne_inf != curseur) {
               /*[Attente d'un ACK ou d'un timeout]*/
                evt = attendre();
                if(evt == PAQUET_RECU){
                    de_reseau(&p_ack);
                    retransmissions = 0; /*[Réinitialisation du compteur de retransmissions]*/

                    if((somme_de_controle(p_ack)  == p_ack.somme_ctrl) && dans_fenetre(borne_inf,p_ack.num_seq,taille_fenetre)){
                        /*[Si ACK OK alors paquet bien reçu, décalage de la fenêtre]*/
                        borne_inf = inc(p_ack.num_seq,SEQ_NUM_SIZE);
                        if(borne_inf == curseur){
                            /*[Tout les paquets sont acquittés]*/
                            arret_temporisateur();
                        }
                    }/*[Sinon erreur p_ack ou pas dans la fenêtre, on ignore l'ACK]*/
                } else {
                    /*[Timeout : retransmission de tous jusqu'au curseur]*/
                    int i = borne_inf;
                    retransmissions++;
                    if (retransmissions >= MAX_RETRANSMISSIONS) {
                        printf("[TRP] Trop de retransmissions, arrêt du protocole.\n");
                        return 1; /*[Arrêt après trop de retransmissions]*/
                    }

                    depart_temporisateur(100);
                    while(i != curseur){
                        vers_reseau(&tab_paquet[i]);
                        i = inc(i,SEQ_NUM_SIZE);
                    }
                }
            }
        }
    }
    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}

/*En cas de perte du tout dernier ACK, 2 possibilité :
    1- Soit on met un max retransmissions côté émetteur
    2- Soit on met en place un time wait() côté réceptpeur
    (attendre 2 secondes pour être sûr que le dernier ACK ne se perde pas)*/