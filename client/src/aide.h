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
* File : aide.h                                                             *
*        Comandi generici per aide                                          *
****************************************************************************/
#ifndef _AIDE_H
#define _AIDE_H   1

/* prototipi delle funzioni in aide.c */
void bbs_shutdown(int mode, int reboot);
void force_scripts(void);
void Read_Syslog(void);
void statistiche_server(void);
void edit_news(void);
void enter_helping_hands(void);
void server_upgrade_flag(void);

#endif /* aide.h */
