/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File: strutture.h                                                         *
*       strutture principali per gestione connessioni e files               *
****************************************************************************/
#ifndef _STRUTTURE_H
#define _STRUTTURE_H    1
#include <time.h>
#include <unistd.h>
#include "compress.h"
#include "config.h"
#include "coda_testo.h"
#include "text.h"
#include "file_messaggi.h"
#include "utente.h"
#include "rooms.h"
#include "parm.h"
#include "messaggi.h"
#include "socket_citta.h"

#define WS_DAYS 7                  /* Statistica settimanale: 7 dati  */

struct dati_idle {
	int cicli;
	int min;
	int ore;
	char warning;
};

struct sessione {
  int    socket_descr;               /* file descriptor del socket     */
  struct dati_ut *utente;            /* struttura contenuta nel file   */
  char   host[LBUF];                 /* nome dell'host di provenienza  */
  long   ora_login;                  /* ora di login                   */
  bool   logged_in;                  /* eseguito il login              */
  int    stato;                      /* stato della connessione        */
  bool   cancella;                   /* sessione da cancellare         */
  int    occupato;                   /* editing o altro...             */
  char   canale_chat;                /* ascolta canale di chat #       */
  unsigned long chat_timeout;        /* Timeout per la chat            */
  int    num_rcpt;                   /* Numero destinatari salvati     */
  long   rcpt[MAX_RCPT];             /* matricole dei destinatari      */
  int    attesa;                     /* per quanti cicli aspetta       */
  char   *doing;                     /* doing field                    */
  /* Variabili per la gestione delle room.                             */
  struct room *room;                 /* Puntatore alla room corrente   */
  long   lastread;                   /* num ultimo msg letto in room   */
  long   newmsg;                     /* Fullrm della room corrente     */
  char   rdir;                       /* Direzione di lettura (+/-1)    */
  long   *fullroom;                  /* Ultimo messaggio letto / room  */
  long   *room_flags;                /* Flags associati alle room      */
  long   *room_gen;                  /* Flags associati alle room      */
  long   reply_num;                  /* Local # msg a cui si replica   */
  char   reply_to[MAXLEN_UTNAME];    /* autore ultimo post letto       */
  char   reply_subj[MAXLEN_SUBJECT]; /* subject ultimo post letto      */
  long   rlock_timeout;              /* timeout per le room con lock   */
  long   last_msgnum;                /* msgnum dell'ultimo post inviato*/
  int    upload_bookings;            /* Numero prenotazioni per il post*/
#ifdef USE_FLOORS
  struct floor *floor;               /* Floor corrente                 */
#endif 
  /* Variabili per la gestione dei mail.                               */
  struct msg_list *mail;             /* Lista dei mail                 */
#ifdef USE_BLOG
  /* Variabili per la gestione dei blog.                               */
  struct msg_list *blog;             /* Lista dei blog                 */
  long *blog_fullroom;               /* Fullroom per i blog            */
#endif
  /* Variabili per la trasmissione dei parametri di un comando         */
  struct parameter *parm;            /* lista coi parametri in arrivo  */
  int parm_com;                      /* Comando associato ai parametri */
  int parm_num;                      /* Numero parametri ricevuti      */
  /* Statistiche di uso                                                */
  unsigned long bytes_in;            /* Bytes ricevuti dal client      */
  unsigned long bytes_out;           /* Bytes inviati al client        */
  unsigned long num_cmd;             /* Numero comandi ricevuti        */
#ifdef USE_IMAP4
  char tag[LBUF];                    /* tag of current client command  */
#endif
  /* Trasmissione in binario                                           */
  FILE *fp;                          /* file descriptor per l'upload   */
  unsigned long binsize;             /* bytes in bin da leggere        */
  /* Variabili per la manipolazione di testi.                          */
  int    riga;                       /* contatore strutt. precedente   */
  struct text *text;                 /* Testo in arrivo da elaborare   */
  long   text_max;                   /* Lunghezza massima del testo    */ 
  int    text_com;                   /* Comando associato al testo     */
#ifdef USE_STRING_TEXT
  char   *txt;                       /* Array di char per i testi      */
  size_t len;                        /* Lunghezza dell'array txt       */
#endif
  /* Variabili per gestire la compressione del socket.                 */
  char   compressione[MAXLEN_COMPRESS]; /* tipo compressione           */
#if ALLOW_COMPRESSION
  void * compress_data;
  void * zlib_data;
  char   compressing;
#endif
  struct iobuf iobuf;                /* Buffer input/output            */
  struct coda_testo input;           /* input da elaborare             */
  struct coda_testo output;          /* output da mandare al client    */
  struct dati_idle idle;             /* da quanto non si hanno segnali */  
  struct sessione *prossima;         /* puntatore sessione successiva  */
};

