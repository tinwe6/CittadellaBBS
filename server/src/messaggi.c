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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : messaggi.c                                                         *
*        Comandi Server/Client per leggere, scrivere, cancellare i post.    *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "cittaserver.h"
#include "messaggi.h"
#include "blog.h"
#include "extract.h"
#include "file_messaggi.h"
#include "find.h"
#include "mail.h"
#include "memstat.h"
#include "msg_flags.h"
#include "post.h"
#include "rooms.h"
#include "string_utils.h"
#include "utility.h"
#include "x.h"

#ifdef USE_STRING_TEXT
# include "argz.h"
#endif

#ifdef USE_CACHE_POST
#include "cache_post.h"
#endif

/* Informazioni sull'ultimo post inserito nel file dei messaggi           */
long lastpost_author;  /* Autore dell'ultimo post                         */
long lastpost_webroom; /* True se la room dell'ultimo post e' su CittaWeb */
long lastpost_locnum;  /* Local number dell'ultimo post                   */
char lastpost_room[ROOMNAMELEN]; /* Nome room in cui e' stato lasciato    */
time_t lastpost_time;            /* Data e ora dell'ultimo post           */

/* Prototipi funzioni in questo file */
int save_post(struct sessione *t, char *arg);
void cmd_read(struct sessione *t, char *cmd);
bool new_msg(struct sessione *t, struct room *r);
void cmd_mdel(struct sessione *t, char *arg);
void cmd_mmov(struct sessione *t, char *arg);
void cmd_mcpy(struct sessione *t, char *arg);
int citta_post(struct room *room, char *subject, struct text *txt);
static int msg_send(struct sessione *t, struct room *room, int i, bool reply);
static void sysop_mail(int mode, long msgnum, long msgpos);
int citta_mail(struct dati_ut *ut, char *subject, struct text *txt);
int send_mail(struct dati_ut *ut, struct dati_ut *author, char *subject,
	      struct text *txt);

/****************************************************************************
****************************************************************************/
/*
 * Salva il post nel file messaggi e aggiorna le strutture dei messaggi
 */
