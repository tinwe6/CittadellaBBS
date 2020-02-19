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
* File: urna-rw.h *
* Gestione lettura e scrittura delle urne
******************************************************************************/
#ifndef _URNA_RW_H
#define _URNA_RW_H 1

#ifdef USE_REFERENDUM
#include "urna-strutture.h"
#include  <stdio.h>

int rs_save_data();
int load_urna(struct urna *un, unsigned long progressivo);
int load_conf(struct urna_conf *ucf, unsigned long progressivo);
int load_dati(struct urna_dati *udt, unsigned long progressivo, int modo);
int load_stat();
int load_urne();
int load_voci(struct urna_conf *ucf, FILE * fp);
int load_voti(struct urna_voti *uvt, unsigned long progressivo, int num_voci,
			   	int modo);
int save_voci(struct urna_conf *ucf, FILE * fp);
int save_conf(struct urna_conf *ucf, int level, int progressivo);
int save_dati(struct urna_dati *udt, long int progressivo);
int save_stat(int level);
int save_urna(struct urna *ur, int level);
int save_voti(struct urna_voti *uvt, int progressivo, int num_voci);
int save_prop(struct urna_prop *upr, int progressivo);
void rs_load_urne();
void rs_load_data();

/* 
 * Salva urna stat
 * se e` stata modificata
 * o se level!=1
 */

int save_stat(int level);
int load_stat();
int load_urne();
int load_voci(struct urna_conf *ucf, FILE *fp);
int load_voti(struct urna_voti*uvt, unsigned long progressivo, int num_vot,
				int modo);
int save_voci(struct urna_conf *ucf, FILE *fp);
int save_conf(struct urna_conf *ucf, int level, int progressivo);
int save_dati(struct urna_dati *udt, long int progressivo);
int save_stat(int level);
int save_urna(struct urna *ur, int level);
int save_voti(struct urna_voti*uvt,int progressivo,int num_voci);
void rs_load_urne();
int save_prop(struct urna_prop *upr, int progressivo);
int read_prop(unsigned long progressivo, int nvoti, char ***ptesto);

#endif
#endif
