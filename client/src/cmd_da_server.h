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
* File : cmd_da_server.h                                                    *
*        interroga il server ed esegue le operazioni richieste              *
****************************************************************************/
#ifndef _CMD_DA_SERVER_H
#define _CMD_DA_SERVER_H   1
#include "strutt.h"

#define EXEC_NO      0
#define EXEC_BROAD   1
#define EXEC_X       2
#define EXEC_NOTIF   4
#define EXEC_ALL     (EXEC_BROAD | EXEC_X | EXEC_NOTIF)

/* Prototipi delle funzioni in questo file */
int controlla_server(void);
int esegue_urgenti(char *str);
int esegue_comandi(int mode);
void esegue_cmd_old(void);

#endif /* cmd_da_server.h */
