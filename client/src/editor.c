/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : editor.c                                                           *
*        Editor interno per il cittaclient                                  *
****************************************************************************/

/*
  TODO

  When the users enters a message with attachments and decides to "continue"
  to edit or "hold" the text, the attachments are lost. This must be fixed
  but requires some restructuring of how the data is kept outside the editor.

  Check what 'flag' does. If it is to stay make it a bool and rename it.

  Do we require an ending 0 for the Editor_Line.str strings? It is written in
  some places, but not always.

  Do not allow to insert attachments in ASCII art mode as it can mess up. Or
  maybe simply not allow to edit chars reserved for the attachment using the
  ASCII art mode.

  When hitting Enter on space between two words to break the line in two,
  eliminate the leading space in the new line.

*/



#include <assert.h>
#include "editor.h"
#include "cterminfo.h"
#include "room_cmd.h" /* for blog_display_pre */

/* Variabili globali */
int Editor_Pos; /* Riga di inizio dell'editor                           */
int Editor_Hcurs; /* Posizione cursore orizzontale...                   */
int Editor_Vcurs; /*                  ... e verticale.                  */
bool editor_reached_full_size;

#ifdef HAVE_CTI

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "cittaclient.h"
#include "cittacfg.h" /* for CLIENT_TABSIZE */
#include "cml.h"
#include "conn.h"
#include "edit.h"
#include "editor.h"
#include "extract.h"
#include "file_flags.h"
#include "inkey.h"
#include "macro.h"
#include "metadata.h"
#include "metadata_ops.h"
#include "prompt.h"
#include "tabc.h"
#include "text.h"
#include "utility.h"
#include "generic_cmd.h"

#define MODE_INSERT     0
#define MODE_OVERWRITE  1
#define MODE_ASCII_ART  2

typedef struct Editor_Line_ {
	int str[MAX_EDITOR_COL];   /* Stringa in input              */
	int col[MAX_EDITOR_COL];   /* Colori associati ai caratteri */
	int num;                   /* Numero riga                   */
	int pos;                   /* Posizione del cursore         */
	int len;                   /* Lunghezza della stringa       */
	char flag;                 /* 1 se contiene testo           */
	struct Editor_Line_ *prev; /* Linea precedente              */
	struct Editor_Line_ *next; /* Linea successiva              */
} Editor_Line;

typedef struct Editor_Text_ {
	int max;      /* Numero massimo di colonne nel testo      */
	char insert;  /* 1 se in insert mode, 0 altrimenti        */
	int curs_col; /* Colore corrente del cursore              */
	int curr_col; /* Ultimo settaggio di colore sul terminale */
        /* Dati del testo che si sta editando                     */
	Editor_Line *first;   /* Prima riga del testo             */
	Editor_Line *last;    /* Ultima riga                      */
	Editor_Line *curr;    /* Riga corrente                    */
        int riga;             /* Numero righe del testo           */
	int term_nrows;       /* Num rows in terminal             */
        /* Dati del testo nel copy bufferando                     */
	Editor_Line *buf_first; /* Prima riga del testo           */
	Editor_Line *buf_last;  /* Ultima riga                    */
        int buf_riga;           /* Numero righe del testo         */
        bool buf_pasted;  /* true se il buffer e' stato incollato */
        bool copy; /* true se aggiunto riga al copy buf nell'ultima op */
        /* struttura per i metadata */
        Metadata_List *mdlist;
} Editor_Text;

/* Editor_Merge_Lines() result codes */
typedef enum {
	MERGE_NOTHING = 0,     /* The text was not modified */
	MERGE_ABOVE_EMPTY = 1, /* The line above was empty and was deleted */
	MERGE_BELOW_EMPTY = 2, /* The line balow was empty and was deleted */
	MERGE_EXTRA_SPACE = 3, /* An extra space was added befoer merging  */
	MERGE_REGULAR     = 4, /* No extra space needed; merge performed   */
} Merge_Lines_Result;


/* Macro per settare i colori, attributi e mdnum
 * Col ha la seguente struttura
 *     intero  =     8 bit | 8 bit | 8 bit | 4 bit | 4bit
 *                  liberi   mdnum   attr    bgcol   fgvol
 */
#define Editor_Fg_Color(t, c)                                             \
	Editor_Set_Color((t), COLOR((c & 0xf), CC_BG((t)->curs_col),      \
						CC_ATTR((t)->curs_col)))

#define Editor_Bg_Color(t, c)                                             \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col), (c & 0xf),      \
						CC_ATTR((t)->curs_col)))

#define Editor_Attr_Color(t, a)                                           \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col),                 \
                                    CC_BG((t)->curs_col), (a)))           \

#define Editor_Attr_Toggle(t, a)                                          \
	Editor_Set_Color((t), COLOR(CC_FG((t)->curs_col),                 \
                                    CC_BG((t)->curs_col),                 \
                              CC_ATTR((t)->curs_col) ^ (a & ATTR_MASK)))

#define Editor_Get_MDNum(c)      ((c >> 16) & 0xff)
#define Editor_Set_MDNum(c, m)   ( c = (m << 16) | (c & 0xffff) )

/* Colori */
#define COL_HEAD_MD COLOR(WHITE, RED, ATTR_BOLD)
#define COL_EDITOR_HEAD COLOR(YELLOW, BLUE, ATTR_BOLD)
#define COL_HEAD_ERROR COLOR(RED, GRAY, ATTR_BOLD)

/* Per avere informazione sul debugging definire EDITOR_DEBUG */
#undef EDITOR_DEBUG
//#define EDITOR_DEBUG

#ifdef EDITOR_DEBUG
# define DEB(t, s) Editor_Debug_Status((t),(s))
#else
# define DEB(t, s)
#endif

/* Prototipi funzioni in questo file */
int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp,
		  int color, Metadata_List *mdlist);
static int get_line_wrap(Editor_Text *t, bool wrap);
static void Editor_Putchar(Editor_Text *t, int c);
static void Editor_Key_Enter(Editor_Text *t);
static void Editor_Key_Backspace(Editor_Text *t);
static void Editor_Backspace(Editor_Text *t);
static void Editor_Key_Delete(Editor_Text *t);
static void Editor_Delete(Editor_Text *t);
static void Editor_Delete_Word(Editor_Text *t);
static void Editor_Delete_Next_Word(Editor_Text *t);
static void Editor_Newline(Editor_Text *t);
static void Editor_Insert_Line(Editor_Text *t);
static void Editor_Insert_Line_Here(Editor_Text *t);
static void Editor_Copy_Line(Editor_Text *t);
static void Editor_Delete_Line(Editor_Text *t, Editor_Line *l);
static int Editor_Wrap_Word(Editor_Text *t);
static void Editor_Kill_Line(Editor_Text *t);
static void Editor_Yank(Editor_Text *t);
static void Editor_Key_Tab(Editor_Text *t);
static void Editor_Key_Up(Editor_Text *t);
static void Editor_Key_Down(Editor_Text *t);
static void Editor_Key_Right(Editor_Text *t);
static void Editor_Curs_Right(Editor_Text *t);
static void Editor_Key_Left(Editor_Text *t);
static void Editor_Curs_Left(Editor_Text *t);
static void Editor_PageDown(Editor_Text *t);
static void Editor_PageUp(Editor_Text *t);
static Merge_Lines_Result Editor_Merge_Lines(Editor_Text *t);
static void Editor_Scroll_Up(int stop);
static void Editor_Scroll_Down(int start, int stop);
static void Editor_Up(Editor_Text *t);
static void Editor_Down(Editor_Text *t);
static void Editor_Set_Color(Editor_Text *t, int c);
static void Editor_Insert_Metadata(Editor_Text *t);
static void Editor_Insert_Link(Editor_Text *t);
static void Editor_Insert_PostRef(Editor_Text *t);
static void Editor_Insert_Room(Editor_Text *t);
static void Editor_Insert_User(Editor_Text *t);
static void Editor_Insert_File(Editor_Text *t);
static void Editor_Insert_Text(Editor_Text *t);
static void Editor_Save_Text(Editor_Text *t);
static void line_refresh(Editor_Line *line, int vpos, int start);
static void clear_line(int vpos);
static void Editor_Free(Editor_Text *t);
static void Editor_Free_MD(Editor_Text *t, Editor_Line *l);
static void Editor_Free_Copy_Buffer(Editor_Text *t);
static void Editor_Refresh(Editor_Text *t, int start);
static void Editor_Head(Editor_Text *t);
static void Editor_Head_Refresh(Editor_Text *t, bool full_refresh);
static void Editor_Refresh_All(Editor_Text *t);
static void Editor2CML(Editor_Line *line, struct text *txt, int col,
                       Metadata_List *mdlist);
static void text2editor(Editor_Text *t, struct text *txt, int color,
                        int max_col);
static void help_edit(Editor_Text *t);
static int Editor_Ask_Abort(Editor_Text *t);
static void refresh_line_curs(int *pos, int *curs);
#ifdef EDITOR_DEBUG
static void Editor_Debug_Status(Editor_Text *t, char *str);
static void Editor_Debug_Show_Copy_Buffer(Editor_Text *t);
static void sanity_checks(Editor_Text *t);
#endif

/* Erase current line, using current color, and leave cursor in column 0. */
static inline void erase_current_line(void)
{
	printf("\r" ERASE_TO_EOL "\r");
}

/*
 * Fills line vpos with spaces with background color 'color' and leaves
 * the cursor in that row, at column 0.
 */
static void fill_line(int vpos, int color)
{
	cti_mv(0, vpos);
	setcolor(color);
	printf(ERASE_TO_EOL "\r");
}

/*
 * Erases line vpos filling it with spaces of default color.
 */
static inline void clear_line(int vpos)
{
	cti_mv(0, vpos);
	setcolor(COLOR(GRAY, BLACK, ATTR_DEFAULT));
	printf(ERASE_TO_EOL);
}

/***************************************************************************/
/*
 * Le funzioni di editing implementano i seguenti comandi: (Emacs like)
 *                  Ctrl-A  Vai all'inizio della riga
 *                  Ctrl-B  Sposta il cursore di un carattere a sinistra
 *                  Ctrl-C  Abort
 *                  Ctrl-D  Cancella il carattere sotto al cursore
 *                  Ctrl-E  Vai alla fine della riga
 *                  Ctrl-F  Sposta il cursore di un carattere a destra
 *                  Ctrl-K  Cancella dal cursore fino alla fine della riga
 *                  Ctrl-L  Refresh text
 *                  Ctrl-N  Down
 *                  Ctrl-P  Up
 *                  Ctrl-T  Save to file
 *                  Ctrl-U  Cancella tutta la riga corrente.
 *                  Ctrl-V  Page Down
 *                  Ctrl-W  Cancella una parola all'indietro
 *                  Ctrl-X  Save and exit
 *                  Ctrl-Y  Yank (paste from copy buffer)
 *                  Meta-B  bg blu
 *                  Meta-C  bg cyan
 *                  Meta-G  bg green
 *                  Meta-M  bg magenta
 *                  Meta-N  bg black
 *                  Meta-P  pilcrow sign
 *                  Meta-R  bg red
 *                  Meta-S  section sign
 *                  Meta-W  bg gray (white)
 *                  Meta-Y  bg yellow
 *                  Meta-a  Same as Ctrl-C
 *                  Meta-b  fg blu
 *                  Meta-c  fg cyan
 *                  Meta-d  Cancella la parola a destra del cursore
 *                  Meta-f  blink
 *                  Meta-g  fg green
 *                  Meta-h  bold
 *                  Meta-i  Insert Text
 *                  Meta-m  fg magenta
 *                  Meta-n  fg black
 *                  Meta-o  ring
 *                  Meta-r  fg red
 *                  Meta-s  sharp s
 *                  Meta-t  tilde
 *                  Meta-u  underscore
 *                  Meta-v  Page Up
 *                  Meta-w  fg gray (white)
 *                  Meta-y  fg yellow
 *                  Meta-'  acute
 *                  Meta-^  circumflex
 *                  Meta-"  diaeresis
 *                  Meta-`  grave
 *                  Meta-$  money
 *                  Meta-(  parent
 *                  Meta-~  tilde
 *                  Meta-,  cedilla
 *                  Meta-/  slash
 *                  Meta-%  division sign
 *                  Meta-!  inv excl mark
 *                  Meta-?  inv quot mark
 *                  Meta-<  left guillem
 *                  Meta->  right guillem
 *                  Meta-|  broken bar
 *                  Meta--  soft hyphen
 *                  Meta-_  macron
 *                  Meta-+  plus/minus
 *                  Meta-.  middle dot
 *                  Meta-*  mult sign
 *                  F1      Visualizza l'help
 *                  F2      Applica colore e attributi correnti
 *                  F3      Applica col e attr fino a spazio successivo
 *                  F4      Applica col e attr a tutta la riga
 *                  Ins     Insert -> Overwrite -> ASCII Art
 *                  Home
 *                  End
 *                  TAB, Enter, INS, BS, DEL, UP, DOWN, RIGHT, LEFT,
 *                  PageUp, PageDown,
 */
