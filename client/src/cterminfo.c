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
* Cittadella/UX Library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : terminfo.c                                                         *
*        Interrogazione database terminfo e altre routines                  *
* Per la compilazione: -ltermcap -DTERMCAP se uso termcap.h                 *
*                      aggiungere -DUNIX se non uso GNU termcap             *
****************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cterminfo.h"
#include "string_utils.h"

int cti_ok;    /* Se trovo nelle terminfo tutte le informazioni necessarie
		* vale 1, altrimenti vale 0 e passo alla modalita' di
		* testo semplice, senza indirizzamento di cursore. */
#ifdef HAVE_CTI
int NRIGHE;
int NCOL;
int cti_bce;   /* Flag background color erase */
#endif

char *ctistr_ll;

char *clear_string;  /* Cancella lo schermo e cursore in pos di riposo */
char *el_string;     /* Cancella fino all'inizio della riga         */
char *el1_string;    /* Cancella fino alla fine della riga corrente */
char *ed_string;     /* Cancella fino alla fine dello schermo       */
char *smcup_string;  /* Attiva modalita' indirizzamento cursore     */
char *cup_string;    /* Sposta il cursore                           */
char *rmcup_string;  /* Abbandona modalita' indirizzamento cursore  */
char *bel_string;    /* Bell                                        */
char *flash_string;  /* Visual bell                                 */
char *csr_string;    /* Change scroll region                        */
char *cud1_string;   /* down one line                               */
char *home_string;   /* Cursor to home position                     */
char *civis_string;  /* Cursor invisible                            */
char *cnorm_string;  /* Cursor normal                               */
char *cub1_string;   /* Cursor left 1 position                      */
char *cuf1_string;   /* Cursor right 1 position                     */
char *cuu1_string;   /* Cursor up 1 line                            */
char *cnorm_string;  /* Cursor appear normal (undo civis)           */
char *ll_string;     /* Cursor in lower left corner                 */
char *dl1_string;    /* Delete one line                             */
char *blink_string;
char *bold_string;
char *dim_string;    /* Enter half-bright mode                      */
char *smir_string, *rmir_string; /* Enter/Exit insert mode          */
char *rev_string;    /* Turn on reverse video mode                  */
char *smxon_string, *rmxon_string; /* Turn on xon/xoff handshaking  */
char *ind_string, *ri_string;      /* Scroll text up/down           */

/*
char *key_left, *key_right, *key_up, *key_down, *key_home, *key_end;
char *next_page, *prev_page, *key_ins;
char *key_f0, *key_f1, *key_f2, *key_f3, *key_f4, *key_f5, *key_f6;
char *key_f7, *key_f8, *key_f9;
*/

/* Prototipi delle funzioni in questo file */
void cti_init(void);
void cti_term_init(void);
void cti_interroga(void);
void cti_scroll_reg(int start, int end);
void cti_clear_screen(void);

/******************************************************************************
******************************************************************************/

#ifdef HAVE_CTI

void cti_init(void)
{
	int errret;
	char *term;

	/* xterm non gestisce bene lo scrolling -> passa a vt220 */
	term = getenv("TERM");

	if (!strncmp(term, "xterm", 5))
		setenv("TERM", "vt220", 1);

	if (setupterm(NULL, fileno(stdin), &errret) != OK) {
		printf("\n*** Terminale sconosciuto [errret = %d].\n"
		       "    Passo in modalita' testo.\n\n", errret);
		cti_ok = 0;
		return;
	}
	cti_interroga();

	/* Testo la presenza delle capacita' minimali del terminale */
	if ((cursor_address == NULL) || (change_scroll_region == NULL)) {
		printf("\n*** Terminale non sopporta modalita' avanzata.\n"
		       "    Passo in modalita' testo.\n\n");
		cti_ok = 0;
		return;
	}
	if (cursor_to_ll != NULL) {
		ctistr_ll = cursor_to_ll;
	} else {
		ctistr_ll = citta_strdup(tparm(cursor_address, NRIGHE-1, 0));
	}
	key_left = tigetstr("kcub1");
	cti_ok = 1;
}

void cti_test(void)
{
	char *numcaps[3] = {"colors", "pairs", "ncv"};
	char *numname[3] = {"max_colors", "max_pairs", "no_color_video"};
	char *strcaps[14] = {"cup", "csr", "smcup", "initc", "initp", "oc",
		"setb", "scp", "setf", "colornm", "setab", "setaf",
		"setcolor", "ll"};
	char *strname[14] = {"cursor_address", "change_scroll_region",
		"enter_ca_mode", "initialize_color",
		"initialize_pair", "orig_colors", "set_background",
		"set_color_pair", "set_foreground", "color_names",
		"set_a_background", "set_a_foreground", "set_color_band",
	        "cursor_to_ll"};
	char *buf;
	int i, ret;

	printf("\n*** Test capacita' video...\n\n");
	for (i=0; i<3; i++) {
		ret = tigetnum(numcaps[i]);
		if (ret == ERR)
			printf("%-16s: `%s' unsupported\n", numname[i],
			       numcaps[i]);
		else
			printf("%-16s: `%s' is %d\n", numname[i], numcaps[i],
			       ret);
	}
	printf("\n");
	for (i=0; i<14; i++) {
		buf = tigetstr(strcaps[i]);
		if (buf==NULL)
			printf("%-16s: `%s' unsupported\n", strname[i],
			       strcaps[i]);
		else
			printf("%-16s: `%s' is \\E%s\n", strname[i],
			       strcaps[i], buf+1);
	}
	printf("\n");
# if ((16 + 8) < 32)
	printf("\n32 bits\n");
# else
	printf("\n16 bits\n");
# endif
}

void cti_interroga(void)
{
	NCOL = tigetnum("cols");
	NRIGHE = tigetnum("lines");
	cti_bce = tigetflag("bce");
}

void cti_scroll_reg(int start, int end)
{
	tputs(tparm(change_scroll_region, start, end), end-start, putchar);
}

void cti_term_init(void)
{
	putp(enter_ca_mode); /* Inizializza modalita' cursore. */
	cti_ll();             /* Cursor in Lower Left pos. */
}

void cti_term_exit(void)
{
	putp(exit_ca_mode); /* Inizializza modalita' cursore. */
	cti_ll();             /* Cursor in Lower Left pos. */
}
#endif

void cti_clear_screen(void)
{
#ifdef HAVE_CTI
	tputs(clear_screen, NRIGHE, putchar);
#else
	char i;
	for (i = 0; i < NRIGHE; i++)
		putchar('\n');
#endif
}
