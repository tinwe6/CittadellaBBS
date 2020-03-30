/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
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
* File : edit.c                                                             *
*        Funzioni per editare testi.                                        *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "cittaclient.h"
#include "metadata.h"
#include "cittacfg.h"
#include "ansicol.h"
#include "cml.h"
#include "conn.h"
#include "edit.h"
#include "editor.h"
#include "inkey.h"
#include "macro.h"
#include "messaggi.h"
#include "prompt.h"
#include "strutt.h"
#include "terminale.h"
#include "text.h"
#include "utility.h"
#include "generic_cmd.h"
#ifdef LOCAL
# include "local.h"
#else
# include "remote.h"
#endif

int c_getline(char *str, int max, bool maiuscole, bool special);

int get_text(struct text *txt, long max_linee, char max_col, bool abortp);
int get_textl(struct text *txt, size_t max, unsigned int nlines);
int get_textl_cursor(struct text *txt, int max, int nlines, bool chat);
void edit_file(char *filename);
int enter_text(struct text *txt, int max_linee, char mode,
               Metadata_List *mdlist);
void send_text(struct text *txt);
char * getline_col(int max, bool maiuscole, bool special);
int get_text_col(struct text *txt, long max_linee, int max_col, bool abortp);
int gl_extchar(int status);

static int getline_wrap(char *str, int max, bool maiuscole, bool special,
			bool wrap, int pos);

static void refresh_line(char *pos);
static void refresh_line_curs(const char *prompt, int promptlen, int color,
                              char *pos, char *curs, int field);

static void Editor_Fg_Color(int *curr_col, int *curs_col, int col);
static void Editor_Bg_Color(int *curr_col, int *curs_col, int col);
static void Editor_Attr_Toggle(int *curr_col, int *curs_col, int attr);
static void help_edit(void);
static void help_line_edit(void);

/***************************************************************************/
/*
 * Prende una stringa dallo stdinput, lunga |max|, se max<0 visualizza
 * asterischi al posto delle lettere, se special = 0 non accetta
 * i caratteri speciali utilizzati dalla connessione server/client
 * (per ora solo '|'). Se maiuscole == true, il primo carattere di ogni
 * parola e' automaticamente convertito in maiuscola.
 *
 * Restituisce il numero di caratteri immessi, o -1 se abortito.
 *
 * NB: prende max caratteri, la stringa deve aver allocato per lo meno
 *     (max + 1) bytes per contenere anche lo '\0' finale.
 */
int c_getline(char *str, int max, bool maiuscole, bool special)
{
	return getline_wrap(str, max, maiuscole, special, false, 0);
}

/* getline + Ctrl-P + Ctrl-U + Ctrl-X + gestione dei prompt            */
/* restituisce -1 se e` stato abortito                                 */
/* wrap == 1 => usato da gettext con wrap, altrimenti getline normale. */
/* Restituisce il numero di caratteri immessi, o -1 se abortito.       */
#define GETLINE_ABORT -1
#define GETLINE_ERROR -2
#define GETLINE_END   -3

static int getline_wrap(char *str, int max, bool maiuscole, bool special,
			bool wrap, int pos)
{
        int c, len;
        bool hide = false;

        if (max == 0)
                return GETLINE_ERROR;
        if (max < 0) {
                hide = true;
                max = -max;
        }

	len = pos;
        do {
		prompt_txt = str;
                c = inkey_sc(0);
                if ((isascii(c) && isprint(c) && c != '|') || (c == Key_BS)
                    || (special && c == '|') || (c == Ctrl('P'))
		    || (c == Ctrl('U')) || (c == Ctrl('X'))) {
			if ((c == Key_BS) && (len != 0)) {
                                delchar();
                                str[--len] = '\0';
                        } else if (c == Ctrl('P')) {
				while ((len != 0) && (str[len-1] == ' ')) {
					str[--len] = '\0';
					delchar();
				}
				while ((len != 0) && (str[len-1] != ' ')) {
					str[--len] = '\0';
					delchar();
				}
			} else if (c == Ctrl('U')) {
				for( ; len > 0; delchar(), len--);
				str[0] = '\0';
			} else if (c == Ctrl('X')) {
				/* printf("\nAbort.\n"); */
                                /* TODO: verificare che ok anche se ho */
                                /* tolto la riga precedente */
				str[0] = '\0';
				return GETLINE_ABORT;
			} else if ((c != Key_BS) && (len < max)) {
                                if (maiuscole && islower(c) &&
				    ((len == 0) || (str[len - 1] == ' ')))
					c = toupper(c);
                                if (hide)
                                        putchar('*');
                                else
                                        putchar(c);
                                str[len] = c;
                                str[++len] = '\0';
                        }
                }
        } while ((c != Key_CR) && (c != Key_LF)
		 && (!wrap || (len < max)));

	if (!wrap)
		putchar('\n');

	return len;
}

/*
 * get_text() : Semplice editor per testi.
 *
 * Prende in input dall'utente un testo lungo al piu' 'max_linee' righe su
 * 'max_col' colonne e lo inserisce nella struttura 'txt'. Il testo termina
 * quando si lascia una riga vuota oppure si riempono tutte le righe a
 * disposizione. Se (abort!=1), immettendo su una riga vuota la stringa "ABORT"
 * l'editing viene interrotto. Il testo viene formattato automaticamente
 * mandando a capo le parole che non ci stanno in fondo alla riga.
 *
 * ctrl-p : Cancella la parola precedente
 * ctrl-u : Cancella la riga corrente
 * ctrl-x : abort
 *
 * Valori di ritorno: -1 : Abort; 0 : Nessun testo; N : Numero righe scritte.
 */
int get_text(struct text *txt, long max_linee, char max_col, bool abortp)
{
        int riga = 0, pos = 0, wlen, maxchar = 79;
        char str[80], parola[80], prompt_tmp;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDT;

        if (max_col > maxchar)
                max_col = maxchar;
	str[0] = '\0';

        do {
                printf(">%s", str);
		pos = getline_wrap(str, max_col-1, false, true, true, pos);
		if (pos == GETLINE_ABORT) {
				prompt_curr = prompt_tmp;
				return -1;
		}
		parola[0] = '\0';
		wlen = 0;
                if (pos == (max_col - 1)) {
                        do {
                                wlen++;
			} while ((str[pos-wlen] != ' ') && (pos-wlen >= 0));
                        if (pos-wlen >= 0) {
				pos = pos - wlen;
				memcpy(parola, str+pos+1, wlen);
				str[pos] = '\0';
				delnchar(--wlen);
			} else
				wlen = 0;
		}
		printf("\n");
		if (pos == 0) {
			prompt_curr = prompt_tmp;
			if (riga == 0)
				return 0;
			else
				return riga+1;
		} else if (abortp && !strcmp(str, "ABORT")) {
			prompt_curr = prompt_tmp;
			return -1;
		}
		str[pos] = '\n';
		str[pos+1] = '\0';
		txt_put(txt, str);
		memcpy(str, parola, wlen+1);
		pos = wlen;
                riga++;
        } while (riga < (max_linee - 1));

	printf(">%s", str);

	pos = getline_wrap(str, max_col-1, false, true, false, pos);
	prompt_curr = prompt_tmp;
	if (pos == GETLINE_ABORT)
		return -1;

	prompt_curr = prompt_tmp;
	printf("\n");

	if (pos == 0)
		return riga;

	if (abortp && !strcmp(str, "ABORT"))
		return -1;

	str[pos] = '\n';
	str[++pos] = '\0';
	txt_put(txt, str);

	return max_linee;
}

