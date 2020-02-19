/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/*****************************************************************************
 * Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File: post.c                                                              *
 *       comandi per inviare post/mail                                       *
 ****************************************************************************/
#ifndef POST_H
#define POST_H   1
#include "config.h"
#include "cittaserver.h"

/* Prototipi delle funzioni in post.h */
void post_timeout(struct sessione *t);
void cmd_pstb(struct sessione *t, char *txt);
void cmd_pste(struct sessione *t, char *txt);
void notifica_cittapost(struct room *r);

#endif /* post.h */
