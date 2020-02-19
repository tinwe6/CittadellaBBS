/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
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
* File: coda_testo.h                                                          *
*       gestione di code di stringhe                                          *
******************************************************************************/
#ifndef _CODA_TESTO_H
#define _CODA_TESTO_H   1

#include <stdbool.h>
#include "config.h"

struct blocco_testo {
        bool binary;           /* TRUE se si invia in binario */
        size_t len;            /* lunghezza stringa o dati    */
	char *testo;           /* stringa o dati da inviare   */
	struct blocco_testo *prossimo;
};

struct coda_testo {
	struct blocco_testo *partenza;
	struct blocco_testo *termine;
};

struct iobuf {
  char   in[MAX_STRINGA];            /* buffer per input da elaborare  */
  char   out[MAX_STRINGA];           /* buffer per output              */
  size_t ilen;                       /* Lunghezza dei buffer di i/o    */
  size_t olen;
};

#include "strutture.h"

/* Prototipi delle funzioni in coda_testo.c */
int prendi_da_coda(struct coda_testo *coda, char *dest, size_t n);
int prendi_da_coda_o(struct coda_testo *coda, struct iobuf *buf);
void metti_in_coda(struct coda_testo *coda, char *txt, size_t len);
int metti_in_coda_bin(struct coda_testo *coda, char *buf, size_t len);
void flush_coda(struct coda_testo *coda);
void merge_code(struct coda_testo *coda1, struct coda_testo *coda2);
void init_coda(struct coda_testo *coda);
void iobuf_init(struct iobuf *buf);
size_t iobuf_ilen(struct iobuf *buf);
size_t iobuf_olen(struct iobuf *buf);

#endif /* coda_testo.h */
