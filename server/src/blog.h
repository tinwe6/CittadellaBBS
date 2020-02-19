/*
 *  Copyright (C) 2004-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                      (C) M. Caldarelli *
*                                                                             *
* File: blog.h                                                                *
*       funzioni per la gestione delle room personali (blog)                  *
******************************************************************************/
#ifndef _BLOG_H_
#define _BLOG_H_   1

#include "config.h"

#ifdef USE_BLOG

#include <stdbool.h>
#include "strutture.h"
#include "utente.h"
#include "rooms.h"

extern blog_t * blog_first;
extern blog_t * blog_last;

/* La struttura blog e' definita in utente.h */

/* Prototipi delle funzioni in blog.c */
void cmd_blgl(struct sessione *t);
void blog_init(void);
void blog_close(void);
void blog_save(void);
blog_t * blog_find(struct dati_ut *ut);
void blog_newnick(struct dati_ut *ut, char *newnick);
bool blog_post(struct room *r, struct dati_ut *ut);
long blog_num_post(blog_t *b);
void dfr_setfullroom(dfr_t *dfr, long user, long msgnum);
void dfr_setflags(dfr_t *dfr, long user, unsigned long flags);
void dfr_set_flag(dfr_t *dfr, long user, unsigned long flag);
void dfr_setf_all(dfr_t *dfr, unsigned long flags);
void dfr_clear_flag(dfr_t *dfr, long user, unsigned long flag);
long dfr_getfullroom(dfr_t *dfr, long user);
unsigned long dfr_getflags(dfr_t *dfr, long user);

#endif /* USE_BLOG */

#endif /* blog.h */
