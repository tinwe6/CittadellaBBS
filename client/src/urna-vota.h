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
* File : urna-vota.h                    *
*	 funzioni per la gestione dei referendum e sondaggi.                *
****************************************************************************/
#ifndef URNA_VOTA_H
#define URNA_VOTA_H

#include "urna-strutture.h"


void urna_vota(void);
void send_proposte(char * testa_proposte[]);
void stampa_proposte(struct urna_client *dati);
char vota_si_no(struct urna_client *dati, char *risposta);
char vota_singola(struct urna_client *dati, char *risposta);
char vota_multipla(struct urna_client *dati, char *risposta);
char vota_votazione(struct urna_client *dati, char *risposta);
char escidavvero();
#endif