/***************************************************************************/
/*
 * get_text_full() : editor interno full screen per testi.
 *
 * Prende in input dall'utente un testo lungo al piu' 'max_linee' righe su
 * 'max_col' colonne e lo inserisce nella struttura 'txt'. Il testo termina
 * quando si lascia una riga vuota oppure si riempono tutte le righe a
 * disposizione.
 * Se abortp == true, immettendo su una riga vuota la stringa "ABORT"
 * l'editing viene interrotto.
 * Il testo viene formattato  automaticamente mandando a capo le parole che
 * non ci stanno in fondo alla riga.
 * Color e` il colore iniziale del cursore. Se color e` zero usa C_DEFAULT.
 *
 * Valori di ritorno: EDIT_ABORT : Abort;
 *                             0 : Nessun testo;
 *                             N : Numero righe scritte.
 */
int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp,
		  int color, Metadata_List *mdlist)
{
	Editor_Text t;
        char prompt_tmp;
        int ret;

	/* The abortp option is not available (and useless) when the option */
	/* HAVE_CTI is set. The arg is keept only for compatibility with    */
	/* the old editor. Maybe we can consider eliminating it completely  */
	assert(abortp == false);

	/* Inizializza schermo */
	cti_term_init();
	init_window();
	push_color();

	editor_reached_full_size = false;

	t.max = (max_col > MAX_EDITOR_COL) ? MAX_EDITOR_COL : max_col;
	t.term_nrows = NRIGHE;

	Editor_Pos = t.term_nrows - 2;
	Editor_Vcurs = t.term_nrows - 1;
	prompt_tmp = prompt_curr;
	prompt_curr = P_EDITOR;

	/* Inizializza il testo da editare */
	t.riga = 1;
	t.insert = MODE_INSERT;
	t.curs_col = color ? color : C_DEFAULT;
	t.curr_col = t.curs_col;

	CREATE(t.first, Editor_Line, 1, 0);
	t.curr = t.first;
	t.last = t.first;
	t.first->num = 1;
	t.first->pos = 0;
	t.first->len = 0;
	t.first->flag = 1;
	t.first->next = NULL;
	t.first->prev = NULL;

        /* Inizializza il buffer di copia */
        t.buf_first = NULL;
        t.buf_last = NULL;
	t.buf_riga = 0;
	t.buf_pasted = false;
        t.copy = false;

        /* Inizializza il metadata */
        md_init(mdlist);
        t.mdlist = mdlist;

	setcolor(t.curr_col);

	/* Inserisce nell'editor il testo del quote o hold */
	text2editor(&t, txt, color, max_col);
	txt_clear(txt);

	/* Visualizza l'header dell'editor ed eventualmente il testo */
	t.curr = t.first;
	Editor_Head(&t);
	while(t.curr->next) {
		Editor_Newline(&t);
	}
	Editor_Refresh(&t, 0);

	DEB(&t, "---");

	bool fine = false;
	do {
		switch((ret = get_line_wrap(&t, true))) {
		case EDIT_ABORT:
			cti_ll();
			printf(ERASE_TO_EOL "\rAbort.\n");
			fine = true;
			break;

		case EDIT_TIMEOUT:
			cti_ll();
			printf(ERASE_TO_EOL "\rPost timeout.\n");
			fine = true;
			break;

		case EDIT_DONE:
			cti_ll();
			putchar('\n');
			fine = true;
			break;
		}
	} while ((t.riga < max_linee) && (!fine));

	/* TODO: how can ret be EDIT_NEWLINE? it can't... what's the point
         	 of the next block?      */
	if (ret == EDIT_NEWLINE) {
		refresh_line_curs(t.curr->str, t.curr->str + t.curr->pos);

		ret = get_line_wrap(&t, false);
		if (ret == EDIT_DONE) {
			cti_ll();
			putchar('\n');
		}
	}

	Editor2CML(t.first, txt, color, t.mdlist);
	Editor_Free(&t);
	prompt_curr = prompt_tmp;

	if (editor_reached_full_size) {
		window_pop();
	}
	{
		int first, last;
		assert(debug_get_winstack_index() == 0);
		debug_get_current_win(&first, &last);
		assert(first == 0 && last == NRIGHE - 1);
	}

	cti_term_exit();
	pull_color();

	return ret;
}

/*
 * Prende un testo dallo stdinput, lungo 'max'.
 * Il testo viene editato su una singola riga, e se e' piu' lungo dello
 * schermo scrolla.
 */
static int get_line_wrap(Editor_Text *t, bool wrap)
/* TODO why is arg wrap not used here? the two calls to this function have
        wrap = true and wrap = false, why? */
{
	int status = GL_NORMAL;    /* stato dell'editor             */

	IGNORE_UNUSED_PARAMETER(wrap);

	line_refresh(t->curr, Editor_Vcurs, 0);
	setcolor(t->curr_col);
        do {
#ifdef EDITOR_DEBUG
		sanity_checks(t);
		//DEB(t, "^");
		Editor_Debug_Show_Copy_Buffer(t);
#endif
		Editor_Head_Refresh(t, false);

		/* TODO: SISTEMARE PROMPT ?
                   prompt_txt = t->curr->str;
                */
		Editor_Hcurs = t->curr->pos + 1;

		int key = inkey_sc(true);
		bool addchar = false;

		if (key == Key_Window_Changed) {
			if (t->term_nrows < NRIGHE) {
				/* the terminal has grown vertically */
				int extra_rows = NRIGHE - t->term_nrows;
				Editor_Pos += extra_rows;
				Editor_Vcurs += extra_rows;
			} else {
				int extra_rows = t->term_nrows - NRIGHE;
				if (Editor_Pos - extra_rows < MSG_WIN_SIZE) {
					extra_rows = Editor_Pos - MSG_WIN_SIZE;
					//Editor_Pos = MSG_WIN_SIZE;
				}
				Editor_Pos -= extra_rows;
				Editor_Vcurs -= extra_rows;
			}
			t->term_nrows = NRIGHE;
			Editor_Refresh_All(t);
			continue;
		}

		if (key == Key_Timeout) {
			return EDIT_TIMEOUT;
		}

		switch(key) {
		case Key_CR:
		case Key_LF:
			Editor_Key_Enter(t);
			break;

		case Key_TAB:
			Editor_Key_Tab(t);
			break;

			/* Deleting characters */
		case Key_INS:
		case META('I'):
			t->insert = (t->insert + 1) % 3;
			break;
		case Key_BS:
			Editor_Key_Backspace(t);
			break;
		case Key_DEL:
		case Ctrl('D'):
			Editor_Key_Delete(t);
			break;

			/* Deleting Words */
		case Ctrl('W'):
			Editor_Delete_Word(t);
			break;
		case META('d'): /* Cancella la parola a destra del cursore */
			Editor_Delete_Next_Word(t);
			break;

			/* Cursor Motion */
		case Key_UP:
		case Ctrl('P'):
			Editor_Key_Up(t);
			break;
		case Key_DOWN:
		case Ctrl('N'):
			Editor_Key_Down(t);
			break;
		case Key_RIGHT:
		case Ctrl('F'):
			Editor_Key_Right(t);
			break;
		case Key_LEFT:
		case Ctrl('B'):
			Editor_Key_Left(t);
			break;

			/* Extended Motion */
		case Key_PAGEUP:
		case META('v'):
			Editor_PageUp(t);
			break;
		case Key_PAGEDOWN:
		case Ctrl('V'):
			Editor_PageDown(t);
			break;
		case Key_HOME:
			/* case META('<'): */
			t->curr = t->first;
			t->curr->pos = 0;
			Editor_Vcurs = Editor_Pos+1;
			Editor_Refresh(t, 0);
			break;
		case Key_END:
			/* case META('>'): */
			t->curr = t->last;
			t->curr->pos = t->curr->len;
			Editor_Vcurs = NRIGHE-1;
			Editor_Refresh(t, 0);
			break;
		case Ctrl('A'): /* Vai inizio riga */
			t->curr->pos = 0;
			cti_mv(1, Editor_Vcurs);
			break;
		case Ctrl('E'): /* Vai fine riga */
			t->curr->pos = t->curr->len;
			cti_mv(t->curr->pos+1, Editor_Vcurs);
			break;

			/* Abort */
		case Ctrl('C'):
		case META('a'):
			if (Editor_Ask_Abort(t))
				return EDIT_ABORT;
			break;

                case META('i'): /* Insert text */
                        Editor_Insert_Metadata(t);
                        break;
                case Ctrl('T'): /* Save text to file */
			if (local_client) {
				Editor_Save_Text(t);
			}
                        break;
		case Ctrl('K'): /* Kill line */
			Editor_Kill_Line(t);
                        break;
		case Ctrl('Y'):
		case Ctrl('G'):
			Editor_Yank(t);
			break;

		case Ctrl('L'): /* Rivisualizza il contenuto dello schermo */
			Editor_Refresh_All(t);
			break;

		case Key_F(1): /* Visualizza l'Help */
			help_edit(t);
			break;

		case Key_F(2): /* Applica colore e attributi correnti */
			if ((t->curr->pos < t->curr->len) &&
                            (Editor_Get_MDNum(t->curr->col[t->curr->pos])==0)) {
				t->curr->col[t->curr->pos] = t->curr_col;
				putchar(t->curr->str[t->curr->pos]);
                                if (t->insert != MODE_ASCII_ART)
                                        t->curr->pos++;
			} else
				Beep();
			break;

		case Key_F(3): /* Applica colore e attributi correnti fino
                                  allo spazio successivo.                  */
			if (t->curr->pos < t->curr->len) {
                                int tmppos;
                                for (tmppos = t->curr->pos;
                                     (t->curr->pos < t->curr->len)
                                    && (t->curr->str[t->curr->pos]!=' ')
                          && (Editor_Get_MDNum(t->curr->col[t->curr->pos])==0);
                                     t->curr->pos++) {
                                        t->curr->col[t->curr->pos] = t->curr_col;
                                        putchar(t->curr->str[t->curr->pos]);
                                }
                                if (t->insert == MODE_ASCII_ART)
                                        t->curr->pos = tmppos;
                                cti_mv(t->curr->pos+1, Editor_Vcurs);
			} else
				Beep();
			break;

		case Key_F(4): /* Applica colore e attributi correnti a
                                  tutta la riga                          */
			if (t->curr->len != 0) {
                                int tmppos;
                                cti_mv(1, Editor_Vcurs);
                                for (tmppos = 0;
                                     (tmppos < t->curr->len) &&
                                             t->curr->str[t->curr->pos]!=' ';
                                     tmppos++) {
                                        if (Editor_Get_MDNum(t->curr->col[tmppos]) == 0) {
                                                t->curr->col[tmppos] = t->curr_col;
                                                putchar(t->curr->str[tmppos]);
                                        }
                                }
                                cti_mv(t->curr->pos+1, Editor_Vcurs);
			} else
				Beep();
			break;

		case Ctrl('U'): /* Cancella tutto il testo della riga */
                        Editor_Free_MD(t, t->curr);
			t->curr->len = 0;
			*(t->curr->str) = '\0';
			t->curr->pos = 0;
			line_refresh(t->curr, Editor_Vcurs, 0);
			setcolor(t->curr_col);
			break;

		case Ctrl('X'):
			return EDIT_DONE;

			/* ANSI Colors : Foreground */
		case META('b'):
			Editor_Fg_Color(t, COL_BLUE);
			break;
		case META('c'):
			Editor_Fg_Color(t, COL_CYAN);
			break;
		case META('g'):
			Editor_Fg_Color(t, COL_GREEN);
			break;
		case META('m'):
			Editor_Fg_Color(t, COL_MAGENTA);
			break;
		case META('n'):
			Editor_Fg_Color(t, COL_BLACK);
			break;
		case META('r'):
			Editor_Fg_Color(t, COL_RED);
			break;
		case META('w'):
			Editor_Fg_Color(t, COL_GRAY);
			break;
		case META('y'):
			Editor_Fg_Color(t, COL_YELLOW);
			break;
			/* ANSI Colors : Background */
		case META('B'):
			Editor_Bg_Color(t, COL_BLUE);
			break;
		case META('C'):
			Editor_Bg_Color(t, COL_CYAN);
			break;
		case META('G'):
			Editor_Bg_Color(t, COL_GREEN);
			break;
		case META('M'):
			Editor_Bg_Color(t, COL_MAGENTA);
			break;
		case META('N'):
			Editor_Bg_Color(t, COL_BLACK);
			break;
		case META('R'):
			Editor_Bg_Color(t, COL_RED);
			break;
		case META('W'):
			Editor_Bg_Color(t, COL_GRAY);
			break;
		case META('Y'):
			Editor_Bg_Color(t, COL_YELLOW);
			break;
			/* ANSI Colors : Attributes */
		case META('f'):
			Editor_Attr_Toggle(t, ATTR_BLINK);
			break;
		case META('h'):
			Editor_Attr_Toggle(t, ATTR_BOLD);
			break;
		case META('u'):
			Editor_Attr_Toggle(t, ATTR_UNDERSCORE);
			break;

		/* Extended charset keybindings */
		case META('\''):
			status = GL_ACUTE;
			break;
		case META('^'):
			status = GL_CIRCUMFLEX;
			break;
		case META('\"'):
			status = GL_DIAERESIS;
			break;
		case META('`'):
			status = GL_GRAVE;
			break;
		case META('$'):
			status = GL_MONEY;
			break;
		case META('('):
			status = GL_PARENT;
			break;
		case META('~'):
			status = GL_TILDE;
			break;
		case META(','):
			status = GL_CEDILLA;
			break;
		case META('/'):
			status = GL_SLASH;
			break;
		case META('o'):
			status = GL_RING;
			break;
		case META('S'):
			key = Key_SECTION_SIGN;
			addchar = true;
			break;
		case META('s'):
			key = Key_SHARP_S;
			addchar = true;
			break;
                case META('t'):
                        key = '~';
                        addchar = true;
                        break;
		case META('P'):
			key = Key_PILCROW_SIGN;
			addchar = true;
			break;
		case META('%'):
			key = Key_DIVISION_SIGN;
			addchar = true;
			break;
		case META('!'):
			key = Key_INV_EXCL_MARK;
			addchar = true;
			break;
		case META('?'):
			key = Key_INV_QUOT_MARK;
			addchar = true;
			break;
		case META('<'):
			key = Key_LEFT_GUILLEM;
			addchar = true;
			break;
		case META('>'):
			key = Key_RIGHT_GUILLEM;
			addchar = true;
			break;
		case META('|'):
			key = Key_BROKEN_BAR;
			addchar = true;
			break;
		case META('-'):
			key = Key_SOFT_HYPHEN;
			addchar = true;
			break;
		case META('_'):
			key = Key_MACRON;
			addchar = true;
			break;
		case META('+'):
			key = Key_PLUS_MINUS;
			addchar = true;
			break;
		case META('.'):
			key = Key_MIDDLE_DOT;
			addchar = true;
			break;
		case META('*'):
			key = Key_MULT_SIGN;
			addchar = true;
			break;

			/* Add a character */
		default:
			IF_ISMETA(key) {
				;
			} else if ((isascii(key) && isprint(key))
				   || is_isoch(key)) {
				addchar = true;
			}
		}
                if (key != Ctrl('K')) {
                        t->copy = false;
		}
		if (status != GL_NORMAL) {
			key = gl_extchar(status);
			if (key != -1) {
				addchar = true;
			} else {
				Beep();
			}
			status = GL_NORMAL;
		}
		if (addchar) {
			Editor_Putchar(t, key);
		}
	} while (t->curr->len < t->max);

	return EDIT_NULL;
}