int save_post(struct sessione *t, char *arg)
{
	char header[2*LBUF], subject[LBUF], anonick[LBUF], *autore;
	long fmnum, msgnum, msgpos, rcpt, i, flags = 0, matricola;
	bool reply = false;
	int sysm = 0;
        size_t hdrlen;
	struct room *room;
	struct sessione *s;
	time_t ora;
#ifdef USE_STRING_TEXT
	size_t len;
	char *str, *ptr, *strtxt, hdr[2*LBUF];
#endif
	struct dati_ut *ut;
	char destinatario[MAXLEN_UTNAME];

	room = t->room;
	fmnum = room->data->fmnum;
	/* TODO se fmnum -1 allora errore! */

#ifdef USE_STRING_TEXT
	/* Traduce il testo in una stringa  */
	len = 0;
	txt_rewind(t->text);
	while ( (str = txt_get(t->text)))
		len += strlen(str) + 1;
	strtxt = Malloc(len, TYPE_CHAR);
	if (strtxt) {
		txt_rewind(t->text);
		str = txt_get(t->text);
		for (ptr = strtxt; str; str = txt_get(t->text), ptr++)
			ptr = citta_stpcpy(ptr, str);
	} else
		return 0;
#endif

	/* Genera l'header
	 * Header: 0 - Room Name; 1 - Author;    2 - Date;     3 - Subject;
	 *         4 - Flags;     5 - Recipient; 6 - Reply to; 7 - Reply lnum;
         *         8 - Nickname
	 */
        autore = t->utente->nome;
        matricola = t->utente->matricola;
        lastpost_author = matricola;
        anonick[0] = 0;
        if ((room->data->flags & (RF_ANONYMOUS | RF_ASKANONYM))
            && (!(t->utente->sflags[1] & SUT_NOANONYM))
            && extract_int(arg,1)) {
                flags |= MF_ANONYMOUS;
                if (room->data->flags & RF_ASKANONYM)
                        extract(anonick, arg, 6);
                lastpost_author = -1;
        }

	if ((room->data->flags & RF_REPLY) && extract_int(arg, 2)) {
                flags |= MF_REPLY;
		reply = true;
        }
	if ((room->data->flags & RF_SPOILER) && extract_int(arg, 5))
                flags |= MF_SPOILER;

        /* TODO: ha senso una struttura piu' fine con MF_IMAGE, MF_VIDEO,.. ? */
        if (t->upload_bookings)
                flags |= MF_FILE;

        subject[0] = 0;
        if (room->data->flags & RF_SUBJECT) {
		if (reply)
			strcpy(subject, t->reply_subj);
		else
			extractn(subject, arg, 0, MAXLEN_SUBJECT);
	}

	IF_MAILROOM(t)
		flags |= MF_MAIL;

        /* Solo se e' una room normale della BBS */
        /* TODO sistemare il type */
        if (t->room->data->type != ROOM_DYNAMICAL) {
                switch (extract_int(arg, 3)) {
                case 1:
                        if (t->room_flags[t->room->pos] & UTR_ROOMAIDE)
                                flags |= MF_RA;
                        else
                                flags |= MF_RH;
                case 2:
                        flags |= MF_AIDE;
                        break;
                case 3:
                        flags |= MF_SYSOP;
                        break;
                }
        }

	time(&ora);
	hdrlen = sprintf(header, "%s|%s|%ld|", room->data->name, autore, ora);
	if ((room->data->flags & RF_SUBJECT) && (strlen(subject) > 0))
		hdrlen += sprintf(header+hdrlen, "%s", subject);
        hdrlen += sprintf(header+hdrlen, "|%ld|", flags);

        /*
         * Mette il nome giusto in destinatario (lo usa se
         * t->rcpt[0] e` NULL)
         */
	if (room->data->flags & RF_MAIL) {
                if ( (ut = trova_utente_n(t->rcpt[0])))
                        hdrlen += sprintf(header+hdrlen, "%s", ut->nome);
                else {
                        strncpy(destinatario, "***", MAXLEN_UTNAME);
                        switch (t->text_com) {
                        case TXT_MAILSYSOP:
                                strncpy(destinatario, "Sysop", MAXLEN_UTNAME);
                                break;
                        case TXT_MAILAIDE:
                                strncpy(destinatario, "Aide", MAXLEN_UTNAME);
                                break;
                        case TXT_MAILRAIDE:
                                strncpy(destinatario, "Room Aide", MAXLEN_UTNAME);
                                break;
                        }
                        hdrlen += sprintf(header+hdrlen, "%s", destinatario);
                }
	}

        if (reply)
		hdrlen += sprintf(header+hdrlen, "|%s|%ld", t->reply_to,
			t->reply_num);
	else
		hdrlen += sprintf(header+hdrlen, "||");
        hdrlen += sprintf(header+hdrlen, "|%s", anonick);

	/* Invia il post al file messaggi */
#ifdef USE_STRING_TEXT
	msgnum = fm_put(fmnum, strtxt, len, header, &msgpos);
#else
	msgnum = fm_put(fmnum, t->text, header, &msgpos);
#endif
	if (msgnum) {
		/* Aggiorna le strutture dei post */
		if (room->data->flags & RF_MAIL) {
			if (t->utente->flags[5] & UT_MYMAILCC)
				mail_add(t->utente->matricola, msgnum, msgpos);
			dati_server.mails++;
			t->utente->mail++;
			sysm = extract_int(arg, 4);
			if (sysm == 0) {
				rcpt = trova_utente_n(t->rcpt[0])->matricola;
				mail_add(rcpt, msgnum, msgpos);
				/* Notifica il nuovo mail al rcpt */
				if ( (s = collegato_n(rcpt))
				    && (s->utente->flags[3] & UT_NMAIL))
					cprintf(s, "%d\n", NNML);
			} else
				sysop_mail(sysm, msgnum, msgpos);
		} else {
			i = room->data->maxmsg - 1;
			msg_scroll(room->msg, room->data->maxmsg);
			room->msg->num[i] = msgnum;
			room->msg->pos[i] = msgpos;
			room->msg->local[i] = room->msg->highloc;
                        room->msg->last_poster = matricola;
                        if (room->data->flags & RF_BLOG)
                                dati_server.blog++;
                        else
                                dati_server.posts++;
			t->utente->post++;
                        lastpost_locnum = room->msg->highloc;
#ifdef USE_FIND
                        /* Se la room lo prevede, indicizza il messaggio */
			if (t->room->data->flags & RF_FIND)
			        find_insert(t->room, i);
			/* TODO sostituire con un insert che usa i dati
			   che sono gia' disponibili (msgnum, msgpos, etc..) */
#endif
		}
		inc_highloc(t);
                set_highmsg(t, msgnum);
		time(&(room->data->ptime));
                t->last_msgnum = msgnum;
#ifdef USE_FLOORS
		floor_post(room->floor_num, msgnum, room->data->ptime);
#endif
#ifdef USE_STRING_TEXT
		/* Inserisce la stringa nella cache */
		sprintf(hdr, "%ld|%s", msgnum, header);
		cp_put(fmnum, msgnum, msgpos, strtxt, len, hdr);
#endif
                /* Aggiorna i dati sull'ultimo post immesso */
                if (room->data->flags & RF_BLOG)
                        strcpy(lastpost_room, "Blog");
                else
                        strcpy(lastpost_room, room->data->name);
                lastpost_webroom = room->data->flags & RF_CITTAWEB;
                lastpost_time = room->data->ptime;
	}
	return msgnum;
}

