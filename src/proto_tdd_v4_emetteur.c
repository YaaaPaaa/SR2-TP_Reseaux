/*************************************************************
* proto_tdd_v4 -  émetteur                                   *
* TRANSFERT DE DONNEES  v4                                   *
*                                                            *
* Protocole v4 : « Selective Repeat » ARQ                    *
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
    
    /*[Buffer qui enregistre si un ack est reçu sans défaut mais que, pour le momement, on ne peut pas décaler la fenêtre]*/
    int ack_recu[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; 

    /*[Pour éviter de boucler dans la boucle de réajustement de la borne_inf]*/
    int i_taille_fenetre = 0;

    /*[Création de la fenêtre d'anticipation]*/
    int taille_fenetre = 4; /*Taille de la fenêtre à 4 par défaut*/
    unsigned int borne_inf = 0; /*Initialisation de la borne inférieur à 0*/
    unsigned int curseur = 0; /*Initialisation du curseur à 0*/

    /*[Dans ce "if" on test si on veut modifier la taille de la fenêtre dans l'appel de la fonction]*/
    if(argc == 2) taille_fenetre = modifier_taille_fenetre(atoi(argv[1]),1);
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
            
            /*[Départ du temporisateur pour chaque paquet envoyé]*/
            depart_temporisateur_num(tab_paquet[curseur].num_seq,100);

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
                        /*[Paquet acquittés sans erreur donc arrêt de son timer]*/
                        arret_temporisateur_num(p_ack.num_seq);

                        /*[On ne décale la fenêtre que si cela est possible]*/
                        if(p_ack.num_seq == borne_inf){
                            borne_inf = inc(borne_inf,SEQ_NUM_SIZE);
                            
                            /*[Si des ACK ont été reçu et qu'il se trouve après borne_inf, alors on réajuste borne_inf]*/
                            while (ack_recu[borne_inf] == borne_inf && i_taille_fenetre < taille_fenetre){
                                /*[Reset pour le prochain "cycle" de la fenêtre]*/
                                ack_recu[borne_inf] = -1;

                                /* avancer la borne et le curseur */
                                borne_inf = inc(borne_inf, SEQ_NUM_SIZE);
                                i_taille_fenetre++;
                            }
                            i_taille_fenetre = 0;
                        } else {
                            /*[On enregistre dans le buffer les n° des ACK reçu qui sont après borne_inf]*/
                            ack_recu[p_ack.num_seq] = p_ack.num_seq;
                        }
                    }
                    /*[Sinon erreur p_ack ou pas dans la fenêtre, on ignore l'ACK]*/
                } else {
                    /*[Timeout : ne retransmet que le paquet pour lequel le délai a expiré]*/
                    vers_reseau(&tab_paquet[evt]);
                    depart_temporisateur_num(evt,100);

                    retransmissions++;
                    if (retransmissions >= MAX_RETRANSMISSIONS) {
                        printf("[TRP] Trop de retransmissions, arrêt du protocole.\n");
                        return 1; /*[Arrêt après trop de retransmissions]*/
                    }
                }
            }
        }
    }
    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}

/*
    * Un temporisateur devra être utilisé pour chaque paquet envoyé ;
    * A la réception d'un ACK sans erreur inclus dans la fenêtre, on ne 
        décale la fenêtre que si cela est possible (cf. diapo 20 dans le 
        support CTD sur « sliding window ») ;
    * Lors d'un timeout, l'émetteur ne retransmet que le paquet pour 
        lequel le délai a expiré;
*/