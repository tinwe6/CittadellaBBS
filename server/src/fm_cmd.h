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
* File : fm_cmd.h                                                           *
*        Comandi per gestire i file messaggi da client.                     *
****************************************************************************/

#ifndef _FM_CMD_H
#define _FM_CMD_H   1

/* Prototipi funzioni in questo file */
void cmd_fmcr(struct sessione *t, char *buf);
void cmd_fmdp(struct sessione *t, char *buf);
void cmd_fmhd(struct sessione *t, char *buf);
void cmd_fmri(struct sessione *t, char *buf);
void cmd_fmrm(struct sessione *t, char *buf);
void cmd_fmrp(struct sessione *t, char *buf);
void cmd_fmxp(struct sessione *t, char *buf);

/***************************************************************************/

#endif /* fm_cmd.h */
