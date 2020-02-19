/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/*****************************************************************************
 * Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File: post.c                                                              *
 *       comandi per inviare post/mail                                       *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "x.h"
#include "blog.h"
#include "extract.h"
#include "fileserver.h"
#include "mail.h"
#include "memstat.h"
#include "post.h"
#include "utente.h"
#include "strutture.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void post_timeout(struct sessione *t);
void cmd_pstb(struct sessione *t, char *txt);
void cmd_pste(struct sessione *t, char *txt);
char admin_post(struct sessione *t, int mode);

/* Prototipi funzioni statiche in questo file */
static void init_txtp(struct sessione *t, int reply);
static void notifica_post(struct sessione *t);

/*****************************************************************************
 *****************************************************************************/
/*
 * Inizializza l'invio del testo di un post.
 * (come init_text() ma invia in piu' i flags della room al client).
 */
static void init_txtp(struct sessione *t, int reply)
{
        if ((t->text) != NULL)
		txt_free(&t->text);
        t->text = txt_create();
	if (reply && (t->room->data->flags & RF_REPLY))
		cprintf(t, "%d %ld|%ld|%s|%s\n", INVIA_TESTO,
			t->room->data->flags, t->reply_num, t->reply_to,
			t->reply_subj);
	else
		cprintf(t, "%d %ld\n", INVIA_TESTO, t->room->data->flags);
}

/****************************************************************************/
/* Tempo scaduto per il post */
void post_timeout(struct sessione *t)
{
	cprintf(t, "%d\n", PTOU);
	t->room->lock = false;
	reset_text(t);
}

/*
 * Restituisce 1 se e' un post normale o un post di amministrazione e l'utente
 * e' autorizzato ad inviarlo, altrimenti 0.
 */
char admin_post(struct sessione *t, int mode)
{
	switch (mode) {
	 case 0:
		return 1;
	 case 1:
		if (IS_RH(t))
			return 1;
		break;
	 case 2:
		if (IS_AIDE(t))
			return 1;
		break;
	 case 3:
		if (IS_SYSOP(t))
			return 1;
		break;
	}
	return 0;
}

/*
 * Inizializza invio di un post
 */
void cmd_pstb(struct sessione *t, char *txt)
{
        struct dati_ut *ut;
        char recipient[MAXLEN_UTNAME];

        if ((t->utente->livello < MINLVL_POST) 
	    || (t->utente->livello < t->room->data->wlvl)) {
		cprintf(t, "%d Livello insufficiente.\n", ERROR+ACC_PROIBITO);
		return;
	}
        if ((t->utente->sflags[1] & SUT_NOMAIL)
            && (t->room->data->flags & RF_MAIL)) {
		cprintf(t, "%d divieto mail.\n",
                        ERROR+ACC_PROIBITO+MAIL_PROIBITO);
		return;
	}
        if ((t->utente->sflags[1] & SUT_NOANONYM)
            && (t->room->data->flags & RF_ANONYMOUS)) {
		cprintf(t, "%d divieto anonimi.\n",
                        ERROR+ACC_PROIBITO+ANONYM_PROIBITO);
		return;
	}
        if (t->utente->sflags[1] & SUT_NOPOST) {
		cprintf(t, "%d divieto post.\n",
                        ERROR+ACC_PROIBITO+POST_PROIBITO);
		return;
	}
        if (t->room->data->type == ROOM_DYNAMICAL) {
	        if (!(t->room->data->flags & RF_BLOG) ||
		    !blog_post(t->room, t->utente)) {
		  cprintf(t, "%d Non puoi postare qui.\n", ERROR+ACC_PROIBITO);
		  return;
		}
	}
        if ((STEP_POST * t->utente->secondi_per_post
             * (t->utente->post + t->utente->mail - 10))
            > t->utente->time_online){
 		cprintf(t, "%d Hai messo troppi post, attendi...\n",
 			ERROR + ACC_PROIBITO + TOO_MANY);
 		return;
 	}
	if (!strcmp(t->room->data->name, dump_room)) {
		cprintf(t, "%d Non si puo` postare nel cestino.\n",
			ERROR + ACC_PROIBITO);
		return;
	}
	if (!admin_post(t, extract_int(txt, 2))) {
		cprintf(t, "%d Non puoi lasciare messaggi admin.\n",
			ERROR+ACC_PROIBITO+NO_ADMIN);
		return;
	}
	if (t->room->data->flags & RF_POSTLOCK) {
		if (t->room->lock) {
			cprintf(t, "%d Room locked.\n", ERROR+WRONG_STATE);
			return;
		}
		if (t->room->data->flags & RF_L_TIMEOUT)
			t->rlock_timeout = FREQUENZA * POSTLOCK_TIMEOUT;
		else
			t->rlock_timeout = 0;
	}
	if (t->room->data->flags & RF_MAIL) {
		if (extract_int(txt, 1))
			strncpy(recipient, t->reply_to, MAXLEN_UTNAME);
		else
			extractn(recipient, txt, 0, MAXLEN_UTNAME);
		switch(extract_int(txt, 3)) {
		case 0:
                        ut = trova_utente(recipient);
                        if (ut == NULL) {
				cprintf(t,"%d\n", ERROR+NO_SUCH_USER);
				return;
                        }
			if (!mail_access_ut(ut)) {
				cprintf(t,"%d\n", ERROR+ACC_PROIBITO+NO_MAIL);
				return;
			} else if (t->utente->matricola == ut->matricola) {
				cprintf(t,"%d\n", ERROR+ARG_NON_VAL);
				return;
			}
                        t->rcpt[0] = ut->matricola;
                        t->num_rcpt = 1;
			t->text_com = TXT_MAIL;
			break;
		case 1:
			t->text_com = TXT_MAILSYSOP;
			break;
		case 2:
			t->text_com = TXT_MAILAIDE;
			break;
		case 3:
			t->text_com = TXT_MAILRAIDE;
			break;
		}
	} else
		t->text_com = TXT_POST;

	if (t->room->data->flags & RF_POSTLOCK)
		t->room->lock = true;
	init_txtp(t, extract_int(txt, 1));
	t->text_max = MAXLINEEPOST;
	t->stato = CON_POST;
	t->occupato = 1;
}

