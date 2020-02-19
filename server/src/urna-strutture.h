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
* Cittadella/UX server (C) M. Caldarelli and R. Vianello *
* *
* File: urna.c *
* Gestione referendum e sondaggi. *
******************************************************************************/
#ifndef _URNA_STRUTTURE_H
#define _URNA_STRUTTURE_H 1

#ifdef USE_REFERENDUM
#include "urna_comune.h"
#include "urna-cost.h"
#include "x.h"
#include <time.h>

/* Struttura dati */
union tipo_crit{
   long numerico;
   time_t tempo;
};



struct definende{
		struct sessione *sessione;	/*sessione di chi definisce */
		time_t inizio;	/*inizio della definizione */
		int modo;
		struct definende *next; 
};

struct votanti{
		int utente;
		time_t inizio;	/*inizio della definizione */
		int num;
		struct votanti *next; 
};

/*
 * In urna_stat ci sono alcuni dati statistici:
 * in particolare il numero di urne attive (ridondato dal
 * fatto che quelli non attivi sono NULL
 */

struct urna_stat {
unsigned long attive;         /*Numero votazioni in corso */
unsigned long totali;         /*Numero votazioni totali */
unsigned char modificata;  /*0 se urna_stat non e` stata * modificata */ 

unsigned long voti;		   /* Numero totale risposte ricevute */
unsigned long complete;    /* Numero votazioni completate */
/*unsigned long cancellate;    TODO Numero votazioni cancellate */
/*unsigned long prechiuse;    TODO Numero votazioni prechiuse */
unsigned long ultima_urna; /* ultimo progressivo */
unsigned char ultima_lettera; /* + lettere utilizzate */
unsigned long urne_slots;  /* numero di slots allocati */
struct urna **urna_testa;  /* 
						    * testa del vettore di puntatori a urna 
						    * ne vengono allocati LEN_SLOTS alla volta
						    * se vale NULL  urna conclusa/cancellata...
						    */ 
struct votanti *votanti; /* elenco di chi sta votando
							il primo deve essere -1, semplifica
							tutto
						 */
};

#define SSTAT sizeof(struct urna_stat)

/* 
 * le urne vengono accedute da un vettore allocato ogni volta
 * che se ne crea una. non sono piu` delle liste ... Ho
 * sperimentato abbastanza ;-)
 */

struct urna {
   unsigned long progressivo;	/* Numero progressivo del quesito */
   						/* ripetuto in conf, in questo modo questa
						 * struttura non viene salvata su file
						 */
   char semaf;		/* semafori:
					   bit
				 * * 0: in uso
				 * * 1: stanno votando (non ancora in uso)
				 * * 2: conclusa 
				 * * 3: XMIN minuti dalla chiusura (non ancora in uso)
				 * * 4: interrotta prima del previsto
				 */
#if 0
  int votanti;  /* numero di votanti in quel momento */
                /* votanti ora è su ustat */
#endif
   struct urna_conf *conf;	/* configurazione */
   struct urna_dati *dati;	/* dati */
   struct urna_prop *prop;	/* proposte */
};

#define SURNA sizeof(struct urna)

struct urna_conf {
   unsigned long progressivo;	/* Numero progressivo del quesito */
   unsigned char lettera;			/* 
									 * lettera con  ci si riferisce
									 * al quesito (vedere doc per)
							 		 */
   long proponente;		/* matricola del proponente */
   long room_num;		/* matricola della room */
   time_t start;		/* Data di apertura */
   time_t stop;			/* Prima data di chiusura 
				 		 * il controllo va fatto su urna_dati!!!
						 */

   char titolo[U_MAX_LEN_TITOLO+1];  /* titolo */
   char testo[MAXLEN_QUESITO][LEN_TESTO]; /* testo */
   char  **voci;        
   int num_prop;		/* numero massimo di proposte */
   int num_voci;		/* numero delle voci*/
   int max_voci;		/* numero massimo di voci  che si possono 
						 * votare
						 */
   int maxlen_prop;		/* lunghezza massima delle proposte */

   char tipo;            /*referendum / sondaggio */
   char modo;			/* 0=si/no 1 scelta singola
						   2 scelta multipla
						   3 voto
						   4 proposta
						   5 voto+proposta
						*/
   char bianca;         /* 0 non si vota bianca 1 si */
   char astensione;     /* per modo=voto  ci si puo` astenere sulla voce */

   unsigned short int crit;		/* flag dei criteri 
   						 		 * per ora ne accetta solo uno 
								 */

   union tipo_crit val_crit; /* criteri */
};

#define SCONF sizeof(struct urna_conf)

/* 
 * voti per la singola voce
 * Se giudizi verra` tenuto (l'elenco dei
 * voti dati da ciascun utente)
 * Forse giudizi e` un puntatore
 * visto che [10] occuperebbero
 * 13 Long, mentre cosi`
 * solo 4.... (per ogni voce, quindi anche 180 long)
 * per tutti i tipi diversi da voto
 */

struct urna_voti{
   long num_voti;		 /* voti ricevuti SENZA astensioni */
 #if URNE_GIUDIZI
   long *giudizi;	     /* giudizi */
#endif
   long tot_voti;		 /* somma totale dei voti
				          * dati alla voce */
   long astensioni;	 /* numero di astensioni 
				          * della voce */
};

#define SVOTI sizeof(struct urna_voti)

struct urna_prop{
		long matricola; /* matricola del proponente */
		int num; /* numero delle proposte */
		char ** proposte; /* proposte */
		struct urna_prop * next;
};

#define SPROP sizeof(struct urna_prop)
/* dati dinamici dell'urna */

struct urna_dati {
   time_t stop;			/* Data di chiusura */
   int posticipo;		/* Il # di volte che è stata posticipata */
   long bianche;		/* Numero di schede bianche */
						/* se possibile: nel caso del tipo: votazione
						 * dà il numero di bianche ``totali''
						 * quelle voce per voce vanno calcolate
						 * su voci*/
   long nvoti;			/* Numero di utenti che hanno votato */
   long voti_nslots;	/* slots usati */
   int num_voci;		/* numero delle voci*/
   long *uvot;			/* vettore degli utenti che hanno votato */
   struct urna_voti*voti; /* elenco dei voti alle voci, termina con null */
   long ncoll;			/* numero di utenti che si sono collegati */
   long coll_nslots;	/* slots usati */
   long *ucoll;			/* vettore degli utenti che si sono collegati 
						   durante urna*/ 
};

#define SDATI sizeof(struct urna_dati)

/* Variabili globali */

extern struct urna_stat ustat;
extern unsigned short ut_definende;
extern struct definende *ut_def;	/* urne che qualcuno sta definendo */

#endif /* use referendum */

#endif /* urna.h */