/*
 * Prende un testo dallo stdinput, lungo 'max'.
 * Il testo viene editato su una singola riga, e se e' piu' lungo dello
 * schermo scrolla.
 * Se 'chat' allora visualizza interpretando i comandi speciali della chat.
 *
 * Tasti speciali:  (Emacs style)
 *                  Ctrl-A  Vai all'inizio della riga
 *                  Ctrl-E  Vai alla fine della riga
 *                  Ctrl-B  Sposta il cursore di un carattere a sinistra
 *                  Ctrl-F  Sposta il cursore di un carattere a destra
 *                  Ctrl-I  Info (Help)
 *                  Ctrl-L  Refresh line
 *                  Ctrl-P  Cancella una parola all'indietro
 *                  Ctrl-K  Cancella dal cursore fino alla fine della riga
 *                  Ctrl-U  Cancella tutto il testo
 *                  Ctrl-X  Abort
 */
int get_textl_cursor(struct text *txt, int max, int nlines, bool chat)
{
        int len = 0, rpos = 0, spos;
	char *str, *str1;
	char buf[LBUF];

        if ((max == 0) || (nlines == 0))
                return 0;

	CREATE(str, char, (max+1)*nlines, 0);

        len = getline_scroll(">", C_EDIT_PROMPT, str, max*nlines, 0, 0);

	if (len != 0) {
		/* Formatta il testo */
		rpos = 0;
		while ((len - rpos) >= max) {
			spos = max;
			while (spos && (*(str+rpos+spos) != ' '))
				spos--;
			if (spos == 0)
				spos = max;
			memcpy(buf, str+rpos, spos);
			buf[spos] = '\0';
			txt_put(txt, buf);
			rpos += spos;
			while (*(str+rpos) == ' ')
				rpos++;
		}
		txt_put(txt, str+rpos);

		/* Visualizza il testo intero */
		txt_rewind(txt);
		if (chat && (!strncmp(str, "/me ", 4))) {
			printf("\r%79s\r", "");
			str1 = txt_get(txt);
			cml_printf(" <b>%s</b> %s\n", nome, str1+4);
			while ( (str1 = txt_get(txt)))
				printf(" %s\n", str1);
		} else {
			while ( (str1 = txt_get(txt))) {
				refresh_line(str1);
				putchar('\n');
			}
		}
	} else
		putchar('\n');

	Free(str);
	return len;
}

/* Prende una stringa con prompt "prompt" lunga massimo "max caratteri e
 * con un campo di immissione lungo al piu' "field". Se field = 0 arriva fino in
 * fondo alla riga.
 * Se str non e' nulla, l'editing inizia dal fondo della stringa, e i primi
 * "protect" caratteri di "str" non sono editabili.
 * Restituisce la lunghezza della stringa inserita.
 */
int getline_scroll(const char *cmlprompt, int color, char *str, int max,
                   int field, int protect)
{
        int c, len = 0, rpos = 0, ipos = 0, promptlen, tmp;
	char prompt_tmp, *prompt;

        if (max == 0)
                return 0;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDT;
        prompt = cml_eval_max(cmlprompt, &promptlen, -1, &color, NULL);

        /* boundary = promptlen+field+1; */
        if ((field == 0) || ((promptlen+field+1) > 80))
                field = 80 - promptlen - 1;

        if (*str) {
                len = strlen(str);
                ipos = len;
                if (len < (field+1))
                        rpos = 0;
                else
                        rpos = len - field;
        }
        refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);

        do {
		prompt_txt = str+rpos;
                c = inkey_sc(1);
                switch (c) {
                case Key_BS:
                        if ((len != 0) && (ipos > protect)) {
                                if (ipos != len)
                                        memmove(str+ipos-1, str+ipos, len-ipos);
                                str[--len] = '\0';
                                ipos--;
                                if (rpos == 0) {
                                        if (ipos == len)
                                                delchar();
                                        else
                                                refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                                } else
                                        refresh_line_curs(prompt, promptlen, color, str+(--rpos), str+ipos, field);
                        }
                        break;
                case Ctrl('A'):  /* Vai inizio riga */
			ipos = protect;
			rpos = 0;
                        refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                        break;
		case Ctrl('E'):  /* Vai fine riga */
			ipos = len;
			if (len < (field+1))
				rpos = 0;
			else
				rpos = len - field;
                        refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                        break;
                case Key_LEFT:
		case Ctrl('B'):  /* Cursor Left   */
			if (ipos > protect) {
				ipos--;
				if (rpos > 0)
					rpos--;
				refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
			} else
				Beep();
                        break;
                case Key_RIGHT:
		case Ctrl('F'):  /* Cursor Right  */
			if (ipos < len) {
				ipos++;
				if ((ipos - rpos) > field)
					rpos++;
				refresh_line_curs(prompt,promptlen,color,str+rpos, str+ipos, field);
			} else
				Beep();
                        break;
		case Ctrl('K'): /* Cancella fino fine riga */
			len = ipos;
			str[len] = '\0';
			if (len < (field+1))
				rpos = 0;
			else
				rpos = len - field;
                        refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                        break;
		case Ctrl('L'): /* Refresh line  */
			refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                        break;
                case Ctrl('I'):
                case Key_F(1): /* Help  */
		        help_line_edit();
			refresh_line_curs(prompt,promptlen,color,str+rpos, str+ipos, field);
                        break;
		case Ctrl('P'): /* Delete Word   */
			tmp = ipos;
			while ((ipos > protect) && (str[ipos-1] == ' '))
				ipos--;
			while ((ipos > protect) && (str[ipos-1] != ' '))
				ipos--;
			memmove(str+ipos, str+tmp, len-tmp);
			len -= tmp - ipos;
			str[len] = '\0';
			if (len < (field+1))
				rpos = 0;
			else
				rpos = len - field;
			refresh_line_curs(prompt,promptlen,color,str+rpos, str+ipos, field);
                        break;
		case Ctrl('U'): /* Clear Line    */
			len = protect;
			str[protect] = '\0';
			rpos = 0;
			ipos = protect;
                        refresh_line_curs(prompt, promptlen, color, str+rpos, str+ipos, field);
                        break;
		case Ctrl('X'): /* Abort        */
                        str[protect] = 0;
			printf("\r%80s\rAbort.\n", "");
			prompt_curr = prompt_tmp;
                        Free(prompt);
			return 0;
                default:
                        if (isascii(c) && isprint(c) && (len < max)) {
                                if (ipos != len)
                                        memmove(str+ipos+1,str+ipos, len-ipos);
                                str[ipos++] = c;
                                str[++len] = '\0';
                                if (len < (field+1))
                                        rpos = 0;
                                else
                                        rpos++;
                                refresh_line_curs(prompt,promptlen,color,str+rpos, str+ipos, field);
                        }
                }
	} while ((c != Key_LF) && (c != Key_CR));

	prompt_curr = prompt_tmp;
        Free(prompt);

	return len;
}


/*
 * Prende un testo dallo stdinput, lungo 'max'.
 * Il testo viene editato su una singola riga, e se e' piu' lungo dello
 * schermo scrolla.
 *
 * Tasti speciali:  ctrl-p  Cancella l'ultima parola
 *                  ctrl-u  Cancella tutto il testo
 *                  ctrl-x  Abort
 */