/*
 * Legge un messaggio dalla room corrente.
 * Sintassi: "READ mode|N"; { N optional for mode = 6, 7 }
 *           mode: 0 - Forward; 1 - New; 2 - Reverse; 3 - Brief;
 *                 5 - Last 5; 6 - Last N; 7 - from msg #N;
 *                10 - Next;   11 - Prev; 12 - Back;
 * Restituisce prima l'OK con local number e il numero di messaggi rimanenti,
 * poi il LISTING_FOLLOWS con l'header del messaggio ed infine il testo del
 * messaggio terminato con un "000".
 */
void cmd_read(struct sessione *t, char *cmd)
{
        long fmnum, msgnum, lowest, i, fullroom;
	int mode, n = 0;
	struct room *room;

	mode = extract_int(cmd, 0);
	room = t->room;

	PUT_USERMAIL(t);

        /* NON USO FM MULTIPLI
	fmnum_def = room->data->fmnum;
	if (fmnum_def != -1) {
	        fmnum = fmnum_def;
		lowest = fm_lowest(fmnum);
	} else
	        lowest = 0;
        */
	fmnum = room->data->fmnum;
        lowest = fm_lowest(fmnum);

	switch (mode) {
	 case 0: /* Forward */
		t->rdir = 1;
		i = 0L;
		while ((i < room->data->maxmsg) && (room->msg->num[i] < lowest))
			i++;
		break;
	 case 1: /* New */
		t->rdir = 1;
		i = 0L;
		if (room->data->type == ROOM_DYNAMICAL) {
                        if (room->data->flags & RF_BLOG)
                                fullroom = dfr_getfullroom(room->msg->dfr,
                                               t->utente->matricola);
                        else
                                break;
                } else
                        fullroom = t->fullroom[room->pos];
		if (t->utente->flags[4] & UT_LASTOLD)
			while ((i < room->data->maxmsg)
                               && ((room->msg->num[i] < fullroom)
                                   || (room->msg->num[i] < lowest)))
                                i++;
                else
			while ((i < room->data->maxmsg)
                               && ((room->msg->num[i] <= fullroom)
                                   || (room->msg->num[i] < lowest)))
                                i++;
		break;
	 case 2: /* Reverse */
		t->rdir = -1;
		i = room->data->maxmsg - 1;
		if (room->msg->num[i] < lowest)
			i++;
		break;
	 case 5: /* Last Five */
		n = 5;
	 case 6: /* Last N */
		t->rdir = 1;
		if (!n)
			n = extract_int(cmd, 1);
			if(n==0){
					i=-1;
					break;
			}
		if ( (i = room->data->maxmsg - n) < 0)
			i = 0;
		while ((i < room->data->maxmsg) && (room->msg->num[i] < lowest))
			i++;
		break;
	 case 7: /* msg # N */
		t->rdir = 1;
		n = extract_int(cmd, 1);
		i = 0;
		while ((i < room->data->maxmsg) && (room->msg->local[i] != n))
			i++;
		break;
	 case 11: /* Back */
		t->rdir *= -1;
	 case 10: /* Next */
		if (t->rdir > 0) {
			i = 0;
			while((i < room->data->maxmsg) &&
			      (room->msg->num[i] <= t->lastread))
			        i++;
		} else {
			i = room->data->maxmsg - 1;
			while((room->msg->num[i] >= t->lastread) && (i >= 0))
				i--;
			if (room->msg->num[i] < lowest)
				i = -1;
		}
		break;
	 case 12: /* Again */
		i = 0;
		while ((i < room->data->maxmsg)
                       && (room->msg->num[i] != t->lastread))
			i++;
		break;
	}
	if ((i < 0) || (i >= room->data->maxmsg)) {
		cprintf(t, "%d\n", ERROR);
	} else {
                if ( (msgnum = msg_send(t, room, i, true)) > 0) {
                        if (msgnum > fullroom) {
                                if (room->data->type != ROOM_DYNAMICAL)
                                        t->fullroom[room->pos] = msgnum;
                                else if (room->data->flags & RF_BLOG)
                                        dfr_setfullroom(room->msg->dfr,
                                                t->utente->matricola, msgnum);
                        }
                        t->lastread = msgnum;
                }
	}
	REMOVE_USERMAIL(t);
}

