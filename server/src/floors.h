/*
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
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
* File : floors.h                                                            *
*        Funzioni per creare, cancellare, configurare, gestire i floor.     *
****************************************************************************/
#ifndef _FLOORS_H
# define _FLOORS_H   1

#include "config.h"
#ifdef USE_FLOORS

#include "rooms.h"
#include "floor_flags.h"

#define FLOOR_NAME_BASIC "Piazza Centrale"

/* Floor types */
#define FLOOR_BASIC     0
#define FLOOR_NORMAL    1

/* Struttura contenente le informazioni su un floor.                   */
struct floor_data {
        char name[FLOORNAMELEN]; /* Nome del floor                     */
        long num;                /* Numero di matricola                */
	char type;               /* Tipo del floor (Non utilizzato)    */
	int  rlvl;               /* Livello di accesso minimo          */
	int  wlvl;               /* Livello min. per scrivere          */
        long fmnum;              /* file messaggi (Default per room)   */
	long maxmsg;             /* Default #slot nella room           */
        long highmsg;            /* numero messaggio piu' alto         */
        long highloc;            /* numero LOCALE piu' alto            */
	long numpost;            /* numero post lasciati nel floor     */
	long numroom;            /* numero room nel floor              */
        time_t ctime;            /* Creation time                      */
        time_t mtime;            /* Last Modification time             */
        time_t ptime;            /* Last Post time                     */
        unsigned long flags;     /* Flags di configurazione            */
	long def_utf;            /* Flags di default per gli utenti    */
        long master;             /* matricola del Floor-Master         */
};

/* Struttura che permette di creare liste bidirezionali di floors.     */
struct floor {
	long pos;                    /* Posizione nella lista          */
        struct floor_data  *data;    /* Floor Data                     */
	struct f_room_list *rooms;   /* Lista delle room nel floor     */
        struct floor *prev, *next;   /* Floor precedente e successivo  */
};

/* Struttura per memorizzare la lista di floor                         */
struct floor_list {
	struct floor * first;  /* Punta al primo floor presente        */
	struct floor * last;   /* Punta all'ultimo floor presente      */
};

struct f_room_list {
	struct room * room;          /* Struttura della room           */
	struct f_room_list * prev;   /* Room precedente                */
	struct f_room_list * next;   /* Room successiva                */
};

/*
 * Restituisce un valore booleano, TRUE se la sessione S appartiene a un
 * utente con privilegi di Floor Aide o superiore nel floor corrente,
 * altrimenti falso.
 */
#define IS_FA(S,F) ((((S)->utente)                                       \
	&& (((S)->utente->livello >= LVL_AIDE)                           \
	    || (((S)->utente->livello >= LVL_FLOORAIDE) &&               \
		((S)->utente->matricola == (F)->data->master)))))
/*
 * Per ora e' equivalente a IS_FA(), andra' modificata quando verrano
 * implementati i Floor Helpers.
 */
#define IS_FH(S,F) IS_FA((S),(F))

#include "strutture.h"

/* Variabili globali */
extern struct floor_list floor_list;

/* Prototipi delle funzioni in floors.c */
void floor_load_data(void);
void floor_save_data(void);
long floor_create(char *name, long fmnum, long maxmsg, char type);
void floor_delete(struct floor *fl);
void floor_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr);
void floor_change(struct floor *f, struct room *r);
struct floor * floor_find(char *nome);
struct floor * floor_findp(char *nome);
struct floor * floor_findn(long num);
int floor_known(struct sessione *t, struct floor *f);
void floor_free_list(void);
void floor_add_room(struct floor *f, struct room *r);
void floor_remove_room(struct room *r);
void floor_post(long floor_num, long msgnum, time_t ptime);

#endif /* USE_FLOORS */

#endif /* floors.h */

/*
 *  Rappresentazione schematica della struttura dei floors.
 * 
 *                                floor_list
 *                     first  /               \   last
 *                          /                   \
 *              prev     |/                       \|      next
 *         NULL <--- floor <--> floor <--> ... <--> floor ---> NULL
 *                   |   |
 *                   |   |-> foor_data (Struttura con i dati del floor)
 *             rooms |
 *                  \|/
 *  NULL <--- f_room_list <--> f_room_list <--> ... <--> f_room_list ---> NULL
 *       prev       |                |                         |     next
 *                 \|/              \|/                       \|/
 *                 room             room                      room
 */
