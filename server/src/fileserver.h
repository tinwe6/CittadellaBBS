/*
 *  Copyright (C) 2005 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: fileserver.h                                                          *
*       server per la gestione dell'upload/download di files.                 *
******************************************************************************/
#ifndef  _FILESERVER_H
#define _FILESERVER_H   1

#include "strutture.h"

struct file_booking {
        long user;
        unsigned long filenum;
        unsigned long size;
        char filename[MAXLEN_FILENAME+1];
        struct room *room;
        struct file_booking *next;
};

/* Errori per fs_download */
#define EFSNOTFOUND   1
#define EFSWRONGNAME  2
#define EFSNOENT      3
#define EFSCORRUPTED  4

/* Prototipi funzioni fileserver.c */
void fs_init(void);
void fs_close(void);
void fs_save(void);
unsigned long fs_download(unsigned long filenum, char *nome, char **data);
void fs_delete_file(long msgnum);
void fs_delete_overwritten(void);
void fs_addcopy(long msgnum);
void fs_send_ustats(struct sessione *t, long user);
void cmd_fdnl(struct sessione *t, char *arg);
void cmd_fupb(struct sessione *t, char *arg);
void cmd_fupe(struct sessione *t, char *arg);
void cmd_fupl(struct sessione *t, char *arg);
void cmd_fbkd(struct sessione *t, char *arg);
void fs_cancel_all_bookings(long user);

#endif /* fileserver.h */
