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
* File : pager.c                                                            *
*        Pager per la lettura dei testi.                                    *
****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "ansicol.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "cml.h"
#include "conn.h"
#include "cterminfo.h"
#include "help_str.h"
#include "inkey.h"
#include "pager.h"
#include "prompt.h"
#include "text.h"
#include "user_flags.h"
#include "utility.h"

#define PROMPT_STR \
"\r-- <b>more</b> (\\<Space>, \\<Enter>, \\<s>top, \\<?> help) <b>linea %ld/%ld %ld%%<\b> --"

/* Prototipi delle funzioni statiche in questo file */
static void pager_help(void);
static bool pager_newstr(struct text *txt, char *aaa, bool inc_max,
                         Metadata_List *mdlist);

/***************************************************************************/
/*
 * Riceve un testo dal server e lo visualizza con il pager.
 *
 * Se txt e' NULL, viene creato un buffer temporaneo per il testo,
 * che viene poi eliminato prima dell'uscita dal pager.
 *
 * Se (send == 1) il testo arriva dal server, e percio' controlla i sockets
 * in attesa di nuove righe, altrimenti il testo e' gia' tutto in *txt.
 *
 * Se la lunghezza del testo e' nota in anticipo, viene messa fornita in max
 * Start e' il numero di riga dal quale inizia la visualizzazione del testo.
 *
 * Se cml e' TRUE, il testo e` in CML e viene interpretato prima di venire
 * visualizzato, altrimenti viene visualizzato cosi' com'e'.
 */
void pager(struct text *txt, bool send, long max, int start, bool cml,
           bool spoiler, Metadata_List *mdlist)
{
	char aaa[LBUF], prompt_tmp;
	bool fine = false, refresh = false, freetxt = false, inc_max = true;
	int c, s, ink; /* c: carattere input; s: righe da stampare */

	if (txt == NULL) {
		txt = txt_create();
		freetxt = true;
	} else if (start) {
		txt_jump(txt, start);
	}

	if (max) {
		prompt_pager_max = max;
		inc_max = false;
	} else {
		prompt_pager_max = txt_len(txt);
	}

	prompt_tmp = prompt_curr;
	prompt_curr = P_PAG;
	prompt_pager_n = start;

	s = NRIGHE - 1 - start;
	while (!fine) {
		/* Stampa finche' non raggiungo il prompt o termina il testo */
		while ((s > 0) && (!fine)) {
			if (txt_rpos(txt) == txt_len(txt)) {
				if (send) {
					serv_gets(aaa);
					send = pager_newstr(txt, aaa, inc_max,
                                                            mdlist);
				} else {
					fine = true;
				}
				if (!send) {
					fine = true;
				}
			}
			if (!fine) {
				if (cml) {
					cml_spoiler_print(txt_get(txt), -1,
                                                          spoiler);
					putchar('\n');
				} else
					printf("%s\n", txt_get(txt));
				prompt_pager_n++;
				s--;
			}
		}

		/* Prompt paginatore */
		if (!fine) {
			if (uflags[1] & UT_PAGINATOR
			    && !(uflags[1] & UT_NOPROMPT)) {
				PAGLOOP:
				print_prompt();
				c = 0;
				while ((c!='s') && (c!='q')
				       && (c!='f') && (c!='b') && (c!='n')
				       && (c!='p') && (c!=' ') && (c!='r')
				       && (c!='?') && (c!=Ctrl('L'))
				       && (c!=Ctrl('C')) && (c!=Key_UP)
				       && (c!=Key_DOWN) && (c!=Key_PAGEUP)
				       && (c!=Key_PAGEDOWN) && (c!=Key_CR)
				       && (c!=Key_LF)) {
					ink = inkey_pager(uflags[3] & UT_XPOST,
							aaa, &c);
					if (ink & INKEY_SERVER) {
						send = pager_newstr(txt, aaa,
                                                              inc_max, mdlist);
						if ((!send) && (txt_rpos(txt) == txt_len(txt)))
							fine = true;
						if (inc_max) {
						    push_color();
						    setcolor(C_PAGERPROMPT);
						    cml_printf(PROMPT_STR, prompt_pager_n, prompt_pager_max, (long)prompt_pager_n*100/prompt_pager_max);
						    pull_color();
						}
				        }
				}

				printf("\r%78s\r", "");
				switch (c) {
                                 case Key_CR:
                                 case Key_LF:
                                 case Key_DOWN:
				 case 'f': /* Una riga avanti     */
					s = 1;
					break;
                                 case Key_UP:
				 case 'b': /* Una riga indietro   */
					txt_jump(txt, - NRIGHE);
					prompt_pager_n = txt_rpos(txt);
					s = NRIGHE - 1;
					break;
                                 case Key_PAGEDOWN:
				 case ' ':
				 case 'n': /* Una pagina avanti   */
					s = NRIGHE - 2;
					break;
                                 case Key_PAGEUP:
				 case 'p': /* Una pagina indietro */
					txt_jump(txt, - (2*NRIGHE - 3));
					prompt_pager_n = txt_rpos(txt);
					s = NRIGHE - 1;
					break;
				 case Ctrl('L'):
				 case 'r': /* Rinfresca schermata */
					refresh = true;
					break;
				 case 'q':
				 case 's': /* Esci dal pager      */
				 case Ctrl('C'):
					fine = true;
					break;
				 case '?': /* Aiuto               */
					pager_help();
					refresh = true;
					goto PAGLOOP;
					break;
				}
			} else
				s = NRIGHE - 2;

			if (refresh) {
				txt_jump(txt, -(NRIGHE-1));
				prompt_pager_n = txt_rpos(txt);
				refresh = false;
				s = NRIGHE - 1;
			}
		}
	}

	/* Prende cio' che rimane del testo */
	if (send) {
   	        while (serv_gets(aaa), strcmp(aaa, "000")) {
		        txt_put(txt, aaa+4);
		}
	}

	if (freetxt) {
		txt_free(&txt);
	}

	prompt_curr = prompt_tmp;
}

/****************************************************************************/
/*
 * Stampa l'help per il pager.
 */
static void pager_help(void)
{
	push_color();
	setcolor(C_HELP);
        cml_print(_( "\n" HELP_PAGER_CML "\n"));
	pull_color();
}

/*
 * Restituisce 0 se il testo e' terminato, altrimenti memorizza la
 * nuova stringa nel testo e restituisce 1.
 * Se (inc_max != 0) incrementa il numero totale di righe del testo.
 */
static bool pager_newstr(struct text *txt, char *aaa, bool inc_max,
                         Metadata_List *mdlist)
{
        if (!strcmp(aaa, "000")) {
		return false;
	}
	txt_putf(txt, "%s", aaa+4);

        /* Estrae il metadata */
        cml_extract_md(aaa+4, mdlist);

	if (inc_max) {
		prompt_pager_max++;
	}
	return true;
}
