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
* File : room_ops.h                                                         *
*        Operazioni sulle room della BBS.                                   *
****************************************************************************/

#ifndef _ROOM_OPS_H
#define _ROOM_OPS_H   1

#include "strutture.h"

/* Prototipi delle funzioni in questo file */
char room_type(struct room *room);
void cmd_rdel(struct sessione *t, char *cmd);
void cmd_redt(struct sessione *t, char *cmd);
void cmd_rlst(struct sessione *t, char *cmd);
void cmd_rkrl(struct sessione *t, char *cmd);
void cmd_rnew(struct sessione *t, char *cmd);
void cmd_rinf(struct sessione *t);
void cmd_rzap(struct sessione *t);
void cmd_rzpa(struct sessione *t);
void cmd_ruzp(struct sessione *t);
void cmd_raid(struct sessione *t, char *arg);
void cmd_rinv(struct sessione *t, char *buf);
void cmd_rinw(struct sessione *t);
void cmd_rirq(struct sessione *t, char *buf);
void cmd_rkob(struct sessione *t, char *buf);
void cmd_rkoe(struct sessione *t, char *buf);
void cmd_rkow(struct sessione *t);
void cmd_rnrh(struct sessione *t, char *buf);
void cmd_rswp(struct sessione *t, char *cmd);
void cmd_rlen(struct sessione *t, char *cmd);
void cmd_rall(struct sessione *t);

#endif /* room_ops.h */
