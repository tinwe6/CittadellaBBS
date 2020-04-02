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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "signals.h"
#include "cittaclient.h"
#include "conn.h"
#include "cterminfo.h"
#include "terminale.h"
#include "utility.h"
#include "macro.h"

typedef enum {
	SIG_CLEAR = 0,
	SIG_ALARM = 1 << 0,
	SIG_WINCH = 1 << 1,
	SIG_HUP   = 1 << 2,
} sig_flag;

static volatile sig_atomic_t new_signals; /* Flags for incoming signals */
bool send_keepalive;

/*****************************************************************************/
/*****************************************************************************/
/*
 * Pulisci, salva ed esci
 */
static void handler_hup(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

	new_signals |= SIG_HUP;
}

/*
 * Sospensione client con Ctrl-Z.
 */
static void handler_tstp(int signum)
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
static void handler_cont(int signum)
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
static void handler_alrm(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);
	//        signal(SIGALRM, handler_alrm);

	new_signals |= SIG_ALARM;
}

/*
 * Window size change: SIGWINCH
 */
static void handler_winch(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);

        //signal(SIGWINCH, handler_winch);
        new_signals |= SIG_WINCH;
}

/*********************************************************************/
/*
 * Inizializza i segnali per il client
 */
void setup_segnali(void)
{
        signal(SIGHUP, handler_hup);
        signal(SIGTERM, handler_hup);
        signal(SIGINT, SIG_IGN);       /* Ignore Ctrl-C */

        signal(SIGCONT, handler_cont);
        signal(SIGTSTP, handler_tstp);
        signal(SIGWINCH, handler_winch);

        new_signals = SIG_CLEAR;
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

void signals_ignore_all(void) {
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGCONT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
}


/***************************************************************************/
struct timeval t_sigwinch = {.tv_sec = 0, .tv_usec = 0};

void esegui_segnali(void)
{
	if (t_sigwinch.tv_sec || t_sigwinch.tv_usec) {
		struct timeval t_now, t_elapsed;
		gettimeofday(&t_now, NULL);
		timeval_subtract(&t_elapsed, &t_now, &t_sigwinch);
		if (t_elapsed.tv_usec > 200000) {
			if (cti_record_term_change()) {
				t_sigwinch = (struct timeval){0};
			} else {
				t_sigwinch = t_now;
			}
			fflush(stdout);
		}
	}
	if (new_signals) {
		if (new_signals & SIG_HUP) {
			pulisci_ed_esci(TERMINATE_SIGNAL);
			/* no return */
		}
		if (new_signals & SIG_ALARM) {
			if (send_keepalive) {
				serv_puts("NOOP");
			}
			send_keepalive = true;
			alarm(KEEPALIVE_INTERVAL);
		}
		if (new_signals & SIG_WINCH) {
			gettimeofday(&t_sigwinch, NULL);
		}
		new_signals = SIG_CLEAR;//= keep_winch ? SIG_WINCH : SIG_CLEAR;
	}
}
