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
* File: coda_testo.c                                                          *
*       gestione di code di stringhe                                          *
******************************************************************************/
#include <string.h>
#include "coda_testo.h"
#include "macro.h"
#include "memstat.h"
#include "utility.h"

/* Mette una stringa txt di lunghezza len in coda */
void metti_in_coda(struct coda_testo *coda, char *txt, size_t len)
{
	struct blocco_testo *nuovo;

	CREATE(nuovo, struct blocco_testo, 1, TYPE_BLOCCO_TESTO);
	CREATE(nuovo->testo, char, len+1, TYPE_CHAR);
	memcpy(nuovo->testo, txt, len);
        nuovo->testo[len] = '\0';
        nuovo->len = len;
        nuovo->binary = false;

	/* La coda e' vuota? */
	if (coda->partenza) {
		coda->termine->prossimo = nuovo;
		coda->termine = nuovo;
	} else
		coda->partenza = coda->termine = nuovo;
	nuovo->prossimo = NULL;
}

/* Mette una stringa txt di lunghezza len in coda */
int metti_in_coda_bin(struct coda_testo *coda, char *buf, size_t len)
{
	struct blocco_testo *nuovo;

	CREATE(nuovo, struct blocco_testo, 1, TYPE_BLOCCO_TESTO);
	CREATE(nuovo->testo, char, len, TYPE_CHAR);
	memcpy(nuovo->testo, buf, len);
        nuovo->len = len;
        nuovo->binary = true;

	/* La coda e' vuota? */
	if (coda->partenza) {
		coda->termine->prossimo = nuovo;
		coda->termine = nuovo;
	} else
		coda->partenza = coda->termine = nuovo;
	nuovo->prossimo = NULL;

        return len;
}

/* Prende una riga di testo dalla coda e la mette in dest (max n caratteri,
 * compreso lo '\0' di terminazione.
 * Restituisce il numero di bytes inseriti in dest.                        */
int prendi_da_coda(struct coda_testo *coda, char *dest, size_t n)
{
	struct blocco_testo *tmp;

	/* La coda e' vuota? */
	if (coda->partenza == NULL)
		return 0;

	tmp = coda->partenza;
        if (tmp->binary) { // TODO sistemare
                strncpy(dest, coda->partenza->testo, n-1);
                dest[n-1] = '\0';
                coda->partenza = coda->partenza->prossimo;
        } else {
                strncpy(dest, coda->partenza->testo, n-1);
                dest[n-1] = '\0';
                coda->partenza = coda->partenza->prossimo;
        }

	Free(tmp->testo);
	Free(tmp);

	return strlen(dest);
}

/* Prende da coda e mette nel buffer di partenza */
int prendi_da_coda_o(struct coda_testo *coda, struct iobuf *buf)
{
	struct blocco_testo *tmp;
        size_t tmplen = 0;
        size_t n;

	/* La coda e' vuota? */
	if (coda->partenza == NULL)
		return 0;

	tmp = coda->partenza;
        tmplen = tmp->len;
        n = MAX_STRINGA - buf->olen;

        if (tmp->binary) { /* TODO non sembra esserci diff tra testo e bin */
                if (tmp->len > n) {
                        tmplen = n;
                }
                //citta_logf("PRENDO DA CODA %ld/%ld bytes in bin (MS %ld, olen %ld)", tmplen, tmp->len, MAX_STRINGA, buf->olen);
                memcpy(buf->out + buf->olen, tmp->testo, tmplen);
                buf->olen += tmplen;
                if (tmp->len < tmplen) {
                        /* non dovrebbe capitare mai */
                        citta_log("SYSERR prendi_da_coda_0.");
		}
                tmp->len -= tmplen;

                if (tmp->len > 0) { /* Rimane roba da trattare */
                        memmove(tmp->testo, tmp->testo+tmplen, tmp->len);
                        return tmplen;
                }
        } else { /* Invia del testo */
                if (tmplen >= n)
                        tmplen = n-1;
                //citta_logf("PRENDO DA CODA %ld/%ld bytes in ascii", tmplen, tmp->len);
                memcpy(buf->out + buf->olen, tmp->testo, tmplen);
                buf->olen += tmplen;
                if (tmp->len < tmplen) {
                        /* non dovrebbe capitare mai */
                        citta_log("SYSERR prendi_da_coda_0.");
		}
                tmp->len -= tmplen;
                buf->out[buf->olen] = '\0';
		if (tmp->len > 0) { /* Rimane roba da trattare */
                        memmove(tmp->testo, tmp->testo+tmplen, tmp->len);
                        return tmplen;
                }
        }

	coda->partenza = coda->partenza->prossimo;

	Free(tmp->testo);
	Free(tmp);

	return tmplen;
}


/* Butta il primo elemento della coda */
int elimina_da_coda(struct coda_testo *coda)
{
	struct blocco_testo *tmp;

	/* La coda e' vuota? */
	if (coda->partenza == NULL)
		return 0;

	tmp = coda->partenza;
	coda->partenza = coda->partenza->prossimo;

	Free(tmp->testo);
	Free(tmp);

	return 1;
}

void flush_coda(struct coda_testo *coda)
{
	while (elimina_da_coda(coda))
	        ;
        coda->partenza = NULL;
        coda->termine = NULL;
}

/* Mette la coda2 in coda a coda1 */
void merge_code(struct coda_testo *coda1, struct coda_testo *coda2)
{
	if (coda2->partenza == NULL)
		return;
	if (coda1->partenza == NULL)
		coda1->partenza = coda2->partenza;
	else
		coda1->termine->prossimo = coda2->partenza;
	coda1->termine = coda2->termine;
	coda2->partenza = NULL;
	coda2->termine = NULL;
}

void init_coda(struct coda_testo *coda)
{
        coda->partenza = NULL;
        coda->termine = NULL;
}
/****************************************************************************/
void iobuf_init(struct iobuf *buf)
{
        buf->in[0] = 0;
        buf->out[0] = 0;
        buf->ilen = 0;
        buf->olen = 0;
}

size_t iobuf_ilen(struct iobuf *buf)
{
        return buf->ilen;
}

size_t iobuf_olen(struct iobuf *buf)
{
        if (buf)
                return buf->olen;
        citta_log("ERROR: iobuf_olen called with null pointer");
        return 0;
}

#if 0
void invia_output(sessione *t)
{
        size_t sent = 0;

        //        sent += scrivi_a_desc(t->socket_descr, t->iobuf->);
}
#endif
