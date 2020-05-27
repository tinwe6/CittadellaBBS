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
* File : conn.h                                                             *
*        Gestione sockets                                                   *
****************************************************************************/
#ifndef _CONN_H
#define _CONN_H   1

#include "cittaclient.h"
#include "metadata.h"
#include "text.h"
#include <string.h>

/* Valori di ritorno di elabora_input() */
#define CMD_ESEGUITO       (1 << 0)
#define CMD_IN_CODA        (1 << 1)
#define CMD_BINARY         (1 << 2)
#define CMD_POST_TIMEOUT   (1 << 3)
#define NO_CMD             (1 << 4)

struct serv_buffer {
    char * data;
    size_t start, end, size;
};


/* prototipi delle funzioni in conn.c */
void parse_opt(int argc, char **argv, char **rcfile, bool *no_rc);
void crea_connessione(char *host, unsigned int port);
int conn_server(char *h, int p);
void serv_gets(char *strbuf);
char * serv_fetch(void);
void serv_fetch_text(struct text *txt, Metadata_List *mdlist);
int serv_getc_r(struct serv_buffer * buf);
void serv_puts(const char *string);
void serv_putf(char *format, ...);
void serv_read(char *buf, int bytes);
int serv_getc(void);
int elabora_input(void);
void serv_write(const char *buf, int bytes, bool progressbar);

bool prendi_da_coda(struct coda_testo *coda, char **dest);
bool prendi_da_coda_t(struct coda_testo *coda, char **dest, int mode);
void metti_in_coda(struct coda_testo *coda, char *txt);

/* variabili globali di conn.c */
extern int serv_sock;  /* file descr del socket di connessione al server */
extern const char *str_conn_closed;
extern int server_input_waiting; /* TRUE se dopo un elabora_input() rimane */
                                 /* altro input da server da elaborare     */

#endif /* conn.h */
