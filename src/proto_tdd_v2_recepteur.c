/*************************************************************
* proto_tdd_v2 -  récepteur                                  *
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
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet, p_ack; /*[Paquets utilisés par le protocole]*/
    int fin = 0; /* condition d'arrêt */
    int paquet_attendu = 0; /*[Numéro du paquet attendu]*/

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while (!fin) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);

        /*[Détection d'erreurs basée sur une somme de contrôle]*/
        if(somme_de_controle(paquet) == paquet.somme_ctrl){ /*[Si pas d'erreur]*/
            if (paquet.num_seq == paquet_attendu){ /*[Bon num_seq]*/
                /* extraction des donnees du paquet recu */
                for (int i=0; i<paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }

                /* remise des données à la couche application */
                fin = vers_application(message, paquet.lg_info);
                paquet_attendu = inc(paquet_attendu, 2);
            }
            /*[Envoi de l'acquittement]*/
            p_ack.type = ACK;
            vers_reseau(&p_ack);
        }/*[Sinon on ignore, on laisse passer le timer]*/
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
