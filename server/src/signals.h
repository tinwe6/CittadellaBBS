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
* File: signals.h                                                           *
*       signal trapping e handlers                                          *
****************************************************************************/
#ifndef _SIGNALS_H
#define _SIGNALS_H   1

extern sig_atomic_t segnale_messaggio;

/* Prototipi funzioni in signals.c */
void setup_segnali(void);
void logsig();
void hupsig();
void elimina_soloaide();
void checkpointing();
void elimina_no_nuovi();

#endif /* signals.h */
