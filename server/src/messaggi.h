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
* File : messaggi.h                                                         *
*        Comandi Server/Client per leggere, scrivere, cancellare i post.    *
****************************************************************************/
#ifndef _MESSAGGI_H
#define _MESSAGGI_H   1

extern long lastpost_author;
extern long lastpost_webroom;
extern long lastpost_locnum;
extern char lastpost_room[];
extern time_t lastpost_time;

/* Prototipi funzioni in messaggi.h */
int save_post(struct sessione *t, char *arg);
void cmd_read(struct sessione *t, char *cmd);
void cmd_rmsg(struct sessione *t, char *cmd);
void cmd_tmsg(struct sessione *t, char *cmd);
bool new_msg(struct sessione *t, struct room *r);
void cmd_mdel(struct sessione *t, char *arg);
void cmd_mmov(struct sessione *t, char *arg);
void cmd_mcpy(struct sessione *t, char *arg);
int citta_post(struct room *room, char *subject, struct text *txt);
int citta_mail(struct dati_ut *ut, char *subject, struct text *txt);
int send_mail(struct dati_ut *ut, struct dati_ut *author, char *subject,
	      struct text *txt);

#endif /* messaggi.h */
