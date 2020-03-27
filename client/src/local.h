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
* File : local.h                                                            *
*        funzioni specifiche per il client locale                           *
****************************************************************************/
#ifndef _LOCAL_H
#define _LOCAL_H   1

#ifndef LOCAL
#error
#endif

#include "text.h"

/*Prototipi delle funzioni in local.c */
void subshell (void);
void finger(void);
void msg_dump(struct text *txt, int autolog);

/****************************************************************************/

#endif /* local.h */