/****************************************************************************/
/* Inserisce il carattere 'c' nella posizione corrente del testo. */
static void Editor_Putchar(Editor_Text *t, int c)
{
	int i;

	if ((t->insert == MODE_INSERT) && (t->curr->pos != t->curr->len)) {
		memmove(t->curr->str + t->curr->pos + 1,
			t->curr->str + t->curr->pos,
			(t->curr->len - t->curr->pos) * sizeof(int));
		memmove(t->curr->col + t->curr->pos + 1,
			t->curr->col + t->curr->pos,
			(t->curr->len - t->curr->pos) * sizeof(int));
		t->curr->len++;
	}
	t->curr->str[t->curr->pos] = c;
	t->curr->col[t->curr->pos] = t->curs_col;

	if (t->insert == MODE_ASCII_ART) {
		putchar(c);
		if (t->curr->pos > t->curr->len) {
			printf("\nO-OH!!!!!!!!!!!! Avvisa il tuo sviluppatore di fiducia!!!\n");
		}
		if (t->curr->pos == t->curr->len)
			t->curr->len++;
		return;
	}

	t->curr->pos++;
	if (t->curr->pos > t->curr->len)
		t->curr->len++;
	for (i = t->curr->pos-1; i < t->curr->len; i++) {
		if (t->curr->col[i] != t->curr_col) {
			t->curr_col = t->curr->col[i];
			setcolor(t->curr_col);
		}
		putchar(t->curr->str[i]);
	}
	if (t->curr->len >= t->max)
		Editor_Wrap_Word(t);
	cti_mv(t->curr->pos+1, Editor_Vcurs);
}

/* Processa <Enter> */
static void Editor_Key_Enter(Editor_Text *t)
{
        Editor_Insert_Line(t);
	if (t->curr->pos < t->curr->len) {
		Editor_Line *new_line = t->curr->next;
		memcpy(new_line->str, t->curr->str + t->curr->pos,
		       (t->curr->len - t->curr->pos + 1) * sizeof(int));
		memcpy(new_line->col, t->curr->col + t->curr->pos,
		       (t->curr->len - t->curr->pos + 1) * sizeof(int));
		new_line->len = t->curr->len - t->curr->pos;
		t->curr->len = t->curr->pos;
		// TODO: final \0 is needed?
                setcolor(C_DEFAULT);
		// TODO: Eliminate the loop, use ansi
		for(int i = 0; i < new_line->len; i++) {
			putchar(' ');
		}
	}
        t->curr->next->pos = 0;
        t->curr->pos = 0;
	Editor_Newline(t);

	line_refresh(t->curr, Editor_Vcurs, 0);
	setcolor(t->curr_col);
}

/* Effettua un backspace cancellando gli eventuali allegati */
static void Editor_Key_Backspace(Editor_Text *t)
{
        int mdnum;

        mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos]);
        if ((t->curr->pos)
            && ( (mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos-1])))) {
                do {
                        Editor_Backspace(t);
                } while ((t->curr->pos) &&
                     (mdnum == Editor_Get_MDNum(t->curr->col[t->curr->pos-1])));
                mdop_delete(t->mdlist, mdnum);
        } else
                Editor_Backspace(t);
}

/* Effettua un backspace */
static void Editor_Backspace(Editor_Text *t)
{
	DEB(t, "editor_backspace_*******************");
	/* if the cursor is at the line start, merge with the line above */
	if (t->curr->pos == 0) {
		if (t->curr->prev) {
			//Editor_Line *above = t->curr->prev;
			int above_len = t->curr->prev->len;
			Editor_Line *below = t->curr;
			Editor_Up(t); /* make t->curr the line above */
			switch (Editor_Merge_Lines(t)) {
			case MERGE_EXTRA_SPACE:
				//DEB(t, "extra");
				t->curr->pos = above_len + 1;
				Editor_Refresh(t, Editor_Vcurs);
				break;
			case MERGE_ABOVE_EMPTY:
				//DEB(t, "above");
				/* The above line was deleted */
				t->curr = below;
				t->curr->pos = 0;
				Editor_Refresh(t, Editor_Vcurs);
				break;
			case MERGE_REGULAR:
			case MERGE_BELOW_EMPTY:
				//DEB(t, "regular");
				//DEB(t, "below");
				t->curr->pos = above_len;
				Editor_Refresh(t, Editor_Vcurs);
				break;
			case MERGE_NOTHING: /*  Can I even get here?? */
				//DEB(t, "nothing");
				t->curr->pos = above_len;
				line_refresh(t->curr, Editor_Vcurs, 0);
				break;
			}
			setcolor(t->curr_col);
		} else {
			Beep();
		}
		return;
	}

	/* Cancella il carattere precedente */
	if (t->curr->pos != t->curr->len) {
		int *src_str = t->curr->str + t->curr->pos;
		int *src_col = t->curr->col + t->curr->pos;
		size_t bytes = (t->curr->len - t->curr->pos)*sizeof(int);
		memmove(src_str - 1, src_str, bytes);
		memmove(src_col - 1, src_col, bytes);
	}
	t->curr->str[--t->curr->len] = '\0';
	t->curr->pos--;
	line_refresh(t->curr, Editor_Vcurs, t->curr->pos);
	setcolor(t->curr_col);
}

/* Cancella il carattere sottostante al cursore, eliminando gli allegati */
static void Editor_Key_Delete(Editor_Text *t)
{
        int mdnum;

        if ( (mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos]))) {
                do {
                        Editor_Delete(t);
                } while (mdnum == Editor_Get_MDNum(t->curr->col[t->curr->pos]));
                mdop_delete(t->mdlist, mdnum);
        } else
                Editor_Delete(t);
}

/* Cancella il carattere sottostante al cursore. */
static void Editor_Delete(Editor_Text *t)
{
	int i, tmpcol;

	if (t->curr->pos == t->curr->len) {
		if (t->curr->next) {
		        /* len = t->curr->len; */
			if (Editor_Merge_Lines(t))
				Editor_Refresh(t, Editor_Vcurs);
			/* t->curr->pos = (len > 0) ? len+1 : 0; */
			setcolor(t->curr_col);
		} else
			Beep();
		return;
	}
	memmove(t->curr->str+t->curr->pos, t->curr->str+t->curr->pos+1,
		(t->curr->len-t->curr->pos)*sizeof(int));
	memmove(t->curr->col+t->curr->pos, t->curr->col+t->curr->pos+1,
		(t->curr->len-t->curr->pos)*sizeof(int));
	t->curr->str[--t->curr->len] = '\0';

	tmpcol = t->curr_col;
	for (i = t->curr->pos; i < t->curr->len; i++) {
		if (tmpcol != t->curr->col[i]) {
			setcolor(t->curr->col[i]);
			tmpcol = t->curr->col[i];
		}
		putchar(t->curr->str[i]);
	}
	putchar(' ');
	if (tmpcol != t->curr_col)
		setcolor(t->curr_col);
	cti_mv(t->curr->pos+1, Editor_Vcurs);
}

