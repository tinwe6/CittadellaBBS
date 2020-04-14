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
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "cterminfo.h"
#include "cittacfg.h"
#include "string_utils.h"
#include "utility.h"

int cti_ok;    /* Se trovo nelle terminfo tutte le informazioni necessarie
		* vale 1, altrimenti vale 0 e passo alla modalita' di
		* testo semplice, senza indirizzamento di cursore. */

#define MIN_ROWS             24
#define MIN_COLS             80

#define NROWS_DFLT           24
#define NCOLS_DFLT           80

int term_nrows = NROWS_DFLT;
int term_ncols = NCOLS_DFLT;

struct winsize target_termsize = {.ws_col = NCOLS_DFLT, .ws_row = NROWS_DFLT};
struct winsize current_termsize = {.ws_col = NCOLS_DFLT, .ws_row = NROWS_DFLT};

/* TODO Is this variable really needed? */
static bool scroll_region_is_defined = false;

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

/******************************************************************************
******************************************************************************/
#ifdef HAVE_CTI

static bool winsize_is_allowed(struct winsize *ws)
{
	return (ws->ws_col >= MIN_COLS && ws->ws_row >= MIN_ROWS);
}

/*
 * The BBS works with a terminal size 80x24 or larger. If the current number
 * of rows or cols is lower than that, resize them to their minimum value and
 * return true. If no resizing was necessary, return false
 */
static void cti_enforce_min_winsize(struct winsize *ws)
{
	ws->ws_col = ws->ws_col < MIN_COLS ? MIN_COLS : ws->ws_col;
	ws->ws_row = ws->ws_row < MIN_ROWS ? MIN_ROWS : ws->ws_row;
}

/*
 * Asks the window size in characters to the terminal. If the request fails,
 * assumes the window size matches the default window size. Updates the values
 * in current_termsize, term_nrows, and term_ncols correspondingly.
 */
static void cti_get_winsize(void)
{
	struct winsize ws = {0};
	int ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	if (ret == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
		ws.ws_col = tigetnum("cols");
		ws.ws_row = tigetnum("lines");

		ws.ws_col = ws.ws_col ? ws.ws_col : NCOLS_DFLT;
		ws.ws_row = ws.ws_row ? ws.ws_row : NROWS_DFLT;
	}
	current_termsize = ws;
	term_nrows = current_termsize.ws_row;
	term_ncols = current_termsize.ws_col;
}

/*
 * Asks the terminal to set the window size in characters to the size specified
 * by ws. Returns true if the operation was successful, false otherwise.
 */
static bool cti_set_winsize(struct winsize *ws)
{
	printf("\x1b[8;%d;%dt", ws->ws_row, ws->ws_col);
	fflush(stdout);

	/* NOTE: We need to wait that the terminal sends the WINCH signal
	 *       to acknowledge the window resize. The signal will interrupt
	 *       the sleep() as soon as it arrives.                          */
	//sleep(1);

	cti_get_winsize();

	return (current_termsize.ws_col == ws->ws_col
		&& current_termsize.ws_row == ws->ws_row);
}

/*
 * If the current terminal size differs from the target terminal size, resize
 * the terminal to match the target terminal size. Returns true if successful.
 */
bool cti_update_winsize(void)
{
	cti_enforce_min_winsize(&target_termsize);

	if (current_termsize.ws_row != target_termsize.ws_row
	    || current_termsize.ws_col != target_termsize.ws_col) {
		return cti_set_winsize(&target_termsize);
	}
	return true;
}

/*
 * Grows the winsize 'ws' to match the minimum required size if needed, and
 * then tries to set the terminal window size to 'ws'. Returns true if the
 * operation was successful.
 */
bool cti_record_term_change(void)
{
	cti_get_winsize();
	target_termsize = current_termsize;
	return cti_update_winsize();
}

