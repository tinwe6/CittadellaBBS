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
 *       comandi per la trasmissione di testi da client a server.            *
 ****************************************************************************/
#ifndef X_H
#define X_H   1
#include "config.h"
#include "cittaserver.h"

/* Identificatori testo */
#define TXT_BROADCAST    100
#define TXT_XMSG         101
#define TXT_CHAT         102
#define TXT_BUG_REPORT   200
#define TXT_PROFILE      300
#define TXT_POST         400
#define TXT_MAIL         401
#define TXT_MAILSYSOP    402
#define TXT_MAILAIDE     403
#define TXT_MAILRAIDE    404
#define TXT_FILE         405
#define TXT_ROOMINFO     500
#define TXT_FLOORINFO    501
#define TXT_REFERENDUM   600
#define TXT_NEWS         700
#define TXT_BANNERS      701

/* Prototipi delle funzioni in x.h */
void init_txt(struct sessione *t);
void reset_text(struct sessione *t);
void cmd_text(struct sessione *t, char *txt);
void cmd_atxt(struct sessione *t);

void cmd_bugb(struct sessione *t);
void cmd_buge(struct sessione *t);
void cmd_prfb(struct sessione *t);
void cmd_prfe(struct sessione *t);
void cmd_rieb(struct sessione *t);
void cmd_riee(struct sessione *t);
void cmd_lbeb(struct sessione *t);
void cmd_lbee(struct sessione *t);
#ifdef USE_REFERENDUM
void cmd_scpu(struct sessione *t);
void cmd_srnb(struct sessione *t, char *cmd);
void cmd_srne(struct sessione *t, char *cmd);
void cmd_sris (struct sessione *t,char *cmd);
void cmd_sren (struct sessione *t, char *cmd);
#endif
#ifdef USE_FLOORS
void cmd_fieb(struct sessione *t);
void cmd_fiee(struct sessione *t);
#endif
void cmd_nwsb(struct sessione *t);
void cmd_nwse(struct sessione *t);

#endif /* x.h */
