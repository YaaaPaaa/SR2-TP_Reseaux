/*************************************************************
* proto_tdd_v1 -  émetteur                                  *
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
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t paquet, p_ack; /*[Paquets utilisés par le protocole]*/

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while (taille_msg != 0){
        /* construction paquet */
        for (int i=0; i<taille_msg; i++) {
            paquet.info[i] = message[i];
        }
        paquet.lg_info = taille_msg;
        paquet.type = DATA;
        paquet.somme_ctrl = somme_de_controle(paquet); /*[Détection d'erreurs basée sur une somme de contrôle]*/

        /* remise à la couche reseau */
        vers_reseau(&paquet);

        /*[Renvoie paquet si on n'a pas ACK]*/
        de_reseau(&p_ack);
        while(p_ack.type != ACK){
            printf("[TRP] Pas d'ACK, renvoi du paquet.\n");
            vers_reseau(&paquet);
            de_reseau(&p_ack);
        }
        printf("[TRP] ACK recu, envoi du prochain paquet.\n");

        /* lecture de donnees provenant de la couche application */
        de_application(message, &taille_msg);
    }
    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
