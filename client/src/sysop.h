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
* File : sysop.h                                                            *
*        Comandi riservati ai sysop.                                        *
****************************************************************************/
#ifndef _SYSOP_H
#define _SYSOP_H   1

/* Prototipi delle funzioni sysop.c */
void fm_create(void);
void fm_headers(void);
void fm_read(void);
void fm_remove(void);
void fm_info(void);
void fm_msgdel(void);
void fm_expand(void);
void banners_modify(void);
void banners_rehash(void);
void sysop_reset_consent(void);
void sysop_unregistered_users(void);

#endif /* sysop.h */
