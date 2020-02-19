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
* File: text.h                                                              *
*       file include per text.c                                             *
****************************************************************************/
#ifndef _TEXT_H
#define _TEXT_H    1

#include "chkformat.h"

typedef struct text_block {
	char *str;
	int num;
	struct text_block *next, *prev;
} Text_Block;

typedef struct text {
	int nlines;                /* Numero totale di righe in txt */
        unsigned long size;        /* Numero totale caratteri in txt */
	int end;                   /* 1 se rpos e` in fondo al testo */
	Text_Block *rpos;
	Text_Block *first;
	Text_Block *last;
} Text;

/* Prototipi delle funzioni in questo file */
struct text *txt_create(void);
void txt_free(struct text **txt);
void txt_clear(struct text *txt);
void txt_put(struct text *txt, char *str);
void txt_puts(struct text *txt, char *str);
void txt_putf(struct text *txt, const char *format, ...) CHK_FORMAT_2;
char *txt_get(struct text *txt);
void txt_jump(struct text *txt, long n);
long txt_rpos(struct text *txt);
struct text *txt_dup(struct text *src);
struct text * txt_load(char *filename);
#if 0
struct text *txt_createn(long lstr, long max);
void txt_putc(struct text *txt, char c);
char *txt_getr(struct text *txt, int i);
int txt_append(struct text *txt1, struct text *txt2);
char *txt_read(struct text *txt, int num);
char *txt_prev(struct text *txt);
long txt_tell(struct text *txt);
char txt_ins(struct text *txt, char *str, int num);
char txt_del(struct text *txt, char *str, int num);
void txt_print(struct text *txt);
#endif

/* Restituisce il numero di righe allocate nella struttura *txt. */
/* #define txt_max(txt) (txt)->max */

/* Restituisce il numero di righe riempite nella struttura *txt. */
#define txt_len(txt) ((txt)->nlines)

/* Restituisce la posizione della prossima riga da leggere. */
/* #define txt_rpos(txt) ((txt)->rpos->num) */

/* Azzera la posizione di lettura. */
#define txt_rewind(txt) (txt)->rpos = ((txt)->first)

/* Mette la posizione di scrittura in fondo al testo */
/* #define txt_insend(txt) (txt)->wpos = (txt)->last */

#endif /* text.h */

/******************************************************************************
******************************************************************************/
