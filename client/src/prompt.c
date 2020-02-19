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
* File : prompt.c                                                           *
*        Comandi per la visualizzazione dei prompt.                         *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "ansicol.h"
#include "cittaclient.h"
#include "cml.h"
#include "comandi.h"
#include "comunicaz.h"
#include "messaggi.h"
#include "prompt.h"
#include "room_flags.h"
#include "server_flags.h"
#include "user_flags.h"
#include "utility.h"

/* Variabili globali */
char prompt_curr;        /* Prompt correntemente visualizzato.          */
char *prompt_txt;        /* Puntatore alla stringa che si sta editando. */
long prompt_pager_max, prompt_pager_n; /* #linee per prompt pager.      */
long prompt_xlog_n;      /* Numero messaggio diario corrente.           */
long prompt_xlog_max;    /* Numero messaggi nel diario.                 */
extern long prompt_attach; /* Numero allegati presenti nel post corrente  */
                           /* definito in share/metadata.c                */

/* Prototipi funzioni in questo file */
void room_prompt(char *pr);
void msg_prompt(char *prompt);
void pager_prompt(char *prompt);
void edit_prompt(char *prompt);
void entry_prompt(char *prompt);
void xlog_prompt(char *prompt);
void print_prompt(void);
void del_prompt(void);

/****************************************************************************
****************************************************************************/
/*
 * Prompt della room
 */
void room_prompt(char *pr)
{
        char tmp[LBUF], tmp1[LBUF];

	prompt_curr = P_ROOM;

	if (!(uflags[0] & UT_ESPERTO)) {
		setcolor(C_HELP);
		printf(_(
"\n -----------------------------------------------------------------------\n"
" Room cmds:    <k>nown rooms, <g>oto next room, <j>ump to specific room,\n"
"               <s>kip this room, <a>bandon this room, <z>ap this room,  \n"
" Message cmds: <n>ew msgs, <f>orward read, <r>everse read, <o>ld msgs,  \n"
"               <5> last five msgs, <e>nter a message                    \n"
" General cmds: <?> help, <Q>uit, <t>ime & date, <w>ho is online, <x>-msg\n"
" Misc:         <E> toggle Expert mode, <%%> lock terminal <p>rofile     \n"
"\n"
"  (Type <H> for help menu, <E> to disable this menu)\n"
" -----------------------------------------------------------------------\n"));
	}
	setcolor(C_ROOMPROMPT);
	/* TODO at the moment blog_vroom is unused */
	if (blog_vroom)
	        strcpy(tmp, _("La casa di <b>%s</b>"));
	else
	        strcpy(tmp, "<b>%s</b>%c");
	if ((SERVER_FLOORS) && (uflags[4] & UT_FLOOR)) {
	        sprintf(tmp1, "\n<b>%%s</b>: %s ", tmp); 
		sprintf(pr, tmp1, current_floor, current_room, *room_type);
	} else {
		sprintf(tmp1, "\n%s ", tmp);
		sprintf(pr, tmp1, current_room, *room_type);
	}
}

/*
 * Prompt in modo lettura messaggi.
 */
void msg_prompt(char *prompt)
{
	setcolor(C_MSGPROMPT);
	prompt_curr = P_MSG;
	if (uflags[0] & UT_ESPERTO)
                if (prompt_attach == 0)
                        sprintf(prompt, _("[<b>%s</b>%s msg #<b>%ld</b> (Da leggere: <b>%ld</b>)] -> "), current_room, room_type, locnum, remain);
                else if (prompt_attach == 1)
                        sprintf(prompt, _("[<b>%s</b>%s msg #<b>%ld</b>, <b>1</b> allegato (Da leggere: <b>%ld</b>)] -> "), current_room, room_type, locnum, remain);
                else
                        sprintf(prompt, _("[<b>%s</b>%s msg #<b>%ld</b>, <b>%ld</b> allegati (Da leggere: <b>%ld</b>)] -> "), current_room, room_type, locnum, prompt_attach, remain);
	else {
		sprintf(prompt, "Msg #<b>%ld</b> ", locnum);
		if (rflags & RF_REPLY)
			strcat(prompt, _("\\<<b>r</b>>eply "));
		sprintf(prompt + strlen(prompt), _("\\<<b>b</b>>ack \\<<b>a</b>>gain \\<<b>n</b>>ext \\<<b>s</b>>top \\<<b>?</b>> help (da leggere <b>%ld</b>): "), remain);
	}
}

/*
 * Prompt del pager.
 */
void pager_prompt(char *prompt)
{
	prompt_curr = P_PAG;

	setcolor(C_PAGERPROMPT);
	sprintf(prompt, _("\r-- <b>more</b> (\\<Space\\>, \\<Enter\\>, \\<s\\>top, \\<?\\> help) <b>linea %ld/%ld %ld%%</b> --"), prompt_pager_n, prompt_pager_max, (long)prompt_pager_n*100/prompt_pager_max);
}

/*
 * Prompt dell'editor.
 */
void edit_prompt(char *prompt)
{
	putchar('\n');
	prompt[0] = '>';
	strncpy(prompt+1, prompt_txt, 78);
	prompt[79] = 0;
}

/*
 * Prompt dell'Entry Command.
 */
void entry_prompt(char *prompt)
{
	setcolor(C_ENTRYPROMPT);
	sprintf(prompt, _("Entry cmd (<b>?</b> lista opzioni) -> "));
}

/*
 * Prompt del diario messaggi.
 */
void xlog_prompt(char *prompt)
{
	setcolor(C_XLOGPROMPT);
	sprintf(prompt, _("[Msg #<b>%ld</b> - \\<?\\> per i comandi - (Da leggere: <b>%ld</b>)] -> "), prompt_xlog_n, prompt_xlog_max - prompt_xlog_n);
}

/*
 * Prompt dell'editor di friend-list & co.
 */
void friend_prompt(char *prompt)
{
        setcolor(C_FRIEND_PROMPT);
	sprintf(prompt, _("<a>ggiungi, <r>imuovi, <x> scambia, <s>alva ed esci, <l>ascia perdere -> "));
}

/*
 * Prompt dell'editor di friend-list & co.
 */
void scelta_prompt(char *prompt)
{
	sprintf(prompt, _("Scelta? "));
}

/*
 * Visualizza il prompt corrispondente allo stato dell'utente.
 */
void print_prompt(void)
{
	char prompt[LBUF];
	
	push_color();
	switch(prompt_curr) {
	case P_ROOM:
		room_prompt(prompt);
		break;
	case P_MSG:
		putchar('\n');
		msg_prompt(prompt);
		break;
	case P_PAG:
		pager_prompt(prompt);
		break;
	case P_EDT:
		edit_prompt(prompt);
		break;
	case P_ENT:
		entry_prompt(prompt);
		break;
	case P_XLOG:
		xlog_prompt(prompt);
		break;
	case P_FRIEND:
		friend_prompt(prompt);
		break;
	case P_WRAPPER_URNA:
		scelta_prompt(prompt);
		break;
	}
	cml_printf("%s", prompt);
	pull_color();
	fflush(stdout);
}

/*
 * Cancella il prompt se l'utente usa i 'disappearing prompts'.
 */
void del_prompt(void)
{
	if (uflags[1] & UT_DISAPPEAR)
                cleanline();
	else
		putchar('\n');
}
