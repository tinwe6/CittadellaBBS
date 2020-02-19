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
* File : file_messaggi.h                                                    *
*        Funzioni per creare, cancellare, configurare, gestire i file       *
*        messaggi e per scriverci o leggerci i post.                        *
****************************************************************************/
#ifndef _FILE_MESSAGGI_H
#define _FILE_MESSAGGI_H   1
#include "config.h"
#include "msg_flags.h"
#include "text.h"
#include <time.h>

/* La dimensione iniziale del file messaggi si trova in DFLT_FM_SIZE,
 * definita nel file config.h: #define DFLT_FM_SIZE  100000           */

/*
 * I caratteri >= a FM_MSGINIT segnalano l'inizio di un messaggio.
 * Attualmente ho: 255 messaggio normale, 254 messaggio cancellato.
 */
#define FM_MSGINIT     254
#define FM_MSG_NORMAL  255
#define FM_MSG_DELETED 254

/* Errori standard file messaggi (definiti in msg_flags.h) */
#if 0
#define FMERR_NOFM         1
#define FMERR_ERASED_MSG   2
#define FMERR_NO_SUCH_MSG  3
#define FMERR_OPEN         4
#define FMERR_DELETED_MSG  5
#define FMERR_CHKSUM       6
#define FMERR_MSGNUM       7
#endif

/*
 * Struttura contentente i dati di un file messaggi.
 */
struct file_msg {
        long num;                /* File message number              */
        long size;               /* Size in bytes of file message    */
        long lowest;             /* Lowest message number in fm      */
        long highest;            /* Highest message number in fm     */
        long posizione;          /* Position of next message         */
        long nmsg;               /* Number of messages in fm         */
        long ndel;               /* Number of deleted messages in fm */
        unsigned long flags;     /* Flags associated to the fm       */
	time_t ctime;            /* Creation time                    */
        char fmck;               /* Checksum for file message        */
        char desc[64];           /* Short description of fm          */
};

/*
 * Struttura che permette di creare liste delle strutture dati dei
 * file messaggi.
 */
struct fm_list {
        struct file_msg fm;
        struct fm_list *next;
};


/* Prototipi funzioni in file_messaggi.c */
void fm_load_data(void);
void fm_save_data(void);
long fm_create(long fm_size, char *desc);
char fm_delete(long fmnum);
char fm_resize(long fmnum, long newsize);
char fm_expand(long fmnum, long newsize);
void fm_check(void);
void fm_mantain(void);
void fm_rebuild(void);
#ifdef USE_STRING_TEXT
char fm_get(long fmnum, long msgnum, long msgpos, char **txt, size_t *len,
	    char *header);
long fm_put(long fmnum, char *txt, size_t len, char *header, long *msgpos);
#else
char fm_get(long fmnum, long msgnum, long msgpos, struct text *txt,
	    char *header);
long fm_put(long fmnum, struct text *txt, char *header, long *msgpos);
#endif
char fm_delmsg(long fmnum, long msgnum, long msgpos);
char fm_getheader(long fmnum, long msgnum, long msgpos, char *header);
char fm_headers(long fmnum, struct text *txt);
struct file_msg * fm_find(long fmnum);
long fm_lowest(long fmnum);

/* Definizione variabili globali */
extern struct fm_list *lista_fm;
extern struct fm_list *ultimo_fm;

/***************************************************************************/

#endif /* file_messaggi.h */
