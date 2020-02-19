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
* File : comandi.h                                                          *
*        Interprete comandi per il client.                                  *
****************************************************************************/
#ifndef _COMANDI_H
#define _COMANDI_H   1

extern char barflag;

/* prototipi delle funzioni in comandi.h */
int getcmd(char *c);
int get_msgcmd(char *str, int metadata);

#endif /* comandi.h */
