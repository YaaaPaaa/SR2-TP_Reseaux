/********************************************************************
 *         Simple abstraction of a network layer library            *
 *         (and some other useful tools handling timers)            *
 *                                                                  *
 * v2.4, Sept. 2024 - E. Lavinal                                    *
 * v1.7, Sept. 2022, Sept 2023 - E. Lavinal                         *
 * v1.1, Jan. 2020 - P. Torguet                                     *
 *                                                                  *
 * Département d'Informatique                                       *
 * Faculté Sciences et Ingénierie                                   *
 * Université Toulouse III - Paul Sabatier                          *
 ********************************************************************/

#ifndef __RESEAU_H__
#define __RESEAU_H__

#include "couche_transport.h" /* paquet_t */

/* pour init_reseau() */
#define EMISSION  0
#define RECEPTION 1

/* pour attendre() */
#define PAQUET_RECU -1

/* ============================ */
/* Initialisation couche réseau */
/* ============================ */

/******************************************************
 * Initialisation de la couche réseau en mode local.  *
 *   role : 0 émetteur, ou 1 récepteur                *
 ******************************************************/
void init_reseau(int role);


/**********************************************************************
 * Initialisation de la couche réseau en mode réparti.                *
 *   role : 0 émetteur, ou 1 récepteur                                *
 *   hote_distant : @ IP du destinataire (l'emetteur ou le récepteur) *
 **********************************************************************/
void init_reseau_mode_reparti(int role, char *hote_distant);


/* ================================================= */
/* Primitives de service pour émission et réception  */
/* ================================================= */

/*****************************************************
 * Remet un paquet à la couche réseau pour envoi     *
 * sur le support de communication.                  *
 *****************************************************/
void vers_reseau(paquet_t *paquet);

/***********************************************************
 * Prélève un paquet de la couche réseau (N.B. : fonction  *
 * bloquante tant qu'un paquet n'est pas reçu).            *
 ***********************************************************/
void de_reseau(paquet_t *paquet);

/* ======================================================= */
/* Fonctions utilitaires pour la gestion de temporisateurs */
/* ======================================================= */

/***********************************************************************
 * Démarre le temporisateur numéro n (0 <= n <= 31), qui s'arrêtera    *
 * après ms millisecondes (ms doit être un multiple de 10)             *
 * Valeur minimale : 20 ms (valeur conseillée en salle de TP : 100 ms) *
 ***********************************************************************/
void depart_temporisateur_num(int n, int ms);

/***********************************************************************
 * Démarre un temporisateur qui s'arrêtera après ms millisecondes.     *
 * Valeur minimale : 20 ms (valeur conseillée en salle de TP : 100 ms) *
 * (dans tous les cas ms doit être un multiple de 10)                  *
 ***********************************************************************/
void depart_temporisateur(int ms);

/************************************
 * Arrête le temporisateur numero n *
 ************************************/
void arret_temporisateur_num(int n);

/***************************
 * Arrêt du temporisateur  *
 ***************************/
void arret_temporisateur();

/****************************************************************
 * Attend un évènement : paquet reçu ou timeout.                *
 * (N.B. : fonction bloquante)                                  *
 * Retour :  -1 si un paquet reçu est disponible, sinon         *
 *           le numéro de temporisateur qui a expiré            *
 ****************************************************************/
int attendre();

#endif
