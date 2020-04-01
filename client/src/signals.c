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

typedef enum {
	SIG_ALARM = 1 << 0,
	SIG_WINCH = 1 << 1,
} sig_flag;

static volatile sig_atomic_t new_signals; /* Flags for incoming signals */
bool send_keepalive;

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

	send_keepalive = true;
	alarm(KEEPALIVE_INTERVAL);
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
void hupsig(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

        pulisci_ed_esci(SHOW_EXIT_BANNER);
}

/*
 * Sospensione client con Ctrl-Z.
 */
void handler_tstp(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

        signal(SIGTSTP, SIG_DFL);

        /* resetta il terminale */
        reset_term();

        /* Sospende il programma */
        raise(SIGTSTP);
}

/*
 * Continua il processo interrotto con Ctrl-Z.
 */
void handler_cont(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

        signal(SIGCONT, handler_cont);
        signal(SIGTSTP, handler_tstp);

        /* setta il terminale */
        term_save();
        term_mode();
}

/*
 * Alarm SIGALRM
 */
void handler_alrm(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);
	//        signal(SIGALRM, handler_alrm);

	new_signals |= SIG_ALARM;
}

/*
 * Window size change: SIGWINCH
 */
void handler_winch(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

        //signal(SIGWINCH, handler_winch);
        new_signals |= SIG_WINCH;
}

/***************************************************************************/

void esegui_segnali(void)
{
	if (new_signals) {
		if (new_signals & SIG_ALARM) {
			if (send_keepalive) {
				serv_puts("NOOP");
			}
			send_keepalive = true;
			alarm(KEEPALIVE_INTERVAL);
		}
		if (new_signals & SIG_WINCH) {
			cti_get_winsize();
			cti_validate_winsize();
		}
		new_signals = 0;
	}
}