/* Cancella la parola precedente al cursore. */
static void Editor_Delete_Word(Editor_Text *t)
{
	int tmp;

	if (t->curr->pos == 0) {
		Beep();
		return;
	}
	tmp = t->curr->pos;
	while ((t->curr->pos != 0) && (t->curr->str[t->curr->pos-1] == ' '))
		t->curr->pos--;
	while ((t->curr->pos != 0) && (t->curr->str[t->curr->pos-1] != ' '))
		t->curr->pos--;
	memmove(t->curr->str + t->curr->pos, t->curr->str + tmp,
		(t->curr->len - tmp) * sizeof(int));
	memmove(t->curr->col + t->curr->pos, t->curr->col + tmp,
		(t->curr->len - tmp) * sizeof(int));
	t->curr->len -= tmp - t->curr->pos;
	t->curr->str[t->curr->len] = '\0';

	line_refresh(t->curr, Editor_Vcurs, t->curr->pos);
	setcolor(t->curr_col);
}

/* Cancella la parola successiva al cursore. */
static void Editor_Delete_Next_Word(Editor_Text *t)
{
	int tmp;

	tmp = t->curr->pos;
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] == ' '))
		t->curr->pos++;
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] != ' '))
		t->curr->pos++;
	while ((t->curr->pos != t->curr->len)
	       && (t->curr->str[t->curr->pos+1] == ' '))
		t->curr->pos++;
	memmove(t->curr->str + tmp, t->curr->str + t->curr->pos,
		(t->curr->len - t->curr->pos)*sizeof(int));
	memmove(t->curr->col + tmp, t->curr->col + t->curr->pos,
		(t->curr->len - t->curr->pos) * sizeof(int));
	t->curr->len -= t->curr->pos - tmp;
	t->curr->pos = tmp;
	t->curr->str[t->curr->len] = '\0';

	line_refresh(t->curr, Editor_Vcurs, t->curr->pos);
	setcolor(t->curr_col);
}

/*
 * If the current line is not empty, erase its content, otherwise eliminate
 * the line.
 */
static void Editor_Kill_Line(Editor_Text *t)
{
	if ((t->curr->len == 0) && t->curr->next) {
		Editor_Line *next_line = t->curr->next;
		Editor_Delete_Line(t, t->curr);
		t->curr = next_line;
		t->curr->pos = 0;
		Editor_Refresh(t, Editor_Vcurs);
	} else if (t->curr->pos == t->curr->len) {
		Editor_Delete(t);
	} else {
		/* copy the line to the copy buffer */
		Editor_Copy_Line(t);
		/* Eliminate the text from the cursor to the end of line */
		t->curr->str[t->curr->pos] = '\0';
		t->curr->len = t->curr->pos;
		/* cti_mv(t->curr->pos+1, Editor_Vcurs); */

		/* Update the screen accordingly */
		setcolor(C_DEFAULT);
		printf(ERASE_TO_EOL);
		setcolor(t->curr_col);
		/*
		  for (int i = t->curr->pos; i < t->curr->len; i++) {
		  putchar(' ');
		  }
		*/
	}
	t->copy = true;
}

/* Process <Y>ank */
static void Editor_Yank(Editor_Text *t)
{
        if (t->buf_riga == 0) { /* copy buffer is empty */
                return;
	}
	/* unless the cursor sits in an empty line, insert a new empty
	 * at the cursor's location (breaking the current line if needed) */
	if (t->curr->len) {
		bool cursor_at_line_start = (t->curr->pos == 0);
		bool cursor_at_line_end = (t->curr->pos == t->curr->len);
		Editor_Key_Enter(t);
		if (cursor_at_line_start) {
			Editor_Up(t);
			t->curr->pos = 0;
			t->curr->next->pos = 0;
			Editor_Refresh(t, Editor_Vcurs);
			DEB(t, "Y-1");
		} else if (cursor_at_line_end) {
			/* do nothing */
			DEB(t, "Y-2");
		} else {
			Editor_Insert_Line_Here(t);
			t->curr = t->curr->prev;
			t->curr->pos = 0;
			t->curr->next->pos = 0;
			Editor_Refresh(t, Editor_Vcurs);
			DEB(t, "Y-3");
		}
	}

	assert(t->curr->len == 0);
	assert(t->curr->pos == 0);
        for (Editor_Line *src = t->buf_first; src; ) {
                Editor_Line *new_line = t->curr;
		memcpy(new_line->str, src->str, src->len*sizeof(int));
		memcpy(new_line->col, src->col, src->len*sizeof(int));
		new_line->len = src->len;
		new_line->pos = src->len;
                line_refresh(t->curr, Editor_Vcurs, 0);
                src = src->next;
                if (src) {
                        Editor_Key_Enter(t);
                }
        }
        t->curr->pos = t->curr->len;
        line_refresh(t->curr, Editor_Vcurs, 0);
	setcolor(t->curr_col);
        t->buf_pasted = true;
}

static void Editor_Key_Tab(Editor_Text *t)
{
	if (t->insert == MODE_ASCII_ART) {
		do
			Editor_Key_Right(t);
		while (((t->curr->pos) % CLIENT_TABSIZE));
		return;
	}
	if ((t->max - t->curr->pos) < CLIENT_TABSIZE)
		Editor_Key_Enter(t);
	else {
		do
			Editor_Putchar(t, ' ');
		while (((t->curr->pos) % TABSIZE));
	}
}

static void Editor_Key_Up(Editor_Text *t)
{
	int i, mdnum;

	assert(t->curr);
	if (t->curr->prev == NULL) {
		Beep();
		return;
	}
	Editor_Up(t);
	t->curr->pos = t->curr->next->pos;
	if (t->curr->pos >= t->curr->len) {
		if (t->insert == MODE_ASCII_ART) {
			for (i = t->curr->len; i <= t->curr->pos; i++){
				t->curr->str[i] = ' ';
				t->curr->col[i] = C_DEFAULT;
			}
			t->curr->len = t->curr->pos + 1;
		} else
			t->curr->pos = t->curr->len;
	}

        /* Se finisco in un oggetto metadata, vai indietro */
        if ( (mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos])))
                do {
                        Editor_Curs_Right(t);
                } while (mdnum == Editor_Get_MDNum(t->curr->col[t->curr->pos]));


	line_refresh(t->curr, Editor_Vcurs, 0);
	setcolor(t->curr_col);
}

static void Editor_Key_Down(Editor_Text *t)
{
	int i, mdnum;

	if (t->insert == MODE_ASCII_ART) {
		if (!t->curr->next)
			Editor_Insert_Line(t);
		Editor_Down(t);
		t->curr->pos = t->curr->prev->pos;
		if (t->curr->pos >= t->curr->len) {
			for (i = t->curr->len; i <= t->curr->pos; i++){
				t->curr->str[i] = ' ';
				t->curr->col[i] = C_DEFAULT;
			}
			t->curr->len = t->curr->pos + 1;
		}
	} else if (t->curr->next) {
		Editor_Down(t);
		t->curr->pos = t->curr->prev->pos;
		if (t->curr->pos > t->curr->len)
			t->curr->pos = t->curr->len;
        } else {
                Beep();
                return;
        }

        /* Se finisco in un oggetto metadata, vai indietro */
        if ( (mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos])))
                do {
                        Editor_Curs_Right(t);
                } while (mdnum == Editor_Get_MDNum(t->curr->col[t->curr->pos]));

        line_refresh(t->curr, Editor_Vcurs, 0);
        setcolor(t->curr_col);
}

/* Key right: skips metadata objects */
static void Editor_Key_Right(Editor_Text *t)
{
        int mdnum;

        if ((t->curr->pos < t->curr->len)
            && ( (mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos]))))
                do {
                        Editor_Curs_Right(t);
                } while (mdnum == Editor_Get_MDNum(t->curr->col[t->curr->pos]));
        else
                Editor_Curs_Right(t);
}

/* Push cursor right one character */
static void Editor_Curs_Right(Editor_Text *t)
{
	if (t->insert == MODE_ASCII_ART) {
		if (t->curr->pos < t->curr->len-1) {
			t->curr->pos++;
			cti_mv(t->curr->pos+1, Editor_Vcurs);
			return;
		}
		if (t->curr->len >= t->max-1) {
			if (!t->curr->next)
				Editor_Insert_Line(t);
			Editor_Down(t);
			t->curr->pos = 0;
			line_refresh(t->curr, Editor_Vcurs, 0);
			setcolor(t->curr_col);
		} else {
			if (t->curr->pos == t->curr->len) {
				t->curr->str[t->curr->pos] = ' ';
				t->curr->col[t->curr->pos] = C_DEFAULT;
				t->curr->len++;
			}
			t->curr->len++;
			t->curr->pos++;
			t->curr->str[t->curr->pos] = ' ';
			t->curr->col[t->curr->pos] = C_DEFAULT;
			cti_mv(t->curr->pos+1, Editor_Vcurs);
		}
		return;
	}
	if (t->curr->pos < t->curr->len) {
		cti_mv((++t->curr->pos)+1, Editor_Vcurs);
		return;
	}
	if (t->curr->next) {
		Editor_Down(t);
		t->curr->pos = 0;
		line_refresh(t->curr, Editor_Vcurs, 0);
		setcolor(t->curr_col);
	} else
		Beep();
}

/* Key left: skips metadata objects */
static void Editor_Key_Left(Editor_Text *t)
{
        int mdnum, mdnum1;

        mdnum = Editor_Get_MDNum(t->curr->col[t->curr->pos]);
        if ((t->curr->pos)
            && ( (mdnum1 = Editor_Get_MDNum(t->curr->col[t->curr->pos-1]))
                 != mdnum) && mdnum1) {
                do {
                        Editor_Curs_Left(t);
                } while ((t->curr->pos) &&
                    (mdnum1 == Editor_Get_MDNum(t->curr->col[t->curr->pos-1])));
        } else
                Editor_Curs_Left(t);
}

/* Push cursor left one character */
static void Editor_Curs_Left(Editor_Text *t)
{
	if (t->curr->pos)
		cti_mv(t->curr->pos--, Editor_Vcurs);
	else if (t->curr->prev) {
		Editor_Up(t);
		t->curr->pos = t->curr->len;
		if ((t->insert == MODE_ASCII_ART) && (t->curr->pos > 0))
			t->curr->pos--;
		line_refresh(t->curr, Editor_Vcurs, 0);
		setcolor(t->curr_col);
	} else
		Beep();
}

/* Va alla pagina precedente */
static void Editor_PageUp(Editor_Text *t)
{
	if (t->curr->prev == NULL) {
		Beep();
		t->curr->pos = 0;
		cti_mv(1, Editor_Vcurs);
		return;
	}
	for (int i = Editor_Vcurs; (i > Editor_Pos) && t->curr->prev; i--) {
		t->curr = t->curr->prev;
	}
	if (t->curr->num < (NRIGHE - 1 - Editor_Pos)) {
		t->curr = t->first;
		Editor_Vcurs = Editor_Pos + 1;
	} else {
		Editor_Vcurs = NRIGHE - 1;
	}
	t->curr->pos = 0;
	Editor_Refresh(t, 0);
}

/* Va alla pagina successiva */
static void Editor_PageDown(Editor_Text *t)
{
	int i;

	if (t->curr->next == NULL) {
		Beep();
		t->curr->pos = t->curr->len;
		cti_mv(t->curr->pos+1, Editor_Vcurs);
		return;
	}
	for (i = Editor_Vcurs; (i < NRIGHE) && t->curr->next; i++)
		t->curr = t->curr->next;
	if (t->last->num - t->curr->num < NRIGHE - Editor_Pos - 2) {
		t->curr = t->last;
		Editor_Vcurs = NRIGHE-1;
	} else
		Editor_Vcurs = Editor_Pos+1;
	t->curr->pos = 0;
	Editor_Refresh(t, 0);
}

/* Scroll down the region from the current row to the bottom of the terminal,
 * resulting in an empty line on the current row. */
void EdTerm_Scroll_Down(void)
{
	window_push(Editor_Vcurs, NRIGHE-1);
	cti_mv(0, Editor_Vcurs);
	scroll_down();
	window_pop();
}

/* Va alla riga successiva del testo. */
static void Editor_Newline(Editor_Text *t)
{
	assert(t->curr->next);

	t->curr = t->curr->next;
	if (Editor_Vcurs != NRIGHE-1) {
		Editor_Vcurs++;
		EdTerm_Scroll_Down();
		cti_mv(0, Editor_Vcurs);
	} else {
		Editor_Scroll_Up(NRIGHE-1);
	}
}

