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
* File : messaggi.h                                                         *
*        Comandi di gestione dei messaggi nelle room.                       *
****************************************************************************/
#ifndef _MESSAGGI_H
#define _MESSAGGI_H   1

extern long locnum, remain;
extern struct text *quoted_text;
extern struct text *hold_text;
extern bool post_timeout;

/* Prototipi delle funzioni in messaggi.c */
void enter_message(int mode, bool reply, int aide);
bool read_msg(int mode, long local_number);
void show_msg(char *room, long num);
bool goto_msg(char *room, long num);
void clear_last_msg(bool clear_md);
void set_all_read(void);

#endif /* messaggi.h */
