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
* File : signals.c                                                          *
*        gestione segnali                                                   *
****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#include "signals.h"
#include "cittaclient.h"
#include "conn.h"
#include "cti.h"
#include "terminale.h"
#include "macro.h"

#define SEGNALE_KEEPALIVE     1
#define SEGNALE_WINCH         2

sig_atomic_t new_signals;      /* Flags con i nuovi segnali arrivati */
sig_atomic_t send_keepalive;
sig_atomic_t send_noop_now;

/* Prototipi funzioni in questo file */
void setup_segnali(void);
void setup_keep_alive(void);
void segnali_ign_sigtstp(void);
void segnali_acc_sigtstp(void);
void hupsig(int sig);
void handler_tstp(int sig);
void handler_cont(int sig);
void handler_alrm(int sig);
void handler_winch(int sig);
void esegui_segnali(void);

/*****************************************************************************/
/*****************************************************************************/
/*
 * Inizializza i segnali per il client
 */
void setup_segnali(void)
{
        signal(SIGHUP, hupsig);
        signal(SIGINT, SIG_IGN); /* Ignoro il Ctrl-C */
        signal(SIGTERM, hupsig);

        signal(SIGCONT, handler_cont);
        signal(SIGTSTP, handler_tstp);
        signal(SIGWINCH, handler_winch);

        new_signals = false;
}

/* Inizializza il keepalive */
void setup_keep_alive(void)
{
        signal(SIGALRM, handler_alrm);

	send_keepalive = false;
	send_noop_now = false;
	alarm(KEEP_ALIVE_INTERVAL);
}

void segnali_ign_sigtstp(void)
{
        signal(SIGTSTP, SIG_IGN);
}

void segnali_acc_sigtstp(void)
{
        signal(SIGTSTP, handler_tstp);
}

/*
 * Pulisci, salva ed esci
 */
void hupsig(int sig)
{
        IGNORE_UNUSED_PARAMETER(sig);

        pulisci_ed_esci();
}

/*
 * Sospensione client con Ctrl-Z.
 */
void handler_tstp(int sig)
{
        IGNORE_UNUSED_PARAMETER(sig);

        signal(SIGTSTP, SIG_DFL);

        /* resetta il terminale */
        reset_term();

        /* Sospende il programma */
        raise (SIGTSTP);
}

/*
 * Continua il processo interrotto con Ctrl-Z.
 */
void handler_cont(int sig)
{
        IGNORE_UNUSED_PARAMETER(sig);

        signal(SIGCONT, handler_cont);
        signal(SIGTSTP, handler_tstp);

        /* setta il terminale */
        term_save();
        term_mode();

/* Questo creava problemi con il Ctrl-Z: grazie Abel!
        if (login_eseguito)
                longjmp (ciclo_principale, -1);
*/
}

/*
 * Alarm SIGALRM
 */
void handler_alrm(int sig)
{
        IGNORE_UNUSED_PARAMETER(sig);

	if (send_keepalive) {
                send_noop_now = true;
	}

	send_keepalive = true;
	alarm(KEEP_ALIVE_INTERVAL);
}

/*
 * Window size change: SIGWINCH
 */
void handler_winch(int sig)
{
        IGNORE_UNUSED_PARAMETER(sig);

        signal(SIGWINCH, handler_winch);
        new_signals |= SEGNALE_WINCH;
}

/***************************************************************************/

void esegui_segnali(void)
{
        if (errno!= EINTR) {
                perror("Select");
	   new_signals = false;
	   return;
                exit(1);
        }
        if (new_signals & SEGNALE_KEEPALIVE) {
                if (send_noop_now) {
                        serv_puts("NOOP");
                        send_noop_now = false;
                }
        }
        if (new_signals & SEGNALE_WINCH) {
                cti_interroga();
        }
        new_signals = false;
}
