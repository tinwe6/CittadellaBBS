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
* File: signals.c                                                           *
*       signal trapping e handlers                                          *
****************************************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "config.h"
#include "cittaserver.h"
#include "signals.h"
#include "utility.h"

#define CHECKPOINT_SEC 900    /* 15 minuti */

sig_atomic_t segnale_messaggio;      /* TRUE se nuovo messaggio in arrivo. */

/* Prototipi funzioni in questo file */
void setup_segnali(void);
static void setsig_checkpoint(void);
void logsig();
void hupsig();
void elimina_soloaide();
void checkpointing();
void elimina_no_nuovi();
void handler_messaggio(int sig);

/*****************************************************************************
*****************************************************************************/
void setup_segnali(void)
{
        signal(SIGUSR1, handler_messaggio);
        signal(SIGUSR2, elimina_no_nuovi);
        signal(SIGHUP, hupsig);
        signal(SIGPIPE, SIG_IGN);
        signal(SIGINT, hupsig);
        //signal(SIGALRM, logsig);
        signal(SIGTERM, hupsig);

        /* Si protegge da un blocco del server */
        setsig_checkpoint();

	segnale_messaggio = false;
}

static void setsig_checkpoint(void)
{
        struct itimerval itime;
        struct timeval interval;

        interval.tv_sec = CHECKPOINT_SEC;
        interval.tv_usec = 0;
        itime.it_interval = interval;
        itime.it_value = interval;

        /* TODO timer reale o timer virtuale?!? */
        //setitimer(ITIMER_VIRTUAL, &itime, 0);
        setitimer(ITIMER_REAL, &itime, 0);
        //signal(SIGVTALRM, checkpointing);
        signal(SIGALRM, checkpointing);
}

void checkpointing()
{
        if (!tics) {
                citta_log("CHECKPOINT shutdown: I tics non vengono aggiornati.");
                /* abort(); TODO VEDERE semmai vai di shutdown */
        } else {
                citta_logf("CHECKPOINT tics = %ld/%d", tics,
                      FREQUENZA*CHECKPOINT_SEC);
                tics = 0;
        }
}

void elimina_soloaide()
{
        signal(SIGUSR1, elimina_soloaide);
        citta_log("SIGUSR1 - Eliminata restrizione a soli Aide");
        solo_aide = 0;
}

void elimina_no_nuovi()
{
        signal(SIGUSR2, elimina_no_nuovi);
        citta_log("SIGUSR2 - Eliminata restrizione a soli utenti gia' esistenti.");
        no_nuovi_utenti = 0;
}

/******************************************************************************
******************************************************************************/

void hupsig(void)
{
        citta_log("SYSTEM: Ricevuto SIGHUP, SIGINT, o SIGTERM.  Shut down...");

	shutdown_server();
	citta_log("Dati Cittadella salvati correttamente.");

	citta_reboot = false;
	restart_server();

        exit(0);
}

void logsig(void)
{
        citta_log("SYSTEM: Segnale ricevuto e ignorato.");
}

void handler_messaggio(int sig)
{
	IGNORE_UNUSED_PARAMETER(sig);

        signal(SIGUSR1, handler_messaggio);
	segnale_messaggio = true;
}
