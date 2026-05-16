/*************************************************************
* proto_tdd_v3 -  récepteur                                  *
* TRANSFERT DE DONNEES  v3                                   *
*                                                            *
* Protocole v3 : « Go-Back-N » ARQ                           *
*                                                            *
* Y. Paluch - Univ. de Toulouse III - Paul Sabatier          *
**************************************************************/

#include <stdio.h>
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

    /*[Création de la fenêtre d'anticipation]*/
    int taille_fenetre = 1; /*[Fenêtre de réception = 1]*/
    unsigned int borne_inf = 0; /*Initialisation de la borne inférieur à 0*/
    unsigned int curseur=0; /*Initialisation du curseur à 0*/

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
                for (int i=0; i<=paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }
            
                /*Remise des données à la couche application*/
                fin = vers_application(message, paquet.lg_info);
                
                /*[Création ACK + envoie]*/
                p_ack.num_seq = paquet.num_seq;
                p_ack.somme_ctrl = somme_de_controle(p_ack);
                vers_reseau(&p_ack);
                
                /*[Mise à jour de la fenêtre de réception]*/
                borne_inf = inc(borne_inf,SEQ_NUM_SIZE);

            }/*[On ignore un paquet reçu avec une erreur.]*/
        } else {/*[Sinon on acquitte le dernier paquet reçu correctement]*/
            vers_reseau(&p_ack);
        }
    }
    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}

/*Quelque points sur le récepteur :
    - Le récepteur n'accepte que les paquet en séquence (fenêtre de réception =1)
    - Les ACK émis par le récepteur sont numérotés
    - Chaque paquet reçu sans erreur génère un ACK numérotés:
        *Si le paquet reçu est en séquence alors on acquitte le paquet
        *Sinon on acquitte le dernier paquet reçu correctement
    - On ignore un paquet reçu avec une erreur.*/