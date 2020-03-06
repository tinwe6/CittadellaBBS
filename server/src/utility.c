/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File: utility.c                                                           *
*       Funzioni varie, di utilita' generale.                               *
****************************************************************************/
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
/* #include <arpa/telnet.h> Utilizzate nell'originale per echo ON/OFF */
/* #include <netinet/in.h>  specifico per telnet.                     */
#include "cittaserver.h"
#include "memstat.h"
#include "string_utils.h"
#include "utility.h"

/* Prototipi funzioni in questo file */
void citta_log(char *str);
void citta_logf(const char *format, ...) CHK_FORMAT_1;
void Perror(const char *str);
struct timeval diff_tempo(struct timeval *a, struct timeval *b);
void gen_rand_string(char *vk, int lung);
size_t text2string(struct text *txt, char **strtxt);
void strdate_rfc822(char *str, size_t max, long ora);
char * str2html(char *str);

/* Costanti globali */

/* SPOSTARE IN SHARE!!! */

const char *settimana[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};

const char *settimana_esteso[] = {"Domenica", "Luned&iacute;",
				  "Marted&iacute;", "Mercoled&iacute;",
				  "Gioved&iacute;", "Venerd&iacute;",
				  "Sabato"};

const char *mese[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago",
                      "Set", "Ott", "Nov", "Dic"};

const char *mese_esteso[] = { "Gennaio", "Febbraio", "Marzo", "Aprile",
	"Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre",
	"Novembre", "Dicembre"};

/****************************************************************************/
/*
 * Invia la striga str nel file syslog, preceduta dalla data e ora.
 */
void citta_log(char *str)
{
        long ct;
        char *tmstr;

        ct = time(0);
        tmstr = asctime(localtime(&ct));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        fprintf(logfile, "%-19.19s :: %s\n", tmstr, str);
	fflush(logfile);
}

/*
 * Invia una stringa formattata nel file syslog.
 */
void citta_logf(const char *format, ...)
{
	char str[LBUF];
	va_list ap;

	va_start(ap, format);
	(void) vsnprintf(str, LBUF, format, ap);
	citta_log(str);
	va_end(ap);
}

/*
 * Wrapper per la funzione perror(), la fa precedere da data e ora.
 */
void Perror(const char *str)
{
        long ct;
        char *tmstr;

        ct = time(0);
        tmstr = asctime(localtime(&ct));
        *(tmstr + strlen(tmstr) - 1) = '\0';
        fprintf(logfile, "%-19.19s :: ", tmstr);
	perror(str);
	fflush(logfile);
}

/* Subtract the `struct timeval' values X and Y, storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.
   (from glibc info)                                                        */
int timeval_subtract(struct timeval *result, struct timeval *x,
		     struct timeval *y)
{
	struct timeval z;

