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
* File : friends.c                                                          *
*        funzioni per la gestione della friend-list.                        *
****************************************************************************/
#ifndef _FRIENDS_H
#define _FRIENDS_H   1

#define NFRIENDS 10		/* non modificabile */

#include "tabc.h"

extern struct name_list * friend_list;
extern struct name_list * enemy_list;

/* prototipi delle funzioni in friends.c */
void carica_amici(void);
void vedi_amici(struct name_list * lista);
void edit_amici(struct name_list * nl);
bool is_friend(const char *str);
bool is_enemy(const char *str);
const char * get_friend(int num);
const char * get_enemy(int num);

#endif /* friends.h */