/*
 * Fine Post
 */
void cmd_pste(struct sessione *t, char *txt)
{
	if ((t->text_com != TXT_POST) && (t->text_com != TXT_MAIL)
	    && (t->text_com != TXT_MAILSYSOP) && (t->text_com != TXT_MAILAIDE)
	    && (t->text_com != TXT_MAILRAIDE)) {
		cprintf(t, "%d Post non inizializzato.\n", ERROR+WRONG_STATE);
		return;
	} else if ((t->text == NULL)||(txt_len(t->text) == 0)) {
		cprintf(t, "%d Nessun testo. Post annullato.\n",
			ERROR + ARG_VUOTO);
                fs_cancel_all_bookings(t->utente->matricola);
	} else if (!admin_post(t, extract_int(txt, 3))) {
		cprintf(t, "%d Non puoi lasciare msg admin.\n",
			ERROR + ACC_PROIBITO + NO_ADMIN);
                fs_cancel_all_bookings(t->utente->matricola);
        } else {
		if ((t->text_com == TXT_MAIL) &&
		    (trova_utente_n(t->rcpt[0]) == NULL)) {
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
                        fs_cancel_all_bookings(t->utente->matricola);
		} else if (save_post(t, txt)) {
			/* Notifica Nuovo Post */
			notifica_post(t);
		     	cprintf(t, "%d Post inviato.\n", OK);
		} else {
			cprintf(t, "%d Problema File Messaggi.\n",
				ERROR + NO_SUCH_FM);
                        fs_cancel_all_bookings(t->utente->matricola);
                }
	}
        t->num_rcpt = 0;
	t->room->lock = false;
	t->rlock_timeout = 0;
        t->upload_bookings = 0;
	reset_text(t);
}

static void notifica_post(struct sessione *t)
{
	struct sessione *punto;
	char nm[MAXLEN_UTNAME];

	if (t->room->data->flags & RF_MAIL)
		return; /* Non notifica in mail */

	if ((t->room->data->flags & RF_ANONYMOUS)
	    || (t->room->data->flags & RF_ASKANONYM))
		return; /* Non notifica in room anonime */

	if ((t->room->data->flags & RF_ANONYMOUS)
	    || (t->room->data->flags & RF_ASKANONYM))
		strcpy(nm, ""); /* Non invia il nome se room anonima */
	else
		strncpy(nm, t->utente->nome, MAXLEN_UTNAME);

	if (t->room->data->flags & RF_BLOG) {
                for (punto = lista_sessioni; punto; punto = punto->prossima) {
                        if ((punto != t) && (punto->utente)
                            && (!strcmp(punto->utente->nome,
                                       t->room->data->name))) {
                                cprintf(punto, "%d %s\n", NBLG, nm);
                                return;
                        }
                }
                return;
        }

	for (punto = lista_sessioni; punto; punto = punto->prossima)
		if ((punto != t) && (punto->utente)
		    && (punto->room == t->room)) {
			if (punto->text_com)
				cprintf(punto, "%d %s\n", NPMS, nm);
			else if (punto->utente->flags[4] & UT_NPOST)
				cprintf(punto, "%d %s\n", NPST, nm);
		}
}

void notifica_cittapost(struct room *r)
{
	struct sessione *punto;

	if (r->data->flags & RF_MAIL)
		return; /* Non notifica in mail */

	for (punto = lista_sessioni; punto; punto = punto->prossima)
		if ((punto->utente) && (punto->room == r))
                        cprintf(punto, "%d Cittadella BBS\n", NPST);
}