	z.tv_sec = y->tv_sec;
	z.tv_usec = y->tv_usec;
	/* Perform the carry for the later subtraction by updating Y. */
	if (x->tv_usec < z.tv_usec) {
		int nsec = (z.tv_usec - x->tv_usec) / 1000000 + 1;
		z.tv_usec -= 1000000 * nsec;
		z.tv_sec += nsec;
	}
	if (x->tv_usec - z.tv_usec > 1000000) {
		int nsec = (x->tv_usec - z.tv_usec) / 1000000;
		z.tv_usec += 1000000 * nsec;
		z.tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   `tv_usec' is certainly positive. */
	result->tv_sec = x->tv_sec - z.tv_sec;
	result->tv_usec = x->tv_usec - z.tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < z.tv_sec;
}

/* Add the `struct timeval' values X and Y, storing the result in RESULT.
 * Return 1 if the sum is negative, otherwise 0.                            */
int timeval_add(struct timeval *result, struct timeval *x,
		struct timeval *y)
{
	int nsec;

	if (x->tv_usec + y->tv_usec > 1000000)
		nsec = (x->tv_usec + y->tv_usec) / 1000000;
	else
		nsec = 0;

	result->tv_sec = x->tv_sec + y->tv_sec + nsec;
	result->tv_usec = x->tv_usec + y->tv_usec - 1000000 * nsec;

	if (result->tv_sec > 0)
		return 0;
	if ((result->tv_sec == 0) && (result->tv_usec > 0))
		return 0;
	return 1;
}

/*
 * Genera una stringa casuale di lunghezza lung
 */
void gen_rand_string(char *vk, int lung)
{
        int i;

        srand(time(0));
        for (i = 0; i < lung; i++)
                vk[i] = 97 + (int) (25.0*rand()/(RAND_MAX+1.0));
        vk[lung] = 0;
}

/*
 * Traduce una struttura text in un'unica stringa.
 * La memoria necessaria per la stringa viene allocata dinamicamente con
 * malloc() e va liberata con free() quando non serve piu'.
 * Restituisce la lunghezza della stringa se l'operazione va in porto,
 * altrimenti restituisce 0.
 */
size_t text2string(struct text *txt, char **strtxt)
{
	size_t len;
	register char *str, *ptr;

	len = 0;
	txt_rewind(txt);
	while ( (str = txt_get(txt)))
		len += strlen(str)+1;
	*strtxt = Malloc(len, TYPE_CHAR);
	if (*strtxt) {
		txt_rewind(txt);
		str = txt_get(txt);
		for(ptr = *strtxt; str; str = txt_get(txt), ptr++)
			ptr = citta_stpcpy(ptr, str);
		return len;
	} else
		return 0;
}

void strdate(char *str, long ora)
{
        struct tm *tmst;

	tmst = localtime(&ora);
	sprintf(str, "%d %s %d alle %2.2d:%2.2d", tmst->tm_mday,
		mese[tmst->tm_mon], 1900+tmst->tm_year, tmst->tm_hour,
		tmst->tm_min);
}

/* Trasforma gli spazi in underscore */
char *space2under(char *stringa)
{
    int i;

    for(i = 0; stringa[i] != '\0'; i++)
            if (stringa[i] == ' ') stringa[i] = '_';

    return stringa;
}

/* Trasforma gli underscore in spazi */
char *under2space(char *stringa)
{
    int i;

    for(i = 0; stringa[i] != '\0'; i++)
            if (stringa[i] == '_') stringa[i] = ' ';

    return stringa;
}

void strdate_rfc822(char *str, size_t max, long ora)
{
        struct tm *tmst;

	tmst = localtime(&ora);
	sprintf(str, "%d %s %d alle %2.2d:%2.2d", tmst->tm_mday,
		mese[tmst->tm_mon], 1900+tmst->tm_year, tmst->tm_hour,
		tmst->tm_min);
        strftime(str, max, "%a, %d %b %Y %H:%M:%S %z", tmst);
}


/* Converte una stringa puro ASCII in HTML, traducendo i caratteri
 * \ < > & . La nuova stringa viene creata dinamicamente e liberata con Free */
char * str2html(char *str)
{
	int i, pos, max, len;
	char *out;

	len = strlen(str);
	max = len;
	CREATE(out, char, max+1, TYPE_CHAR);

	pos = 0;
	for (i = 0; i < len; i++) {
		while ((max - pos) < 10) { /* potrebbe non bastare il posto */
			max *= 2;
			RECREATE(out, char, max+1);
		}
		switch(str[i]) {
		case '\\':
			out[pos++] = '\\';
			out[pos++] = '\\';
			break;
		case '<':
			out[pos++] = '&';
			out[pos++] = 'l';
			out[pos++] = 't';
			out[pos++] = ';';
			break;
		case '>':
			out[pos++] = '&';
			out[pos++] = 'g';
			out[pos++] = 't';
			out[pos++] = ';';
			break;
		case '&':
			out[pos++] = '&';
			out[pos++] = 'a';
			out[pos++] = 'm';
			out[pos++] = 'p';
			out[pos++] = ';';
			break;
		default:
			out[pos++] = str[i];
			break;
		}
	}
	out[pos] = '\0';
	return out;
}


/* Strips CML tags, substitues entities with the first char of its name,
 * puts text lowercase and substitutes other characters with spaces.
 * The output string is dynamically allocated and has to be freed       */
char * strstrip(const char *src)
{
  enum {TAG, ENTITY, TEXT, ESCAPE};
        const char *p;
	char *dest, *out;
	int status;

	CREATE(dest, char, strlen(src)+1, TYPE_CHAR);;
	p = src;
	out = dest;

	status = TEXT;

	while (*p) {
                switch (status) {
		case TEXT:
	                if ( ((*p >= 'a') && (*p <= 'z')) || (*p == ' ') )
		                *(out++) = *p;
			else if ( ( (*p) >= 'A') && ( (*p) <= 'Z') )
		                *(out++) = *p - 'A' + 'a';
			else if ( *p == '\\')
			        status = ESCAPE;
			else if (*p == '<')
			        status = TAG;
			else if (*p == '&') {
			        status = ENTITY;
			        p++;
			        *(out++) = *p;
			} else
			  *(out++) = ' ';
			break;
		case ENTITY:
                        if (*p == ';')
	                        status = TEXT;
			break;
		case TAG:
	                if (*p == '>')
		                status = TEXT;
			break;
		case ESCAPE:
		        *(out++) = ' ';
			status = TEXT;
			break;
		}
		p++;
	}
	*out = 0;

	return dest;
}

/* Copia in 'word' la prossima parola nella stringa src.
 * Una parola e' delimitata da spazi.
 * Mette in *end la lunghezza della parola e restituisce un puntatore
 * allo spazio nella stringa che delimita la parola.
 * word deve avere allocato uno spazio di LBUF caratteri.               */
const char * get_word(char *word, const char *src, size_t * len)
{
        const char * end;

        while (*src == ' ')
	        src++;
	end = src;
	while(*end && (*end!=' ') && (end-src < LBUF-1))
	        end++;
	memcpy(word, src, end-src);
	*len = end-src;
	word[*len] = 0;
	return end;
}
