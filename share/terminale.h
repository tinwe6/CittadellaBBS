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
* Cittadella/UX Library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : terminale.h                                                        *
*        settaggio del terminale per eliminazione echo e altre vaccate      *
****************************************************************************/
#ifndef _TERMINALE_H
#define _TERMINALE_H   1

/* Prototipi delle funzioni in terminale.c */
void reset_term(void);
void term_mode(int mode);
void term_save(void);

/* Variabili globali */
extern struct termios save_attr;   /* Settaggi originali del terminale */
extern struct termios term_attr;   /* Settaggi attuali                 */

#endif /* terminale.h */
