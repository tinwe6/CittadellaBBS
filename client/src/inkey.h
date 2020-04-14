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
* File : inkey.h                                                            *
*        gestione input da utente                                           *
****************************************************************************/
#ifndef _INKEY_H
#define _INKEY_H   1

#include <stdbool.h>

#include "iso8859.h"

#define IF_ISMETA(c) if (((c) >= Key_META) && ((c) < Key_META + 0xff))

/* Valori di ritorno inkey */
#define INKEY_KEY    (0x1)
#define INKEY_SERVER (0x2)
#define INKEY_EXEC   (0x4)
#define INKEY_QUEUE  (0x8)
#define INKEY_EXIT   (INKEY_KEY | INKEY_SERVER)

/* Caratteri estesi */

#define Key_UP          300
#define Key_DOWN        301
#define Key_RIGHT       302
#define Key_LEFT        303
#define Key_HOME        304
#define Key_INS         305
#define Key_DELETE      306
#define Key_END         307
#define Key_PAGEUP      308
#define Key_PAGEDOWN    309
/* Meta keys encoded from 400 to 400 + 255 = 655 */
#define Key_META        400
#define Key_F0          700
/* Special codes that notify special events that occurred while waiting */
#define Key_Signal      800 /* If an incoming signal interrupted inkey  */
#define Key_Window_Changed 801 /* The term window has been resized      */
#define Key_Timeout     802 /* Server sent post timeout message         */

#define META(c)  (Key_META + (c))
#define Key_F(n)    (Key_F0 + (n))

/* Prototipi funzioni in inkey.c */
int inkey_sc(bool esegue);
int inkey_pager(int esegue, char *str, int *c);
int inkey_elenco(const char *elenco);
int inkey_elenco_def(const char *elenco, int def);


#endif /* inkey.h */
