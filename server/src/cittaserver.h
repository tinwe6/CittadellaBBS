/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: cittaserver.h                                                         *
*       File di headers per cittaserver.h                                     *
******************************************************************************/
#ifndef _CITTASERVER_H
#define _CITTASERVER_H   1

#include <stdio.h>

#include "config.h"
#include "comm.h"    /* Costanti per protocolli comunicazione client/server */
#include "user_flags.h"
#include "global.h"
#include "macro.h"
#include "chkformat.h"

#include "strutture.h"

 /* Core */
#define CORE   1048576
#define MAXCORE   (1048576*10)
 
#define MAX_DESC_DISP    256
#define MAX_L_INPUT      (1024)  /* Massimo numero di caratteri accettabili */
                                 /* in una riga di input dal client.        */

extern FILE * logfile;
extern const char empty_string[];

/* Prototipi delle funzioni in cittaserver.c */
void avvio_server(int porta);
void cprintf(struct sessione *t, char *format, ...) CHK_FORMAT_2;
void chiusura_server(int s);
void shutdown_server(void);
void restart_server(void);
void crash_save_perfavore(void);
void carica_dati_server(void);
void salva_dati_server(void);
void chiudi_sessione(struct sessione *d);
void cmd_fsct(struct sessione *t);

#endif /* cittaserver.h */
