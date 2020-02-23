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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: global.h                                                            *
*       Variabili globali per il cittaserver                                *
****************************************************************************/
#ifndef _GLOBAL_H
#define _GLOBAL_H   1
#include <stdbool.h>

/* variabili globali in cittaserver.c */
extern struct sessione *lista_sessioni;
extern struct dati_server dati_server;
extern int solo_aide;
extern int no_nuovi_utenti;
extern long logged_users;         /* Numero utenti correntemente loggati     */
extern int citta_shutdown;        /* spegnimento pulito del server           */
extern int citta_sdwntimer;       /* tempo al shutdown in sec.               */
extern char chiudi;               /* spegni il server                        */
extern bool citta_reboot;         /* riavvia il server dopo lo shutdown      */
extern int daily_reboot;          /* Periodo reboot giornaliero in minuti    */
extern int conn_madre;            /* file descriptor della connessione madre */
#ifdef USE_CITTAWEB
extern int conn_web;              /* file descriptor connessione cittaweb    */
#endif
extern int maxdesc;               /* piu' alto file desc utilizzato          */
extern int max_num_desc;          /* numero massimo di file desc disponibili */
extern long tics;                 /* contatore cicli per controllo esterno   */
extern char citta_soft[50];       /* Info sul server                         */
extern char citta_ver[10];
extern char citta_nodo[50];
extern char citta_dove[50];
extern char citta_ver_client[10]; /* Versione del client piu' recente        */
extern char livello_iniziale; /* DA SPOSTARE IN SYSCONFIG */
/* Variabili globali in config.c */
extern long TIC_TAC;
extern int FREQUENZA;

#endif /* global.h */
