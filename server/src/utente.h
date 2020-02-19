/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: utente.h                                                            *
*       routines che riguardano operazioni sui dati dell'utente             *
****************************************************************************/
#ifndef _UTENTE_H
#define _UTENTE_H   1
#include "user_flags.h"

#define NO_USER        -1
#define ANONYMOUS_USER -2

/* Struttura dati dell'utente.                                        */
/*
 * WARNING! saved raw on disk: conversion of files needed if struct changed!
 */
struct dati_ut {
  char nome[MAXLEN_UTNAME];       /* Nickname dell'utente             */
  char password[MAXLEN_PASSWORD]; /* Password                         */
  long matricola;                 /* Eternal number                   */
  int livello;                    /* Livello di accesso               */
  char registrato;                /* Se 0 deve ancora registrarsi     */
  /* TODO: registrato -> bool? */
  char val_key[MAXLEN_VALKEY];    /* Validation Key                   */
                                  /* Se val_key[0]==0 e' validato     */
  unsigned char secondi_per_post; /* Minimo num. secondi tra 2 post   */

  /* Dati Personali */
  char nome_reale[MAXLEN_RNAME];  /* Nome 'Real-life'                 */
  char via[MAXLEN_VIA];		  /* Street address                   */
  char citta[MAXLEN_CITTA];	  /* Municipality                     */
  char stato[MAXLEN_STATO];	  /* State or province                */
  char cap[MAXLEN_CAP];		  /* Codice di avviamento postale     */
  char tel[MAXLEN_TEL];		  /* Numero di telefono               */
  char email[MAXLEN_EMAIL];	  /* Indirizzo E-mail                 */
  char url[MAXLEN_URL];           /* Indirizzo Home-Page              */
  /* TODO: Aggiungere data di nascita */
        
  /* Dati di accesso */
  int chiamate;                   /* Numero di chiamate               */
  int post;                       /* Numero di post                   */
  int mail;                       /* Numero di mail                   */ 
  int x_msg;                      /* Numero di X-msg                  */ 
  int chat;                       /* Numero messaggi in chat          */
  /* TODO: aggiungere numero blog messages immessi ?                    */
  long firstcall;                 /* Prima chiamata                   */
  long lastcall;                  /* Ultima chiamata                  */
  long time_online;               /* Tempo trascorso online           */
  char lasthost[MAXLEN_LASTHOST]; /* Provenienza dell'ultima chiamata */

  /* Flags di configurazione */
  char flags[NUM_UTFLAGS];        /* 64 bits di configurazione        */
  char sflags[NUM_UTSFLAGS];      /* bits conf. non modif. dall'utente*/    
  long friends[2*NFRIENDS];       /* Matricole utenti nella friendlist*/
                 /* Primi NFRIENDS -> amici, seconda meta' -> nemici. */

  /* TODO 'prossimo' non viene piu' utilizzato: eliminarlo!           */
  struct dati_ut *prossimo;       /* Prossima struttura di utente     */
};

/* Struttura dati per i blog */
typedef struct blog {
        long pos;                   /* Posizione nella lista  TODO: serve? */
        struct dati_ut   *user;
        struct room_data *room_data;
        struct msg_list  *msg;
        struct blog      *next;
        int dirty;                  /* TRUE se e' stato modificato */
} blog_t;

/* Struttura per creare la lista degli utenti. */
struct lista_ut {
	struct dati_ut  *dati;
        blog_t          *blog;
	struct lista_ut *prossimo;
};

extern struct lista_ut *lista_utenti, *ultimo_utente;

/* Prototipi delle funzioni in questo file */
void carica_utenti(void);
void salva_utenti(void);
void free_lista_utenti(void);
struct dati_ut * trova_utente(const char *nome);
struct dati_ut * trova_utente_n(long matr);
const char * nome_utente_n(long matr);
struct sessione *collegato(char *ut);
struct sessione * collegato_n(long num);
char check_password(char *pwd, char *real_pwd); /* verifica la password */
void cripta(char *pwd);
bool is_friend(struct dati_ut *amico, struct dati_ut *utente);
bool is_enemy(struct dati_ut *nemico, struct dati_ut *utente);
void sut_set_all(int num, char flag);

#ifdef USE_VALIDATION_KEY
void invia_val_key(char *valkey, char *addr);
void purge_invalid(void);
#endif

/***************************************************************************/
#endif /* utente.h */
