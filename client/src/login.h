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
* File : login.h                                                            *
*        Procedura di login.                                                *
****************************************************************************/
#ifndef _LOGIN_H
#define _LOGIN_H   1

/* prototipi delle funzioni in login.c */
int login(void);
void chiedi_valkey(void);
void login_banner(void);
void user_config(int type);

#endif /* login.h */
