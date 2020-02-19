/*
 * Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

/******************************************************************************
* Cittadella/UX server (C) M. Caldarelli and R. Vianello *
* *
* File: urna-crea.h *
* crea/cancella/etc un urna
******************************************************************************/

#ifndef _URNA_CREA_H 
#define _URNA_CREA_H 1

#ifdef USE_REFERENDUM

#include "urna-strutture.h"
#include "strutture.h"
#include <time.h>

int init_urna(struct sessione *t);
int rs_new(struct sessione *t, char *buf, char *lettera);
int rs_del(struct urna *u);
void rs_postpone(struct sessione *t, struct urna_dati *udt, time_t stop);
void cmd_sdel(struct sessione *t, char *buf);
void cmd_spst(struct sessione *t, char *buf);
void cmd_sstp(struct sessione *t, char *buf);
struct urna_conf *rs_new_ucf(char *buf,  struct sessione *t);
int rs_ucf_testo(struct urna_conf *ucf);
struct urna_dati *rs_new_udt(struct urna_conf *ucf);
struct urna_prop *rs_new_prop();
struct urna_voti *rs_new_uvt(struct urna_conf *ucf);
int rs_del_conf(struct urna_conf *ucf);
int rs_del_voci(char **voci,int num_voci);
int rs_del_voti(struct urna_voti *uvt);
int rs_del_dati(struct urna_dati *udt);
int rs_del_prop(struct urna_prop *upr);
void avvisa_utente(long matricola, char *testo);
unsigned char lettera_successiva();

#endif
#endif
