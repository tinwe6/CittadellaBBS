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
* File: text.c                                                              *
*       struttura e funzioni di manipolazione testo                         *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#include "text.h"
#include "macro.h"

#define LBUF 256 /* DA SPOSTARE */

/* Prototipi funzioni statiche in questo file */
static void _txt_lines(struct text *txt);
static void txt_error(void);

/******************************************************************************
******************************************************************************/
struct text *txt_create(void)
{
        struct text *txt;

        CREATE(txt, struct text, 1, 1);
        CREATE(txt->txt, char, LBUF*TXT_STDMAX, 0);
        txt->max = TXT_STDMAX;
        txt->lstr = LBUF;
        txt->rpos = 0;
        txt->wpos = 0;
        txt->last = 0;
        return(txt);
}

/*
 * Funzione di creazione di una struttura di testo
 * Restituisce il puntatore alla struttura e alloca (max) righe nella
 * struttura. Inizializza i contatori.
 */
struct text *txt_createn(long lstr, long max)
{
        struct text *txt;

        CREATE(txt, struct text, 1, 1);
        CREATE((txt->txt), char, lstr*max, 0);
        txt->max = max;
        txt->lstr = lstr;
        txt->rpos = 0;
        txt->wpos = 0;
        txt->last = 0;
        return(txt);
}

/*
 * Libera la memoria occupata dalla struttura text.
 */
void txt_free(struct text **txt)
{
        if (*txt == NULL) txt_error();

        Free((*txt)->txt);
        Free(*txt);
	*txt = NULL;
}

/*
 * Inserisce una riga in fondo al testo. Nessun controllo sulla dimensione
 * della riga e del testo: non va utilizzata direttamente fuori da questo
 * file.
 */
static void _txt_lines(struct text *txt)
{
        if (txt->last >= txt->max) {
                txt->max += TXT_LINES_ADD;
                RECREATE(txt->txt, char, (txt->lstr)*(txt->max));
        }

}

/*
 * Aggiunge una riga in fondo al testo. Se le righe allocate sono tutte
 * utilizzate, rialloca il testo aggiungendo TXT_LINES_ADD linee vuote.
 * Se la riga da aggiungere supera (lstr) caratteri, la divide in piu'
 * righe.
 */
void txt_put(struct text *txt, char *str)
{
        size_t len;
        long lstr;
	
        if (!txt) txt_error();
	lstr = txt->lstr;
        if (txt->wpos > 0) {
                (txt->last)++;
                txt->wpos = 0;
        }
        len = strlen(str);
	
	if (len == 0)
		return;
	
        while (len >= (txt->lstr)) {
                _txt_lines(txt);
                memcpy((txt->txt) + (txt->last)*lstr, str, lstr-1);
                txt->txt[(txt->last +1)*lstr-1] = 0;
                str += lstr;
                (txt->last)++;
                len -= lstr-1;
        }
        _txt_lines(txt);
        strcpy((txt->txt) + (txt->last)*lstr, str);
        (txt->last)++;
}

/*
 * txt put formattato
 */
void txt_putf(struct text *txt, const char *format, ...)
{
	char str[LBUF];
	va_list ap;

	va_start(ap, format);
        vsprintf(str, format, ap);
	txt_put(txt, str);
	va_end(ap);
}

/*
 * Aggiunge un carattere alla linea corrente.
 */
void txt_putc(struct text *txt, char c)
{
        if (!txt) txt_error();

        if (txt->wpos == 0)
                _txt_lines(txt);
        else if (txt->wpos > txt->lstr-2) {
                txt->txt[txt->last*txt->lstr+txt->wpos] = 0;
                (txt->last)++;
                txt->wpos = 0;
                _txt_lines(txt);
        }
        switch (c) {
        case '\n':
                txt->txt[txt->last*txt->lstr+(txt->wpos)++] = '\n';
        case '\0':
                txt->txt[txt->last*txt->lstr+txt->wpos] = 0;
                txt->wpos = 0;
                (txt->last)++;
                break;
        default:
                txt->txt[txt->last*txt->lstr+(txt->wpos)++] = c;
                txt->txt[txt->last*txt->lstr+txt->wpos] = 0;
                break;
        }
}

