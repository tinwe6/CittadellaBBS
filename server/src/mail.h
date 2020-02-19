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
* File : mail.h                                                             *
*        Funzioni per la gestione della posta degli utenti.                 *
****************************************************************************/
#ifndef _MAIL_H
#define _MAIL_H   1
#include "cittaserver.h"

#define PUT_USERMAIL(S)                               \
         do { if ((S)->room->data->flags & RF_MAIL)   \
                    (S)->room->msg = (S)->mail;       \
	 } while (0)

#define REMOVE_USERMAIL(S)                            \
         do { if ((S)->room->data->flags & RF_MAIL)   \
                    (S)->room->msg = NULL;            \
	 } while (0)

void mail_load(struct sessione *t);
void mail_load1(struct dati_ut *utente, struct msg_list *mail_list);
void mail_save(struct sessione *t);
void mail_save1(struct dati_ut *utente, struct msg_list *mail_list);
void mail_check(void);
struct msg_list * mail_alloc(void);
void mail_free(struct msg_list *mail);
void mail_add(long user, long msgnum, long msgpos);
bool mail_access(char *nome);
bool mail_access_ut(struct dati_ut * ut);
int mail_new(struct sessione *t);
int mail_new_name(char *name);
void cmd_biff(struct sessione *t, char *name);

#endif /* mail.h */
