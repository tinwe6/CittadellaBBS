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
#include "editor.h"
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

	//write(STDOUT_FILENO, "<Winch>", 7);
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

/*
 * Returns true if a timer is set up that requires that the function is
 * called more frequently.
 */
void esegui_segnali(bool *window_changed, bool *fast_mode)
{
	bool window_has_changed = false;

	if (t_sigwinch.tv_sec || t_sigwinch.tv_usec) {
		struct timeval t_now, t_elapsed;
		gettimeofday(&t_now, NULL);
		timeval_subtract(&t_elapsed, &t_now, &t_sigwinch);
		/*
		 * On macos Terminal.app we must make sure that the user
		 * released the mouse before we change the window size,
		 * otherwise the terminal size goes out of sync with the
		 * effective size. We simply wait WINCH_TIMEOUT_MS millisecs.
		 * It's a compromize: the terminal size remains reactive and
		 * the user will usually have done resizing; if not he will
		 * have to resize the window again.
		 */
		/*
		 * TODO: is there any way to detect when the user is done? */
#define WINCH_TIMEOUT_MS 500
#define MSEC_TO_USEC 1000
		if (t_elapsed.tv_usec > WINCH_TIMEOUT_MS*MSEC_TO_USEC
		    || t_elapsed.tv_sec > 1) {
			if (cti_record_term_change()) {
				//write(STDOUT_FILENO, "<OK>", 4);
				t_sigwinch = (struct timeval){0};
				if (window_changed) {
					window_has_changed = true;
				}
			} else {
				//write(STDOUT_FILENO, "r", 1);
				/* if it didn't work, try again next time */
				t_sigwinch = t_now;
			}
			fflush(stdout);
		}
	}

	/*
	if (new_signals > (SIG_ALARM|SIG_WINCH|SIG_HUP)) {
		printf("new signals %d -- impossible! \n", new_signals);
	}
	*/

	if (new_signals) {
		//write(STDOUT_FILENO, "S", 1);
		//printf("Sig mask = %d...\n", new_signals);
		if (new_signals & SIG_HUP) {
			//write(STDOUT_FILENO, "H", 1);
			set_scroll_full();
			// assert(false);
			pulisci_ed_esci(TERMINATE_SIGNAL);
			/* no return */
		}
		if (new_signals & SIG_ALARM) {
			//write(STDOUT_FILENO, "A", 1);
			if (send_keepalive) {
				serv_puts("NOOP");
			}
			send_keepalive = true;
			alarm(KEEPALIVE_INTERVAL);
		}
		if (new_signals & SIG_WINCH) {
			//write(STDOUT_FILENO, "W", 1);
			gettimeofday(&t_sigwinch, NULL);
		}
		new_signals = SIG_CLEAR;
	}

	if (window_changed) {
		*window_changed = window_has_changed;
	}
	if (fast_mode) {
		*fast_mode = (t_sigwinch.tv_sec || t_sigwinch.tv_usec);
	}
}
