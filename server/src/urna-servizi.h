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
* File: urna-servizi.h *
* file che non so dove mettere
******************************************************************************/

/*Wed Oct 15 00:39:41 CEST 2003 */

#ifndef _URNA_SERVIZI_H 
#define _URNA_SERVIZI_H 1

#ifdef USE_REFERENDUM

#include "urna-strutture.h"
#include "strutture.h"

int rs_del_conf(struct urna_conf *ucf);
int rs_del_voci(char **voci, int num_voci);
int rs_del_voti(struct urna_voti *uvt);
int rs_del_dati(struct urna_dati *udt);
int rs_del_prop(struct urna_prop *upr);
void rs_expire(void);
int rs_trova(long progressivo);
struct urna *rs_trova_lettera(char strlettera[3]);
int rs_ha_votato(struct urna *u, long user);
int rs_ha_visto(struct urna *u, long user);
void chipuo(struct urna_conf *ucf, char *buf);
void cod_lettera(unsigned char lettera, char * let);
unsigned char lettera_cod(char *stringa);
void debugga_server(struct urna *u, int level);
void dealloca_urna(struct urna *u);
void deall_prop(struct urna_prop *upr);
void deall_dati(struct urna_dati *udt);
void deall_conf(struct urna_conf* ucf);
void rs_free_data();
void rs_free_urne();
void rs_free_stat();
void u_err(struct sessione *t,int err);
void u_err2(struct sessione *t,int err,int lev);
#ifdef U_DEBUG
void dump_urna(struct urna *u);
void dump_stat();
#endif
#endif
#endif