int get_textl(struct text *txt, size_t max, unsigned int nlines)
{
        int c;
	size_t len = 0;
	char *str, *rpos;
	char buf[LBUF], prompt_tmp;

        if ((max == 0) || (nlines == 0))
                return 0;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDT;

	CREATE(str, char, (max + 1)*nlines, 0);
	rpos = str;
	putchar('>');
        do {
		prompt_txt = rpos;
                c = inkey_sc(1);
                if ((isascii(c) && isprint(c)) || (c == Key_BS)
		    || (c == Ctrl('P')) || (c == Ctrl('U'))
		    || (c == Ctrl('X'))) {
                        if ((c == Key_BS) && (len != 0)) {
                                str[--len] = '\0';
                                if (rpos == str)
					delchar();
				else
					refresh_line(--rpos);
                        } else if (c == Ctrl('P')) {
				while ((len != 0) && (str[len-1] == ' ')) {
					str[--len] = '\0';
					if (rpos != str)
						rpos--;
				}
				while ((len != 0) && (str[len-1] != ' ')) {
					str[--len] = '\0';
					if (rpos != str)
						rpos--;
				}
				refresh_line(rpos);
                        } else if (c == Ctrl('U')) {
				len = 0;
				str[0] = '\0';
				rpos = str;
				refresh_line(rpos);
                        } else if (c == Ctrl('X')) {
				Free(str);
				printf("\r%80s\rAbort.\n", "");
				prompt_curr = prompt_tmp;
				return 0;
                        } else if ((c != Key_BS) && (len < max*nlines)) {
                                str[len] = c;
                                str[++len] = '\0';
				if (len < 79)
					putchar(c);
				else
					refresh_line(++rpos);
                        }
                }
        } while ((c != Key_LF) && (c != Key_CR));

	if (len != 0) {
		rpos = str;
		while (strlen(rpos) >= max) {
			memcpy(buf, rpos, max);
			buf[max] = '\0';
			txt_put(txt, buf);
			rpos += max;
		}
		txt_put(txt, rpos);

		/* Visualizza il testo intero */
		txt_rewind(txt);
		rpos = txt_get(txt);
		refresh_line(rpos);
		putchar('\n');
		while ((rpos = txt_get(txt)))
			printf(">%s\n", rpos);
	} else
		putchar('\n');

	Free(str);
	prompt_curr = prompt_tmp;
	return len;
}

/*
 *
 */
void edit_file(char *filename)
{
        int editor_pid, editor_exit;
        int b, i;
        FILE *fp;

        fflush(stdout);

        if (EDITOR != NULL) {
                editor_pid = fork();
                if (editor_pid == 0) { /* Child: lancia l'editor */
                        reset_term();
                        execlp(EDITOR, EDITOR, filename, NULL);
                        exit(1);
                }
                if (editor_pid > 0) {
                        i=0;
                        do {
                                /* manda segnali keepalive al server */
                                us_sleep(100000); /* un decimo di secondo */
                                i++;
                                if (i == 600) { /* Un minuto circa */
                                        serv_puts("NOOP");
                                        i = 0;
                                }
                                editor_exit = 0;
                                b=waitpid(editor_pid, &editor_exit, WNOHANG);
                        } while((b != editor_pid) && (b >= 0));
                }
                editor_pid=(-1);
                term_save();
                term_mode();
        } else {
                printf(_("Inserisci il testo.\n"
			 "Premi return due volte per terminare.\n"));
                fp = fopen(filename,"r+");

                fclose(fp);
        }
}

/*
 * Prende un testo dall'utente, lungo al piu' max_linee righe, e lo memorizza
 * nella struttura *txt. Se mode e' zero uso l'editor 'interno', altrimenti,
 * se e' definita la variabile di environment EDITOR, usa l'editor esterno.
 * Se txt contiene gia' un testo, lo continua (ma allora hold e quote non
 * sono piu' attivi).
 *
 * Restituisce (-1) se c'e' stato un abort nell'editor interno.
 */
int enter_text(struct text *txt, int max_linee, char mode,
               Metadata_List *mdlist)
{
	int editor_pid, editor_exit;
	char *filename, buf[LBUF*6], *qt;
	int b, i, riga;
	int fd;
	FILE *fp;

        fflush(stdout);

        if (mode && (EDITOR != NULL)) { /* Usa l'editor esterno */
		filename = astrcat(TMPDIR, TEMP_EDIT_TEMPLATE);
		fd = mkstemp(filename);
		close(fd);
		chmod(filename, 0660);
		/* Se c'e' un quote, lo inserisce nel file */
		if (quoted_text || hold_text || (txt_len(txt)>0)) {
			if ( (fp = fopen(filename, "w")) == NULL) {
				printf(_("*** Abort - Problemi di I/O.\n"));
				Free(filename);
				return -1;
			}
			if (txt_len(txt)>0) { /* Continue */
				fprintf(fp, "<cml>\n");
				txt_rewind(txt);
				while ((qt = txt_get(txt)))
					fprintf(fp, "%s", qt);
				txt_clear(txt);
			} else {
				fprintf(fp, "<cml>\n");
				if (quoted_text) {
					txt_rewind(quoted_text);
					/* Header in quote?
					txt_get(quoted_text);
					txt_get(quoted_text);
					txt_get(quoted_text);
					*/
					while ((qt = txt_get(quoted_text)))
						fprintf(fp, "> %s\n", qt);
					txt_free(&quoted_text);
					if (hold_text)
						fprintf(fp, "\n");
				}
				if (hold_text) {
					txt_rewind(hold_text);
					while ((qt = txt_get(hold_text)))
						fprintf(fp, "%s", qt);
					txt_free(&hold_text);
				}
			}
			fclose(fp);
		}
                editor_pid = fork();
                if (editor_pid == 0) { /* Child: lancia l'editor */
                        reset_term();
                        execlp(EDITOR, EDITOR, filename, NULL);
                        exit(1);
                }
                if (editor_pid > 0) {
                        i = 0;
                        do {
                                /* manda segnali keepalive al server */
                                us_sleep(100000); /* un decimo di secondo */
                                i++;
                                if (i == 600) { /* Un minuto circa */
                                        serv_puts("NOOP");
                                        i = 0;
                                }
                                editor_exit = 0;
                                b=waitpid(editor_pid, &editor_exit, WNOHANG);
                        } while((b != editor_pid) && (b >= 0));
                }
                editor_pid = (-1);
                term_save();
                term_mode();
		if ( (fp = fopen(filename, "r")) ) {
			riga = 0;
			if (txt == NULL)
				txt = txt_create();
			if ((fgets(buf, 80, fp) != NULL)
			       && (riga < max_linee)) {
				if (!strncmp(buf, "<cml>", 5))
					while ((fgets(buf, LBUF*6, fp) != NULL)
					       && (riga < max_linee)) {
						txt_put(txt, buf);
						riga++;
					}
				else {
					do {
						char *out;
						out = iso2cml(buf, LBUF);
						txt_puts(txt, out);
						riga++;
					} while ((fgets(buf, LBUF*6, fp) != NULL)
						 && (riga < max_linee));
				}
			}
			fclose(fp);
			unlink(filename);
		}
		free(filename);
        } else { /* Usa l'editor interno. */
		if (mode)
			printf(_(
" * Per utilizzare l'editor esterno devi definire la variabile di\n"
"   environment EDITOR, e assegnarle il nome del tuo editor,\n"
"   oppure assegnare la variabile EDITOR nel tuo file di configurazione.\n\n"
));
		if (uflags[0] & UT_OLD_EDT) {
		        printf(_("Inserisci il testo.                                                             \n"));
			push_color();
			setcolor(C_EDITOR);
			printf(_("--- Editor: Premi return due volte per terminare.                  [Help: F1]---"));
			pull_color();
			putchar('\n');
			return get_text_col(txt, max_linee, 79, 0);
		}

		/* Se c'e` un quote, lo inserisce nel file */
		if (quoted_text || hold_text || (txt_len(txt)>0)) {
			if (txt_len(txt) > 0) { /* Continue */
				/* txt_rewind(txt); */
			} else {
				if (quoted_text) {
					char cmlstr[LBUF];

					txt_rewind(quoted_text);
					color2cml(cmlstr, C_QUOTE);
					while ((qt = txt_get(quoted_text)))
						txt_putf(txt, "%s> %s",
							 cmlstr, qt);
					txt_free(&quoted_text);
					if (hold_text)
						txt_put(txt, "");
				}
				if (hold_text) {
					txt_rewind(hold_text);
					while ((qt = txt_get(hold_text)))
						txt_putf(txt, "%s", qt);
					txt_free(&hold_text);
				}
			}
		}
		return get_text_full(txt, max_linee, 79, false, 0, mdlist);
        }
	return 1;
}

