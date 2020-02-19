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
* File : floor_ops.h                                                        *
*        Operazioni sui floor della BBS.                                    *
****************************************************************************/
#ifndef _FLOOM_OPS_H
#define _FLOOM_OPS_H   1

#include "strutture.h"

/* Prototipi delle funzioni in questo file */
void cmd_fnew(struct sessione *t, char *cmd);
void cmd_fdel(struct sessione *t, char *cmd);
void cmd_flst(struct sessione *t, char *cmd);
void cmd_fmvr(struct sessione *t, char *cmd);
void cmd_faid(struct sessione *t, char *arg);
void cmd_finf(struct sessione *t);
void cmd_fkrl(struct sessione *t, char *cmd);
void cmd_fkrm(struct sessione *t, char *cmd);
void cmd_fkra(struct sessione *t, char *cmd);

void cmd_fedt(struct sessione *t, char *cmd);

#endif /* floor_ops.h */
