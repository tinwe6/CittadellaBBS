
/*
 * Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: urna.c                                                                *
*       Gestione referendum e sondaggi.                                       *
******************************************************************************/
#ifndef _URNA_H
#define _URNA_H 1

#ifdef USE_REFERENDUM
#include "urna_comune.h"
/* Numero massimo di referendum e sondaggi contemporanei */
#define MAX_REFERENDUM  20
#define MAX_SONDAGGI    20
#define MAGIC_CODE	"\336\017"
#define VERSIONE	'@'
#define FILE_STAT	"urnastat"
#define FILE_DATA	"urnadati"
#define FILE_VOCI	"urnavoci"
#define FILE_RES	"urnares"
#define FILE_ROOM_LIKE	"urnares-roomlike"
#define MAGIC_STAT	'S'
#define MAGIC_LISTA	'L'
#define MAGIC_RES	'R'
#define MAGIC_DATA	'D'
#define MAGIC_VOCI	'V'
#define LEN_SLOTS	10	/*lunghezza dello slot per le varie strutture */
#define NUM_GIUDIZI	15	/*slot allocati una volta per tutte */
#define MAX_LONG        2000000000
#define MAX_REFERENDUM  20
#define MAX_SONDAGGI    20
#define MAX_SLOT   	50
#define SEGUE_VOTABILI  221

/* Struttura dati */

/*elenco delle voci per un sondaggi*/

struct voce
{
  char scelta[MAXLEN_VOCE];	/* testo                                */
  long num_voti;		/* voti * SENZA astensioni              */
  long giudizi[NUM_GIUDIZI];	/* -puntatore al primo el.              */
  long tot_voti;		/* somma totale dei voti dati alla voce */
  long astensioni;		/* numero di astensioni della voce      */
};

/*dati dell'urna -- trasformarlo in almeno 2 struct */

struct urna_data
{
  long proponente;		/* Numero di matricola del proponente   */
  long room_num;		/* Numero di matricola della room       */
  char tipo;			/* Tipo: 1 Referendum, 0 Sondaggio      */
  char modo;			/* modo: 0 sí/no 1 sing scelta 2 sc m. 
				   3 voto */
  char testo[MAXLEN_QUESITO][LEN_TESTO];
  char bianca;			/* bianca possibile                     */
  char astensione_scelta;	/* astensioni possibili sulle votazioni */
  char titolo[LEN_TITOLO];	/* Titolo del sondaggio                 */
  char criterio;		/* Criterio di voto             */
  long val_crit;		/* Valore del criterio          */
  time_t start;			/* Data di apertura                     */
  time_t stop;			/* Data di chiusura                     */
  int posticipo;		/* Il # di volte che è stata posticipata */
  int num_voci;			/* numero delle voci                    */
  int max_voci;			/* numero delle voci massimo di voci 
				   che si possono votare                */
  struct voce *testa_voci;	/* voci, con i voti per ciascuna voce   */
  time_t anzianita;		/*                                      */
  long bianche;			/* Numero di schede bianche    */
  /* se possibile: nel caso del tipo: votazione
   * dà il numero di bianche ``totali''
   * quelle voce per voce vanno calcolate
   * su voci
   */
  long nvoti;			/* Numero di utenti che hanno votato 
				   * comprese le bianche (serve per uvot!)
				 */
  long voti_nslots;		/*  Numero di slot allocati              */
  long *uvot;			/* vettore degli utenti che hanno votato */
};

struct urna
{
  long num;			/* Numero del quesito           */
  struct urna_data *data;
  struct urna *next;
  struct urna *prev;
};

struct urna_stat
{
  long voti;			/* Numero totale risposte ricevute */
  long num;			/* Numero di quesiti proposti */
  long complete;		/* Numero votazioni completate */
  long attive;			/* Numero votazioni in corso */
  long q_nslots;		/* numero_slot_allocati       */
  long *quesito_testa;		/* Vettore delle vot in corso */
};

/* Variabili globali */

extern struct urna_stat ustat;
extern struct urna *urna_testa;
extern struct urna *urna_ultima;	/*ultima lista creata */


/* Prototipi funzioni in urna.c */

void rs_load_data (void);
void rs_save_data (void);
int load_urna (void);
int load_stat (void);
int save_dati (struct urna_data *ud, long int num_quesito);
int load_dati (struct urna_data **udp, long num_quesito);
int rs_save (void);
int save_urna (void);
void rs_free_list (void);
void rs_userload (struct sessione *t);
void rs_usersave (struct sessione *t);
int rs_new (struct sessione *t, char *buf, struct text *txt);
void rs_expire (void);
int rs_del (struct urna *u);
int rs_end (struct urna_data *ud, long num);
int rs_vota (struct dati_ut *ut, struct urna_data *ur);
int modificata (long num);
void azzera_modifica (void);
void add_modifica (long num);
void send_ustat (struct sessione *t);
void cmd_sdel (struct sessione *t, char *buf);
void cmd_spst (struct sessione *t, char *buf);
void cmd_sstp (struct sessione *t, char *buf);
void cmd_slst (struct sessione *t);
void cmd_scvl (struct sessione *t, char *arg);
void cmd_sinf (struct sessione *t, char *buf);
void cmd_sris (struct sessione *t, char *buf);

#endif /* use referendum */

#endif /* urna.h */