/*
 * Invia al server il testo puntato da txt mediante il comando TEXT.
 */
void send_text(struct text *txt)
{
	char *str;

	txt_rewind(txt);
	while ((str = txt_get(txt)) != NULL)
		serv_putf("TEXT %s", str);
}

static void refresh_line(char *pos)
{
	int i;

	push_color();
	setcolor(C_EDIT_PROMPT);
	printf("\r%79s\r>", "");
	pull_color();
	for (i=0; (i < 78) && (pos[i] != 0); i++)
		putchar(pos[i]);
	fflush(stdout);
}

static void refresh_line_curs(const char *prompt, int promptlen, int color,
                              char *pos, char *curs, int field)
{
	int i;

	IGNORE_UNUSED_PARAMETER(promptlen);

	printf("\r%s", prompt);
	for (i = 0; (i < field) && (pos[i] != 0); putchar(pos[i++]));
	for (   ; i < field; putchar(' '), i++);
	push_color();
	setcolor(color);

	printf("\r%s", prompt);
	pull_color();
	for (i = 0; (i < field) && (pos+i != curs); putchar(pos[i++]));
	fflush(stdout);
}

/***************************************************************************/
/*
 *  Prende una stringa dallo stdinput, lunga |max|, se max<0 visualizza
 *  asterischi al posto delle lettere, se special = 0 non accetta
 *  i caratteri speciali utilizzati dalla connessione server/client
 *  (per ora solo '|'). Se maiuscole!=0, il primo carattere di ogni parola
 *  e' automaticamente convertito in maiuscola.
 *
 * NB: prende max caratteri, la stringa deve aver allocato per lo meno
 *     (max + 1) bytes per contenere anche lo '\0' finale.
 */

/* Macro per settare i colori                                     */

static void Editor_Fg_Color(int *curr_col, int *curs_col, int col)
{
	*curs_col = COLOR(col & 0xf, CC_BG(*curs_col), CC_ATTR(*curs_col));
	if (*curr_col != *curs_col) {
		*curr_col = *curs_col;
		/* ansiset_fg(col); */
		setcolor(*curr_col);
	}
}

static void Editor_Bg_Color(int *curr_col, int *curs_col, int col)
{
	*curs_col = COLOR(CC_FG(*curs_col), col & 0xf, CC_ATTR(*curs_col));
	if (*curr_col != *curs_col) {
		*curr_col = *curs_col;
                /* ansiset_bg(col); */
		setcolor(*curr_col);
	}
}

static void Editor_Attr_Toggle(int *curr_col, int *curs_col, int attr)
{
	*curs_col = COLOR(CC_FG(*curs_col), CC_BG(*curs_col),
			  CC_ATTR(*curs_col) ^ (attr & ATTR_MASK));
	if (*curr_col != *curs_col) {
		*curr_col = *curs_col;
                /* ansiset_attr(attr); */
		setcolor(*curr_col);
	}
}

static int getline_col_wrap(int **str0, int **col0, int max, bool maiuscole,
			    bool special, bool wrap, int pos);

char * getline_col(int max, bool maiuscole, bool special)
{
	int *str;
	int *col;
	char *out;
	int len;
	int color;

	str = NULL;
	len = getline_col_wrap(&str, &col, max, maiuscole, special, false, 0);
	/*	setcolor(C_DEFAULT);
		color = C_DEFAULT; */
	color = getcolor();
	if (len != GETLINE_ABORT)
		out = editor2cml(str, col, &color, len, NULL);
	else
		out = citta_strdup("");
	if (str) {
		Free(str);
		Free(col);
	}
	return out;
}

/* In *color c'e` il colore di partenza, e vi viene restituito il colore
   finale. */
static int getline_col_wrap(int **str0, int **col0, int max,
			    bool maiuscole, bool special, bool wrap, int pos)
{
        int c;                     /* carattere corrente            */
        bool hide = false;
	int status = GL_NORMAL;    /* stato dell'editor             */
	int *str;                  /* Stringa in input              */
	int *col;                  /* Colori associati ai caratteri */
	/* int pos; */             /* Posizione del cursore         */
	int len;                   /* Lunghezza della stringa       */
	int curs_col;   /* Colore corrente del cursore              */
	int curr_col;   /* Ultimo settaggio di colore sul terminale */
	bool addchar;
	int tmpcol;
	char *out;

	str = *str0;
	col = *col0;

        if (max == 0)
                return GETLINE_ERROR;
        if (max < 0) {
                hide = true;
                max = -max;
        }

	len = pos;

	curs_col = getcolor();
	curr_col = curs_col;

	if (str == NULL) {
		CREATE(str, int, max + 1, 0);
		CREATE(col, int, max + 1, 0);
		str[0] = '\0';
		pos = 0;
		len = 0;
	}

	do {
		addchar = false;
		prompt_txt = (char *)str; /* SISTEMARE!!! */
                c = inkey_sc(0);
		if (post_timeout) {
			printf("\r%79s", "");
			str[0] = '\0';
			*str0 = str;
			*col0 = col;
			return GETLINE_ABORT;
		}
		switch(c) {
		case Key_BS:
		case Key_DEL:
			if (pos == 0) {
				Beep();
				break;
			}
			str[--len] = '\0';
			pos--;
			delchar();
			break;
		case Key_CR:
		case Key_LF:
			break;
		case Ctrl('U'):
			len = 0;
			for( ; pos > 0; delchar(), pos--);
			str[0] = '\0';
			setcolor(curr_col);
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
			c = Key_SECTION_SIGN;
			addchar = true;
			break;
		case META('s'):
			c = Key_SHARP_S;
			addchar = true;
			break;
                case META('t'):
                        c = '~';
                        addchar = true;
                        break;
		case META('P'):
			c = Key_PILCROW_SIGN;
			addchar = true;
			break;
		case META('%'):
			c = Key_DIVISION_SIGN;
			addchar = true;
			break;
		case META('!'):
			c = Key_INV_EXCL_MARK;
			addchar = true;
			break;
		case META('?'):
			c = Key_INV_QUOT_MARK;
			addchar = true;
			break;
		case META('<'):
			c = Key_LEFT_GUILLEM;
			addchar = true;
			break;
		case META('>'):
			c = Key_RIGHT_GUILLEM;
			addchar = true;
			break;
		case META('|'):
			c = Key_BROKEN_BAR;
			addchar = true;
			break;
		case META('-'):
			c = Key_SOFT_HYPHEN;
			addchar = true;
			break;
		case META('_'):
			c = Key_MACRON;
			addchar = true;
			break;
		case META('+'):
			c = Key_PLUS_MINUS;
			addchar = true;
			break;
		case META('.'):
			c = Key_MIDDLE_DOT;
			addchar = true;
			break;
		case META('*'):
			c = Key_MULT_SIGN;
			addchar = true;
			break;
#if 0
		case Ctrl('C'):
			*str0 = str;
			*col0 = col;
			return EDIT_ABORT;
		case Ctrl('L'):
			Editor_Refresh(t);
			break;
#endif
		case Key_F(1):
			help_edit();
			push_color();
			setcolor(C_EDIT_PROMPT);
			putchar('>');
			pull_color();
			tmpcol = curr_col;
			out = editor2cml(str, col, &tmpcol, pos, NULL);
			cml_print(out);
			putcolor(tmpcol);
			Free(out);
			break;
		case META('a'):
			printf("\nAbort.\n");
			str[0] = '\0';
			*str0 = str;
			*col0 = col;
			return GETLINE_ABORT;

			/* ANSI Colors : Foreground */
		case META('b'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_BLUE);
			break;
		case META('c'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_CYAN);
			break;
		case META('g'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_GREEN);
			break;
		case META('m'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_MAGENTA);
			break;
		case META('n'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_BLACK);
			break;
		case META('r'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_RED);
			break;
		case META('w'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_GRAY);
			break;
		case META('y'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_YELLOW);
			break;
			/* ANSI Colors : Background */
		case META('B'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_BLUE);
			break;
		case META('C'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_CYAN);
			break;
		case META('G'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_GREEN);
			break;
		case META('M'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_MAGENTA);
			break;
		case META('N'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_BLACK);
			break;
		case META('R'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_RED);
			break;
		case META('W'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_GRAY);
			break;
		case META('Y'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_YELLOW);
			break;
			/* ANSI Colors : Attributes */
		case META('h'):
			Editor_Attr_Toggle(&curr_col, &curs_col, ATTR_BOLD);
			break;
		case META('f'):
			Editor_Attr_Toggle(&curr_col, &curs_col, ATTR_BLINK);
			break;
		case META('u'):
			Editor_Attr_Toggle(&curr_col, &curs_col,
					   ATTR_UNDERSCORE);
			break;

		default: /* Add a character */
			IF_ISMETA(c)
				break;
			addchar = true;
			if (len >= max) {
				Beep();
				break;
			}
			if (c == Key_TAB) {
				if (max-len < CLIENT_TABSIZE)
					c = Key_CR;
				else {
					c = ' ';
				        do {
						str[pos] = c;
						col[pos++] = curs_col;
						str[++len] = '\0';
						putchar(c);
					} while (((pos+1) % CLIENT_TABSIZE));
				}
			} else if ((isascii(c) && isprint(c) && (c != '|'))
			    || (is_isoch(c)) || (special && c=='|')) {
				if (maiuscole && islower(c) &&
				    ((pos == 0) || (str[pos - 1] == ' ')))
					c = toupper(c);
                                if (hide)
                                        c = '*';
                        }
			break;
                }
		if (status != GL_NORMAL) {
			if (len >= max)
				Beep();
			else {
				c = gl_extchar(status);
				if (c != -1)
					addchar = true;
				else
					Beep();
			}
			status = GL_NORMAL;
		}
		if (addchar) {
			if (len >= max)
				Beep();
			else {
				str[pos] = c;
				col[pos++] = curs_col;
				str[++len] = '\0';
				putchar(c);
			}
		}
        } while ((c != Key_CR) && (c != Key_LF)
		 && (!wrap || (len < max)));

	if (!wrap)
		putchar('\n');

	*str0 = str;
	*col0 = col;

	return len;
}

