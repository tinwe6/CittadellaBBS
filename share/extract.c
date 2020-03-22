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
* Cittadella/UX Library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: extract.c                                                           *
*       Funzioni per estrarre argomenti dalle stringhe.                     *
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>

#include "extract.h"
#include "macro.h"

#define LBUF 256 /* DA SPOSTARE */

/* Prototipi funzioni in questo file */
int num_parms(const char *source);
void extract(char *dest, const char *source, int parmnum);
void extractn(char *dest, const char *source, int parmnum, size_t len);
char * extracta(const char *source, int parmnum);
int extract_int(const char *source, int parmnum);
long extract_long(const char *source, int parmnum);
unsigned long extract_ulong(const char *source, int parmnum);
void clear_parm(char *source, int parmnum);

#if 0  /* not used */
static void *strccpy(void *dest, const void *src, int ucharstop, size_t size);
#endif

/****************************************************************************/
/*
 * Determina il numero di parametri in una stringa
 * Il numero di parametri e' il numero di '|' piu' uno nella stringa.
 * Questo significa che una stringa con un '|' iniziale ha un parametro
 * iniziale nullo, e analogamente una stringa che termina con '|' ha un
 * parametro finale nullo.
 */
int num_parms(const char *source)
{
        const char *src;
        int count = 1;

	src = source;
	while (*src)
		if (*src++ == '|')
			count++;
	return count;
}

/*
 * Estrae una sottostringa separata da "|"
 *
 * Restituisce in dest la sottostringa numero parnum che trova in source
 * separata da '|'. La prima sottostringa e' la numero 0. Una sottostringa
 * che e' in "||" e' vuota, e una sottostringa vuota puo' esserci sia
 * all'inizio che alla fine della stringa. Se parmnum > numero di parametri
 * in source, restituisce "".
 * In ogni caso la stringa copiata in dest e' sempre di lunghezza inferiore
 * a LBUF = 256 caratteri, ma nessun altro controllo viene eseguito.
 * USARE PERCIO' SEMPRE extractn() quando si estraggono parametri in buffer
 * che possono essere di lunghezza inferiore!!!
 *
 * Esempio: source = "|1||3|"
 *          parmnum = 0 --> NULL
 *          parmnum = 1 --> "1"
 *          parmnum = 2 --> NULL
 *          parmnum = 3 --> "3"
 *          parmnum = 4 --> NULL
 */
void extract(char *dest, const char *source, int parmnum)
{
	extractn(dest, source, parmnum, LBUF);
}


/*
 * Come extract() ma estrae al piu' 'len' caratteri da source.
 * Il risultato in dest e' sempre una stringa che termina con '\0'.
 * Vengono copiati in dest al piu' len caratteri, comprensivi dello '\0'
 * finale.
 * Almeno nel server, andrebbe SEMPRE USATA QUESTA, per evitare dei buffer
 * overflow.
 */
void extractn(char *dest, const char *source, int parmnum, size_t len)
{
	const char *src;
	const char *end;

	if (len <= 0)
		return;

	src = source;
        while (*src && parmnum) {
		if (*(src++) == '|')
			parmnum--;
        }
	if (*src == '\0') {
		*dest = '\0';
		return;
	}
	end = src;
	while(*end && (*end != '|') && (end < src + len - 1))
		end++;
	memcpy(dest, src, end - src);
	dest[end - src] = '\0';
}

/*
 * Come extract() ma estrae in una stringa allocata dinamicamente con
 * CREATE(), allo stesso modo di citta_strdup().
 * Il risultato in dest e' sempre una stringa che termina con '\0'.
 */
char * extracta(const char *source, int parmnum)
{
	const char *src;
	const char *end;
	char *dest;

	src = source;
        while (*src && parmnum) {
		if (*(src++) == '|')
			parmnum--;
        }
	if (*src == '\0')
		return citta_strdup("");
	end = src;
	while (*end && (*end!='|'))
		end++;

	CREATE(dest, char, end-src+1, 0);
	memcpy(dest, src, end-src);
	dest[end-src] = '\0';
	return dest;
}

/*
 * Restituisce un bool estraendolo come parnumesimo parametro in source
 * Nota: restituisce false se il parametro e' 0, true altrimenti.
 */
bool extract_bool(const char *source, int parmnum)
{
        char buf[LBUF];

        extract(buf, source, parmnum);
        return (strtol(buf, NULL, 10) != 0);
}

/*
 * Restituisce un int estraendolo come parnumesimo parametro in source
 */
int extract_int(const char *source, int parmnum)
{
        char buf[LBUF];

        extract(buf, source, parmnum);
        return(strtol(buf, NULL, 10));
}

/*
 * Restituisce un long estraendolo come parnumesimo parametro in source
 */
long extract_long(const char *source, int parmnum)
{
        char buf[LBUF];

        extract(buf, source, parmnum);
        return(strtol(buf, NULL, 10));
}

/*
 * Restituisce un unsigned long estraendolo come parnumesimo param in source
 */
unsigned long extract_ulong(const char *source, int parmnum)
{
        char buf[LBUF];

        extract(buf, source, parmnum);
        return(strtoul(buf, NULL, 10));
}

/*
 * Svuota il parametro parmnumesimo da source
 */
void clear_parm(char *source, int parmnum)
{
	int count = 0, n;
	char *tmp, *tmp1;

        n = num_parms(source);
        if (parmnum >= n)
                return;
        if ((source) && (*source == '|')) {
                count++;
                if (!parmnum)
                        return;
        }
        if (!parmnum) {
                tmp = (strchr(source, '|'));
                memmove(source, tmp, strlen(tmp)+1);
                return;
        }
        tmp = source;
        while (tmp && (count < parmnum)) {
                count++;
                tmp = (strchr(tmp+1, '|'));
        }
        if (parmnum < n-1) {
                tmp1 = (strchr(tmp+1, '|'));
                memmove(tmp+1, tmp1, strlen(tmp1)+1);
        } else
                *(tmp+1) = 0;
}

/*
 * Copia in dest al piu' size caratteri partendo da src, fermandosi alla
 * prima occorrenza del carattere ucharstop, oppure la fine della stringa
 * src se e' piu' lunga di size bytes.
 * Restituisce un puntatore al carattere successivo in dest a ucharstop
 * o a '\0'.
 */
#if 0
static void *strccpy(void *dest, const void *src, int ucharstop, size_t size)
{
	char *d;
	const char *s;
	size_t n;
	int uc;

	if (size <= 0)
		return ((void *) NULL);
	s = (char *) src;
	d = (char *) dest;
	uc = (unsigned char) ucharstop;
	for (n = size; n; n--)
		if (((unsigned char) (*d++ = *s) == (char) '\0')
		    || (*s++ == (char) uc))
			return ((void *) d);
	return((void *) NULL);
}
#endif
