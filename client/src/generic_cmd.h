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
* File : generic_cmd.h                                                      *
*        Comandi generici del client                                        *
****************************************************************************/
#ifndef _GENERIC_CMD_H
#define _GENERIC_CMD_H   1

#include "file_flags.h"

/* prototipi delle funzioni in generic_cmd.c */
void list_host(void);
void ora_data(void);
void lock_term(void);
void bug_report(void);
void biff(void);
void leggi_file(Stdmsg_type type, int id);
void help(void);
void Help(void);
void leggi_news(void);
void edit_doing(void);
void clear_doing(void);
void edit_term(void);
void cerca(void);
void toggle_debug_mode(void);
void Beep(void);
void upgrade_client(int vcode);

#endif /* generic_cmd.h */
