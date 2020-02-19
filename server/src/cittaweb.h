/*
 *  Copyright (C) 2002-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                     (C) M. Caldarelli  *
*                                                              G. Garberoglio *
*                                                                             *
* File: cittaweb.c                                                            *
*       gestione delle richieste HTTP al cittaserver                          *
******************************************************************************/
#ifndef _CITTAWEB_H
#define _CITTAWEB_H   1

#include "config.h"

#ifdef USE_CITTAWEB

#include "strutture.h"

struct http_sess {
  int desc;
  char host[256];                /* nome host di provenienza */
  long conntime;
  int finished;
  struct coda_testo input;
  struct coda_testo output;
  struct coda_testo out_header;
  struct iobuf iobuf;
  unsigned int length;           /* Body length */
  struct dati_idle idle;
  struct http_sess *next;
};

extern struct http_sess *sessioni_http;
extern int http_maxdesc;

int http_nuovo_descr(int s);
void http_chiudi_socket(struct http_sess *h);
void http_chiusura_cittaweb(int s);
int http_elabora_input(struct http_sess *t);
void http_interprete_comandi(struct http_sess *p, char *command);

/*
int http_nuova_conn(int s);
*/

#endif /* USE_CITTAWEB */
#endif /* cittaweb.h */
