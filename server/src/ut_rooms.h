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
* File : ut_rooms.h                                                         *
*        Gestione delle strutture dati dinamiche degli utenti.              *
****************************************************************************/
#ifndef _UT_ROOMS_H
#define _UT_ROOMS_H   1

/* Aggiunge le slot 10 alla volta limitare il carico */
#define UTR_NSLOTADD  10

/* Prototipi delle funzioni in questo file */
void utr_load(struct sessione *t);
void utr_load1(struct dati_ut *utente, long *fullroom, long *room_flags,
	       long *room_gen);
void utr_save(struct sessione *t);
void utr_check(void);
void utr_alloc(struct sessione *t);
void utr_alloc1(long **fullroom, long **room_flags, long **room_gen);
void utr_free(struct sessione *t);
void utr_setf(long user, long pos, long flag);
void utr_resetf(long user, long pos, long flag);
int utr_togglef(long user, long pos, long flag);
void utr_setf_all(long pos, long flag);
void utr_resetf_all(long pos, long flag);
void utr_resetf_all_lvl(long pos, long flag, int lvl);
void utr_swap(long user, long pos1, long pos2);
void utr_swap_all(long pos1, long pos2);
void utr_rmslot(long user, long slot);
void utr_rmslot_all(long slot);
void utr_resize(long user, long n, long oldn);
void utr_resize_all(long nslots);
void utr_chval(long user, long pos, long fr, long rf, long rg);
void utr_chval_all(long pos, long fr, long rf, long rg);
long utr_getf(long user, long pos);
long utr_get_fullroom(long user, long pos);

/* Variabile globale */
extern char FILE_UTR[256];

/**************************************************************************/
#endif /* ut_rooms.h */
