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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : march.h                                                            *
*        Gestisce la marcia dell'utente fra le room.                        *
****************************************************************************/

#ifndef _MARCH_H
#define _MARCH_H   1

#include "strutture.h"

/* Prototipi delle funzioni in questo file */
void cmd_goto(struct sessione *t, char *cmd);
void cr8_virtual_room(struct sessione *t, char *name, long n, long fmnum,
		      long rflags, struct room_data *rd);
void kill_virtual_room(struct sessione *t);

#endif /* march.h */
