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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : urna_client.c                                                      *
*        funzioni per la gestione dei referendum e sondaggi.                *
****************************************************************************/
#include "urna_comune.h"
#include "user_flags.h"
#include "room_flags.h"
#include <time.h>

#ifndef _URNA_CLIENT_H
#define _URNA_CLIENT_H   1

#define TIPO_REFERENDUM  1
#define MAX_URNE    20

struct urna_client {
	int num;	
	char proponente[MAXLEN_UTNAME]; /* sul server e` la matricola */
	char room[ROOMNAMELEN];
        int tipo; /*tipo*/ /* TODO bool */
	int modo;
	char titolo[LEN_TITOLO];
	struct text* testo;
	char criterio;
	long val_crit;
	time_t start;
	time_t stop;
	int num_voci;
	int max_voci;
	int max_prop;
	int max_len_prop;
	char lettera[3];
	char* testa_voci[MAX_VOCI];
	char* testa_proposte[MAX_VOCI];
	int len[MAX_VOCI]; /* lunghezza di ogni voce */
	int max_len; /* lunghezza massima */

  	time_t anzianita;		
	char bianca; 
        char ast_voto; /* TODO bool */ 
	struct parameter * parm;
};

#endif /* urna-strutture.h */