#define MAXCHAR 79        /* Numero massimo di caratteri per riga */

int get_text_col(struct text *txt, long max_linee, int max_col, bool abortp)
{
        int parola[256], col_parola[256], prompt_tmp, tmpcol;
        int riga = 0, pos = 0, wlen;
	int *str, *col;
	char *out;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDT;

        if (max_col > MAXCHAR) {
                max_col = MAXCHAR;
	}

	CREATE(str, int, MAXCHAR+1, 0);
	CREATE(col, int, MAXCHAR+1, 0);
	str[0] = '\0';

	setcolor(C_DEFAULT);
	tmpcol = C_DEFAULT;
	push_color();
	setcolor(C_EDIT_PROMPT);
        do {
		putchar('>');
		pull_color();
		out = editor2cml(str, col, &tmpcol, pos, NULL);
		cml_print(out);
		Free(out);
		pos = getline_col_wrap(&str, &col, max_col-1, false, true,
				       true, pos);
		if (pos == GETLINE_ABORT) {
				prompt_curr = prompt_tmp;
				Free(str);
				Free(col);
				return -1;
		}
		push_color();
		setcolor(C_EDIT_PROMPT);

		parola[0] = '\0';
		wlen = 0;
                if (pos == (max_col - 1)) {
                        do {
                                wlen++;
			} while ((str[pos - wlen] != (int)' ')
				 && (pos - wlen >= 0));
                        if ((pos - wlen) >= 0) {
				pos = pos - wlen;
				memcpy(parola, str+pos+1, wlen*sizeof(int));
				memcpy(col_parola, col+pos+1, wlen*sizeof(int));
				str[pos] = 0;
				delnchar(wlen--);
			} else
				wlen = 0;
		}
		printf("\n");
		if (pos == 0) {
			prompt_curr = prompt_tmp;
			Free(str);
			Free(col);
			return (riga == 0) ? 0 : riga+1;
		} else if (abortp && !strcmp((char *)str, "ABORT")) {
		        /* TODO */
			prompt_curr = prompt_tmp;
			Free(str);
			Free(col);
			return -1;
		}
		str[pos] = '\n';
		str[pos+1] = '\0';
		out = editor2cml(str, col, &tmpcol, pos, NULL);
		txt_puts(txt, out);
		memcpy(str, parola, (wlen+1)*sizeof(int));
		memcpy(col, col_parola, (wlen+1)*sizeof(int));
		pos = wlen;
                riga++;
        } while (riga < (max_linee - 1));

	putchar('>');
	pull_color();
	out = editor2cml(str, col, &tmpcol, pos, NULL);
	cml_print(out);
	Free(out);

	pos = getline_col_wrap(&str, &col, max_col-1, false, true, false, pos);
	prompt_curr = prompt_tmp;
	if (pos == GETLINE_ABORT) {
		Free(str);
		Free(col);
		return -1;
	}

	prompt_curr = prompt_tmp;
	printf("\n");

	if (pos == 0) {
		Free(str);
		Free(col);
		return riga;
	}

	if (abortp && (str[0] == 'A') && (str[1] == 'B') && (str[2] == 'O')
	    && (str[3] == 'R') && (str[4] == 'T')) {
		Free(str);
		Free(col);
		return -1;
	}

	str[pos] = '\n';
	str[++pos] = '\0';
	out = editor2cml(str, col, &tmpcol, pos, NULL);
	txt_puts(txt, out);
	Free(str);
	Free(col);

	return max_linee;
}

