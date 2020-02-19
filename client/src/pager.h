/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
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
* File : pager.h                                                            *
*        Pager per la lettura dei testi.                                    *
****************************************************************************/
#ifndef _PAGER_H
#define _PAGER_H   1
#include "text.h"

extern long prompt_xlog_n;      /* Numero messaggio diario corrente.    */

/* Prototipi delle funzioni in messaggi.c */
void pager(struct text *txt, bool send, long max, int start, bool cml,
           bool spoiler, Metadata_List *mdlist);

/* Macro per i vari utilizzi del pager */
#define txt_pager(TXT)            pager((TXT), false, 0, 0, false, false, NULL)
#define msg_pager(TXT, LEN, START, CML, SPOILER, MDLIST)  pager((TXT), true, (LEN)+(START), (START), (CML), (SPOILER), (MDLIST))
#define file_pager(TXT)           pager((TXT), true, 0, 0, false, false, NULL)

#endif /* pager.h */
