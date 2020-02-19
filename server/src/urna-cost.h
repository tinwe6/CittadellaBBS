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
* File: urna-const.h *
* Costanti per urna
******************************************************************************/


#ifndef _URNA_COST_H 
#define _URNA_COST_H 1

#ifdef USE_REFERENDUM

/* Numero massimo di referendum e sondaggi contemporanei */

#define FILE_ROOM_LIKE "urnares-roomlike"
#define FILE_UCONF "urnaconf"
#define FILE_UDATA "urnadati"
#define FILE_UDATI "urnadati"
#define FILE_UPROP "urnaproposte"
#define FILE_URES "urnares"
#define FILE_USTAT "urnastat"

#ifdef SANE_STAT
#define SANE_FILE_STAT "sane_ustat"
#endif

#define FILE_UVOCI "urnavoci"
#define FILE_UVOTI "urnavoti"
#define LEN_SLOTS 10	/*lungh slot per le strutture */
#define MAGIC_CODE	"\336\017"
#define MAGIC_CONF 'C'
#define MAGIC_DATA 'D'
#define MAGIC_LISTA 'L'
#define MAGIC_RES 'R'
#define MAGIC_STAT 'S'
#define MAGIC_VOCI 'V'
#define MAGIC_VOTI 'V'
#define MAGIC_PROP 'P'
#define MAX_LONG 2000000000
#define MAX_REFERENDUM 20
#define MAX_SLOT 50
#define MAX_SONDAGGI    20
#define MAX_UTENTI 1000
#define NUM_GIUDIZI 15		/*slot allocati una volta per tutte */
#define SEGUE_VOTABILI 221

#define SEM_U_ATTIVA 0x01
#define SEM_U_CONCLUSA 0x02
#define SEM_U_PRECHIUSA 0x04

#define U_MAX_LEN_TESTO 300 
#define U_MAX_LEN_TITOLO 75
#define VERSIONE	'@'
#define VERSIONE_URNA 'A'
#define SI "S&iacute"
#define NO "No"


#endif /* use referendum */

#endif /* urna.h */