int gl_extchar(int status)
{
	int c;

	c = inkey_sc(0);
	switch (status) {
	case GL_ACUTE:
		switch(c) {
		case 'a':
			return (Key_a_ACUTE);
		case 'e':
			return (Key_e_ACUTE);
		case 'i':
			return (Key_i_ACUTE);
		case 'o':
			return (Key_o_ACUTE);
		case 'u':
			return (Key_u_ACUTE);
		case 'y':
			return (Key_y_ACUTE);
		case 'A':
			return (Key_A_ACUTE);
		case 'E':
			return (Key_E_ACUTE);
		case 'I':
			return (Key_I_ACUTE);
		case 'O':
			return (Key_O_ACUTE);
		case 'U':
			return (Key_U_ACUTE);
		case 'Y':
			return (Key_Y_ACUTE);
		case ' ':
			return (Key_ACUTE_ACC);
		}
	case GL_CEDILLA:
		switch(c) {
		case 'c':
			return (Key_c_CEDILLA);
		case 'C':
			return (Key_C_CEDILLA);
		case ' ':
			return (Key_CEDILLA);
		}
	case GL_CIRCUMFLEX:
		switch(c) {
		case 'a':
			return (Key_a_CIRCUMFLEX);
		case 'e':
			return (Key_e_CIRCUMFLEX);
		case 'i':
			return (Key_i_CIRCUMFLEX);
		case 'o':
			return (Key_o_CIRCUMFLEX);
		case 'u':
			return (Key_u_CIRCUMFLEX);
		case 'A':
			return (Key_A_CIRCUMFLEX);
		case 'E':
			return (Key_E_CIRCUMFLEX);
		case 'I':
			return (Key_I_CIRCUMFLEX);
		case 'O':
			return (Key_O_CIRCUMFLEX);
		case 'U':
			return (Key_U_CIRCUMFLEX);
		case '1':
			return (Key_SUPER_ONE);
		case '2':
			return (Key_SUPER_TWO);
		case '3':
			return (Key_SUPER_THREE);
		case 'f':
			return (Key_FEM_ORDINAL);
		case 'm':
			return (Key_MASC_ORDINAL);
		}
	case GL_DIAERESIS:
		switch(c) {
		case 'a':
			return (Key_a_DIAERESIS);
		case 'e':
			return (Key_e_DIAERESIS);
		case 'i':
			return (Key_i_DIAERESIS);
		case 'o':
			return (Key_o_DIAERESIS);
		case 'u':
			return (Key_u_DIAERESIS);
		case 'y':
			return (Key_y_DIAERESIS);
		case 'A':
			return (Key_A_DIAERESIS);
		case 'E':
			return (Key_E_DIAERESIS);
		case 'I':
			return (Key_I_DIAERESIS);
		case 'O':
			return (Key_O_DIAERESIS);
		case 'U':
			return (Key_U_DIAERESIS);
		case ' ':
			return (Key_DIAERESIS);
		}
	case GL_GRAVE:
		switch(c) {
		case 'a':
			return (Key_a_GRAVE);
		case 'e':
			return (Key_e_GRAVE);
		case 'i':
			return (Key_i_GRAVE);
		case 'o':
			return (Key_o_GRAVE);
		case 'u':
			return (Key_u_GRAVE);
		case 'A':
			return (Key_A_GRAVE);
		case 'E':
			return (Key_E_GRAVE);
		case 'I':
			return (Key_I_GRAVE);
		case 'O':
			return (Key_O_GRAVE);
		case 'U':
			return (Key_U_GRAVE);
		}
	case GL_MONEY:
		switch(c) {
		case 'c':
			return (Key_CENT_SIGN);
		case 'o':
			return (Key_CURRENCY_SIGN);
		case 'p':
			return (Key_POUND_SIGN);
		case 'y':
			return (Key_YEN_SIGN);
		}
	case GL_PARENT:
		switch(c) {
		case 'c':
			return (Key_COPYRIGHT);
		case 'r':
			return (Key_REGISTERED);
		}
	case GL_RING:
		switch(c) {
		case 'a':
			return (Key_a_RING);
		case 'A':
			return (Key_A_RING);
		case ' ':
			return (Key_DEGREE_SIGN);
		}
	case GL_SLASH:
		switch(c) {
		case 'o':
			return (Key_o_SLASH);
		case 'O':
			return (Key_O_SLASH);
		case '1':
			return (Key_ONE_QUARTER);
		case '2':
			return (Key_ONE_HALF);
		case '3':
			return (Key_THREE_QUART);
		}
	case GL_TILDE:
		switch(c) {
		case 'a':
			return (Key_a_TILDE);
		case 'n':
			return (Key_n_TILDE);
		case 'o':
			return (Key_o_TILDE);
		case 'A':
			return (Key_A_TILDE);
		case 'N':
			return (Key_N_TILDE);
		case 'O':
			return (Key_O_TILDE);
		}
	}
	Beep();
	return -1;
}

#if 0
const char * help_getline =
"Movimento del cursore: Utililizzare i tasti di cursore oppure\n"
"  Ctrl-B  Sposta il cursore di un carattere a sinistra (Backward).\n"
"  Ctrl-F  Sposta il cursore di un carattere a destra   (Forward).\n"
"  Ctrl-N  Vai alla riga successiva (Next).\n"
"  Ctrl-P  Vai alla riga precedente (Previous).\n"
"  Ctrl-A  Vai all'inizio della riga.\n"
"  Ctrl-E  Vai alla fine della riga.\n"
"  Ctrl-V , <Page Down>  Vai alla pagina successiva.\n"
"  Alt-v  , <Page Up>    Vai alla pagina precedente.\n"
"  Esc-<  , <Home>       Vai in cima al testo.\n"
"  Esc->  , <End>        Vai in fondo al testo.\n\n"

"Comandi di cancellazione:\n"
"  Ctrl-D  Cancella il carattere sotto al cursore.\n"
"  Ctrl-K  Cancella dal cursore fino alla fine della riga.\n"
"  Ctrl-U  Cancella tutto il testo.\n"
"  Ctrl-W  Cancella una parola all'indietro.\n\n"

"Altri Comandi:\n"
"  F1      Visualizza questo Help.\n"
"  Ctrl-L  Ripulisce lo schermo.\n"
"  Ctrl-C  Abort: lascia perdere.\n"
"  Ctrl-X  Termina l'immissione del testo.%s\n\n";
#endif
const char * help_getline =
"<b>Comandi di cancellazione:</b>\n"
"  Ctrl-U  Cancella tutto il testo.\n\n"
/*"  Ctrl-W  Cancella una parola all'indietro.\n\n"*/

"<b>Altri Comandi:</b>\n"
/*"  Ctrl-L  Ripulisce lo schermo.\n"*/
"  F1      Visualizza questo Help.\n"
"  Alt-a   Abort: lascia perdere.\n\n";
/*"  Ctrl-X  Termina l'immissione del testo.%s\n\n";*/

const char * help_8bit =
"<b>Caratteri estesi: "

"Modificatori carattere:</b>\n"
"  Alt-' acuto   Alt-^ circonflesso   Alt-\" dieresi   Alt-, cediglia\n"
"  Alt-` grave   Alt-~ tilde          Alt-/ slash\n\n"

"<b>Le combinazioni permesse sono:</b>\n"
"  Alt-' + ( A , E , I , O , U , Y , a , e , i , o , u , y , SPACE )\n"
"        &raquo;   &Aacute;   &Eacute;   &Iacute;   &Oacute;   &Uacute;   &Yacute;   &aacute;   &eacute;   &iacute;   &oacute;   &uacute;   &yacute;   &acute;\n"
"  Alt-^ + ( A , E , I , O , U , a , e , i , o , u , 1 , 2 , 3 , f , m )\n"
"        &raquo;   &Acirc;   &Ecirc;   &Icirc;   &Ocirc;   &Ucirc;   &acirc;   &ecirc;   &icirc;   &ocirc;   &ucirc;   &sup1;   &sup2;   &sup3;   &ordf;   &ordm;\n"
"  Alt-\" + ( A , E , I , O , U , a , e , i , o , u , y , SPACE )\n"
"        &raquo;   &Auml;   &Euml;   &Iuml;   &Ouml;   &Uuml;   &auml;   &euml;   &iuml;   &ouml;   &uuml;   &yuml;   &uml;\n"
"  Alt-` + ( A , E , I , O , U , a , e , i , o , u )\n"
"        &raquo;   &Agrave;   &Egrave;   &Igrave;   &Ograve;   &Ugrave;   &agrave;   &egrave;   &igrave;   &ograve;   &ugrave;\n"
"  Alt-~ + ( A , N , O , a , n , o)  &raquo; ( &Atilde;   &Ntilde;   &Otilde;   &atilde;   &ntilde;   &otilde; )\n"
"  Alt-/ + ( O , o , 1 , 2 , 3 )     &raquo; ( &Oslash;   &oslash;   &frac14;   &frac12;   &frac34; )\n"
"  Alt-o + ( A , a , SPACE )         &raquo; ( &Aring;   &aring;   &deg; )\n"
"  Alt-, + ( C , c , SPACE )         &raquo; ( &Ccedil;   &ccedil;   &cedil; )\n"
"  Alt-( + ( c , r )                 &raquo; ( &copy;   &reg; )\n"
"  Alt-$ + ( c , o , p , y )         &raquo; ( &cent;   &curren;   &pound;   &yen; )\n\n"

