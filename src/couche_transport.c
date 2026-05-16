#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "couche_transport.h"
#include "services_reseau.h"
#include "application.h"

/* ************************************************************************** */
/* *************** Fonctions utilitaires couche transport ******************* */
/* ************************************************************************** */

/*--------------------------------------*/
/*Pour tdd_v1 on a besoin d'une une détection d'erreurs basée sur une somme de contrôle*/
/*--------------------------------------*/
uint8_t somme_de_controle(paquet_t paquet) {
    uint8_t somme = 0;
    somme ^= paquet.type;
    somme ^= paquet.num_seq;
    somme ^= paquet.lg_info;
    for(int i=0; i < paquet.lg_info;i++){
        somme ^= (uint8_t)paquet.info[i];
    }
    return somme;
}

/*--------------------------------------*/
/*Pour tdd_v2 on a besoin de la fonction inc pour incrémenter num modulo mod*/
/*--------------------------------------*/
int inc(int num, int mod) {
    if (mod <= 0) return num;
    num++;
    if (num >= mod) num -= mod;
    return num;
}

/*--------------------------------------*/
/*Pour tdd_v3.2 on a besoin de la fonction dec pour decrémenter num modulo mod*/
/*--------------------------------------*/
int dec(int num, int mod) {
    if (num >= 1) num--;
    if (num == 0) num = mod - 1;
    return num;
}

/*--------------------------------------*/
/*Pour tdd_v3 et v4 la taille de la fenêtre est définie à l'appel du programme.*/
/*--------------------------------------*/
int modifier_taille_fenetre(int taille, int mode) {
    /*[Dans les deux cas]*/
    if (taille < 1){ 
        printf("[ERR] Taille fenêtre trop petite, elle doit être au minimum égale à 1\n");
        return -1;
    }
    /*[Mode « Go-Back-N »]*/
    else if (mode == 0) return taille; 
    /*[Mode « Selective Repeat »]*/
    else if (taille <= (SEQ_NUM_SIZE/2) && mode == 1) return taille;
    else {
        printf("[ERR] Taille fenêtre trop grande, elle doit être au maximum égale à la moitié de la capacité de numérotation (=%d)\n",SEQ_NUM_SIZE);
        return -1;
    }
}


/*--------------------------------------*/
/* Fonction d'inclusion dans la fenetre */
/*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille) {

    unsigned int sup = (inf+taille-1) % SEQ_NUM_SIZE;

    return
        /* inf <= pointeur <= sup */
        ( inf <= sup && pointeur >= inf && pointeur <= sup ) ||
        /* sup < inf <= pointeur */
        ( sup < inf && pointeur >= inf) ||
        /* pointeur <= sup < inf */
        ( sup < inf && pointeur <= sup);
}