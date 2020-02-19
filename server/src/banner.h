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
* File : banner.h                                                           *
*        Funzioni per gestire i login banners di Cittadella/UX.             *
****************************************************************************/
#ifndef _BANNER_H
#define _BANNER_H    1

/* Prototipi funzioni in questo file */
void banner_load_hash(void);
void hash_banners(void);
void cmd_lban(struct sessione *t, char *cmd);
void cmd_lbgt(struct sessione *t);
void save_banners(struct sessione *t);

#endif /* banner.h */
