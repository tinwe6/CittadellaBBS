/*
 *  Copyright (C) 1999 - 2003 by Marco Caldarelli and Riccardo Vianello
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
* File : patacche.h                                                         *
*        Gestione dei distintivi assegnati agli utenti.                     *
****************************************************************************/
#ifndef _PATACCHE_H
#define _PATACCHE_H    1

#ifdef USE_BADGES

#define MAXLEN_BADGE 76

struct badge {
	int num;
	char str[MAXLEN_BADGE];
};

#endif /* USE_BADGES */
#endif /* patacche.h */
