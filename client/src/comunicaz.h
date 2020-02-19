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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : comunicaz.h                                                        *
*        broadcasts, express e cazzabubboli vari...                         *
****************************************************************************/
#ifndef _COMUNICAZ_H
#define _COMUNICAZ_H   1
#include "text.h"
#include "name_list.h"

#define XLOG_XIN    0   /* X-msg ricevuto */
#define XLOG_XOUT   1   /* X-msg inviato  */
#define XLOG_BIN    2   /* Broadcast ricevuto */
#define XLOG_BOUT   3   /* Broadcast inviato  */

/* Prototipi delle funzioni in comunicaz.c */
void broadcast(void);
void express(char *rcpt);
void ascolta_chat (void);
void lascia_chat (void);
void chat_msg(void);
void chat_who(void);

void xlog_init(void);
void xlog_put_out(struct text *txt, struct name_list *dest, int type);
void xlog_put_in(struct text *txt, struct name_list *dest);
void xlog_new(char *name, int ore, int min, int type);
void xlog_browse(void);

#ifdef LOCAL
void chat_dump(char *nome, struct text *txt, bool mode);
#endif

#endif /* comunicaz.h */
