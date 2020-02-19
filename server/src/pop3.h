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
* File: pop3.h                                                                *
*       gestione delle richieste POP3 al cittaserver                          *
******************************************************************************/
#ifndef _POP3_H
#define _POP3_H   1

#include "config.h"

#ifdef USE_POP3

#include "strutture.h"

struct pop3_msg {
	size_t size;
	long id;
	int mark_deleted;
	struct text *msg;
};

/* POP3 session structure                                               */
struct pop3_sess {
	int desc;                     /* File descriptor del socket     */
	int stato;                    /* Stato della connessione        */  
	int finished;                 /* Sessione terminata.            */
        char host[LBUF];              /* nome dell'host di provenienza  */
	struct dati_ut * utente;      /* Dati utente                    */
	long *fullroom;               /* Ultimo messaggio letto / room  */
	long *room_flags;             /* Flags associati alle room      */
	long *room_gen;               /* Flags associati alle room      */

	struct msg_list *mail_list;

	long mail_num;
	long mail_size;
	long deleted;

	struct pop3_msg *msg;

	struct coda_testo out_header;
	unsigned int length;          /* Body length                    */
	char  in_buf[MAX_STRINGA];
	char out_buf[MAX_STRINGA];
	long conntime;
	struct coda_testo input;     /* input da elaborare             */
	struct coda_testo output;    /* output da mandare al client    */
	struct dati_idle idle;
	struct pop3_sess *next;

#ifdef USE_STRING_TEXT
        char   *txt;                 /* Array di char per i testi      */
        size_t len;                  /* Lunghezza dell'array txt       */
#endif
};

extern struct pop3_sess *sessioni_pop3;
extern int pop3_maxdesc;

int pop3_nuovo_descr(int s);
void pop3_chiudi_socket(struct pop3_sess *h);
void pop3_chiusura_server(int s);
int pop3_elabora_input(struct pop3_sess *t);
void pop3_interprete_comandi(struct pop3_sess *p, char *command);
void pop3_kill_user(char *ut);

/*
int pop3_nuova_conn(int s);
*/

#endif /* USE_POP3 */
#endif /* pop3.h */
