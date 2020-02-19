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
* File: generic.h                                                           *
*       Comandi generici dal client                                         *
****************************************************************************/

#ifndef _GENERIC_H
#define _GENERIC_H   1

/*Prototipi delle funzioni in generic.h */
void cmd_info(struct sessione *t, char *cmd);
void cmd_hwho(struct sessione *t);
void cmd_time(struct sessione *t);
void legge_file(struct sessione *t,char *nome);
void cmd_help(struct sessione *t);
void cmd_lssh(struct sessione *t, char *cmd);
void cmd_ltrm(struct sessione *t, char *lock);
void cmd_msgi(struct sessione *t, char *buf);
void cmd_sdwn(struct sessione *t, char *buf);
void cmd_tabc(struct sessione *t, char *cmd);
void cmd_edng(struct sessione *t, char *buf);
void cmd_upgs(struct sessione *t);
void cmd_upgr(struct sessione *t);

#endif /* generic.h */
