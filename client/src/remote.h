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

extern char BBS_EDITOR[];

/*Prototipi delle funzioni in remote.c */
#ifdef LOGIN_PORT
char * get_hostname(char *ip_addr);
#else
void find_remote_host(char *rhost);
#endif

void msg_dump(struct text *txt, int autolog);

/**************************************************************************/

#endif /* remote.h */
