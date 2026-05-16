/*************************************************************
* proto_tdd_v4 -  récepteur                                  *
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
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[]){
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet; /*[Paquet qui sera reçu]*/
    int fin = 0; /* condition d'arrêt */

    /*[Buffer qui enregistre un paquet s'il est reçu sans défaut mais que, pour le momement, on ne peut pas décaler la fenêtre]*/
    paquet_t paquet_recu[SEQ_NUM_SIZE];
    for(int i=0; i<SEQ_NUM_SIZE; i++){
        paquet_recu[i].num_seq = -1;
        paquet_recu[i].lg_info = 0;
    }
    /*[Création de la fenêtre d'anticipation]*/
    int taille_fenetre = 4; /*Fenêtre de réception à 4 par défaut*/
    unsigned int borne_inf = 0; /*Initialisation de la borne inférieur à 0*/
    unsigned int curseur=0; /*Initialisation du curseur à 0*/

    /*[Pour éviter de boucler dans la boucle de réajustement de la borne_inf]*/
    int i_taille_fenetre = 0;

    /*[Dans ce "if" on test si on veut modifier la taille de la fenêtre dans l'appel de la fonction]*/
    if(argc == 2) taille_fenetre = modifier_taille_fenetre(atoi(argv[1]),1);
    if (taille_fenetre == -1) return 1;

    /*[Tableau de num_seq déjà envoyé pour les cas de perte et d'erreur dans envoie ACK]*/
    int ack_envoye[taille_fenetre];
    for(int i=0; i<taille_fenetre; i++){
        ack_envoye[i] = -1; /*[Initialisation]*/
    }
    int i_ack_envoye = 0;

    init_reseau(RECEPTION);
    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /*[Paquet d'acquittement]*/
    paquet_t p_ack;
    p_ack.type = ACK;

    /*[Tant que le récepteur reçoit des données]*/
    while (!fin ) {
        de_reseau(&paquet);
        curseur = paquet.num_seq;

        if(dans_fenetre(borne_inf,curseur,taille_fenetre)){
            /*[Détection d'erreurs basée sur une somme de contrôle]*/
            if(somme_de_controle(paquet) == paquet.somme_ctrl){ /*[Si pas d'erreur]*/
                /* extraction des donnees du paquet recu */
                for (int i=0; i<paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }
                
                /*[On ne décale la fenêtre que si cela est possible]*/
                if(paquet.num_seq == borne_inf){
                    borne_inf = inc(borne_inf,SEQ_NUM_SIZE);

                    /*Remise des données à la couche application*/
                    fin = vers_application(message, paquet.lg_info);    
                    
                    /*[Si des paquets ont été reçu et qu'ils se trouvent après la borne_inf, alors on réajuste la borne_inf]*/
                    while (paquet_recu[borne_inf].num_seq == (int)borne_inf && i_taille_fenetre < taille_fenetre){
                        /*Remise des données à la couche application pour le paquet bufferisé à l'indice borne_inf */
                        fin = vers_application(paquet_recu[borne_inf].info, paquet_recu[borne_inf].lg_info);

                        /*[Reset pour le prochain "cycle" de la fenêtre]*/
                        paquet_recu[borne_inf].num_seq = -1;
                        paquet_recu[borne_inf].lg_info = 0;

                        /*[Décalage de la fenêtre]
                            incrémentation de la borne et du curseur */
                        borne_inf = inc(borne_inf,SEQ_NUM_SIZE);

                        i_taille_fenetre++;
                    }
                    i_taille_fenetre = 0;

                    /*[On enregistre le numéro de séquence de l'ACK envoyé pour gérer les cas de perte ou d'erreur]*/
                    ack_envoye[i_ack_envoye] = paquet.num_seq;
                    i_ack_envoye = inc(i_ack_envoye, taille_fenetre);
                } else {
                    /*[On enregistre dans le buffer les paquets reçu qui sont après la borne inf]*/
                    paquet_recu[paquet.num_seq].num_seq = paquet.num_seq;
                    paquet_recu[paquet.num_seq].lg_info = paquet.lg_info;
                    for (int i=0; i<paquet.lg_info; i++) {
                        paquet_recu[paquet.num_seq].info[i] = paquet.info[i];
                    }

                    /*[On enregistre le numéro de séquence de l'ACK envoyé pour gérer les cas de perte ou d'erreur]*/
                    ack_envoye[i_ack_envoye] = paquet.num_seq;
                    i_ack_envoye = inc(i_ack_envoye, taille_fenetre);
                }
                /*[Chaque paquet reçu sans erreur doit générer un ACK. On acquitte le paquet qu'il soit en séquence ou hors séquence.]*/
                p_ack.lg_info = 0;
                p_ack.type = ACK;
                p_ack.num_seq = paquet.num_seq;
                p_ack.somme_ctrl = somme_de_controle(p_ack);
                vers_reseau(&p_ack);
            }/*[On ignore un paquet reçu avec une erreur.]*/
        } else {
            for(int i=0; i<taille_fenetre; i++){
                if (ack_envoye[i] == paquet.num_seq){
                    /*[Si on l'a déjà reçue mais erreur ou perte ACK, renvoie ACK]*/
                    p_ack.lg_info = 0;
                    p_ack.type = ACK;
                    p_ack.num_seq = paquet.num_seq;
                    p_ack.somme_ctrl = somme_de_controle(p_ack);
                    vers_reseau(&p_ack);
                }
            }
        }
        
    }
    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}

/*
    * Il peut accepter des paquets hors séquence mais dans la fenêtre, 
        dans ce cas ces paquets sont « bufferisés » ;
    * Chaque paquet reçu sans erreur génère un ACK. On acquitte le paquet 
        qu'il soit en séquence ou hors séquence.
*/