/*
 * Insert a new line below the current line. The current line is not modified.
 */
static void Editor_Insert_Line(Editor_Text *t)
{
	Editor_Line *new_line;
	int num;

	CREATE(new_line, Editor_Line, 1, 0);
	new_line->len = 0;
	new_line->prev = t->curr;
	new_line->next = t->curr->next;
	t->curr->next = new_line;
	if (new_line->next) {
		new_line->next->prev = new_line;
	} else {
		t->last = new_line;
	}
	num = t->curr->num;
	for (Editor_Line *l = new_line; l; l = l->next) {
		l->num = ++num;
	}
	t->riga++;
}

/* Insert a new line above the current line, without changing the
 * current line. */
static void Editor_Insert_Line_Here(Editor_Text *t)
{
	Editor_Line *new_line;

	CREATE(new_line, Editor_Line, 1, 0);
	new_line->len = 0;
	new_line->prev = t->curr->prev;
	new_line->next = t->curr;
	t->curr->prev = new_line;
	if (new_line->prev) {
		new_line->prev->next = new_line;
	} else {
		t->first = new_line;
	}

	int num = t->curr->num;
	for (Editor_Line *line = new_line; line; line = line->next) {
		line->num = num++;
	}
	t->riga++;
}

/*
 * Copies the text in the current line (t->curr) from the cursor position to
 * the end into a new line that is appended to the copy buffer. The current
 * line is not modified.
 */
static void Editor_Copy_Line(Editor_Text *t)
{
	Editor_Line *new_line;

        if (t->copy == false) {
                Editor_Free_Copy_Buffer(t);
	}

	/* Allocate a new line and append it at the end of the buffer list */
	CREATE(new_line, Editor_Line, 1, 0);
        new_line->next = NULL;
        if (t->buf_riga == 0) {
                new_line->prev = NULL;
                t->buf_first = new_line;
        } else {
                new_line->prev = t->buf_last;
                t->buf_last->next = new_line;
        }
        t->buf_last = new_line;
	t->buf_riga++;

	/* copy the contents of current line starting from cursor */
	Editor_Line *src = t->curr;
	int *str = src->str + src->pos;
	int *col = src->col + src->pos;
	int count = src->len - src->pos;
	memcpy(new_line->str, str, count*sizeof(int));
	memcpy(new_line->col, col, count*sizeof(int));
        new_line->len = count;
	new_line->str[new_line->len] = '\0';
}

/* Elimina la riga *line */
static void Editor_Delete_Line(Editor_Text *t, Editor_Line *line)
{
        for (Editor_Line *tmp = line->next; tmp; tmp = tmp->next) {
		tmp->num--;
	}
	t->riga--;

	if (line == t->first) {
		t->first = line->next;
	}
	if (line == t->last) {
		t->last = line->prev;
	}
	if (line->next) {
		line->next->prev = line->prev;
	}
	if (line->prev) {
		line->prev->next = line->next;
	}
	assert(t->first->prev == NULL);
	assert(t->last->next == NULL);
	//DEB(t, "delete line OK");

	/*
	if (line->next) {
		line->next->prev = line->prev;
	} else {
		t->last = line->prev;
	}

	if (line->prev) {
		line->prev->next = line->next;
	} else {
		t->first = line->next;
	}
	*/
	free(line);
}

/*
 * Sposta l'ultima parola della riga corrente alla riga successiva se c'e`
 * posto, altrimenti in una nuova riga che inserisce.
 * Ritorna 1 se ha effettuato l'operazione, 0 se non c'e` posto.
 */
static int Editor_Wrap_Word(Editor_Text *t)
{
	Editor_Line *nl;
	int len, wlen = 1, parolone = FALSE;
        int mdnum;

	len = t->curr->len;
	if (t->curr->next == NULL)
		Editor_Insert_Line(t);
	nl = t->curr->next;

        while ((wlen <= len) && (t->curr->str[len-wlen] == ' '))
                wlen++;

        mdnum = Editor_Get_MDNum(t->curr->col[len-wlen]);
        if (mdnum) { /* Wrappa l'allegato in blocco */
                while ((Editor_Get_MDNum(t->curr->col[len-wlen]) == mdnum)
                       && (wlen <= len))
                        wlen++;
        } else { /* Normale algoritmo per la parola */
                while ((wlen <= len) && (t->curr->str[len-wlen] != ' '))
                        wlen++;
        }

	if (wlen >= len) {/* un'unico lungo parolone senza senso... */
		wlen = len - t->curr->pos + 1;
		if (wlen <= 0)
			wlen = 1;
		if (wlen > len)
			wlen = 10;
		parolone = TRUE;
	}

	if ((nl->len + wlen + 1) >= t->max) {
		/* La parola non ci sta nella riga successiva */
		Editor_Insert_Line(t);
		nl = t->curr->next;
		Editor_Scroll_Down(Editor_Vcurs + 1, NRIGHE-1);
		line_refresh(t->curr->next, Editor_Vcurs + 1, 0);
		setcolor(t->curr_col);
		cti_mv(t->curr->pos + 1, Editor_Vcurs);
	}
	len -= wlen; /* Tolgo la parola */

	if (nl->len > 0) {
		/* Riga succ. non vuota: sposto il contenuto */
		memmove(nl->str + wlen, nl->str,
			nl->len * sizeof(int));
		memmove(nl->col + wlen, nl->col,
			nl->len * sizeof(int));
		nl->str[wlen-1] = ' ';
                if (Editor_Get_MDNum(nl->col[wlen]) == 0)
                        nl->col[wlen-1] = nl->col[wlen];
                else
                        nl->col[wlen-1] = C_DEFAULT;
		nl->len += wlen;
	} else
		nl->len = wlen - 1;

	if (parolone) {
		nl->len++;
		memcpy(nl->str, t->curr->str+len, wlen * sizeof(int));
		memcpy(nl->col, t->curr->col+len, wlen * sizeof(int));
	} else {
		/* Ricopio la parola */
		memcpy(nl->str, t->curr->str+len+1, (wlen-1) * sizeof(int));
		memcpy(nl->col, t->curr->col+len+1, (wlen-1) * sizeof(int));
	}

	t->curr->len = len;

	if (t->curr->pos > len) {
		nl->pos = t->curr->pos - len - 1;
		if (parolone)
			nl->pos++;
		t->curr = nl;
		if (Editor_Vcurs != NRIGHE-1) {
			Editor_Vcurs++;
			cti_mv(t->curr->pos + 1, Editor_Vcurs);
		} else
			Editor_Scroll_Up(NRIGHE-1);
		line_refresh(t->curr->prev, Editor_Vcurs - 1, 0);
	}

	Editor_Refresh(t, Editor_Vcurs);
	cti_mv(t->curr->pos + 1, Editor_Vcurs);
	return 1;
}

/*
 * Merges the line 'above' with the line 'below'. If everything fits on a
 * single line the contents of the line below (if any) are appended to
 * the above line and the line below is eliminated. Similarly, if the
 * above line is empty, it is simply eliminated.
 * Otherwise, moves as much as fits from the line below to the above
 * line, inserting an extra space if needed to separate the words, and
 * wrapping the line at word boundaries.
 *
 * Returns an Editor_Merge_Result value specifying how the merge was
 * performed (see the enum definition for the descriptions).
 */
static Merge_Lines_Result text_merge_lines(Editor_Text *t, Editor_Line *above,
					   Editor_Line *below)
{
	if (above == NULL || below == NULL) {
		return MERGE_NOTHING;
	}

	/* If line above is empty, just eliminate it. */
	if (above->len == 0) {
		Editor_Delete_Line(t, above);
		return MERGE_ABOVE_EMPTY;
	}

	/* If the line below is empty, just eliminate it. */
	if (below->len == 0) {
		Editor_Delete_Line(t, below);
		return MERGE_BELOW_EMPTY;
	}

	/* Is an extra space necessary to keep words separated after merge? */
	bool need_extra_space = (above->str[above->len - 1] != ' '
				 && below->str[0] != ' ');
	int extra_space = need_extra_space ? 1 : 0;
	/* Remaining space available in the above line */
	int avail_chars = t->max - above->len - extra_space;

	/* Find out how much of the string below can be appended to the
	   string above without breaking any word */
	int len = 0;
	if (avail_chars < below->len) {
		for (int i = 0; i < avail_chars; i++) {
			if (below->str[i] == ' ') {
				len = i;
			}
		}
	} else {
		len = below->len;
	}

	/* If a substring fits, move it above. */
	if (len > 0) {
		int ret;
		/* BUG HAS HERE */
		if (need_extra_space) {
			assert(above->len > 0);
			above->str[above->len] = ' ';
			above->col[above->len] = above->col[above->len - 1];
			above->len++;
			ret = MERGE_EXTRA_SPACE;
		} else {
			ret = MERGE_REGULAR;
		}
		int *dest_str = above->str + above->len;
		int *dest_col = above->col + above->len;
		int *src_str = below->str;
		int *src_col = below->col;
		int *rest_str = below->str + len + 1;
		int *rest_col = below->col + len + 1;
		size_t bytes_to_copy = len*sizeof(int);
		size_t remaining_bytes = (below->len - len)*sizeof(int);
		memcpy(dest_str, src_str, bytes_to_copy);
		memcpy(dest_col, src_col, bytes_to_copy);
		memmove(src_str, rest_str, remaining_bytes);
		memmove(src_col, rest_col, remaining_bytes);

		above->len += len;
		below->len -= len;
		if (below->len <= 0) {
			DEB(t, "delete below");
			Editor_Delete_Line(t, below);
			//ret = MERGE_BELOW_EMPTY; /* CHECK */
		}
		return ret;
	}
	/* There's no room in the above line to fit something from below */
	return MERGE_NOTHING;
}

static Merge_Lines_Result Editor_Merge_Lines(Editor_Text *t)
{
	return text_merge_lines(t, t->curr, t->curr->next);
}

/* Scrolla in su di una riga dello schermo. Se l'editor raggiunge la dimensione
 * massima, setta la finestra di scrolling sullo spazio disponibile.         */
static void Editor_Scroll_Up(int stop)
{
	if (!editor_reached_full_size) {
		if (Editor_Pos > MSG_WIN_SIZE) { /*L'editor e' ancora piccino*/
			Editor_Pos--;
			if (stop != NRIGHE-1) {
				window_push(0, stop);
				cti_mv(0, stop);
				scroll_up();
				window_pop();
				cti_mv(0, stop);
			}
			scroll_up();
		} else { /* Estensione massima dell'editor: setta finestra */
			window_push(Editor_Pos + 1, stop);
			editor_reached_full_size = true;
			cti_mv(0, stop);
			scroll_up();
		}
	} else if (stop != NRIGHE - 1){ /* E' una sottofinestra */
		window_push(Editor_Pos + 1, stop);
		cti_mv(0, stop);
		scroll_up();
		window_pop();
		cti_mv(0, stop);
	} else /* Scrolla tutto il testo */
		scroll_up();
}

/* Scrolla il giu' la regione di testo tra start e stop */
static void Editor_Scroll_Down(int start, int stop)
{
	if (editor_reached_full_size) {
		window_push(start, stop);
		cti_mv(0, start);
		scroll_down();
		window_pop();
	} else {
		Editor_Scroll_Up(start - 1);
		Editor_Vcurs--;
	}
	cti_mv(Editor_Hcurs, Editor_Vcurs);
}

/* Moves the cursor to the line above and updates the display */
static void Editor_Up(Editor_Text *t)
{
	assert(t->curr->prev);

	t->curr = t->curr->prev;
	if (editor_reached_full_size && (Editor_Vcurs == Editor_Pos + 1)) {
		scroll_down();
	} else {
		Editor_Vcurs--;
	}
}

/* Vai alla riga successiva */
static void Editor_Down(Editor_Text *t)
{
	t->curr = t->curr->next;
	if (Editor_Vcurs == NRIGHE-1) {
		Editor_Scroll_Up(NRIGHE-1);
	} else {
		Editor_Vcurs++;
	}
}

