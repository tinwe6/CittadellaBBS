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
* File : remote.h                                                           *
*        funzioni specifiche per il client remoto                           *
****************************************************************************/
#ifndef _REMOTE_H
#define _REMOTE_H   1

#ifdef LOCAL
#error
#endif

#include "text.h"

extern char BBS_EDITOR[];

extern char remote_key[LBUF];  /* Chiave per entrare come connessione remota */
extern char remote_host[LBUF]; /* Host dal quale si collega l'utente         */

/*Prototipi delle funzioni in remote.c */
#ifdef LOGIN_PORT
void load_remote_key(char *key, int key_size);
char * get_hostname(char *ip_addr);
#else
void find_remote_host(char *rhost);
#endif

void msg_dump(struct text *txt, int autolog);

/**************************************************************************/

#endif /* remote.h */
