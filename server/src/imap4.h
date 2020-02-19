/*
 *  Copyright (C) 2003-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: imap4.h                                                               *
*       gestione delle richieste IMAP4 al cittaserver                         *
******************************************************************************/
#ifndef _IMAP4_H
#define _IMAP4_H   1

#include "config.h"

#ifdef USE_IMAP4

#include "strutture.h"

struct imap4_msg {
	size_t size;
	long id;
	int mark_deleted;
	struct text *msg;
};

/* IMAP4 session structure                                              */
struct imap4_sess {
	int desc;                     /* File descriptor del socket     */
	int stato;                    /* Stato della connessione        */  
	int finished;                 /* Sessione terminata.            */
        char host[LBUF];              /* nome dell'host di provenienza  */
	struct dati_ut * utente;      /* Dati utente                    */
	long *fullroom;               /* Ultimo messaggio letto / room  */
	long *room_flags;             /* Flags associati alle room      */
	long *room_gen;               /* Flags associati alle room      */

	struct msg_list *mail_list;
	long mail_highmsg;
	long mail_highloc;


	struct coda_testo out_header;
	unsigned int length;          /* Body length                    */
	char  in_buf[MAX_STRINGA];
	char out_buf[MAX_STRINGA];
	long conntime;
	struct coda_testo input;     /* input da elaborare             */
	struct coda_testo output;    /* output da mandare al client    */
	struct dati_idle idle;
	struct imap4_sess *next;

#ifdef USE_STRING_TEXT
        char   *txt;                 /* Array di char per i testi      */
        size_t len;                  /* Lunghezza dell'array txt       */
#endif

	struct imap4_msg *msg;
	long mail_num;
	long mail_size;
	long deleted;

        char tag[LBUF];               /* tag of current client command  */
};

extern struct imap4_sess *sessioni_imap4;
extern int imap4_maxdesc;

int imap4_nuovo_descr(int s);
void imap4_chiudi_socket(struct sessione *h);
void imap4_chiusura_server(int s);
int imap4_elabora_input(struct sessione *t);
void imap4_interprete_comandi(struct sessione *p, char *command);
void imap4_kill_user(char *ut);

/*
int imap4_nuova_conn(int s);
*/

#endif /* USE_IMAP4 */
#endif /* imap4.h */