"<b>Altri caratteri speciali:</b>\n"
"  &szlig; Alt-s   &sect; Alt-S   &para; Alt-P   &divide; Alt-%   &times; Alt-*   &iexcl; Alt-!   &iquest; Alt-?   ~ Alt-t\n"
"  &laquo; Alt-\\<   &raquo; Alt->   &brvbar; Alt-|   &shy; Alt--   &macr; Alt-_   &plusmn; Alt-+   &middot; Alt-.\n";

const char *help_colors =
"<b>ANSI Colors:</b>\n\n"

"<u>Colore carattere</u>: Alt-lettera o Escape seguito dalla lettera.\n"
"  Alt-b <fg=4>blue<fg=7>   Alt-c <fg=6>cyan<fg=7>    Alt-g <fg=2>green<fg=7>   Alt-m <fg=5>magenta<fg=7>\n"
"  Alt-r <fg=1>red<fg=7>    Alt-y <fg=3>yellow<fg=7>  Alt-w <fg=7>white<fg=7>   Alt-n <fg=0;bg=7>black<fg=7;bg=0>\n\n"

/*
"<u>Colore carattere highlight</u>:\n"
"  Alt-b <b;fg=4>blue</b;fg=7>   Alt-c <b;fg=6>cyan</b;fg=7>    Alt-g <b;fg=2>green</b;fg=7>   Alt-m <b;fg=5>magenta</b;fg=7>\n"
"  Alt-r <b;fg=1>red</b;fg=7>    Alt-y <b;fg=3>yellow</b;fg=7>  Alt-w <b;fg=7>white</b;fg=7>   Alt-n <b;fg=0>black</b;fg=7>\n\n"
*/
"<u>Colore di fondo</u>: Escape seguito da Shift-lettera.\n"
"  Esc-B <bg=4>blue<bg=0>   Esc-C <bg=6>cyan<bg=0>    Esc-G <bg=2>green<bg=0>   Esc-M <bg=5>magenta<bg=0>\n"
"  Esc-R <bg=1>red<bg=0>    Esc-Y <bg=3>yellow<bg=0>  Esc-W <bg=7;fg=0>white<bg=0;fg=7>   Esc-N black\n\n"

"<u>Attributi del carattere</u>:\n"
"  Alt-h <b>Highlight</b> (grassetto)\n"
"  Alt-u <u>Underscore</u> (sottolineato)\n"
"  Alt-f Flash (lampeggia) [Non funziona su tutti i terminali]\n";

const char *help_line_editor =
  "<b>Help editor di linea:</b>\n"
  "  Enter   Invia il messaggio\n"
  "  F1      Visualizza questo Help.\n"
  "  Ctrl-A  Vai all'inizio della riga\n"
  "  Ctrl-E  Vai alla fine della riga\n"
  "  Ctrl-B  Sposta il cursore di un carattere a sinistra\n"
  "  Ctrl-F  Sposta il cursore di un carattere a destra\n"
  "  Ctrl-L  Refresh line\n"
  "  Ctrl-P  Cancella una parola all'indietro\n"
  "  Ctrl-K  Cancella dal cursore fino alla fine della riga\n"
  "  Ctrl-U  Cancella tutto il testo\n"
  "  Ctrl-C  Abort: lascia perdere.\n\n";

static void help_edit(void)
{
	push_color();
	setcolor(C_HELP);
	printf("\r%80s\n", "");
	cml_printf("%s", help_getline);
	hit_any_key();
	cml_print(help_8bit);
	hit_any_key();
	cml_printf("\n\n\n\n%s\n", help_colors);
	hit_any_key();
	pull_color();
}


