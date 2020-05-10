/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : editor.h                                                           *
*        Implementazione Editor integrato per Cittaclient.                  *
****************************************************************************/
#ifndef _EDITOR_H
#define _EDITOR_H   1

#include <stdbool.h>
#include "ansicol.h"
#include "text.h"

/* Numero massimo di colonne per l'editor */
#define MAX_EDITOR_COL 128

#define EDIT_TIMEOUT   -2
#define EDIT_ABORT     -1
#define EDIT_NULL       0
#define EDIT_DONE       1
#define EDIT_CONT       2
#define EDIT_NEWLINE    3
#define EDIT_UP         4
#define EDIT_DOWN       5
#define EDIT_LEFT       6
#define EDIT_RIGHT      7
#define EDIT_PAGEUP     8
#define EDIT_PAGEDOWN   9
#define EDIT_BS        10
#define EDIT_DEL       11

/* Numero di righe sopra all'editor riservate ai messaggi urgenti */
#define MSG_WIN_SIZE 10

/* Questi colori sono standard */
#define COLOR_LINK     COLOR(COL_CYAN,    COL_BLACK, ATTR_UNDERSCORE)
#define COLOR_FILE     COLOR(COL_GREEN,   COL_BLACK, ATTR_UNDERSCORE)
#define COLOR_USER     COLOR(COL_BLUE,    COL_BLACK, ATTR_BOLD)
#define COLOR_ROOM     COLOR(COL_YELLOW,  COL_BLACK, ATTR_BOLD)
#define COLOR_ROOMTYPE COLOR(COL_YELLOW,  COL_BLACK, ATTR_DEFAULT)
#define COLOR_LOCNUM   COLOR(COL_MAGENTA, COL_BLACK, ATTR_DEFAULT)

/* Variabili globali */
extern int gl_Editor_Pos;   /* Riga di inizio dell'editor              */
extern int gl_Editor_Hcurs; /* Posizione cursore orizzontale...        */
extern int gl_Editor_Vcurs; /*                  ... e verticale.       */
//extern bool Editor_needs_refresh;

#include "metadata.h"

/* Prototipi funzioni in questo editor.c */
int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp,
		  int color, Metadata_List *mdlist);

/***************************************************************************/
#endif /* editor.h */