/* Read MeSsaGe: "RMSG Room|locnum" */
/* TODO test e errori in comune con RMSG, unificare! */
void cmd_rmsg(struct sessione *t, char *cmd)
{
        long locnum, i;
	char roomname[LBUF];
	struct room *room;

        extractn(roomname, cmd, 0, ROOMNAMELEN);
        locnum = extract_long(cmd, 1);
	room = room_find(roomname);

        if (room == NULL) {
                cprintf(t, "%d No such room\n", ERROR+NO_SUCH_ROOM);
                return;
        }
        if (room_known(t, room) == 0) {
                cprintf(t, "%d Non hai accesso alla room\n",
                        ERROR+ACC_PROIBITO);
                return;
        }
        if (room->data->flags & RF_MAIL) {
                cprintf(t, "%d Vietato nella mailroom\n", ERROR+ARG_NON_VAL);
                return;
        }
        for (i = 0; i < room->data->maxmsg; i++) {
                if (room->msg->local[i] == locnum) {
                        msg_send(t, room, i, false);
                        return;
                }
        }
        cprintf(t, "%d Messaggio inesistente\n", ERROR);
}


/* Test MeSsaGe: "TMSG Room|locnum"
 * Restituisce OK se il messaggio e' disponibile e accessibile all'utente,
 * altrimenti invia ERROR. */
void cmd_tmsg(struct sessione *t, char *cmd)
{
        long locnum, i;
	char roomname[LBUF];
	struct room *room;

        extractn(roomname, cmd, 0, ROOMNAMELEN);
        locnum = extract_long(cmd, 1);
	room = room_find(roomname);

        if (room == NULL) {
                cprintf(t, "%d No such room\n", ERROR+NO_SUCH_ROOM);
                return;
        }
        if (room_known(t, room) == 0) {
                cprintf(t, "%d Non hai accesso alla room\n",
                        ERROR+ACC_PROIBITO);
                return;
        }
        if (room->data->flags & RF_MAIL) {
                cprintf(t, "%d Vietato nella mailroom\n", ERROR+ARG_NON_VAL);
                return;
        }

        for (i = 0; i < room->data->maxmsg; i++) {
                if (room->msg->local[i] == locnum) {
                        cprintf(t, "%d Messaggio accessibile\n", OK);
                        return;
                }
        }
        cprintf(t, "%d Messaggio inesistente\n", ERROR);
}

/* Invia alla sessione t il messaggio nella slot i della room room
 * Restituisce TRUE se il messaggio e' stato inviato, FALSE altrimenti.
 * Se reply e' TRUE, aggiorna i dati per il reply */