/*
 * Restituisce il puntatore alla prossima riga non letta e incrementa
 * la posizione rpos di lettura.
 */
char *txt_get(struct text *txt)
{
        if (!txt) txt_error();

        if (txt->rpos < txt->last)
                return( txt->txt + (txt->lstr)*(txt->rpos)++ );
	else
                return NULL;
}

/*
 * Restituisce il puntatore alla riga precedente e decrementa
 * la posizione rpos di lettura.
 */
char *txt_prev(struct text *txt)
{
        if (!txt) txt_error();

        if (txt->rpos > 0)
                return( txt->txt + (txt->lstr)*(--txt->rpos) );
        else
                return NULL;
}

/*
 * Sposta il puntatore di lettura di n righe.
 */
void txt_jump(struct text *txt, long n)
{
        if (!txt) txt_error();
	txt->rpos += n;
	if (txt->rpos < 0)
		txt->rpos = 0;
	else if (txt->rpos > txt->last)
		txt->rpos = txt->last;
}

/*
 * Restituisce la dimensione del testo in bytes.
 */
long txt_tell(struct text *txt)
{
        long len = 0;
        long i;

        if (!txt) txt_error();
        for (i = 0; i < (txt->last); i++)
                len += strlen(txt->txt + txt->lstr*i);
        return len;
}

/*
 * Restituisce puntatore alla riga numero (num) del testo o NULL se la riga
 * non esiste.
 */
char *txt_read(struct text *txt, int num)
{
        if (!txt) txt_error();
        if (num < txt->last)
                return(txt->txt + (txt->lstr)*num);
        else
                return(NULL);
}

char txt_ins(struct text *txt, char *str, int num)
{
        return 1;
}

char txt_del(struct text *txt, char *str, int num)
{
        return 1;
}

/*
 * Restituisce il puntatore alla i-esima riga successiva a quella
 * puntata da rpos.
 */
char *txt_getr(struct text *txt, int i)
{
        if (!txt) txt_error();

        if ((txt->rpos+i < txt->max) && (txt->rpos+i >= 0))
                return( txt->txt + (txt->lstr)*(txt->rpos+i) );
        else
                return NULL;
}

/*
 * Appende il testo in txt2 in fondo al testo txt1.
 * Le due strutture text devono essere state create con la stessa lunghezza
 * delle righe.
 * Restituisce -1 se c'e' stato un problema, 0 se tutto e' ok.
 */
int txt_append(struct text *txt1, struct text *txt2)
{
	long last, lstr;
	
        if (!txt1 || !txt2)
		return -1;
	lstr = txt1->lstr;
	if (lstr != txt2->lstr)
		return -1;
	if (txt2->last == 0)
		return 0;
	
	last = txt1->last;
	txt1->last += txt2->last;
	if (txt1->last >= txt1->max)
		RECREATE(txt1->txt, char, lstr*(txt1->last));
	memcpy(txt1->txt + last*lstr, txt2->txt, txt2->last*lstr);
	return 0;
}

/*
 * Funzione di errore --> abort program.
 */
static void txt_error(void)
{
        perror("Accesso a struttura text non allocata.");
        exit(0);
}

/*
 * Visualizza la struttura txt (per debugging)
 */
void txt_print(struct text *txt)
{
	if (!txt) {
		printf("Stuttura txt non inizializzata.\n");
		return;
	}
	printf("txt->max  = %ld\n", txt->max);
	printf("txt->lstr = %ld\n", txt->lstr);
	printf("txt->rpos = %ld\n", txt->rpos);
	printf("txt->wpos = %ld\n", txt->wpos);
	printf("txt->last = %ld\n", txt->last);
}

/* Queste funzioni sono state sostituite da macro */
#if 0
inline long txt_max(struct text *txt)
{
        if (!txt) txt_error();
        return(txt->max);
}

inline long txt_len(struct text *txt)
{
        if (!txt) txt_error();
        return(txt->last);
}

inline long txt_rpos(struct text *txt)
{
        if (!txt) txt_error();
        return(txt->rpos);
}

void txt_rewind(struct text *txt)
{
        if (!txt) txt_error();
        txt->rpos = 0;
}
#endif
