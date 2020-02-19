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
 * File : urna-comm.h 
 * comunivazioni col client 
****************************************************************************/
#ifdef USE_REFERENDUM
#ifndef URNA_COMM_H
#define URNA_COMM_H 1

#include "urna-strutture.h"
int crea_stringa_per_client(char *string, struct urna *u);
#if 0
void cmd_scvl(struct sessione *t, char *arg);
#endif
void cmd_sinf(struct sessione *t, char *buf);
void cmd_slst(struct sessione *t, char *arg);
void send_ustat(struct sessione *t);
void urna_vota(struct sessione *t, char *buf);
#endif
#endif
