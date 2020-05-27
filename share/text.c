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
/*
 * Modulo per manipolare i testi. Usa una lista per memorizzare i testi.
 * Esiste un'altra versione con la stessa interfaccia e che memorizza i
 * testi in un array di char: text_arr.c. E piu' efficiente, ma spreca
 * molta piu' memoria.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include "text.h"
#include "macro.h"

#define STR_SIZE 100

/* Prototipi funzioni statiche in questo file */
static void txt_error(void);

/* Queste devono corrispondere a quelle in memstat.h!! */
#define TYPE_CHAR         0
#define TYPE_TEXT         1
#define TYPE_TEXT_BLOCK  24
/******************************************************************************
******************************************************************************/

struct text *txt_create(void)
{
        Text *txt;

        CREATE(txt, Text, 1, TYPE_TEXT);
        txt->nlines = 0;
	txt->size = 0UL;
	txt->end = 0;
        txt->rpos = NULL;
	txt->first = NULL;
        txt->last = NULL;
        return(txt);
}

/*
 * Libera la memoria occupata dalla struttura text.
 */
void txt_free(struct text **txt)
{
	register Text_Block *ptr, *next;

        if (*txt == NULL) txt_error();

	for (ptr = (*txt)->first; ptr; ptr = next) {
		next = ptr->next;
		Free(ptr->str);
		Free(ptr);
	}
        Free(*txt);
	*txt = NULL;
}

/*
 * Cancella il testo.
 */
void txt_clear(struct text *txt)
{
	register Text_Block *ptr, *next;

        if (txt == NULL) txt_error();

	for (ptr = txt->first; ptr; ptr = next) {
		next = ptr->next;
		Free(ptr->str);
		Free(ptr);
	}
        txt->nlines = 0;
	txt->size = 0UL;
	txt->end = 0;
        txt->rpos = NULL;
	txt->first = NULL;
        txt->last = NULL;
}

/*
 * Aggiunge una riga in fondo al testo. Se le righe allocate sono tutte
 * utilizzate, rialloca il testo aggiungendo TXT_LINES_ADD linee vuote.
 */
void txt_put(struct text *txt, char *str)
{
	Text_Block *block;

        if (txt == NULL) txt_error();

	size_t len = strlen(str);
        CREATE(block, Text_Block, 1, TYPE_TEXT_BLOCK);
	CREATE(block->str, char, len + 1, TYPE_CHAR);
	memcpy(block->str, str, len + 1);
	if (txt->first == NULL) {
		txt->first = block;
		txt->rpos = txt->first;
		block->prev = NULL;
	} else {
		txt->last->next = block;
		block->prev = txt->last;
	}
	block->next = NULL;
	txt->last = block;
	block->num = txt->nlines++;
	txt->size += len;
	if (txt->end) {
		txt->rpos = txt->last;
		txt->end = 0;
	}
}


/*
 * Aggiunge una riga in fondo al testo.
 * La stringa str viene inserita direttamente nel testo, e quindi
 * successivamente free()ata quando non serve piu'. Deve essere percio`
 * stata allocata dinamicamente, e non va piu' modificata direttamente.
 */
void txt_puts(struct text *txt, char *str)
{
	Text_Block *block;
        size_t len;

        if (txt == NULL) txt_error();

	len = strlen(str);
        CREATE(block, Text_Block, 1, TYPE_TEXT_BLOCK);
	block->str = str;
	if (txt->first == NULL) {
		txt->first = block;
		txt->rpos = txt->first;
		block->prev = NULL;
	} else {
		txt->last->next = block;
		block->prev = txt->last;
	}
	block->next = NULL;
	txt->last = block;
	block->num = txt->nlines++;
	txt->size += len;
	if (txt->end) {
		txt->rpos = txt->last;
		txt->end = 0;
	}
}

/*
 * txt put formattato
 */
void txt_putf(struct text *txt, const char *format, ...)
{
	va_list ap;
	char *str;
	int len = 128;
	int ret, ok = 0;

	/* Uses vsnprintf(), conforms to BSD, ISOC9X, UNIX98 standards */
	va_start(ap, format);
	CREATE(str, char, len, TYPE_CHAR);
	while (!ok) {
		ret = vsnprintf(str, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(str, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	txt_puts(txt, str);
	va_end(ap);
}

/*
 * Restituisce il puntatore alla prossima riga non letta e incrementa
 * la posizione rpos di lettura.
 */
char *txt_get(struct text *txt)
{
	char *str;

        if (!txt) txt_error();

        if (txt->rpos != NULL) {
		str = txt->rpos->str;
		txt->rpos = txt->rpos->next;
		if (txt->rpos == NULL)
			txt->end = 1;
                return str;
	} else
                return NULL;
}

/*
 * Sposta il puntatore di lettura di n righe.
 */
void txt_jump(struct text *txt, long n)
{
        if (!txt) txt_error();

		
	if (n > 0) {
		if (txt->rpos == NULL)
			return;
		for(; (txt->rpos) && n; txt->rpos = txt->rpos->next, n--);
		if (txt->rpos == NULL)
			txt->rpos = txt->last;
	} else {
		if (txt->end) {
			txt->rpos = txt->last;
			n++;
			txt->end = 0;
		}
		for(; (txt->rpos) && n; txt->rpos = txt->rpos->prev, n++);
		if (txt->rpos == NULL)
			txt->rpos = txt->first;
	}
}

long txt_rpos(struct text *txt)
{
        if (!txt) {
	        txt_error();
	}
	if (txt->rpos) {
		return txt->rpos->num;
	}
	return txt->nlines;
}

/* Duplica una struttura di testo */
struct text *txt_dup(struct text *src)
{
	Text *txt;
	Text_Block *rpos;
	char *str;

	txt = txt_create();
	rpos = src->rpos;
	txt_rewind(src);
	while ( (str = txt_get(src)))
		txt_put(txt, str);
	src->rpos = rpos;
	return txt;
}

/*
 * Carica il contenuto del file 'filename' di testo in una struttura text.
 * Restituisce il puntatore alla struttura.
 */
struct text * txt_load(char *filename)
{
	Text *txt;
        char buf[STR_SIZE];
        FILE *fp;

	txt = txt_create();
        if (!txt)
                return NULL;

        fp = fopen(filename, "r");
        if (fp == NULL)
                return NULL;

        while (fgets(buf, STR_SIZE, fp) != NULL)
                txt_put(txt, buf);

        fclose(fp);
        return txt;
}

/*
 * Funzione di errore --> abort program.
 */
static void txt_error(void)
{
        perror("Accesso a struttura text non allocata.");
        exit(0);
}
