/*
 *  Copyright (C) 2003 by Marco Caldarelli and Riccardo Vianello
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
* File : cestino.h                                                          *
*        Funzioni per la gestione del cestino (del/undel dei post).         *
****************************************************************************/
#ifndef _CESTINO_H
#define _CESTINO_H    1
#include "config.h"

#ifdef USE_CESTINO

#include "rooms.h"

/* Prototipi delle funzioni in cestino.c */
void msg_dump(struct room *room, long msgnum);
int msg_undelete(struct room *room, long msgnum, char *roomname);
void msg_load_dump(void);
void msg_save_dump(void);
void msg_dump_free(void);
void msg_dump_resize(long oldsize, long newsize);

#endif

#endif /* cestino.h */
