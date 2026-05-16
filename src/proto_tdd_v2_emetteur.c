/*************************************************************
* proto_tdd_v2 -  émetteur                                   *
* TRANSFERT DE DONNEES  v2                                   *
*                                                            *
* Protocole v2 : « Stop-and-Wait » ARQ                       *
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
    paquet_t paquet, p_ack; /* paquet utilisé par le protocole */
    Evenement evt;
    int prochain_paquet = 0; /*[Numéro du prochain paquet à envoyer]*/

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
        paquet.num_seq = prochain_paquet;
        paquet.somme_ctrl = somme_de_controle(paquet);/*[Détection d'erreurs basée sur une somme de contrôle]*/

        /* remise à la couche reseau */
        vers_reseau(&paquet);

        /*[Compteur de retransmissions pour gérer le cas de perte du dernier ACK]*/
        int retransmissions = 0; 

        /*[On démarre un timer]*/
        depart_temporisateur(100);
        evt = attendre();
        while (evt != PAQUET_RECU) {
            /* remise à la couche reseau */
            vers_reseau(&paquet);

            retransmissions++;
            if (retransmissions >= MAX_RETRANSMISSIONS) {
                printf("[TRP] Trop de retransmissions, arrêt du protocole.\n");
                return 1; /*[Arrêt après trop de retransmissions]*/
            }

            /*[On redémarre un timer]*/
            depart_temporisateur(100);
            evt = attendre();
        }

        /*[Réception de l'acquittement]*/
        de_reseau(&p_ack);
        arret_temporisateur();/*[On arrête le timer]*/

        /*[Alternance des numéros de séquence 0 et 1]*/
        prochain_paquet = inc(prochain_paquet,2); 

        /* lecture de donnees provenant de la couche application */
        de_application(message, &taille_msg);
    }
    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