/* Modifica il colore del cursore */
static void Editor_Set_Color(Editor_Text *t, int c)
{
	t->curs_col = c;
	if ((t->curr_col & 0xffff) != (t->curs_col & 0xffff)) {
		t->curr_col = t->curs_col;
		setcolor(t->curs_col & 0xffff);
		fflush(stdout);
	}
}

/* Inserisce del metadata */
static void Editor_Insert_Metadata(Editor_Text *t)
{
        int c;

	/*
	  NOTE: it is responsibility of the Editor_Insert_* functions called
	        below to execute window_pop() _before_ the text buffer is
	        modified, and anyways before returning.
	*/
        if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	fill_line(Editor_Pos, COL_HEAD_MD);
        make_cursor_invisible();
        cml_printf(_("<b>--- Scegli ---</b> \\<<b>f</b>>ile upload \\<<b>l</b>>ink \\<<b>p</b>>ost \\<<b>r</b>>oom \\<<b>t</b>>ext file \\<<b>u</b>>ser \\<<b>a</b>>bort"));
        do {
                c = inkey_sc(true);
        } while ((c == 0) || !index("aflprtu\x1b", c));

        switch (c) {
        case 'f':
                Editor_Insert_File(t);
                break;
        case 'l':
                Editor_Insert_Link(t);
                break;
        case 'p':
                Editor_Insert_PostRef(t);
                break;
        case 'r':
                Editor_Insert_Room(t);
                break;
        case 't':
                Editor_Insert_Text(t);
                break;
        case 'u':
                Editor_Insert_User(t);
                break;
        }
        make_cursor_visible();
        Editor_Head_Refresh(t, true);
}

