/*
 *  Copyright (C) 2004 by Marco Caldarelli and Riccardo Vianello
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
* File: find.h                                                                *
*       funzioni per la ricerca delle parole nei messaggi                     *
******************************************************************************/
#ifndef  _FIND_H
#define _FIND_H   1
#include "rooms.h"

typedef struct msg_ref {
        long fmnum;
        long msgnum;
        long msgpos;
        long local;
        long room;
        long flags;
        long autore;
        int nrefs;
} msg_ref_t;

/* Prototipi funzioni find.c */
void find_init(void);
void find_free(void);
long find_cr8index(struct room *r);
void find_insert(struct room *r, long i);
void cmd_find(struct sessione *t, char *str);

#endif /* find.h */
