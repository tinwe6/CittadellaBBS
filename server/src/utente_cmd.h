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
* File: utente_cmd.h                                                        *
*       comandi del server per la gestione degli utenti.                    *
****************************************************************************/
#ifndef _UTENTE_CMD_H
#define _UTENTE_CMD_H   1

#include "strutture.h"

/* Prototipi delle funzioni in questo file */
void cmd_user(struct sessione *t, char *nome);
void cmd_usr1(struct sessione *t, char *nome);
void cmd_chek(struct sessione *t);
void cmd_quit(struct sessione *t);
void cmd_prfl(struct sessione *t, char *nome);
void cmd_rusr(struct sessione *t);
void cmd_rgst(struct sessione *t, char *buf);
void cmd_breg(struct sessione *t);
void cmd_greg(struct sessione *t);
void cmd_cnst(struct sessione *t, char *buf);
char cmd_cusr(struct sessione *t, char *nome, char notifica);
void cmd_kusr(struct sessione *t, char *nome);
void cmd_eusr(struct sessione *t, char *buf);
void cmd_gusr(struct sessione *t, char *nome);
#ifdef USE_VALIDATION_KEY
void cmd_aval(struct sessione *t, char *vk);
#endif
void cmd_gval(struct sessione *t);
void cmd_pwdc(struct sessione *t, char *pwd);
void cmd_pwdn(struct sessione *t, char *buf);
void cmd_pwdu(struct sessione *t, char *buf);
void cmd_prfg (struct sessione *t, char *nome);
void cmd_cfgg(struct sessione *t, char *cmd);
void cmd_cfgp(struct sessione *t, char *arg);
void cmd_frdg(struct sessione *t);
void cmd_frdp(struct sessione *t, char *arg);
void cmd_gmtr(struct sessione *t, char *nome);
void notify_logout(struct sessione *t, int tipo);

/***************************************************************************/
#endif /* utente_cmd.h */