/* Inserisce il riferimento a un post */
static void Editor_Insert_PostRef(Editor_Text *t)
{
        char roomname[LBUF], locstr[LBUF];
        char *ptr, tmpcol;
        int id, col, offset = 0;
        long local_number;

	erase_current_line();
        cml_printf("<b>--- Room [%s]:</b> ", postref_room);
        get_roomname("", roomname, true);
        if (roomname[0] == 0) {
                strcpy(roomname, postref_room);
        }
	if (roomname[0]) {
		if (roomname[0] == ':') {
			offset++;
		}
		cti_mv(40, Editor_Pos);
		local_number = new_long_def(" <b>msg #</b>", postref_locnum);
	}

	if (editor_reached_full_size) {
		window_pop();
	}

	if (roomname[0] == 0) {
		return;
	}

        Editor_Head_Refresh(t, true);
        id = md_insert_post(t->mdlist, roomname, local_number);

        tmpcol = t->curs_col;
        col = COLOR_ROOM;
        Editor_Set_MDNum(col, id);
        Editor_Set_Color(t, col);
        for (ptr = roomname+offset; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        col = COLOR_ROOMTYPE;
        Editor_Set_MDNum(col, id);
        Editor_Set_Color(t, col);
        if (offset) {
                Editor_Putchar(t, ':');
	} else {
                Editor_Putchar(t, '>');
	}

        if (local_number) {
                col = COLOR_LOCNUM;
                Editor_Set_MDNum(col, id);
                Editor_Set_Color(t, col);
                Editor_Putchar(t, ' ');
                Editor_Putchar(t, '#');
                sprintf(locstr, "%ld", local_number);
                for (ptr = locstr; *ptr; ptr++) {
                        Editor_Putchar(t, *ptr);
		}
        }

        Editor_Set_Color(t, tmpcol);
}

/* Inserisce il riferimento a una room */
static void Editor_Insert_Room(Editor_Text *t)
{
        char roomname[LBUF];
        const char *ptr;
	char tmpcol;
        int id, col;

	erase_current_line();
        cml_printf("<b>--- Room [%s]:</b> ", postref_room);
        get_roomname("", roomname, true);
        if (roomname[0] == 0) {
                strcpy(roomname, postref_room);
        }
        if (editor_reached_full_size) {
		window_pop();
	}
        if (roomname[0] == 0) {
		return;
	}

        Editor_Head_Refresh(t, true);

        id = md_insert_room(t->mdlist, roomname);

        tmpcol = t->curs_col;

	int offset = 0;
	if (roomname[0] == ':') { /* Blog room */
		col = COLOR_ROOMTYPE;
		Editor_Set_MDNum(col, id);
		Editor_Set_Color(t, col);
		for (ptr = blog_display_pre; *ptr; ptr++) {
			Editor_Putchar(t, *ptr);
		}
		offset = 1;
        }

        col = COLOR_ROOM;
        Editor_Set_MDNum(col, id);
        Editor_Set_Color(t, col);
        for (ptr = roomname + offset; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        col = COLOR_ROOMTYPE;
        Editor_Set_MDNum(col, id);
        Editor_Set_Color(t, col);
        Editor_Putchar(t, '>');

        Editor_Set_Color(t, tmpcol);
}

/* Inserisce il riferimento a utente */
static void Editor_Insert_User(Editor_Text *t)
{
        char username[LBUF];
        char *ptr, tmpcol;
        int id, col;

	erase_current_line();
        cml_printf("<b>--- Utente [%s]: </b> ", last_profile);
        get_username("", username);
        if (username[0] == 0) {
                strncpy(username, last_profile, MAXLEN_UTNAME);
        }

	if (editor_reached_full_size) {
		window_pop();
	}

	if (username[0] == 0) {
		return;
	}

        Editor_Head_Refresh(t, true);

        id = md_insert_user(t->mdlist, username);

        tmpcol = t->curs_col;
        col = COLOR_USER;
        Editor_Set_MDNum(col, id);
        Editor_Set_Color(t, col);
        for (ptr = username; *ptr; ptr++) {
                Editor_Putchar(t, *ptr);
	}

        Editor_Set_Color(t, tmpcol);
}

/* Upload di un file */
static void Editor_Insert_File(Editor_Text *t)
{
        char buf[LBUF], path[LBUF], filename[LBUF];
        int id, tmpcol, filecol, c=0;
        char *ptr, *fullpath;
        size_t pathlen;
        FILE *fp;
        struct stat filestat;
        unsigned long filenum, len, flags;

	if (local_client) {
		erase_current_line();
        } else {
                setcolor(COL_HEAD_ERROR);
		erase_current_line();
                cml_printf(_(
" *** Server il client locale per l'upload dei file.    -- Premi un tasto --"
			     ));
                while (c == 0) {
                        c = getchar();
		}
		if (editor_reached_full_size) {
			window_pop();
		}
                return;
        }

	erase_current_line();
	path[0] = 0;
        if ( (pathlen = getline_scroll("<b>File name:</b> ", COL_HEAD_MD, path,
                                       LBUF-1, 0, 0, Editor_Pos) > 0)) {
	        find_filename(path, filename, sizeof(filename));
                if (filename[0] == 0) {
			if (editor_reached_full_size) {
				window_pop();
			}
                        return;
		}
                fullpath = interpreta_tilde_dir(path);

                fp = fopen(fullpath, "r");
                if (fp == NULL) {
			fill_line(Editor_Pos, COL_HEAD_ERROR);
                        cml_printf(_(
" *** File inesistente.            -- Premi un tasto per continuare --"
				     ));
                        while (c == 0) {
                                c = getchar();
			}
                        if (fullpath) {
                                Free(fullpath);
			}
			if (editor_reached_full_size) {
				window_pop();
			}
                        return;
                }
                fstat(fileno(fp), &filestat);
                len = (unsigned long)filestat.st_size;
                if (len == 0) {
			fill_line(Editor_Pos, COL_HEAD_ERROR);
                        cml_printf(_(" *** Mi rifiuto di allegare file vuoti!     -- Premi un tasto per continuare --"));
                        while (c == 0)
                                c = getchar();
                        if (fullpath)
                                Free(fullpath);
                        fclose(fp);
			if (editor_reached_full_size) {
				window_pop();
			}
                        return;
                }

                serv_putf("FUPB %s|%lu", filename, len);
                serv_gets(buf);

                if (buf[0] == '1') {
			fill_line(Editor_Pos, COL_HEAD_ERROR);
                        switch (buf[1]) {
                        case '3':
                                cml_printf(_(" *** Non si possono allegare file in questa room.     -- Premi un tasto --"));
                                break;
                        case '4':
                                cml_printf(_(" *** Questo file &egrave; troppo grande per essere allegato!   --- Premi un tasto ---"));
                                break;
                        case '5':
                                cml_printf(_(" *** Non hai spazio personale sufficiente! -- Premi un tasto per continuare --"));
                                break;
                        case '6':
                                cml_printf(_(" *** Hai gia uploadato troppi file!    --- Premi un tasto per continuare ---  "));
                                break;
                        case '7':
                                cml_printf(_(" *** Non c'&egrave; spazio a sufficienza nel server!       --- Premi un tasto ---  "));
                                break;
                        }
                        while (c == 0) {
                                c = getchar();
			}
                        if (fullpath) {
                                Free(fullpath);
			}
			if (editor_reached_full_size) {
				window_pop();
			}
                        return;
                }
                filenum = extract_ulong(buf+4, 0); /* no. prenotazione */

                // TODO check the flagsflags
                flags = 0;

		if (editor_reached_full_size) {
			window_pop();
		}

                Editor_Head_Refresh(t, true);
                id = md_insert_file(t->mdlist, filename, fullpath, filenum,
				    len, flags);

                tmpcol = t->curs_col;
                filecol = COLOR_FILE;

                Editor_Set_MDNum(filecol, id);
                Editor_Set_Color(t, filecol);
                for (ptr = (t->mdlist->md[id-1])->content; *ptr; ptr++) {
                        if ((isascii(*ptr) && isprint(*ptr))
                            || is_isoch(*ptr)) {
                                Editor_Putchar(t, *ptr);
			}
                }
                Editor_Set_Color(t, tmpcol);
        }
}

/* Inserisce un testo da un file */
static void Editor_Insert_Link(Editor_Text *t)
{
        char buf[LBUF], label[LBUF];
        char *ptr, tmpcol;
        int id, linkcol;

	erase_current_line();
	buf[0] = 0;

        if (getline_scroll("<b>Insert Link:</b> ", COL_HEAD_MD, buf,
                           LBUF-8, 0, 0, Editor_Pos) > 0) {
		erase_current_line();
                label[0] = 0;
                getline_scroll("<b>Etichetta (opzionale):</b> ", COL_HEAD_MD,
                               label, NCOL - 2, 0, 0, Editor_Pos);
                id = md_insert_link(t->mdlist, buf, label);
                if (label[0] == 0) {
                        strncpy(label, buf, 75);
                        if (strlen(buf) > 75)
                                strcpy(label+75, "...");
                }

		if (editor_reached_full_size) {
			window_pop();
		}

                Editor_Head_Refresh(t, true);
                tmpcol = t->curs_col;
                linkcol = COLOR_LINK;
                Editor_Set_MDNum(linkcol, id);
                Editor_Set_Color(t, linkcol);
                for (ptr = label; *ptr; ptr++) {
                        if ((isascii(*ptr) && isprint(*ptr))
                            || is_isoch(*ptr))
                                Editor_Putchar(t, *ptr);
                }
                Editor_Set_Color(t, tmpcol);
        } else {
		if (editor_reached_full_size) {
			window_pop();
		}
	}
}

/* Inserisce un testo da un file */
static void Editor_Insert_Text(Editor_Text *t)
{
        char file_path[LBUF], *filename, buf[LBUF];
        FILE *fp;
        Editor_Line *nl;
	int len, wlen, color, i;
        int c = 0;


	/*
        if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	*/
	fill_line(Editor_Pos, COL_HEAD_ERROR);
        file_path[0] = 0;
        if (getline_scroll("<b>Inserisci file:</b> ", COL_HEAD_MD, file_path,
                           LBUF-1, 0, 0, Editor_Pos) > 0) {
                filename = interpreta_tilde_dir(file_path);
                fp = fopen(filename, "r");
                if (filename) {
                        Free(filename);
		}
                if (fp != NULL) {
			if (editor_reached_full_size) {
				window_pop();
			}
                        Editor_Head_Refresh(t, true);
                        fgets(buf, 6, fp);
                        if (!strncmp(buf, "<cml>", 5)) {
                                color = C_DEFAULT;
                                if (t->curr->len) {
                                        if (t->curr->pos)
                                                Editor_Key_Enter(t);
                                        Editor_Key_Enter(t);
                                        Editor_Key_Up(t);
                                }
                                while (fgets(buf, LBUF, fp) != NULL) {
                                        for (i=0; buf[i]; i++) {
                                                if (buf[i]=='\n')
                                                        buf[i] = 0;
                                        }
                                        t->curr->len = cml2editor(buf, t->curr->str, t->curr->col, &len, t->max, &color, t->mdlist);
                                        t->curr->pos = 0;
                                        t->curr->flag = 1;
                                        if (t->curr->len >= t->max) { /* wrap word */
                                                //Editor_Key_Enter(t);
                                                Editor_Insert_Line(t);

                                                nl = t->curr->next;
                                                wlen = 0;
                                                len = t->curr->len;
                                                while ((wlen <= len) && (t->curr->str[len-wlen]==' '))
                                                        wlen++;
                                                while ((wlen <= len) && (t->curr->str[len-wlen]!=' '))
                                                        wlen++;
                                                if (wlen >= (len-2))
                                                        wlen = len - t->max + 1;
                                                len -= wlen;
                                                nl->len = wlen + 2;
                                                memcpy(nl->str+2,
						       t->curr->str+len,
						       wlen*sizeof(int));
                                                memcpy(nl->col+2,
						       t->curr->col+len,
						       wlen*sizeof(int));
                                                t->curr->len = len;
						line_refresh(t->curr,
							     Editor_Vcurs, 0);
						setcolor(t->curr_col);
                                                nl->str[0] = ' ';
                                                nl->str[1] = ' ';
                                                nl->col[0] = C_DEFAULT;
                                                nl->col[1] = C_DEFAULT;
                                                nl->pos = 0;
                                                nl->flag = 1;

                                                Editor_Newline(t);
                                                //t->curr = nl;
                                        }
					line_refresh(t->curr, Editor_Vcurs, 0);
					setcolor(t->curr_col);
                                        //Editor_Key_Enter(t);
                                        Editor_Insert_Line(t);
                                        Editor_Newline(t);
                                }
				line_refresh(t->curr, Editor_Vcurs, 0);
				setcolor(t->curr_col);
                        } else {
                                rewind(fp);
                                while( (c = fgetc(fp)) != EOF) {
                                        if ((c == Key_CR) || (c == Key_LF))
                                                Editor_Key_Enter(t);
                                        else if ((isascii(c) && isprint(c)) ||
						 is_isoch(c)) {

                                                Editor_Putchar(t, c);
                                                if (t->curr->len >= t->max) {

                                                }
                                        }
                                }
                        }
                        fclose(fp);
                        return;
                } else {
                        cti_mv(0, Editor_Pos);
                        setcolor(COL_HEAD_ERROR);
                        printf("%-80s\r", " *** File non trovato o non leggibile!! -- Premi un tasto per continuare --");
                        while (c == 0) {
                                c = getchar();
			}
			if (editor_reached_full_size) {
				window_pop();
			}
                }
        } else {
		if (editor_reached_full_size) {
			window_pop();
		}
	}

        Editor_Head_Refresh(t, true);
}



/* Save the text to a file */
static void Editor_Save_Text(Editor_Text *t)
{
        char file_path[LBUF], *filename, *out;
        FILE *fp;
        Editor_Line *line;
        int col;
        int c = 0;

        if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	fill_line(Editor_Pos, COL_HEAD_MD);
        file_path[0] = 0;
        if (getline_scroll("<b>Nome file:</b> ", COL_HEAD_MD, file_path,
                           LBUF-1, 0, 0, Editor_Pos) > 0) {
                filename = interpreta_tilde_dir(file_path);
                fp = fopen(filename, "w");
                if (filename) {
                        Free(filename);
		}
                if (fp != NULL) {
                        col = C_DEFAULT;
                        fprintf(fp, "<cml>\n");
                        for (line = t->first; line; line = line->next) {
                                out = editor2cml(line->str, line->col, &col, line->len, t->mdlist);
                                fprintf(fp, "%s\n", out);
                                Free(out);
                        }
                        fclose(fp);
                } else {
                        cti_mv(0, Editor_Pos);
                        setcolor(COL_HEAD_ERROR);
                        printf("%-80s\r", " *** Non posso creare o modificare il file! -- Premi un tasto per continuare --");
                        while (c == 0) {
                                c = getchar();
			}
                }
        }
	if (editor_reached_full_size) {
		window_pop();
	}
        Editor_Head_Refresh(t, true);
}


/*
 * Ristampa la linea 'line' nella riga 'vpos' dello schermo a partire
 * dal carattere in posizione start
 */
static void line_refresh(Editor_Line *line, int vpos, int start)
{
	int i, col = 0;

	if (start == 0) {
		cti_mv(0, vpos);
		col = COLOR(YELLOW, BLACK, ATTR_BOLD);
		setcolor(col);
		putchar('>');
	} else
		cti_mv(start+1, vpos);
	for (i = start; i < line->len; i++) {
		if (line->col[i] != col) {
			col = line->col[i];
			setcolor(col);
		}
		putchar(line->str[i]);
	}
	setcolor(COLOR(GRAY, BLACK, ATTR_DEFAULT));
	for ( ; i < NCOL - 1; i++)
		putchar(' ');
	setcolor(col);
	cti_mv(line->pos+1, Editor_Vcurs);
}

/* Libera la memoria allocata al testo. */
static void Editor_Free(Editor_Text *t)
{
	Editor_Line *l, *tmp;

	for (l = t->first; l; l = tmp) {
		tmp = l->next;
		free(l);
	}
        Editor_Free_Copy_Buffer(t);
}

/* Elimina il metadata associato alla linea l */
static void Editor_Free_MD(Editor_Text *t, Editor_Line *l)
{
        int mdnum, i;

        for (i = 0; i < l->len; i++) {
                mdnum = Editor_Get_MDNum(l->col[i]);
                if (mdnum) {
                        mdop_delete(t->mdlist, mdnum);
                        do
                                i++;
                        while (Editor_Get_MDNum(l->col[i]) == mdnum);
                }
        }
}

/* Libera la memoria allocata al buffer di copia. */
static void Editor_Free_Copy_Buffer(Editor_Text *t)
{
	Editor_Line *tmp;

	for (Editor_Line *line = t->buf_first; line; line = tmp) {
		tmp = line->next;
                if (!t->buf_pasted) {
                        Editor_Free_MD(t, line);
		}
		free(line);
	}
        t->buf_riga = 0;
        t->buf_pasted = false;
        t->buf_first = NULL;
        t->buf_last = NULL;
}

/* Refresh the displayed text, from terminal row 'start' to the bottom of the
 * terminal window. If 'start' is zero, refreshes the full text display.   */
static void Editor_Refresh(Editor_Text *t, int start)
{
	int i;

	if (start < (Editor_Pos + 1)) {
		start = Editor_Pos + 1;
	}

	/* find the first line to be refreshed */
	Editor_Line *line = t->curr;
	for (i = Editor_Vcurs; i > start && line->prev; i--) {
		line = line->prev;
	}
	assert(line);

	/* refresh down to the bottom of the screen, or text end */
	for (i = start; i < NRIGHE && line; i++) {
		line_refresh(line, i, 0);
		line = line->next;
	}

	/* clear the remaining rows if the text did not fill the display */
	for ( ; i < NRIGHE; i++) {
		clear_line(i);
	}
}

const char status_front[] =
	"--- Inserisci il testo ---   Help: F1   Exit: Ctrl-X   [ ";
const char status_back[] =  "%s %3d/%3d,%2d ] ";
const char *insmode[3] = {"Insert   ", "Overwrite", "ASCII Art"};

static void Editor_Head(Editor_Text *t)
{
	push_color();
	setcolor(C_EDITOR);
	printf(status_front);
	printf(status_back, insmode[(int)t->insert], t->curr->num, t->riga,
	       t->curr->pos + 1);
	pull_color();
	/* TODO: what is the role of the following '\n' (which is needed) */
	putchar('\n');
}

/* Aggiorna l'header dell'editor, con modalita' di scrittura e pos cursore */
/* Se mode e' TRUE refresh dell'intero header, non solamente la parte      */
/* finale che puo' cambiare.                                               */
static void Editor_Head_Refresh(Editor_Text *t, bool full_refresh)
{
	if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	setcolor(COL_EDITOR_HEAD);
        if (full_refresh) {
                cti_mv(0, Editor_Pos);
		printf(status_front);
        } else {
                cti_mv(strlen(status_front), Editor_Pos);
	}

	printf(status_back, insmode[(int) t->insert], t->curr->num, t->riga,
	       t->curr->pos+1);

	if (editor_reached_full_size) {
		window_pop();
	}
	setcolor(t->curr_col);
	cti_mv(t->curr->pos + 1, Editor_Vcurs);
}

/* Refresh everything: header and text */
static void Editor_Refresh_All(Editor_Text *t)
{
	/*
	  We clear the line just above the editor status bar because the
	  apple Terminal.app behaves erraticaly when resizing the terminal
	  window, sometimes doing an extra scroll up. Erasing the line above
	  the status bar is fine because it is anyway not used.
	*/
	cti_mv(0, Editor_Pos - 1);
	printf(ERASE_TO_EOL);

	Editor_Head_Refresh(t, true);
	Editor_Refresh(t, 0);
}

static int Editor_Ask_Abort(Editor_Text *t)
{
	if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	cti_mv(0, Editor_Pos - 2);
	setcolor(C_EDITOR_DEBUG);
	printf(sesso
	       ? _("\nSei sicura di voler lasciar perdere il testo (s/n)? ")
	       : _("\nSei sicuro di voler lasciar perdere il testo (s/n)? "));
	if (si_no() == 'n')
		return false;
	if (editor_reached_full_size) {
		window_pop();
	}
	setcolor(t->curr_col);
	cti_mv(t->curr->pos + 1, Editor_Vcurs);
	return true;
}

static void Editor2CML(Editor_Line *line, struct text *txt, int col,
                       Metadata_List *mdlist)
{
	char *out;

	if (col == 0)
		col = C_DEFAULT;
	for (; line; line = line->next) {
		out = editor2cml(line->str, line->col, &col, line->len, mdlist);
		txt_puts(txt, out);
	}
}

/* Inserisce il testo nella struttura txt nell'editor. */
static void text2editor(Editor_Text *t, struct text *txt, int color,
                        int max_col)
{
        Metadata_List *mdlist;
	Editor_Line *nl;
	char *str;
	int len, wlen;

	if (color == 0)
		color = C_DEFAULT;

	if (txt == NULL)
		return;
	txt_rewind(txt);

        mdlist = t->mdlist;
	while( (str = txt_get(txt))) {
		t->curr->len = cml2editor(str, t->curr->str, t->curr->col,
					  &len, t->max, &color, mdlist);
		t->curr->pos = 0;
		t->curr->flag = 1;
                if (t->curr->len >= max_col) { /* wrap word */
                        Editor_Insert_Line(t);
                        nl = t->curr->next;
                        wlen = 0;
                        len = t->curr->len;
                        while ((wlen <= len) && (t->curr->str[len-wlen]==' '))
                                wlen++;
                        while ((wlen <= len) && (t->curr->str[len-wlen]!=' '))
                                wlen++;
                        if (wlen >= (len-2))
                                wlen = len - max_col + 1;
                        len -= wlen;
                        nl->len = wlen + 2;
                        memcpy(nl->str+2, t->curr->str+len, wlen*sizeof(int));
                        memcpy(nl->col+2, t->curr->col+len, wlen*sizeof(int));
                        t->curr->len = len;
                        nl->str[0] = ' ';
                        nl->str[1] = ' ';
                        nl->col[0] = C_DEFAULT;
                        nl->col[1] = C_DEFAULT;
                        nl->pos = 0;
                        nl->flag = 1;
                        t->curr = nl;
                }
		Editor_Insert_Line(t);
		t->curr = t->curr->next;
	}
	/* qui devo eliminare l'ultima riga */
}

/****************************************************************************/
/*
 * Visualizza l'help per l'editor interno.
 */
static void help_edit(Editor_Text *t)
{
	cti_clear_screen();
	if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}
	/* TODO: define the help strings as constants, possibly in another
	 *       file. Unify with the help strings in edit.c .             */
	cml_printf(_(
//"        <b>o---O Help Editor Interno O---o</b>\n\n"
"<b>Attenzione</b>: le combinazioni di tasti Alt- vanno composte su alcuni terminali\n"
"            con la pressione del tasto Escape seguita dal tasto dato dalla\n"
"            combinazione.\n\n"
"<b>Movimento del cursore:</b> Utililizzare i tasti di cursore oppure\n"
"  Ctrl-B  Sposta il cursore di un carattere a sinistra (Backward).\n"
"  Ctrl-F  Sposta il cursore di un carattere a destra   (Forward).\n"
"  Ctrl-N  Vai alla riga successiva (Next).\n"
"  Ctrl-P  Vai alla riga precedente (Previous).\n"
"  Ctrl-A  Vai all'inizio della riga.\n"
"  Ctrl-E  Vai alla fine della riga.\n"
"  Ctrl-V, \\<Page Down\\>  Vai alla pagina successiva.\n"
"  Alt-v , \\<Page Up\\>    Vai alla pagina precedente.\n"
"  \\<Home\\>                Vai in cima al testo.\n"
"  \\<End\\>                 Vai in fondo al testo.\n\n"
"<b>Comandi di cancellazione:</b>\n"
"  Ctrl-D, \\<Del\\>  Cancella il carattere sotto al cursore.\n"
"  Ctrl-G  Incolla il testo tagliato con Ctrl-K.\n"
"  Ctrl-K  Cancella dal cursore fino alla fine della riga.\n"
"  Ctrl-U  Cancella tutto il testo.\n"
"  Ctrl-W  Cancella una parola all'indietro.\n"
"  Alt-d   Cancella la parola a destra del cursore.\n"
		     ));
	hit_any_key();
	cti_clear_screen();
        cml_printf(_(
"<b>Inserimento allegati:</b>\n\n"
"  Premendo Alt-i &egrave; possibile inserire in allegato al post\n"
"  dei file, dei link a pagine web, o riferimenti a post, room e utenti.\n\n"
"<b>Salvataggio post su file e inserimento file testo:</b> (solo client locale)\n\n"
"  Alt-i \\<t>ext  Inserisce nel post un file di testo.\n"
"  Ctrl-T        Salva il post in un file di testo.\n"
		     ));
        cml_printf(_(
"\n<b>Altri Comandi:</b>\n\n"
"  Alt-a   Abort: lascia perdere (chiede conferma).\n"
"  Ctrl-L  Rinfresca la schermata.\n"
"  Ctrl-X  Termina l'immissione del testo.\n"
		     ));
	cml_printf(_(
"\n<b>Modalit&agrave; di funzionamento dell'editor:</b>\n\n"
"Le modalit&agrave; disponibili sono <b>Insert</b> per l'inserimento del testo,\n"
"<b>Overwrite</b> per la sovrascrizione e <b>ASCII-Art</b> per creare disegni.\n"
"La selezione avviene mediante la pressione ripetuta del tasto \\<<b>Ins</b>\\>.\n\n"
		     ));
	hit_any_key();
	cti_clear_screen();
	cml_print(help_colors);
	cml_print(_(
"  \\<<b>F2</b>\\>  Applica colore e attributi correnti al carattere sotto al cursore.\n"
"  \\<<b>F3</b>\\>  Applica colore e attributi correnti fino al prossimo spazio.\n"
"  \\<<b>F4</b>\\>  Applica colore e attributi correnti a tutta la riga.\n\n"
		    ));
	hit_any_key();
	cti_clear_screen();
	cml_print(help_8bit);
	hit_any_key();
	cti_mv(0, Editor_Pos);
	Editor_Head(t);
	if (editor_reached_full_size) {
		window_pop();
	}
	Editor_Refresh(t, 0);
}

static void refresh_line_curs(int *pos, int *curs)
{
	int i;

	putchar('\r');
	putchar('>');
	for (i=0; (i < NCOL - 2) && (pos[i] != 0); putchar(pos[i++]));
	for ( ; i < NCOL - 2; i++, putchar(' '));
	cti_mv((int)(curs-pos+1), NRIGHE-1);
	fflush(stdout);
}

#else
   /* Se non ho l'accesso alle capacita' del terminale editor interno... */
# include "edit.h"
# include "text.h"

int get_text_full(struct text *txt, long max_linee, int max_col, bool abortp)
{
	txt_clear(txt);
	return get_text(txt, max_linee, max_col, abortp);
}

#endif /* HAVE_CTI */

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

#if 0 /* Unused */
static void editor2text(Editor_Line *line, struct text *txt)
{
	int i, pos, col;
	char str[LBUF];

	col = COLOR(C_NORMAL, BLACK, ATTR_DEFAULT);
	for (; line; line = line->next) {
		pos = 0;
		for (i = 0; i < line->len; i++) {
			if (line->col[i] != col) {
				str[pos++] = '\x1b';
				str[pos++] = '[';
				if (CC_ATTR(line->col[i]) != CC_ATTR(col))
					str[pos++] = CC_ATTR(line->col[1]+'0');
				str[pos++] = ';';
				if (CC_FG(line->col[i]) != CC_FG(col)){
					str[pos++] = '3';
					str[pos++] = CC_FG(line->col[i])+'0';
				}
				if (CC_BG(line->col[i]) != CC_BG(col)){
					str[pos++] = ';';
					str[pos++] = '3';
					str[pos++] = CC_BG(line->col[i])+'0';
				}
				str[pos++] = 'm';
				col = line->col[i];
			}
			str[pos++] = line->str[i];
		}
		str[pos++] = '\n';
		str[pos] = '\0';
		txt_put(txt, str);
	}
}

static void Editor_Wrap_Word_noecho(Editor_Text *t)
{
	Editor_Line *nl;
	int len, wlen = 0;

	len = t->curr->len;
	while ((len-wlen >= 0) && (t->curr->str[len-wlen] != ' '))
		wlen++;
	if (len-wlen < 0) {
		Editor_Insert_Line(t);
		t->curr->next->pos = 0;
		t->curr->pos = 0;
		t->curr = t->curr->next;
		return;
	}
	Editor_Insert_Line(t);
	nl = t->curr->next;
	len -= wlen;
	memcpy(nl->str, t->curr->str+len+1, wlen * sizeof(int));
	memcpy(nl->col, t->curr->col+len+1, wlen * sizeof(int));
	nl->len = wlen - 1;
	nl->pos = nl->len;
	t->curr->len = len;
	t->curr = nl;
}
#endif

#if 0
/* Inserisce il testo nella struttura txt nell'editor. */
static void text2editor(Editor_Text *t, struct text *txt)
{
	char *str;
	int i;

	if (txt == NULL)
		return;
	txt_rewind(txt);
	while( (str = txt_get(txt))) {
		i = 0;
		while(str[i]) {
			if ((str[i] == Key_ESC) || (str[i] == '\n')) {
				/* Qui tratto il colore */
				i++;
			} else {
				t->curr->str[t->curr->pos] = str[i];
				t->curr->col[t->curr->pos++] = t->curs_col;
				++t->curr->len;
				if (t->curr->len == t->max-1)
					Editor_Wrap_Word_noecho(t);
				i++;
			}
		}
		/* Newline */
		Editor_Insert_Line(t);
		t->curr->next->pos = 0;
		t->curr->pos = 0;
		t->curr = t->curr->next;
	}
}
#endif

/**************************************************************************/
/* Editor debugging utilities.                                            */
/**************************************************************************/
#ifdef EDITOR_DEBUG

static void Editor_Debug_Status(Editor_Text *t, char *str)
{
	static char debug_str[LBUF] = {0};

	if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}

	if (*str) {
		strcpy(debug_str, str);
	}

	/* cti_mv(0, Editor_Pos - 2); */
	cti_mv(0, 0);
	setcolor(C_EDITOR_DEBUG);
	erase_current_line();
	printf("Debug Status: %s [ws: %d]", debug_str,
	       debug_get_winstack_index());

	cti_mv(0, 1);
	int ws_count = debug_get_winstack_index();
	int first, last;
	debug_get_current_win(&first, &last);
	printf("C %d,%d; ", first, last);
	for (int i = 0; i != ws_count; i++) {
		debug_get_winstack(i, &first, &last);
		printf("%d %d,%d; ", i, first, last);
	}

	if (editor_reached_full_size) {
		window_pop();
	}
	setcolor(t->curr_col);
	cti_mv(t->curr->pos + 1, Editor_Vcurs);
}

