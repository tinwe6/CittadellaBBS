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
* Cittadella/UX client			(C) M. Caldarelli and R. Vianello   *
*									    *
* File :    urna-gestione.h                    *:
*	 funzioni per la gestione dei referendum e sondaggi.                *
****************************************************************************/
#ifndef URNA_GESTIONE_H
#define URNA_GESTIONE_H
void urna_completa(void);
void urna_delete(void);
void urna_postpone(void);
void stampa_errore(char* buf);
#endif
