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
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : histo.h                                                            *
*        Funzioni per tracciare istogrammi in ascii.                        *
****************************************************************************/
#ifndef _HISTO_H
#define _HISTO_H   1

void print_histo(long * data, int num, int nrow, int nlabel, char all,
                 char * xlab, char * xtick[]);

void print_histo_num(long * data, int num, int nrow, int nlabel, char all,
                 char * xlab);

#endif /* histo.h */