static int msg_send(struct sessione *t, struct room *room, int i, bool reply)
{
        long msgnum, msgpos, fmnum;
        int err, ret;
        char header[LBUF], *riga;
#ifdef USE_STRING_TEXT
	char *txt;
	size_t len;
	int nrighe;
#else
	struct text *txt;
#endif
#ifdef USE_CACHE_POST
	char *header1;
# ifndef USE_STRING_TEXT
	struct text *txt1;
# endif
#endif

        msgnum = room->msg->num[i];
        msgpos = room->msg->pos[i];
	fmnum = room->data->fmnum;
        /* NON USO FM MULTIPLI
	if (fmnum == -1)
                fmnum = room->msg->fmnum[i];
        */

        cprintf(t, "%d %ld|%ld\n", OK, room->msg->local[i],
                room->data->maxmsg - 1 - i);
#ifdef USE_CACHE_POST
# ifdef USE_STRING_TEXT
        err = cp_get(fmnum, msgnum, msgpos, &txt, &len, &header1);
# else
        err = cp_get(fmnum, msgnum, msgpos, &txt1, &header1);
        txt = txt1;
# endif
#else
        txt = txt_create();
        err = fm_get(fmnum, msgnum, msgpos, txt, header);
#endif
        if (err == 0) {
#ifdef USE_CACHE_POST
# ifdef USE_STRING_TEXT
                nrighe = argz_count(txt, len);
# endif
                strcpy(header, header1);
#endif
                /* Invia l'header
                 * Header: 0 - Room Name; 1 - Author; 2 - Date;
                 *         3 - Subject; 4 - Flags; 5 - Recipient;
                 *         6 - Reply to; 7 - Reply lnum;
                 *         8 - Nickname
                 */

                if (extract_long(header, 5) & MF_ANONYMOUS)
                        clear_parm(header, 2);

                /* Questo test e' per compatibilita'... da togliere
                   quando avremo fatto il giro del fullroom.       */
                if (num_parms(header) > 8) {
#ifdef USE_STRING_TEXT
                        cprintf(t, "%d %s|%d\n", SEGUE_LISTA, header,
                                nrighe);
#else
                        cprintf(t, "%d %s|%ld\n", SEGUE_LISTA, header,
                                txt_len(txt));
#endif
                } else {
#ifdef USE_STRING_TEXT
                        cprintf(t, "%d %s||%d\n", SEGUE_LISTA,
                                header, nrighe);
#else
                        cprintf(t, "%d %s||%ld\n", SEGUE_LISTA,
                                header, txt_len(txt));
#endif
                }
                if (reply) {
                        extract(t->reply_to, header, 2);
                        if (t->reply_to[0] == 0)
                                extract(t->reply_to, header, 9);
                        extract(t->reply_subj, header, 4);
                        t->reply_num = room->msg->local[i];
                }
#ifdef USE_STRING_TEXT
                for (riga = txt; riga;
                     riga = argz_next(txt, len, riga))
                        cprintf(t, "%d %s\n", OK, riga);
#else
                txt_rewind(txt);
                while (riga = txt_get(txt), riga)
                        cprintf(t, "%d %s\n", OK, riga);
#endif
                cprintf(t, "000\n");
                ret = msgnum;
        } else {
                cprintf(t, "%d %d|%ld|%ld|%ld\n", ERROR, err, fmnum,
                        msgnum, msgpos);
                ret = 0;
        }
#ifndef USE_CACHE_POST
        txt_free(&txt);
#endif
        return ret;
}

/*
 * Restituisce true se ci sono nuovi messaggi in r per t, altrimenti false.
 * Se la room r e' dinamica, restituisce false
 */
bool new_msg(struct sessione *t, struct room *r)
{
        if (r->data->type != ROOM_DYNAMICAL)
                return t->fullroom[r->pos] < get_highmsg_room(t,r);
        return false;
}

/*
 * Numero nuovi messaggi
 */
long new_msg_num(struct sessione *t, struct room *r)
{
	long i, nmsg, maxmsg;

	nmsg = 0;
	maxmsg = r->data->maxmsg;
	for (i = 0; i < maxmsg; i++)
		if (r->msg->num[i] > t->fullroom[r->pos])
			nmsg++;
	return nmsg;
}

/*
 * Cancella messaggio corrente (se in Cestino -> undelete)
 */
