/*************************************************************
* proto_tdd_v1 -  récepteur                                  *
* TRANSFERT DE DONNEES  v1                                   *
*                                                            *
* Protocole v1 : « Stop-and-Wait » avec acquittement négatif *
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
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet, p_ack; /*[Paquets utilisés par le protocole]*/
    int fin = 0; /* condition d'arrêt */

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while (!fin) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);

        /*[Détection d'erreurs basée sur une somme de contrôle]*/
        if(somme_de_controle(paquet) == paquet.somme_ctrl){ /* si pas d'erreur */
            /* extraction des donnees du paquet recu */
            for (int i=0; i<paquet.lg_info; i++) {
                message[i] = paquet.info[i];
            }

            /* remise des données à la couche application */
            fin = vers_application(message, paquet.lg_info);
            p_ack.type = ACK; /*[Acquittement positif]*/
            printf("[TRP] Envoi d'un ACK\n");
        } else {
            p_ack.type = NACK;/*[Acquittement négatif en cas d'erreur]*/
            printf("[TRP] Envoi d'un NACK\n");
        }
        /*[Envoie de l'acquittement à la couche réseau]*/
        vers_reseau(&p_ack);
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
