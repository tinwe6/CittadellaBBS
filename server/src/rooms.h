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
* File: rooms.h                                                             *
*       definizione strutture e costanti per la gestione delle room.        *
****************************************************************************/
#ifndef _ROOMS_H
#define _ROOMS_H    1
#include <stdbool.h>
#include <time.h>

#include "config.h"
#include "floors.h"
#include "room_flags.h"

/* Room types */
#define ROOM_BASIC      0
#define ROOM_NORMAL     1
#define ROOM_DIRECTORY  2
#define ROOM_GUESS      3  /* Guess si puo' eliminare */
#define ROOM_FLOOR      4
#define ROOM_DYNAMICAL  5 /* Room temporanea, creata dinamicamente, non nella
                             march list */

#define IF_MAILROOM(S) if ((S)->room->data->flags & RF_MAIL)

/*
 * Restituisce un valore booleano, TRUE se la sessione S appartiene
 * a un utente con privilegi di Room Aide nella room corrente,
 * altrimenti falso.
 */
#define IS_RA(S) ((((S)->utente)                                         \
	&& (((S)->utente->livello >= LVL_AIDE)                           \
	    || (((S)->utente->livello >= LVL_ROOMAIDE) &&                \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMAIDE)))))
/* Room Aide Only */
#define IS_RAO(S) ( ((S)->utente)                                        \
        && ((S)->utente->livello >= LVL_ROOMAIDE) &&                     \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMAIDE) )

/*
 * Restituisce un valore booleano, TRUE se la sessione S appartiene
 * a un utente con privilegi di Room Helper nella room corrente,
 * altrimenti falso.
 */
#define IS_RH(S) ((((S)->utente)                                         \
	&& (((S)->utente->livello >= LVL_AIDE)                           \
	    || (((S)->utente->livello >= LVL_ROOMAIDE) &&                \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMAIDE))        \
	    || (((S)->utente->livello >= LVL_NORMALE) &&                 \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMHELPER)))))

/* Stessa cosa, per una room generica R */
#define IS_RHR(S, R) ((((S)->utente)                                     \
	&& (((S)->utente->livello >= LVL_AIDE)                           \
	    || (((S)->utente->livello >= LVL_ROOMAIDE) &&                \
		((S)->room_flags[(R)->pos] & UTR_ROOMAIDE))              \
	    || (((S)->utente->livello >= LVL_NORMALE) &&                 \
		((S)->room_flags[(R)->pos] & UTR_ROOMHELPER)))))

/* Room Helper Only */
#define IS_RHO(S) ((((S)->utente)                                        \
	&& ( ( ((S)->utente->livello >= LVL_ROOMAIDE) &&                 \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMAIDE))        \
	    || (((S)->utente->livello >= LVL_NORMALE) &&                 \
		((S)->room_flags[(S)->room->pos] & UTR_ROOMHELPER)))))

#define IS_AIDE(S) (((S)->utente)&&((S)->utente->livello >= LVL_AIDE))

#define IS_SYSOP(S) (((S)->utente)&&((S)->utente->livello >= LVL_SYSOP))

/* Struttura contenente le informazioni su una room.          */
struct room_data {
        char name[ROOMNAMELEN];
        long num;               /* Numero di matricola        */
	char type;              /* Tipo della room            */
	int  rlvl;              /* Livello di accesso minimo  */
	int  wlvl;              /* Livello min. per scrivere  */
        long fmnum;             /* file messaggi              */
	long maxmsg;            /* numero messaggi nella room */
        long highmsg;           /* numero messaggio piu' alto */
        long highloc;           /* numero LOCALE piu' alto    */
        time_t ctime;           /* Creation time              */
        time_t mtime;           /* Last Modification time     */
        time_t ptime;           /* Last Post time             */
        unsigned long flags;    /* Flag di configurazione     */
	long def_utr;           /* Flag di default per gli utenti */
        long master;            /* matricola del Room-Master  */
};

/* Struttura che permette di creare liste bidirezionali di room.    */
struct room {
	long pos;                   /* Posizione nella lista        */
	bool lock;                  /* Lock dei post                */
        struct room_data *data;     /* Room Data                    */
	struct msg_list *msg;       /* Messaggi nella room          */
        struct room *prev, *next;   /* Room precedente e successiva */
#ifdef USE_FLOORS
	long floor_num;             /* Numero del floor             */
	struct f_room_list *f_march; /* Lista room nel floor        */
#endif
};

typedef struct dfr {
        long *user;           /* # matricola utente                   */
        long *fr;             /* fullroom per l'utente corrispondente */
        unsigned long *flags; /* flags per l'utente corrispondente    */
        long num;             /* Numero di slot usate                 */
        long alloc;           /* Lunghezza del vettore allocato       */
        bool dirty;
} dfr_t;

/* Struttura per memorizzare la lista di room                       */
struct room_list {
	struct room *first;
	struct room *last;
};

struct msg_list {
	long *num;
	long *pos;
	long *local;
#if 0        
        /* NON USO FM multipli */
        long *fmnum;  /* File message number, only for virtual rooms   */
#endif
        long highmsg;           /* numero messaggio piu' alto          */
        long highloc;           /* numero LOCALE piu' alto             */
        long last_poster;       /* Matricola autore ultimo post        */
        dfr_t *dfr;   /* Puntatore al fullroom dinamico, solo per blog */
        bool dirty;
};

/* Variabile globale con la lista delle room. */
extern struct room_list lista_room;
/* Numeri e nomi delle rooms principali */
extern char lobby[];
extern char sysop_room[];
extern char aide_room[];
extern char ra_room[];
extern char mail_room[];
extern char twit_room[];
extern char dump_room[];
extern char find_room[];
extern char blog_room[];

#include "strutture.h"

/* Prototipi delle funzioni in rooms.c */
void room_load_data(void);
void room_save_data(void);
void room_free_list(void);
void room_free(struct room *punto);
void room_delete(struct room *room);
struct room * room_find(char *nome);
struct room * room_findp(struct sessione *t, char *nome);
long room_create(char *name, long fmnum, long maxmsg, char type);
void room_create_basic(void);
int room_known(struct sessione *t, struct room *r);
int room_guess(struct sessione *t, struct room *r, char *nome);
struct room * room_findn(long num);
void room_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr);
int room_swap(long rnum1, long rnum2);
void room_empty(struct room *r, struct sessione *t);

struct msg_list * msg_create(long maxmsg);
void msg_load_data(struct msg_list *msg, char *filename, long maxmsg);
void msg_save_data(struct msg_list *msg, char *filename, long maxmsg);
void msg_free(struct msg_list *msg);
void msg_resize(struct room *room, long newsize);
void msg_scroll(struct msg_list *msg, long nslots);
long msg_pos(struct room *room, long msgnum);
long msg_loc(struct room *room, long msgnum);
void msg_del(struct room *room, long msgnum);
void msg_move(struct room *dest, struct room *source, long msgnum);
void msg_copy(struct room *dest, struct room *source, long msgnum);

#ifdef USE_CESTINO
void msg_dump(struct room *room, long msgnum);
int msg_undelete(struct room *room, long msgnum, char *roomname);
#endif

long get_highloc(struct sessione *t);
long get_highloc_room(struct sessione *t, struct room *r);
void set_highloc(struct sessione *t, long highloc);
long inc_highloc(struct sessione *t);
long get_highmsg(struct sessione *t);
long get_highmsg_room(struct sessione *t, struct room *r);
void set_highmsg(struct sessione *t, long highmsg);

#endif /* rooms.h */
