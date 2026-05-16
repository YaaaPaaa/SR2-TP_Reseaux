/****************************************************************
 *  Lecture de variables de configuration à partir d'un fichier * 
 *                                                              *
 * E. Lavinal - Université de Toulouse III - Paul Sabatier      *
 ****************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONF_FILE "config.txt"

#define FILE_TO_SEND "FICHIER_IN"
#define FILE_TO_RECEIVE "FICHIER_OUT"

#define LOSS_PROBA_S "PROBA_PERTE_E"
#define LOSS_PROBA_R "PROBA_PERTE_R"

#define ERROR_PROBA_S "PROBA_ERREUR_E"
#define ERROR_PROBA_R "PROBA_ERREUR_R"

#define LOSS_CONNECTION_REQ "PERTE_CON_REQ"
#define LOSS_CONNECTION_REP "PERTE_CON_ACCEPT"

#define LOSS_DECONNECTION "PERTE_CON_CLOSE"
#define LOSS_DECONNECTION_ACK "PERTE_CON_CLOSE_ACK"

#define LOSS_LAST_ACK "BOOL_PERTE_LAST_ACK"

#define PLOT_PERIOD_THROUGHPUT "PERIODE_CALCUL_DEBIT"

// Network layer config.
typedef struct netlib_config_s {
    float loss_proba;
    float error_proba;
    int loss_connect;
    int loss_disconnect;
    int loss_last_ack;
    int plot_period_ms;
} netlib_config_t;

void conf_app_sender(char *file_to_send);
void conf_net_sender(netlib_config_t *nl_conf);

void conf_app_receiver(char *file_to_receive);
void conf_net_receiver(netlib_config_t *nl_conf);

#endif
