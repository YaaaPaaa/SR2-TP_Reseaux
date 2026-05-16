#ifndef __COUCHE_TRANSPORT_H__
#define __COUCHE_TRANSPORT_H__

#include <stdint.h> /* uint8_t */

#define MAX_INFO 124

/*************************
 * Structure d'un paquet *
 *************************/

typedef struct paquet_s {
    uint8_t type;         /* type de paquet, cf. ci-dessous */
    uint8_t num_seq;      /* numéro de séquence */
    uint8_t lg_info;      /* longueur du champ info */
    uint8_t somme_ctrl;   /* somme de contrôle */
    unsigned char info[MAX_INFO];  /* données utiles du paquet */
} paquet_t;

typedef enum {paquet_arrive, timeout} Evenement;

/*[Nombre maximum de retransmissions avant d'abandonner, utile pour le cas de perte du dernier ACK]*/
#define MAX_RETRANSMISSIONS 50

/******************
 * Types de paquet *
 ******************/
#define DATA          1  /* données de l'application */
#define ACK           2  /* accusé de réception des données */
#define NACK          3  /* accusé de réception négatif */
#define CON_REQ       4  /* demande d'établissement de connexion */
#define CON_ACCEPT    5  /* acceptation de connexion */
#define CON_REFUSE    6  /* refus d'établissement de connexion */
#define CON_CLOSE     7  /* notification de déconnexion */
#define CON_CLOSE_ACK 8  /* accusé de réception de la déconnexion */
#define OTHER         9  /* extensions */

/* Capacite de numerotation pour l'anticipation */
#define SEQ_NUM_SIZE 16

/* ************************************** */
/* Fonctions utilitaires couche transport */
/* ************************************** */

/*--------------------------------------*
* Fonction d'inclusion dans la fenetre *
*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille);

/*--------------------------------------*/
/*Pour tdd_v1 on a besoin d'une une détection d'erreurs basée sur une somme de contrôle*/
/*--------------------------------------*/
uint8_t somme_de_controle(paquet_t paquet);

/*--------------------------------------*/
/*Pour tdd_v2 on a besoin de la fonctoin inc pour incrémenter num modulo mod*/
/*--------------------------------------*/
int inc(int num, int mod);

/*--------------------------------------*/
/*Pour tdd_v3.2 on a besoin de la fonction dec pour decrémenter num modulo mod*/
/*--------------------------------------*/
int dec(int num, int mod);

/*--------------------------------------*/
/*Pour tdd_v3 et v4 la taille de la fenêtre est définie à l'appel du programme.*/
/*--------------------------------------*/
int modifier_taille_fenetre(int taille, int mode);

#endif
