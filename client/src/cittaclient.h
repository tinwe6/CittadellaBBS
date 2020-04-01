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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cittaclient.h                                                      *
*        main(), ciclo base del client                                      *
****************************************************************************/
#ifndef _CITTACLIENT_H
#define _CITTACLIENT_H   1

#define _CITTACLIENT_

#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include "strutt.h"
#include "floor_flags.h"
#include "room_flags.h"
#include "user_flags.h"
#include "utente_client.h"


/* Tempo in secondi tra due keep alive (NOOP) inviati al server */
#define KEEPALIVE_INTERVAL 60

/* Dimensione buffer di caratteri standard */
#define LBUF        256

/* Per il gettext() */
#define _(A) A

#define IFNEHAK { if (!(uflags[0] & UT_ESPERTO)) { \
                             printf("\n");         \
                             hit_any_key();      } }

/* size in bytes of flag sets */
#define SIZE_FLAGS 8

/* variabili globali in cittaclient.c */
extern struct serverinfo serverinfo;
extern struct coda_testo comandi;
extern bool local_client;            /* TRUE se client locale, FALSE: remoto */
extern char nome[MAXLEN_UTNAME];     /* Nickname dell'utente                 */
extern int livello;                  /* Livello dell'utente                  */
extern bool ospite;                  /* L'utente e' un ospite?               */
extern uint8_t uflags[SIZE_FLAGS];   /* Flags di configurazione utente       */
extern uint8_t sflags[SIZE_FLAGS];   /* Flags di sistema dell'utente         */
extern long rflags;                  /* Flags della room corrente            */
extern long utrflags;                /* Flags UTR per la room corrente       */
extern int sesso;                    /* Sesso dell'utente                    */
extern char last_profile[MAXLEN_UTNAME]; /* Utente di default per profile    */
extern char last_mail[MAXLEN_UTNAME]; /* Utente di default per mail          */
extern char last_x[MAXLEN_UTNAME];   /* Utente di default per Xmsg           */
extern char last_xfu[MAXLEN_UTNAME]; /* Utente di default per Follow-up Xmsg */
extern int  riga;
extern char canale_chat;             /* n. canale, o 0 se utente non in chat */
extern jmp_buf ciclo_principale;
extern bool login_eseguito;
extern char oa[];                          /* oa[sesso] e' la lettera finale */
extern char current_floor[FLOORNAMELEN];   /* Floor corrente                 */
/* extern char current_room[ROOMNAMELEN]; */
extern char current_room[LBUF];
extern char room_type[];
extern bool cestino;
extern bool find_vroom;  /* TRUE se in room virtuale con i risultati ricerca */
extern bool blog_vroom;  /* TRUE se in room virtuale dei blog                */
/* extern bool debug_mode; */  /* TRUE se in debug mode          */

/* Riferimento a post */
extern char postref_room[];
extern long postref_locnum;

/* pulisci_ed_esci arg */
typedef enum {
        NO_EXIT_BANNER,
        SHOW_EXIT_BANNER,
} exit_banner;

/* prototipi delle funzioni in cittaclient.c */
char * interpreta_tilde_dir(const char *buf);
void pulisci_ed_esci(exit_banner show_banner);

#endif /* cittaclient.h */