void cmd_mdel(struct sessione *t, char *arg)
{
        char header[LBUF], author[LBUF], roomname[LBUF];
	long msgnum, msgpos, locnum, i;
	struct room *room;
	struct dati_ut *ut;
	struct text *txt;

	IGNORE_UNUSED_PARAMETER(arg);

	/* Verifica che il messaggio esiste e estrae l'autore. */
	PUT_USERMAIL(t);
	msgnum = t->lastread;
	for (i = 0; i < t->room->data->maxmsg; i++)
		if (t->room->msg->num[i] == msgnum) {
			msgpos = t->room->msg->pos[i];
			locnum = t->room->msg->local[i];
			i = t->room->data->maxmsg;
		}
	/*
          msgpos = msg_pos(t->room, msgnum);
         */
	if (msgpos < 0L) {
		cprintf(t, "%d\n", ERROR + NO_SUCH_MSG);
		REMOVE_USERMAIL(t);
		return;
	}

	fm_getheader(t->room->data->fmnum, msgnum, msgpos, header);
	extract(author, header, 2);

#ifdef USE_CESTINO
	/* Se sono nel cestino, eseguo un undelete. */
	if (!strcmp(t->room->data->name, dump_room)) {
		if (msg_undelete(t->room, t->lastread, roomname)) {
			cprintf(t, "%d\n", OK);
			/* Notifica in Aide */
			if ((room = room_find(aide_room))) {
				txt = txt_create();
				if (extract_long(header, 5) & MF_ANONYMOUS)
				        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>Un Utente Anonimo</b> in <b>%s</b> riammesso da <b>%s</b>.",
						 locnum, roomname,
						 t->utente->nome);
				else
				        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>%s</b> in <b>%s</b> riammesso da <b>%s</b>.",
						 locnum, author, roomname,
						 t->utente->nome);
				citta_post(room,"Riesumazione messaggio",txt);
				txt_free(&txt);
			}
#ifdef USE_NOTIFICA_UNDEL
			/* Notifica in mail all'utente */
			if ( (ut = trova_utente(author))) {
				txt = txt_create();
				txt_putf(txt, "Il tuo post #<b>%ld</b> &egrave; stato riammesso in <b>%s</b>.", locnum, roomname);
				citta_mail(ut, "Riesumazione messaggio.",txt);
				txt_free(&txt);
			}
#endif
		} else
			cprintf(t, "%d\n", ERROR);
		return;
	}
#endif /* USE_CESTINO */

	/* E` un mail oppure l'utente e` l'autore del post: esegui il delete */
	if ((t->room->data->flags & RF_MAIL)
	    || (!strcmp(t->utente->nome, author))) {
		msg_del(t->room, msgnum);
		cprintf(t, "%d\n", OK);
		REMOVE_USERMAIL(t);
		return;
	}

	/* Solo i Sysop possono cancellare i post di servizio */
	if (!strcmp(author, "Cittadella BBS")
	    && (t->utente->livello < LVL_SYSOP)) {
		cprintf(t, "%d\n", ERROR + ACC_PROIBITO);
		return;
	}

	/* Sposta il messaggio nel cestino */
	if ((t->utente->livello >= LVL_AIDE)
	    || ((t->utente->livello >= LVL_ROOMAIDE) &&
		(t->utente->matricola == t->room->data->master))
	    || (t->room_flags[t->room->pos] & UTR_ROOMHELPER)) {
#ifdef USE_CESTINO
		msg_dump(t->room, msgnum);
#else
		msg_del(t->room, msgnum);
#endif
		cprintf(t, "%d\n", OK);
		/* Notifica in Aide */
		if ( (room = room_find(aide_room))) {
			txt = txt_create();
			if (extract_long(header, 5) & MF_ANONYMOUS)
			        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>%s</b> in <b>Un Utente Anonimo</b> cancellato da <b>%s</b>.",
					 locnum, t->room->data->name,
					 t->utente->nome);
			else
			        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>%s</b> in <b>%s</b> cancellato da <b>%s</b>.",
					 locnum, author, t->room->data->name,
					 t->utente->nome);
			citta_post(room,"Cancellazione messaggio",txt);
			txt_free(&txt);
		}
#ifdef USE_NOTIFICA_DEL
		/* Notifica in mail all'utente */
		if ( (ut = trova_utente(author))) {
			txt = txt_create();
			txt_putf(txt, "Il tuo post #<b>%ld</b> in <b>%s</b> &egrave; stato cancellato.",
				 locnum, t->room->data->name);
			citta_mail(ut, "Cancellazione messaggio.", txt);
			txt_free(&txt);
		}
#endif
	} else
		cprintf(t, "%d\n", ERROR + ACC_PROIBITO);
}

/*
 * Sposta messaggio corrente.
 */