void cti_init(void)
{
	int errret;
	char *term;

	/* xterm non gestisce bene lo scrolling -> passa a vt220 */
	term = getenv("TERM");
	/* printf("TERM = %s\n", term); */

	if (!strncmp(term, "xterm", 5)) {
		setenv("TERM", "vt220", 1);
	}

	if (setupterm(NULL, fileno(stdin), &errret) != OK) {
		printf("\n*** Terminale sconosciuto [errret = %d].\n"
		       "    Passo in modalita' testo.\n\n", errret);
		cti_ok = 0;
		return;
	}

	/* Checks the dimensions of the terminal window */
	cti_get_winsize();
	target_termsize = current_termsize;
	if (!winsize_is_allowed(&target_termsize)) {
		if (cti_update_winsize()) {
			printf(_(
"\n"
"   Il tuo terminale e' stato ridimensionato: le dimensioni minima per\n"
"   assicurare un funzionamento ottimale della BBS sono di %dx%d caratteri.\n"
"\n"
				 ), MIN_COLS, MIN_ROWS);
			hit_any_key();
		} else {
			printf(_(
"\n"
"   Le dimensioni minime del terminale per assicurare un funzionamento\n"
"   ottimale della BBS sono di %dx%d caratteri. Provo a ridimensionare\n"
"   il tuo terminale; se l'operazione non ha avuto successo prova a\n"
"   ricollegarti da un terminale di dimensioni maggiori.\n"
"\n"
				 ), MIN_COLS, MIN_ROWS);
			hit_any_key();
		}
	}

	/*
	printf("\nTerm size %dx%d (curr %dx%d, targ %dx%d)\n", term_ncols,
	       term_nrows, current_termsize.ws_col, current_termsize.ws_row,
	       target_termsize.ws_col, target_termsize.ws_row);
	fflush(stdout);
	hit_any_key();
	*/

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
		ctistr_ll = citta_strdup(tparm(cursor_address,
					       term_nrows - 1, 0));
	}
	key_left = tigetstr("kcub1");
	cti_ok = 1;
}

/* Enter cursor mode and move the cursor to the lower left corner */
void cti_term_init(void)
{
	putp(enter_ca_mode);
	cti_ll();
}

/* Exit cursor mode and move the cursor to the lower left corner */
void cti_term_exit(void)
{
	putp(exit_ca_mode);
	cti_ll();
}

/*
 * Asks the terminal to perform the scrolling in the region defined between
 * line 'first' and line 'last'.
 */
static void set_scroll_region(int first, int last)
{
	tputs(tparm(change_scroll_region, first, last), last - first, putchar);
	scroll_region_is_defined = true;
}

void set_scroll_full(void)
{
	set_scroll_region(0, NRIGHE-1);
	scroll_region_is_defined = false;
}

/*
 * Cancels the scroll region and makes the terminal to perform the scrolling
 * in the full window.
 */
void reset_scroll_region(void)
{
	if (scroll_region_is_defined) {
		set_scroll_full();
		cti_ll();
	}
}

typedef enum {
	WIN_TOP, WIN_BOTTOM, WIN_FULL
} window_pos;

typedef struct {
	int first_row;
	int last_row;
} window;

#define WIN_STACK_SIZE 4
static window current_window;
static window win_stack[WIN_STACK_SIZE];
static int win_stack_index;

int debug_get_winstack_index(void)
{
	return win_stack_index;
}

void debug_get_winstack(int index, int *first, int *last)
{
	assert(win_stack_index > index);
	*first = win_stack[index].first_row;
	*last = win_stack[index].last_row;
}

void debug_get_current_win(int *first, int *last)
{
	*first = current_window.first_row;
	*last = current_window.last_row;
}

void init_window(void)
{
	current_window = (window){.first_row = 0, .last_row = NRIGHE - 1};
	win_stack_index = 0;
}

void window_push(int first_row, int last_row)
{
	assert(win_stack_index < WIN_STACK_SIZE);

	win_stack[win_stack_index++] = current_window;
	set_scroll_region(first_row, last_row);
	current_window = (window){
		.first_row = first_row,
		.last_row = last_row
	};
}

void window_pop(void)
{
	assert(win_stack_index > 0);

	--win_stack_index;

	current_window = win_stack[win_stack_index];
	set_scroll_region(current_window.first_row, current_window.last_row);
}


/*****************************************************************************/
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

