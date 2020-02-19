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
* File : edit.h                                                             *
*        Funzioni per editare testi. (headers file)                         *
****************************************************************************/

#ifndef _EDIT_H
#define _EDIT_H   1
#include "metadata.h"
#include "text.h"

/* const char getline_help[] = ""; */
extern const char * help_8bit;
extern const char * help_colors;

int c_getline(char *str, int max, bool maiuscole, bool special);
int get_text(struct text *txt, long max_linee, char max_col, bool abort);
void edit_file(char *filename);
int enter_text(struct text *txt, int max_linee, char mode,
               Metadata_List *mdlist);
void send_text(struct text *txt);
int get_textl(struct text *txt, int max, int nlines);
int get_textl_cursor(struct text *txt, int max, int nlines, bool chat);
int getline_scroll(const char *prompt, int color, char *str, int max,
		   int field, int protect);

void print_ht_max(char *str, int max);
char * elabora_testo_max(char *str, int maxlen);
char * getline_col(int max, bool maiuscole, bool special);
int get_text_col(struct text *txt, long max_linee, int max_col, bool abort);
int gl_extchar(int status);

#define print_ht(STR) print_ht_max((STR), -1)
#define elabora_testo(STR) elabora_testo_max((STR), -1)

enum { GL_NORMAL, GL_ACUTE, GL_CEDILLA, GL_CIRCUMFLEX, GL_DIAERESIS,
       GL_GRAVE, GL_MONEY, GL_PARENT, GL_RING, GL_SLASH, GL_TILDE };

/***************************************************************************/
#endif /* edit.h */
