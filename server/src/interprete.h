/*
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
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
* File: interprete.c                                                          *
*       interpreta ed esegue i comandi in arrivo dal client.                  *
******************************************************************************/
#ifndef _INTERPRETE_H
#define _INTERPRETE_H   1

void interprete_comandi(struct sessione *t, char *com);

#endif /* interprete.h */