void cmd_mmov(struct sessione *t, char *arg)
{
	char header[LBUF], author[LBUF], rn[LBUF];
	long msgnum, msgpos, locnum;
	long i;
	struct room *room;
	struct dati_ut *ut;
	struct text *txt;

        /* Move accessibile solo da room normali */
        if (t->room->data->type == ROOM_DYNAMICAL) {
		cprintf(t, "%d\n", ERROR);
		return;
	}

	if ((t->utente->livello < MINLVL_MOVE) && (!IS_RH(t))) {
		cprintf(t, "%d\n", ERROR);
		return;
	}
	if (!strcmp(t->room->data->name, dump_room)) {
		cprintf(t, "%d\n", ERROR);
		return;
	}
	msgnum = t->lastread;
	for (i = 0; i < t->room->data->maxmsg; i++)
		if (t->room->msg->num[i] == msgnum) {
			msgpos = t->room->msg->pos[i];
			locnum = t->room->msg->local[i];
			i = t->room->data->maxmsg;
		}
	/*
          msgpos = msg_pos(t->room, msgnum);
        */
	if (msgpos < 0L) {
		cprintf(t, "%d\n", ERROR+NO_SUCH_MSG);
		return;
	}
	extract(rn, arg, 0);
	room = room_find(rn);
	if (room == NULL) {
		cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
		return;
	} else if (room->data->flags & RF_MAIL) { /* No move in Mail) */
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		return;
	}
	fm_getheader(t->room->data->fmnum, msgnum, msgpos, header);
	extract(author, header, 2);
	if (t->utente
	    && ((t->utente->livello >= LVL_AIDE)
		|| (t->utente->livello >= LVL_ROOMAIDE &&
		    t->utente->matricola == t->room->data->master)
		|| (t->room_flags[t->room->pos] & UTR_ROOMHELPER))
            /* || (!strcmp(t->utente->nome, author)) */
	    && (t->room->data->fmnum == room->data->fmnum)
	    && (strcmp(author, "Cittadella BBS"))) {
		msg_move(room, t->room, msgnum);
		cprintf(t, "%d\n", OK);
		if (strcmp(t->utente->nome, author)) {
			/* Notifica in Aide */
			if ((room = room_find(aide_room))) {
			        txt = txt_create();
				if (extract_long(header, 5) & MF_ANONYMOUS)
				        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>Un Utente Anonimo</b> spostato da <b>%s</b> in <b>%s</b> da <b>%s</b>.",
						 locnum, t->room->data->name,
						 rn, t->utente->nome);
				else
				        txt_putf(txt, "Messaggio #<b>%ld</b> di <b>%s</b> spostato da <b>%s</b> in <b>%s</b> da <b>%s</b>.",
						 locnum, author,
						 t->room->data->name,
						 rn, t->utente->nome);
				citta_post(room, "Spostamento messaggio", txt);
				txt_free(&txt);
			}
#ifdef USE_NOTIFICA_MOVE
			/* Notifica in mail all'utente */
			if ( (ut = trova_utente(author))) {
				txt = txt_create();
				txt_putf(txt, "Il tuo post #<b>%ld</b> &egrave; stato spostato da <b>%s</b> in <b>%s</b>.",
					 locnum, t->room->data->name, rn);
				citta_mail(ut, "Spostamento messaggio.", txt);
				txt_free(&txt);
			}
#endif
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Copia messaggio corrente.
 */
void cmd_mcpy(struct sessione *t, char *arg)
{
	char buf[LBUF], author[LBUF], rn[LBUF];
	long msgnum, msgpos;
	struct room *room;

        /* Copy accessibile solo da room normali */
        if (t->room->data->type == ROOM_DYNAMICAL) {
		cprintf(t, "%d\n", ERROR);
		return;
	}

	if ((t->utente == NULL) || (t->utente->livello < MINLVL_COPY)) {
		cprintf(t, "%d\n", ERROR);
		return;
	}

	msgnum = t->lastread;
	msgpos = msg_pos(t->room, msgnum);
	if (msgpos < 0L) {
		cprintf(t, "%d\n", ERROR+NO_SUCH_MSG);
		return;
	}
	extract(rn, arg, 0);
	room = room_find(rn);
	if (room == NULL) {
		cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
		return;
	} else if (room->data->flags & RF_MAIL) { /* No copy in Mail) */
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		return;
	}
	fm_getheader(t->room->data->fmnum, msgnum, msgpos, buf);
	extract(author, buf, 2);
	if (t->utente
	    && ((t->utente->livello >= LVL_AIDE)
		|| (t->utente->livello >= LVL_ROOMAIDE &&
		    t->utente->matricola == t->room->data->master)
		|| (t->room_flags[t->room->pos] & UTR_ROOMHELPER))
/*		|| (!strcmp(t->utente->nome, author)) */
	    && (t->room->data->fmnum == room->data->fmnum)) {
		msg_copy(room, t->room, msgnum);
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Post di amministrazione postato da cittadella.
 */
int citta_post(struct room *room, char *subject, struct text *txt)
{
	char header[LBUF];
	long fmnum, msgnum, msgpos, i, flags = 0;
	time_t ora;
#ifdef USE_STRING_TEXT
	char *strtxt;
	size_t len;
#endif

	fmnum = room->data->fmnum;
	flags |= MF_CITTA;
	time(&ora);
	sprintf(header, "%s|Cittadella BBS|%ld|%s|%ld||||", room->data->name,
		ora, subject, flags);
#ifdef USE_STRING_TEXT
	len = text2string(txt, &strtxt);
	if (len > 0) {
		msgnum = fm_put(fmnum, strtxt, len, header, &msgpos);
		Free(strtxt);
	} else
		msgnum = 0;
#else
	msgnum = fm_put(fmnum, txt, header, &msgpos);
#endif
	if (msgnum) {
		i = room->data->maxmsg - 1;
		msg_scroll(room->msg, room->data->maxmsg);
		room->msg->num[i] = msgnum;
		room->msg->pos[i] = msgpos;
		room->msg->local[i] = (room->msg->highloc)++;
		room->msg->highmsg = msgnum;
		time(&(room->data->ptime));
		(dati_server.posts)++;
#ifdef USE_FLOORS
		floor_post(room->floor_num, msgnum, room->data->ptime);
#endif
                notifica_cittapost(room);
	}
	return msgnum;
}

/*
 * Invia un post nella room apposita degli amministratori.
 * mode: 1 Sysop, 2 Aide, 3 Room Aide.
 */
static void sysop_mail(int mode, long msgnum, long msgpos)
{
	struct room * room = NULL;
	long i;

	switch(mode) {
	 case 1:
		room = room_find(sysop_room);
		break;
	 case 2:
		room = room_find(aide_room);
		break;
	 case 3:
		room = room_find(ra_room);
		break;
	}
	if (!room)
		return;
	i = room->data->maxmsg - 1;
	msg_scroll(room->msg, room->data->maxmsg);
	room->msg->num[i] = msgnum;
	room->msg->pos[i] = msgpos;
	room->msg->local[i] = (room->msg->highloc)++;
	room->msg->highmsg = msgnum;
	time(&(room->data->ptime));
#ifdef USE_FLOORS
		floor_post(room->floor_num, msgnum, room->data->ptime);
#endif
}

/*
 * Invia un mail di amministrazione all'utente 'ut', con subject 'subject'
 * e testo 'txt'
 */
int citta_mail(struct dati_ut *ut, char *subject, struct text *txt)
{
	return send_mail(ut, NULL, subject, txt);
}

/*
 * Invia un mail all'utente 'ut', con subject 'subject' e testo 'txt'
 * da parte di author.
 */
int send_mail(struct dati_ut *ut, struct dati_ut *author, char *subject,
	      struct text *txt)
{
        struct room *room;
	char header[LBUF];
	long fmnum, msgnum, msgpos, flags = 0;
	time_t ora;
	struct sessione *s;
#ifdef USE_STRING_TEXT
	char *strtxt;
	size_t len;
#endif

	room = room_find(mail_room);
	fmnum = room->data->fmnum;
	time(&ora);
	flags |= MF_MAIL;
	if (author == NULL) {
		flags |= MF_CITTA;
		sprintf(header, "%s|Cittadella BBS|%ld|%s|%ld|%s||",
			room->data->name, ora, subject, flags, ut->nome);
	} else
		sprintf(header, "%s|%s|%ld|%s|%ld|%s||", room->data->name,
			author->nome, ora, subject, flags, ut->nome);
#ifdef USE_STRING_TEXT
	len = text2string(txt, &strtxt);
	if (len > 0) {
		msgnum = fm_put(fmnum, strtxt, len, header, &msgpos);
		Free(strtxt);
	} else
		msgnum = 0;
#else
	msgnum = fm_put(fmnum, txt, header, &msgpos);
#endif
	if (msgnum) {
	        mail_add(ut->matricola, msgnum, msgpos);
		if (author)
			mail_add(author->matricola, msgnum, msgpos);
		room->data->highmsg = msgnum;
		(room->data->highloc)++;
		time(&(room->data->ptime));
		(dati_server.mails)++;
		if ((s = collegato_n(ut->matricola))
		    && (s->utente->flags[3] & UT_NMAIL))
		        cprintf(s, "%d\n", NNML);
#ifdef USE_FLOORS
		floor_post(room->floor_num, msgnum, room->data->ptime);
#endif
	}
	return msgnum;
}
