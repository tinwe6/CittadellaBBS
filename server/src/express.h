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
 * File: x.h                                                                 *
 *       comandi di broadcast, express message e chat.                       *
 ****************************************************************************/
#ifndef EXPRESS_H
#define EXPRESS_H   1
#include "config.h"
#include "cittaserver.h"

/* Prototipi delle funzioni express.h */
void cmd_brdc(struct sessione *t);
void cmd_bend(struct sessione *t);
void citta_broadcast(char *msg);
void cmd_xmsg(struct sessione *t, char *a_chi);
void cmd_dest(struct sessione *t, char *a_chi);
void cmd_xend(struct sessione *t);
void cmd_xinm(struct sessione *t, char *cmd);
void cmd_cmsg(struct sessione *t);
void cmd_cend(struct sessione *t);
void cmd_casc(struct sessione *t, char *cc);
void cmd_clas(struct sessione *t);
void cmd_cwho(struct sessione *t);
void chat_timeout(struct sessione *t);

#endif /* x.h */
