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
* File : floor_cmd.h                                                        *
*        Comandi per la gestione dei floor                                  *
****************************************************************************/
#ifndef _FLOOR_CMD_H
#define _FLOOR_CMD_H   1

#define FLOORNAMELEN 20
#define DFLT_MSG_PER_ROOM 150
#define MAXLINEEFLOORINFO 100

/* prototipi delle funzioni in questo file */
void floor_create(void);
void floor_delete(void);
void floor_list (void);
void floor_move_room(void);
void floor_new_fa(void);
void floor_info(int det);
void floor_edit_info(void);
int floor_known_rooms(void);

void floor_edit(int mode);
void floor_goto(int mode, int gotonext);
void floor_list_known(char mode);
void floor_swap(void);
void floor_new_fh(void);
void floor_zap(void);

#endif /* floor_cmd.h */
