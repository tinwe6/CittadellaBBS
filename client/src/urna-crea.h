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
* File : urna-crea.h                    *
*	 funzioni per la gestione dei referendum e sondaggi.                *
****************************************************************************/
#ifndef URNA_CREA_H 
#define URNA_CREA_H 

#include "urna-strutture.h"
void urna_new(void);
int crea_stringa_per_server(char *str, struct urna_client *u);

#endif
