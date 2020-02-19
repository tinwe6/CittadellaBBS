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
* File : tabc.h                                                             *
*        routines per Tab Completion dei nomi (utenti, room, etc.)          *
****************************************************************************/
#ifndef TABC_H
#define TABC_H   1

#include "tabc_flags.h"
#include "name_list.h"

#define TC_ERROR      0
#define TC_ENTER      1
#define TC_CONTINUE   2
#define TC_TAB        4
#define TC_ABORT      8
#define TC_MODIF     16

/* Prototipi delle funzioni in tabc.c */
int tcnewprompt(const char *prompt, char *str, int len, int mode);
int get_username(const char *prompt, char *str);
int get_username_def(const char *prompt, char *def, char *str);
int get_blogname(const char *prompt, char *str);
int get_blogname_def(const char *prompt, char *def, char *str);
int get_roomname(const char *prompt, char *str, bool blog);
int get_floorname(const char *prompt, char *str);
bool get_recipient(char *caio, int mode);
struct name_list * get_multi_recipient(int mode);

#endif /* tabc.h */
