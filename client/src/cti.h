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
* File : cti.h                                                              *
*        Prototipi funzioni in cterminfo.h                                  *
****************************************************************************/
#ifndef _CTI_H
#define _CTI_H   1

#include <stdbool.h>

/* Prototipi delle funzioni in cterminfo.c */
void cti_init(void);
void cti_term_init(void);
void cti_term_exit(void);
void cti_get_winsize(void);
void cti_set_winsize(int rows, int cols);
bool cti_validate_winsize(void);
void cti_clear_screen(void);
void cti_scroll_reg(int start, int end);

#endif /* cterminfo.h */
