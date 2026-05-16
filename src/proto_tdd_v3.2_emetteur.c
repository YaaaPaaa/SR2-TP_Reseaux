/*************************************************************
* proto_tdd_v3.2 -  émetteur                                  *
* TRANSFERT DE DONNEES  v3                                   *
*                                                            *
* Protocole v3.2 : « Go-Back-N » avec « Fast Retransmit »    *
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

    /*[Compteur d'acquittement dupliqué]*/
    int ack_dupliques = 0;
    int Pas_meme_ACK = -1; /*[Variable pour pouvoir utiliser le FR qu'une seule fois par ACK et initialisation à -1 pour ne pas être un num_seq]*/
                            

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

                        /*[Réinitialisation du compteur d'ACK dupliqués si ACK reçu différent]*/
                        ack_dupliques = 0;

                        if(borne_inf == curseur){
                            /*[Tout les paquets sont acquittés]*/
                            arret_temporisateur();
                        }
                    /*[On détecte un acquittement dupliqué s'il est égal à la valeur qui précède la borne inférieure de la fenêtre]*/
                    } else if(somme_de_controle(p_ack)  == p_ack.somme_ctrl && p_ack.num_seq == dec(borne_inf, SEQ_NUM_SIZE) && p_ack.num_seq != Pas_meme_ACK){
                        ack_dupliques++;
                        retransmissions = 0; /*[Réinitialisation du compteur de retransmissions]*/

                        /*[A la réception de 3 ACK dupliqués, on retransmet et le temporisateur est relancé]*/
                        if(ack_dupliques == 3){
                            /*[Retransmission du paquet attendu]*/
                            printf("[TRP] Fast Retransmit : retransmission du paquet %d\n", borne_inf);
                            vers_reseau(&tab_paquet[borne_inf]);
                            
                            /*[Réinitialisation du compteur d'ACK dupliqués]*/
                            ack_dupliques = 0; 

                            /*[Le mécanisme de Fast Retransmit n'est déclenché qu'une seule fois pour un même numéro d'acquittement]*/
                            Pas_meme_ACK = p_ack.num_seq;
                        }
                    }
                    /*[Sinon erreur p_ack ou pas dans la fenêtre, on ignore l'ACK]*/
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

/*  * On détecte un acquittement dupliqué s'il est égal à la valeur qui précède la borne
    inférieure de la fenêtre (dans l'exemple de la figure, la borne inf est 1,
    ACK 0 est donc un ACK dupliqué) ;

    * A la réception de 3 ACK dupliqués, on retransmet et le temporisateur est relancé ;

    * Le mécanisme de Fast Retransmit n'est déclenché qu'une seule fois pour un même numéro
    d'acquittement (dans l'exemple de la figure, la retransmission n'est pas refaite lors
    de la réception de l'ACK 0 dupliqué la 4ème et la 5ème fois).   */