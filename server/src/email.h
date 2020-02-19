/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File: email.h                                                             *
*       Invia Email in rete.                                                *
****************************************************************************/
#ifndef _EMAIL_H
#define _EMAIL_H   1

#include <stdbool.h>
#include "text.h"

#define EMAIL_SINGLE     1
#define EMAIL_ALL        2
#define EMAIL_VALID      3
#define EMAIL_NEWSLETTER 4

/* Prototipi delle funzioni in email.h */
bool send_email(struct text *txt, char * filename, char *subj, char *rcpt,
	       int mode);
#ifdef NO_DOUBLE_EMAIL
int check_double_email(char *email);
#endif /* NO_DOUBLE_EMAIL */

#endif /* email.h */