/* Questa struttura contiene le statistiche sull'uso del server        */
struct dati_server {
  long server_runs;  /* Numero di volte che il server e' partito  */
  long matricola;    /* Ultimo # matricola assegnato              */
/*
 * connessioni: incrementato da nuova_conn() quando viene creata la conn.
 * local_cl   : incrementato da cmd_info() quando la connessione e remota.
 * remote_cl  : idem quando la connessione e' locale.
 * login, ospiti, nuovi_ut: aggiornati in cmd_usr1() a login avvenuto.
 */
  long connessioni;  /* Numero totale di connessioni              */
  long local_cl;     /* Numero di connessioni con client locale   */
  long remote_cl;    /* Numero di connessioni con client remoto   */
  long login;        /* Numero login al server                    */
  long ospiti;       /* Numero di login da parte di ospiti        */
  long nuovi_ut;     /* Numero di nuovi utenti avuti              */
  long validazioni;  /* Numero di validazioni eseguite            */
  long max_users;    /* Record di connessioni contemporanee       */
  time_t max_users_time; /* Data e ora del record                 */
  /* Statistiche connessioni al CittaWeb                          */
  long http_conn;    /* Numero totale di connessioni http         */
  long http_conn_ok; /* Numero connessioni servite correttamente  */
  long http_room;    /* Richieste http di lettura di una room     */
  long http_userlist;/* Richiesta http della lista degli utenti   */
  long http_help;    /* Richieste http di lettura file di help    */
  long http_profile; /* Richieste http di profile di un utente    */
  long http_blog;    /* Richieste http di lettura di un blog      */
  /* Operazioni eseguite                                          */
  long X;            /* Numero di X inviati                       */
  long broadcast;    /* Numero di broadcast inviati               */
  long mails;        /* Numero di mail inviati                    */
  long posts;        /* Numero di posts immessi                   */
  long chat;         /* Numero messaggi in chat                   */
  long blog;         /* Numero di blog immessi                    */
  long referendum;   /* Numero referendum                         */
  long sondaggi;     /* Numero sondaggi                           */
  /* File messaggi                                                */
  long fm_num;       /* Ultimo numero di file messaggi assegnato  */
  long fm_curr;      /* Numero di file messaggi presenti          */
  long fm_basic;     /* File messaggi assegnato alle room di base */
  long fm_mail;      /* File messaggi assegnato alle mailbox      */
  long fm_normal;    /* File messaggi di default per le altre room*/
  /* Room                                                         */
  long room_num;     /* Ultima matricola di room assegnata        */
  long room_curr;    /* Numero di room presenti                   */
  long utr_nslot;    /* Numero di slots allocate per dati UTR     */
  long mail_nslot;   /* Numero di slots allocate per lista mail   */
  long mailroom;     /* Numero della room adibita a mailroom      */
  long baseroom;     /* Numero della room adibita a baseroom      */
  /* Blog                                                         */
  long blog_nslot;   /* Numero di slots allocate per lista mail   */
  long fm_blog;      /* File messaggi assegnato ai blog           */
  /* Floors                                                       */
  long floor_num;    /* Ultima matricola di floor assegnata       */
  long floor_curr;   /* Numero di floor presenti                  */
  /* Statistiche sui collegamenti                                 */
  long stat_ore[24];   /* Distribuzione login nella giornata      */
  long stat_giorni[7]; /* Distribuzione login nella settimana     */
  long stat_mesi[12];  /* Distribuzione login nei mesi            */
  /* Statistiche di uso del server settimanale (week stats).      */
  time_t ws_start;              /* data inizio statistiche settimanali       */
  long ws_connessioni[WS_DAYS]; /* Numero totale di connessioni              */
  long ws_local_cl[WS_DAYS];    /* Numero di connessioni con client locale   */
  long ws_remote_cl[WS_DAYS];   /* Numero di connessioni con client remoto   */
  long ws_login[WS_DAYS];       /* Numero login al server                    */
  long ws_ospiti[WS_DAYS];      /* Numero di login da parte di ospiti        */
  long ws_nuovi_ut[WS_DAYS];    /* Numero di nuovi utenti avuti              */
  long ws_validazioni[WS_DAYS]; /* Numero di validazioni eseguite            */
  long ws_X[WS_DAYS];           /* Numero di X inviati                       */
  long ws_broadcast[WS_DAYS];   /* Numero di broadcast inviati               */
  long ws_mails[WS_DAYS];       /* Numero di mail inviati                    */
  long ws_posts[WS_DAYS];       /* Numero di posts immessi                   */
  long ws_read[WS_DAYS];        /* Numero di post letti                      */
  long ws_chat[WS_DAYS];        /* Numero messaggi in chat                   */
};

#endif /* strutture.h */
