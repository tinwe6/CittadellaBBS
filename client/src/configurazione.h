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
* File : configurazione.h                                                   *
*        Routine di configurazione utente e client.                         *
****************************************************************************/
#ifndef _CONFIGURAZIONE_H
#define _CONFIGURAZIONE_H   1

#include <stdbool.h>

#define HAS_ISO8859 (uflags[1] & UT_ISO_8859)

/* prototipi delle funzioni in configurazione.c */
void carica_userconfig(int mode);
char edit_uc(char mode);
void edit_userconfig(void);
void vedi_userconfig(void);
bool save_userconfig(bool update);
bool toggle_uflag(int slot, long flag);
void update_cmode(void);

#endif /* configurazione.h */
