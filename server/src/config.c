/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: config.c                                                            *
*       configurazione di alcune opzioni                                    *
****************************************************************************/
#include <stdbool.h>
#include "cittaserver.h"
#include "config.h"
#include "versione.h"
#include "room_flags.h"

/* Variabili che vengono definite tramite il file sysconfig.rc */
/* con i loro valori di default.                               */

/* Configurazione del sistema */
bool nameserver_lento = false;
long TIC_TAC = 10000;    /* usec tra cicli del server      */
int  FREQUENZA = 100;    /* il server esegue 100 cicli/sec */

/* Opzioni relative al crashsave */
bool auto_save = true;
int tempo_auto_save = 10;

/* Opzioni relative all'uso della BBS */ 
int livello_alla_creazione = NUOVO_UTENTE; /* Attualmente non utilizzati */
bool express_per_tutti = false;
int min_msgs_per_x = 1000;

/* Info sul server     */

char citta_soft[50] = "Cittadella/UX";
char citta_ver[10] = SERVER_RELEASE;
char citta_ver_client[10] = CLIENT_RELEASE; /* versione ultimo client */
char citta_nodo[50] = "sysop@domain.bbs";
char citta_dove[50] = "City, Country";

/* Opzioni di timeout */

bool butta_fuori_se_idle = true;
int min_idle_warning = 10;
int min_idle_logout = 20;
int max_min_idle = 21;
int login_timeout_min = 1; 

/* Nomi delle rooms principali */

char lobby[ROOMNAMELEN] = "Lobby";
char sysop_room[ROOMNAMELEN] = "Salotto";
char aide_room[ROOMNAMELEN] = "Aide";
char ra_room[ROOMNAMELEN] = "Room Aide";
char mail_room[ROOMNAMELEN] = "Mail";
char twit_room[ROOMNAMELEN] = "Rompiballe";
char dump_room[ROOMNAMELEN] = "Cestino";
char find_room[ROOMNAMELEN] = "Risultati Ricerca";
char blog_room[ROOMNAMELEN] = "Blog";
