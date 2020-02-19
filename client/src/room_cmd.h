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
* File : room_cmd.h                                                         *
*        Comandi per la gestione delle room                                 *
****************************************************************************/
#ifndef _ROOM_CMD_H
#define _ROOM_CMD_H   1

#define DFLT_MSG_PER_ROOM 150

/* prototipi delle funzioni in questo file */
void room_list(void);
void room_list_known(int mode);
void room_goto(int mode, bool gotonext, const char *destroom);
void room_create(void);
void room_edit(int mode);
void room_delete(void);
void room_info(bool detailed, bool prof);
void room_edit_info(void);
void room_zap(void);
void room_invite(void);
void room_invited_who(void);
void room_invite_req(void);
void room_kick(void);
void room_kick_end(void);
void room_kicked_who(void);
void room_new_ra(void);
void room_new_rh(void);
void room_swap(void);
void room_resize(void);
void room_zap_all(void);
void room_unzap_all(void);

#endif /* room_cmd.h */