static void Editor_Debug_Show_Copy_Buffer(Editor_Text *t)
{
	static int num_rows = 0;

	if (editor_reached_full_size) {
		window_push(0, NRIGHE - 1);
	}

	int row = 3;

	if (t->buf_riga) {
		cti_mv(0, row++);
		setcolor(C_EDITOR_DEBUG);
		erase_current_line();
		printf("----- copy buffer -----");
		for (Editor_Line *line = t->buf_first; line; line=line->next) {
			cti_mv(0, row++);
			erase_current_line();
			for (int i = 0; (i<NCOL-2) && (line->str[i]!=0); i++) {
				putchar(line->str[i]);
			}
		}
		cti_mv(0, row++);
		erase_current_line();
		printf("--- end copy buffer ---");
	}
	int rows_written = row;
	while(row < num_rows) {
		cti_mv(0, row++);
		erase_current_line();
	}
	fflush(stdout);

	if (editor_reached_full_size) {
		window_pop();
	}
	setcolor(t->curr_col);
	cti_mv(t->curr->pos + 1, Editor_Vcurs);

	num_rows = rows_written;
}

static void sanity_checks(Editor_Text *t)
{
	if (t->first == NULL) {
		assert(t->last == NULL);
		assert(t->curr == NULL);
		goto SANITY_COPY_BUFFER;
	}
	assert(t->first->prev == NULL);
	assert(t->last->next == NULL);

	{
		int num = 1;
		Editor_Line *line = t->first;
		for (;;) {
			assert(line->num == num);
			if (line->next == NULL) {
				assert(line == t->last);
				break;
			}
			if (line->prev) {
				assert(line->prev->next == line);
			} else {
				assert(line == t->first);
			}
			num++;
			line = line->next;
		}
		assert(t->riga == num);
	}
	{
		Editor_Line *line = t->last;
		for (;;) {
			if (line->prev == NULL) {
				assert(line == t->first);
				break;
			}
			if (line->next) {
				assert(line->next->prev == line);
			} else {
				assert(line == t->last);
			}
			line = line->prev;
		}
	}

	/* Test the copy buffer sanity */
 SANITY_COPY_BUFFER:

	if (t->buf_first == NULL) {
		assert(t->buf_last == NULL);
		return;
	}
	assert(t->buf_first->prev == NULL);
	assert(t->buf_last->next == NULL);

	{
		int num = 1;
		Editor_Line *line = t->buf_first;
		for (;;) {
			//assert(line->num == num);
			if (line->next == NULL) {
				assert(line == t->buf_last);
				break;
			}
			if (line->prev) {
				assert(line->prev->next == line);
			} else {
				assert(line == t->buf_first);
			}
			num++;
			line = line->next;
		}
		assert(t->buf_riga == num);
	}
	{
		Editor_Line *line = t->buf_last;
		for (;;) {
			if (line->prev == NULL) {
				assert(line == t->buf_first);
				break;
			}
			if (line->next) {
				assert(line->next->prev == line);
			} else {
				assert(line == t->buf_last);
			}
			line = line->prev;
		}
	}
}
#endif