static void help_line_edit(void)
{
	push_color();
	setcolor(C_HELP);
	printf("\r%80s\n", "");
	cml_printf("%s", help_line_editor);
	pull_color();
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#if 0 /* getline originale */
void getline(char *str, int max, char maiuscole, char special)
{
        int c, len = 0;
        int hide = FALSE;

        if (max == 0)
                return;
        if (max < 0) {
                hide = TRUE;
                max = -max;
        }

        do {
                c = inkey_sc(0);
                if ((isascii(c) && isprint(c) && c != '|') || (c == Key_BS)
                    || (special && c == '|')) {
                        if ((c == Key_BS) && (len != 0)) {
                                delchar();
                                str[--len] = '\0';
                        } else if ((c != Key_BS) && (len < max)) {
                                if (maiuscole && islower(c) &&
                                    ((len == 0) || (str[len - 1] == ' ')))
					c = toupper(c);
                                if (hide)
                                        putchar('*');
                                else
                                        putchar(c);
                                str[len] = c;
                                str[++len] = '\0';
                        }
                }
        } while ((c != Key_CR) && (c != Key_LF));

        putchar('\n');
}

int get_text(struct text *txt, long max_linee, char max_col, bool abortp)
{
        int riga = 0, pos = 0, wlen, maxchar = 79;
        char str[80], parola[80], prompt_tmp;
        int ch;

	prompt_tmp = prompt_curr;
	prompt_curr = P_EDT;

        if (max_col > maxchar)
                max_col = maxchar;
	str[0] = '\0';

        do {
                printf(">%s", str);
                do {
			prompt_txt = str;
                        ch = inkey_sc(0);
                        if ((isascii(ch) && isprint(ch)) || (ch == Key_BS)
			    || (ch == Ctrl('P')) || (ch == Ctrl('U'))
			    || (ch == Ctrl('X'))) {
                                if ((ch == Key_BS) && (pos != 0)) {
                                        delchar();
                                        str[--pos] = '\0';
				} else if (ch == Ctrl('P')) {
					while ((pos!=0) && (str[pos-1]==' ')) {
						str[--pos] = '\0';
						delchar();
					}
					while ((pos!=0) && (str[pos-1]!=' ')) {
						str[--pos] = '\0';
						delchar();
					}
				} else if (ch == Ctrl('U')) {
					printf("\r%80s\r>", "");
					pos = 0;
                                        str[0] = '\0';
				} else if (ch == Ctrl('X')) {
					printf("\nAbort.\n");
					prompt_curr = prompt_tmp;
					return -1;
                                } else if (ch != Key_BS) {
                                        putchar(ch);
                                        str[pos] = ch;
                                        str[++pos] = '\0';
                                }
                        }
                } while (((ch != Key_LF) && (ch != Key_CR))
			 && (pos < (max_col - 1)));

		parola[0] = '\0';
		wlen = 0;
                if (pos == (max_col - 1)) {
                        do {
                                wlen++;
			} while ((str[pos-wlen] != ' ') && (pos-wlen >= 0));
                        if (pos-wlen >= 0) {
				pos = pos - wlen;
				memcpy(parola, str+pos+1, wlen);
				str[pos] = '\0';
				delnchar(--wlen);
			} else
				wlen = 0;
		}
		printf("\n");
		if (pos == 0) {
			prompt_curr = prompt_tmp;
			if (riga == 0)
				return 0;
			else
				return riga+1;
		} else if (abortp && !strcmp(str, "ABORT")) {
			prompt_curr = prompt_tmp;
			return -1;
		}
		str[pos] = '\n';
		str[pos+1] = '\0';
		txt_put(txt, str);
		memcpy(str, parola, wlen+1);
		pos = wlen;
                riga++;
        } while (riga < (max_linee - 1));

	printf(">%s", str);
	do {
		prompt_txt = str;
		ch = inkey_sc(0);
		if ((isascii(ch) && isprint(ch)) || (ch == Key_BS)
		    || (ch == Ctrl('P')) || (ch == Ctrl('U'))
		    || (ch == Ctrl('X'))) {
			if ((ch == Key_BS) && (pos != 0)) {
				delchar();
				str[--pos] = '\0';
			} else if (ch == Ctrl('P')) {
				while ((pos!=0) && (str[pos-1]==' ')) {
					str[--pos] = '\0';
					delchar();
				}
				while ((pos!=0) && (str[pos-1]!=' ')) {
					str[--pos] = '\0';
					delchar();
				}
			} else if (ch == Ctrl('U')) {
				printf("\r%80s\r>", "");
				pos = 0;
				str[0] = '\0';
			} else if (ch == Ctrl('X')) {
				printf("\nAbort.\n");
				prompt_curr = prompt_tmp;
				return -1;
			} else if ((ch != Key_BS) && (pos < (max_col-1))) {
				putchar(ch);
				str[pos] = ch;
				str[++pos] = '\0';
			}
		}
	} while ((ch != Key_LF) && (ch != Key_CR));

	prompt_curr = prompt_tmp;
	printf("\n");

	if (pos == 0)
		return riga;

	if (abortp && !strcmp(str, "ABORT"))
		return -1;

	str[pos] = '\n';
	str[++pos] = '\0';
	txt_put(txt, str);

	return max_linee;
}
#endif

#if 0 /* L'originale */
char * getline_col(int max, char maiuscole, char special)
{
        int c;                     /* carattere corrente            */
        int hide = FALSE;
	int status = GL_NORMAL;    /* stato dell'editor             */
	int *str;                  /* Stringa in input              */
	int *col;                  /* Colori associati ai caratteri */
	int pos;                   /* Posizione del cursore         */
	int len;                   /* Lunghezza della stringa       */
	int curs_col;   /* Colore corrente del cursore              */
	int curr_col;   /* Ultimo settaggio di colore sul terminale */
	int addchar;

        if (max == 0)
                return NULL;
        if (max < 0) {
                hide = TRUE;
                max = -max;
        }

#if 0
	curs_col = C_DEFAULT;
	curr_col = C_DEFAULT;
#endif
	curs_col = C_BOLD; /* Solo perche' ora tratto il doing! */
	curr_col = C_BOLD;

	setcolor(curs_col);

	CREATE(str, int, max + 1, 0);
	CREATE(col, int, max + 1, 0);
	str[0] = '\0';
	pos = 0;
	len = 0;

	do {
		addchar = FALSE;
                c = inkey_sc(0);
		switch(c) {
		case Key_BS:
		case Key_DEL:
			if (pos == 0) {
				Beep();
				break;
			}
			str[--len] = '\0';
			pos--;
			delchar();
			break;
		case Key_CR:
		case Key_LF:
			break;
		case Ctrl('U'):
			len = 0;
			for( ; pos > 0; delchar(), pos--);
			str[0] = '\0';
			setcolor(curr_col);
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
			c = Key_SECTION_SIGN;
			addchar = TRUE;
			break;
		case META('s'):
			c = Key_SHARP_S;
			addchar = TRUE;
			break;
		case META('P'):
			c = Key_PILCROW_SIGN;
			addchar = TRUE;
			break;
		case META('%'):
			c = Key_DIVISION_SIGN;
			addchar = TRUE;
			break;
		case META('!'):
			c = Key_INV_EXCL_MARK;
			addchar = TRUE;
			break;
		case META('?'):
			c = Key_INV_QUOT_MARK;
			addchar = TRUE;
			break;
		case META('<'):
			c = Key_LEFT_GUILLEM;
			addchar = TRUE;
			break;
		case META('>'):
			c = Key_RIGHT_GUILLEM;
			addchar = TRUE;
			break;
		case META('|'):
			c = Key_BROKEN_BAR;
			addchar = TRUE;
			break;
		case META('-'):
			c = Key_SOFT_HYPHEN;
			addchar = TRUE;
			break;
		case META('_'):
			c = Key_MACRON;
			addchar = TRUE;
			break;
		case META('+'):
			c = Key_PLUS_MINUS;
			addchar = TRUE;
			break;
		case META('.'):
			c = Key_MIDDLE_DOT;
			addchar = TRUE;
			break;
		case META('*'):
			c = Key_MULT_SIGN;
			addchar = TRUE;
			break;
#if 0
		case Ctrl('C'):
			return EDIT_ABORT;
		case Ctrl('L'):
			Editor_Refresh(t);
			break;
		case Key_F(1):
			help_edit(t);
			break;
		case Ctrl('X'):
			return EDIT_DONE;
#endif

			/* ANSI Colors : Foreground */
		case META('b'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_BLUE);
			break;
		case META('c'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_CYAN);
			break;
		case META('g'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_GREEN);
			break;
		case META('m'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_MAGENTA);
			break;
		case META('n'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_BLACK);
			break;
		case META('r'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_RED);
			break;
		case META('w'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_GRAY);
			break;
		case META('y'):
			Editor_Fg_Color(&curr_col, &curs_col, COL_YELLOW);
			break;
			/* ANSI Colors : Background */
		case META('B'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_BLUE);
			break;
		case META('C'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_CYAN);
			break;
		case META('G'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_GREEN);
			break;
		case META('M'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_MAGENTA);
			break;
		case META('N'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_BLACK);
			break;
		case META('R'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_RED);
			break;
		case META('W'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_GRAY);
			break;
		case META('Y'):
			Editor_Bg_Color(&curr_col, &curs_col, COL_YELLOW);
			break;
			/* ANSI Colors : Attributes */
		case META('h'):
			Editor_Attr_Toggle(&curr_col, &curs_col, ATTR_BOLD);
			break;
		case META('f'):
			Editor_Attr_Toggle(&curr_col, &curs_col, ATTR_BLINK);
			break;
		case META('u'):
			Editor_Attr_Toggle(&curr_col, &curs_col,
					   ATTR_UNDERSCORE);
			break;

		default: /* Add a character */
			IF_ISMETA(c)
				break;
			addchar = TRUE;
			if (len >= max) {
				Beep();
				break;
			}
			if ((isascii(c) && isprint(c) && (c != '|'))
			    || (is_isoch(c)) || (special && c=='|')) {
				if (maiuscole && islower(c) &&
				    ((pos == 0) || (str[pos - 1] == ' ')))
					c = toupper(c);
                                if (hide)
                                        c = '*';
                        }
			break;
                }
		if (status != GL_NORMAL) {
			if (len >= max)
				Beep();
			else {
				c = gl_extchar(status);
				if (c != -1)
					addchar = TRUE;
				else
					Beep();
			}
			status = GL_NORMAL;
		}
		if (addchar == TRUE) {
			if (len >= max)
				Beep();
			else {
				str[pos] = c;
				col[pos++] = curs_col;
				str[++len] = '\0';
				putchar(c);
			}
		}
        } while ((c != Key_CR) && (c != Key_LF));

	setcolor(C_DEFAULT);
        putchar('\n');
	return editor2cml(str, col, len, NULL);
}
#endif
