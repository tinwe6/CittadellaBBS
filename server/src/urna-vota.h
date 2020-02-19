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
 * Cittadella/UX client			(C) M. Caldarelli and R. Vianello
 * 
 * 
 * File : urna-vota.h 
 * funzioni per il voto
****************************************************************************/
#ifdef USE_REFERENDUM
#ifndef URNA_VOTA_H
#define URNA_VOTA_H 1

#include "urna-strutture.h"
#include "strutture.h"

int rs_vota(struct sessione *t, struct urna_conf *ucf);
void urna_vota(struct sessione *t, char *buf);
void rs_add_vote(struct urna *u, struct sessione * t, char *ris);
int rs_add_proposta();
void rs_add_slots(struct urna *u);
void rs_add_ques(struct urna_stat *ustat);
int rs_inizia_voto(struct sessione *t, int num);
int cerca_votante(int num, unsigned char flag);
int del_votante(int num, long int matricola);
int add_votante(int num, long int matricola);
int rs_vota(struct sessione *t, struct urna_conf *ucf);
int rs_vota_ut(struct dati_ut *ut, struct urna_conf *ucf);

#endif
#endif
