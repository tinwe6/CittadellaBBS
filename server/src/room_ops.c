/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File : room_ops.c                                                         *
*        Operazioni sulle room della BBS.                                   *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "cittaserver.h"
#include "messaggi.h"
#include "room_ops.h"
#include "blog.h"
#include "extract.h"
#include "find.h"
#include "mail.h"
#include "text.h"
#include "utente.h"
#include "ut_rooms.h"
#include "utility.h"

extern char lobby[];
extern char sysop_room[];
extern char aide_room[];
extern char ra_room[];
extern char mail_room[];
extern char twit_room[];
extern char dump_room[];

/* Prototipi delle funzioni in questo file */
char room_type(struct room *room);
void cmd_rdel(struct sessione *t, char *cmd);
void cmd_redt(struct sessione *t, char *cmd);
void cmd_rlst(struct sessione *t, char *cmd);
void cmd_rkrl(struct sessione *t, char *cmd);
void cmd_rnew(struct sessione *t, char *cmd);
void cmd_rinf(struct sessione *t);
void cmd_rzap(struct sessione *t);
void cmd_rzpa(struct sessione *t);
void cmd_ruzp(struct sessione *t);
void cmd_raid(struct sessione *t, char *arg);
void cmd_rinv(struct sessione *t, char *buf);
void cmd_rinw(struct sessione *t);
void cmd_rirq(struct sessione *t, char *buf);
void cmd_rkob(struct sessione *t, char *buf);
void cmd_rkoe(struct sessione *t, char *buf);
void cmd_rkow(struct sessione *t);
void cmd_rnrh(struct sessione *t, char *buf);
void cmd_rswp(struct sessione *t, char *cmd);
void cmd_rlen(struct sessione *t, char *cmd);
void cmd_rall(struct sessione *t);

/***************************************************************************/
/*
 * Restituisce il carattere corrispondente al tipo di room.
 * TODO: Questo va delegato al client.
 */
char room_type(struct room *room)
{
        if (room->data->flags & RF_GUESS)
                return '?';
        switch (room->data->type) {
        case ROOM_BASIC:
                return ')';
        case ROOM_NORMAL:
                return '>';
        case ROOM_DIRECTORY:
                return '}';
        case ROOM_GUESS:
                return '?';
        case ROOM_FLOOR:
                return ']';
        case ROOM_DYNAMICAL:
                return ':';
	}
	return ' ';
}

/*
 * Invia la lista delle room con la loro configurazione.
 */
void cmd_rlst(struct sessione *t, char *cmd)
{
	char buf[LBUF];
        struct room *punto;
	struct dati_ut *ra;

	cprintf(t, "%d Lista Room (RLST)\n", SEGUE_LISTA);

	for (punto = lista_room.first; punto; punto = punto->next) {
		if (t->utente->livello >= punto->data->rlvl) {
			ra = trova_utente_n(punto->data->master);
			if (ra)
				strcpy(buf, ra->nome);
			else
				strcpy(buf, "");
			cprintf(t, "%d %ld|%s|%c|%ld|%ld|%ld|%ld|%ld|%ld|%d|%d|%s\n", OK,
				punto->data->num, punto->data->name,
				room_type(punto), punto->data->fmnum,
				punto->data->maxmsg, punto->data->ctime,
				punto->data->ptime, punto->data->flags,
				get_highloc_room(t, punto), punto->data->rlvl,
				punto->data->wlvl, buf);
		}
	}
	cprintf(t, "000\n");
}

/*
 * Invia la lista delle room conosciute.
 * Sintassi : "RKRL mode"
 *  mode : 0 - Tutte le room conosciute (anche zappate)
 *         1 - con messaggi nuovi
 *         2 - senza messaggi nuovi
 *         3 - room zappate
 * Restituisce: "Nome Room|Num Room|Zap"
 *           - Zap: 1 se zappata.
 */
void cmd_rkrl(struct sessione *t, char *cmd)
{
	int mode, zap;
        struct room *punto;

	mode = extract_int(cmd, 0);
        cprintf(t, "%d\n", SEGUE_LISTA);

	switch (mode) {
	case 0:
		for (punto = lista_room.first; punto; punto = punto->next)
			if ( (zap = room_known(t, punto) > 0))
				cprintf(t, "%d %s%c|%ld|%d\n", OK,
					punto->data->name, room_type(punto),
					punto->data->num, zap - 1);
		break;
	case 1:
		for (punto = lista_room.first; punto; punto = punto->next)
			if ((room_known(t, punto) == 1) && new_msg(t, punto))
				cprintf(t, "%d %s%c|%ld|0\n", OK,
					punto->data->name, room_type(punto),
					punto->data->num);
		break;
	case 2:
		for (punto = lista_room.first; punto; punto = punto->next)
			if ((room_known(t, punto) == 1) && !new_msg(t, punto))
				cprintf(t, "%d %s%c|%ld|0\n", OK,
					punto->data->name, room_type(punto),
					punto->data->num);
		break;
	case 3:
		for (punto = lista_room.first; punto; punto = punto->next)
			if ((room_known(t, punto) == 2)
			    && !(punto->data->flags & RF_GUESS))
				cprintf(t, "%d %s%c|%ld|1\n", OK,
					punto->data->name, room_type(punto),
					punto->data->num);
		break;
	}
	cprintf(t, "000\n");
}

/*
 * Crea una nuova room.
 * "RNEW roomname|fmnum|maxmsg"
 */
void cmd_rnew(struct sessione *t, char *cmd)
{
	char nome[LBUF];
	long fmnum, maxmsg, num;
	struct room *room;
	struct text *txt;

#ifdef USE_FLOORS
	if ((t->utente) && ((t->utente->livello >= MINLVL_NEWROOM)
			    || (IS_FA(t, t->floor))))
#else
	if ((t->utente) && (t->utente->livello >= MINLVL_NEWROOM))
#endif
	  {
		extractn(nome, cmd, 0, ROOMNAMELEN);
		if (strlen(nome) == 0) {
			cprintf(t, "%d Serve il nome della room.\n", ERROR);
			return;
		}
		if (room_find(nome)) {
			cprintf(t, "%d La room %s esiste gia`.\n", ERROR,nome);
			return;
		}
                /* NON USO FM MULTIPLI
		fmnum = extract_long(cmd, 1);
                */
		fmnum = dati_server.fm_basic;
		maxmsg = extract_long(cmd, 2);
		num = room_create(nome, fmnum, maxmsg, ROOM_NORMAL);
		if (num != -1) {
#ifdef USE_FLOORS
			floor_add_room(t->floor, room_findn(num));
#endif
			citta_logf("ROOM [%s] creata da [%s].",nome,t->utente->nome);
			if ((room = room_find(aide_room))) {
				txt = txt_create();
				txt_putf(txt, "Room [%s] creata da %s.\n",
					 nome, t->utente->nome);
				citta_post(room, "Nuova room.", txt);
				txt_free(&txt);
			}
			cprintf(t, "%d %ld\n", OK, num);
		} else
			cprintf(t, "%d\n", ERROR+NO_SUCH_FM);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Edita la room corrente
 */
void cmd_redt(struct sessione *t, char *cmd)
{
	char nome[LBUF];
	long fmask;
        int rfind;
	struct room_data *rd;

	if (IS_RH(t)) {
		rd = t->room->data;
		if (rd->type == ROOM_DYNAMICAL) {
		        cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			return;
		}
		switch(cmd[0]) {
		case '0': /* send config */
			cprintf(t, "%d %s|%d|%d|%d|%ld|%ld|%ld|%ld\n", OK,
				rd->name, rd->type, rd->rlvl, rd->wlvl,
				rd->fmnum, rd->maxmsg, rd->flags, rd->master);
			t->stato = CON_ROOM_EDIT;
			break;
		case '1': /* modify configuration */
                        rfind = rd->flags & RF_FIND;
			PUT_USERMAIL(t);
			extractn(nome, cmd, 1, ROOMNAMELEN);
			if (!((*nome == '\0') || (rd->flags & RF_MAIL))) 
				strcpy(rd->name, nome);
			rd->rlvl = extract_int(cmd, 2);
			rd->wlvl = extract_int(cmd, 3);
			fmask = rd->flags & RF_RESERVED;
			rd->flags = fmask|(extract_long(cmd,4) & ~RF_RESERVED);
                        if ((rd->flags & RF_FIND) && (!rfind))
                                find_cr8index(t->room);
			time(&(rd->mtime));
			utr_setf_all(t->room->pos, UTR_NEWGEN);
			/* Se e' una Guess Room, la devono dimenticare tutti */
			if (rd->flags & RF_GUESS) {
				utr_resetf_all_lvl(t->room->pos, UTR_KNOWN,
						   LVL_AIDE);
                                rd->def_utr &= ~UTR_KNOWN;
				room_empty(t->room, t);
			} else {
                                utr_setf_all(t->room->pos, UTR_KNOWN);
                                rd->def_utr |= UTR_KNOWN;
                        }
			citta_logf("ROOM #%ld edited by [%s].", t->room->data->num,
			     t->utente->nome);
			cprintf(t, "%d\n", OK);
			REMOVE_USERMAIL(t);
			t->stato = CON_COMANDI;
			break;
		case '2': /* abort */
			t->stato = CON_COMANDI;
			cprintf(t, "%d\n", OK);
			break;
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Elimina la room corrente.
 */
void cmd_rdel(struct sessione *t, char *cmd)
{
	struct room *r, *room;
	struct text *txt;
	
	if (t->utente->livello < t->room->data->wlvl) {
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
		return;
	}
	r = t->room;
	if (r->data->type == ROOM_DYNAMICAL) {
		cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
		return;
	}
	if ((r->data->flags & (RF_BASE | RF_FLOOR))) {
		cprintf(t, "%d\n", ERROR+ERR_BASIC_ROOM);
		return;
	}
	t->room = lista_room.first;
	citta_logf("ROOM [%s] cancellata da [%s].", r->data->name,
	     t->utente->nome);
	if ((room = room_find(aide_room))) {
		txt = txt_create();
		txt_putf(txt, "La room [%s] e' stata cancellata da %s.\n",
			 r->data->name, t->utente->nome);
		citta_post(room, "Cancellazione Room.", txt);
		txt_free(&txt);
	}
	room_delete(r);
	cprintf(t, "%d\n", OK);
}

/*
 * Read INFo room corrente
 */
void cmd_rinf(struct sessione *t)
{
	FILE *fp;
        char filename[LBUF];
        char buf[LBUF], aaa[LBUF], raide[LBUF];
	struct room_data *rd;
	struct dati_ut *ra;

        if (t->room->data->type != ROOM_DYNAMICAL)
                t->room_flags[t->room->pos] &= ~UTR_NEWGEN;
        else if (t->room->data->flags & RF_BLOG)
                dfr_clear_flag(t->room->msg->dfr, t->utente->matricola,
                               UTR_NEWGEN);

	rd = t->room->data;
	ra = trova_utente_n(rd->master);
	if (ra)
		strcpy(raide, ra->nome);
	else
		strcpy(raide, "nessuno");
	sprintf(aaa, "%d|%d|%ld|%ld|%ld|%ld|%s|%s|%ld", rd->rlvl, rd->wlvl,
		get_highloc(t), rd->ctime, rd->mtime, rd->ptime, raide,
		rd->name, rd->flags);
        if ((t->room->data->type == ROOM_DYNAMICAL)
            && (!(t->room->data->flags & RF_BLOG))) {
		cprintf(t, "%d %s\n", OK, aaa);
		return;
        }
        if (t->room->data->flags & RF_BLOG)
                sprintf(filename, "%s%ld", FILE_BLOGINFO, t->room->data->num);
        else
                sprintf(filename, "%s/%ld", ROOMS_INFO, t->room->data->num);
	if ((fp = fopen(filename, "r")) == NULL) {
		cprintf(t, "%d %s\n", OK, aaa);
		return;
	}
	cprintf(t, "%d %s\n", SEGUE_LISTA, aaa);
	
	while (fgets(buf, LBUF, fp) != NULL)
		cprintf(t, "%d %s", OK, buf);

	fclose(fp);
	cprintf(t, "000\n");
}

/*
 * Zap current room.
 */
void cmd_rzap(struct sessione *t)
{
	if (t->room->data->type == ROOM_DYNAMICAL) {
		cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
		return;
	}
	if ((t->utente) && (t->utente->livello < LVL_AIDE)
	    && !IS_RH(t) && (t->room->data->num != 1)
	    && !(t->room->data->flags & RF_MAIL)) {
		/* Andra' messo baseroom */
		t->room_flags[t->room->pos] |= UTR_ZAPPED;
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR);
}

/*
 * Zap all rooms.
 */
void cmd_rzpa(struct sessione *t)
{
	struct room *r;
	
	if ((t->utente) && (t->utente->livello < LVL_AIDE)) {
		for (r = lista_room.first; r; r = r->next)
			if (room_known(t, r) && (r->data->num != 1)
			    && !IS_RHR(t, r) && !(r->data->flags & RF_MAIL))
				t->room_flags[r->pos] |= UTR_ZAPPED;
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR);
}


/*
 * UnZap all rooms.
 */
void cmd_ruzp(struct sessione *t)
{
	struct room *r;
	
	if (t->utente) {
		for (r = lista_room.first; r; r = r->next)
			if ((room_known(t, r) == 2)
			    && !(r->data->flags & RF_GUESS))
				t->room_flags[r->pos] &= ~UTR_ZAPPED;
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR);
}

/*
 * Set Room Aide for current room.
 * MinLvl: Aide
 * Sintassi : "RAID name"
 *            name: Nome del RA.
 */
void cmd_raid(struct sessione *t, char *arg)
{
	char nome[LBUF];
	struct dati_ut *ut;
	struct room *room;
	struct text *txt;
	
	if (t->room->data->type == ROOM_DYNAMICAL) {
		cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
		return;
	}
	extract(nome, arg, 0);
	ut = trova_utente(nome);
	if (ut == NULL)
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
	else if ((ut->livello < LVL_NORMALE)
		 || (LVL_ROOMAIDE < t->room->data->rlvl))
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
	else {
		t->room->data->master = ut->matricola;
		if (ut->livello < LVL_ROOMAIDE)
			ut->livello = LVL_ROOMAIDE;
		/* La prossima forse fa troppo lavoro... */
		utr_resetf_all(t->room->pos, UTR_ROOMAIDE);
		utr_setf(ut->matricola, t->room->pos, UTR_ROOMAIDE);
		citta_logf("ROOM: [%s] nuovo RA di [%s]", ut->nome,
		     t->room->data->name);
		if ((room = room_find(ra_room))) {
			txt = txt_create();
			txt_putf(txt, "%s e' il nuovo Room Aide di [%s].\n",
				 ut->nome, t->room->data->name);
			citta_post(room, "Nuovo Room Aide.", txt);
			txt_free(&txt);
		}
		cprintf(t, "%d\n", OK);
	}
}

/*
 * Da privilegi di room helper a un utente nella room corrente
 * Sintassi: "RNRH nome"
 *           dove 'nome' e' il nome nuovo Room Helper.
 */
void cmd_rnrh(struct sessione *t, char *buf)
{
	struct dati_ut *ut;
	char nome[LBUF];

	if (IS_RA(t)) {
	        if (t->room->data->type == ROOM_DYNAMICAL) {
		        cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			return;
		}
		extract(nome, buf, 0);
		ut = trova_utente(nome);
		if (ut == NULL)
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
		else if ((ut->livello < LVL_NORMALE)
			 || (ut->livello < t->room->data->rlvl))
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		else
			cprintf(t, "%d %d\n", OK, utr_togglef(ut->matricola,
							      t->room->pos,
							      UTR_ROOMHELPER));
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Invita un utente nella room corrente
 * Sintassi: "RINV nome"
 *           dove 'nome' e' il nome dell'utente da invitare.
 */
void cmd_rinv(struct sessione *t, char *buf)
{
	struct dati_ut *ut;
	char nome[LBUF];

	if (IS_RH(t)) {
		extract(nome, buf, 0);
		ut = trova_utente(nome);
		if (!(t->room->data->flags & RF_INVITO))
			cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
		else if (ut == NULL)
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
		else if (ut->livello < t->room->data->rlvl)
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		else {
			utr_setf(ut->matricola, t->room->pos, UTR_INVITED);
			cprintf(t, "%d\n", OK);
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Lista degli utenti invitati nella room.
 */
void cmd_rinw(struct sessione *t)
{
        struct lista_ut *punto;

	if (IS_RH(t)) {
		if (!(t->room->data->flags & RF_INVITO)) {
			cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
			return;
		}
		cprintf(t, "%d\n", SEGUE_LISTA);
		for (punto = lista_utenti; punto; punto = punto->prossimo)
			if (utr_getf(punto->dati->matricola, t->room->pos)
			    & UTR_INVITED)
				cprintf(t, "%d %s\n", OK, punto->dati->nome);
		cprintf(t, "000\n");
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Richiesta di invito a una room.
 * Sintassi: "RIRQ nome_room"
 *           dove 'nome_room' e' il nome della room.
 */
void cmd_rirq(struct sessione *t, char *buf)
{
	char name[LBUF];
	struct room *r, *r1;
	struct text *txt;

	extract(name, buf, 0);
	if (!(r = room_find(name)))
		cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
	else if ((t->utente->livello < r->data->rlvl))
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
	else if (!(r->data->flags & RF_INVITO))
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
	else {
		if (r->data->flags & RF_AUTOINV) {
			utr_setf(t->utente->matricola, r->pos, UTR_INVITED);
			cprintf(t, "%d\n", OK);
		} else {
			/* Posta la richiesta di invito in Room Aide */
			if ((r1 = room_find(ra_room))) {
				txt = txt_create();
				txt_putf(txt, "%s chiede di essere invitato nella room [%s].\n",
					 t->utente->nome, name);
				citta_post(r1,"Richiesta Invito.",txt);
				txt_free(&txt);
			}
			cprintf(t, "%d\n", OK + 1);
		}
	}
}

/*
 * Kick out di un utente dalla room corrente
 * Sintassi: "RKOB nome|motivo"
 *           dove 'nome' e' il nome dell'utente da cacciare,
 *           e 'motivo' e` una spiegazione del motivo.
 */
void cmd_rkob(struct sessione *t, char *buf)
{
	struct dati_ut *ut;
	char nome[LBUF];
	char motivo[LBUF];
	struct room *room;
	struct text *txt;

	if (IS_RH(t)) {
	        if (t->room->data->type == ROOM_DYNAMICAL) {
		        cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			return;
		}
		extract(nome, buf, 0);
		extract(motivo, buf, 1);
		ut = trova_utente(nome);
		if (ut == NULL)
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
		else if (ut->livello < t->room->data->rlvl)
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		else if (utr_getf(ut->matricola, t->room->pos) & UTR_KICKOUT)
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL+1);
		else {
			utr_setf(ut->matricola, t->room->pos, UTR_KICKOUT);
			cprintf(t, "%d\n", OK);
			/* Notifica in Room Aide */
			if ((room = room_find(aide_room))) {
				txt = txt_create();
				txt_putf(txt, "L'utente <b>%s</b> &egrave; stato cacciato dalla room <b>%s</b> da %s",
					 nome, t->room->data->name,
					 t->utente->nome);
				txt_put(txt, "per il seguente motivo:");
				txt_putf(txt, "<b>%s</b>", motivo);
				citta_post(room, "Kick Out utente.", txt);
				txt_free(&txt);
			}
			/* Notifica in mail all'utente */
			txt = txt_create();
			txt_putf(txt, "Sei stato cacciato dalla room <b>%s</b> per il seguente motivo:", t->room->data->name);
			txt_putf(txt, "<b>%s</b>", motivo);
			citta_mail(ut, "Kick Out.", txt);
			txt_free(&txt);
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Riammissione di un utente che ha precedentemente subito un Kick out
 * dalla room corrente
 * Sintassi: "RKOB nome"
 *           dove 'nome' e' il nome dell'utente da riammettere.
 */
void cmd_rkoe(struct sessione *t, char *buf)
{
	struct dati_ut *ut;
	char nome[LBUF];
	struct room *room;
	struct text *txt;

	if (IS_RH(t)) {
	        if (t->room->data->type == ROOM_DYNAMICAL) {
		         cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			 return;
		}
		extract(nome, buf, 0);
		ut = trova_utente(nome);
		if (ut == NULL)
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
		else if (ut->livello < t->room->data->rlvl)
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		else if (!(utr_getf(ut->matricola,t->room->pos) & UTR_KICKOUT))
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL+1);
		else {
			utr_resetf(ut->matricola, t->room->pos, UTR_KICKOUT);
			/* Notifica in Room Aide */
			if ((room = room_find(aide_room))) {
				txt = txt_create();
				txt_putf(txt, "L'utente <b>%s</b> &egrave; stato riammesso nella room <b>%s</b> da %s",
					 nome, t->room->data->name,
					 t->utente->nome);
				citta_post(room, "Kick Out: fine pena.",
					   txt);
				txt_free(&txt);
			}
			/* Notifica in mail all'utente */
			txt = txt_create();
			txt_putf(txt, "Sei stato riammesso nella room <b>%s</b>.", t->room->data->name);
			txt_put(txt, "Fai il bravo! ;-)\n");
			citta_mail(ut, "Kick Out: pena espiata.", txt);
			txt_free(&txt);
			cprintf(t, "%d\n", OK);
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Lista degli utenti kickati dalla room.
 */
void cmd_rkow(struct sessione *t)
{
        struct lista_ut *punto;

	if (IS_RH(t)) {
	        if (t->room->data->type == ROOM_DYNAMICAL) {
		        cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			return;
		}
		cprintf(t, "%d\n", SEGUE_LISTA);
		for (punto = lista_utenti; punto; punto = punto->prossimo)
			if (utr_getf(punto->dati->matricola, t->room->pos)
			    & UTR_KICKOUT)
				cprintf(t, "%d %s\n", OK, punto->dati->nome);
		cprintf(t, "000\n");
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Swap position of two rooms.
 * Sintassi: "RSWP roomname1|roomname2"
 */
void cmd_rswp(struct sessione *t, char *cmd)
{
	char buf[LBUF];
	struct room *r1, *r2;

	extract(buf, cmd, 0);
	r1 = room_find(buf);
	if (r1 == NULL) {
		cprintf(t, "%d %d\n", ERROR, 1);
		return;
	}
	extract(buf, cmd, 1);
	r2 = room_find(buf);
	if (r2 == NULL) {
		cprintf(t, "%d %d\n", ERROR, 2);
		return;
	}
	room_swap(r1->data->num, r2->data->num);
	citta_logf("ROOM Swap [%s] e [%s] da %s.", r1->data->name,
	     r2->data->name, t->utente->nome);
	cprintf(t, "%d\n", OK);
}

/*
 * Modifica lunghezza room corrente (numero slot per messaggi)
 * Sintassi: "RLEN length"
 */
void cmd_rlen(struct sessione *t, char *cmd)
{
	long len;

	if (IS_RH(t)) {
		IF_MAILROOM(t) {
			cprintf(t, "%d Mailroom\n", ERROR);
			return;
		}
		if (t->room->data->type == ROOM_DYNAMICAL) {
		        cprintf(t, "%d\n", ERROR+ERR_VIRTUAL_ROOM);
			return;
		}
		len = extract_long(cmd, 0);
		if ((len < 2) || (len > MAXROOMLEN)) {
			cprintf(t, "%d\n", ERROR);
			return;
		}
		msg_resize(t->room, len);
		citta_logf("ROOM Resize: Maxmsg = %ld per [%s] da %s.",
		     len, t->room->data->name, t->utente->nome);
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Read ALL: set all messages in all known rooms as read.
 *
 * Aide and Sysop cannot use this command, mails and posts in rooms where
 * the user is RH or RA are not set as read.
 * Segna come letti tutti i post nelle room conosciute tranne in Lobby,
 * Mail e nelle room dove si e` RA o RH.
 * Aide e Sysop non sono autorizzati a utilizzare questo comando.
 */
void cmd_rall(struct sessione *t)
{
	struct room *r;
	
	if (t->utente->livello < LVL_AIDE) {
		for (r = lista_room.first; r; r = r->next)
			if (room_known(t, r) && (r->data->num != 1)
			    && !IS_RHR(t, r) && !(r->data->flags & RF_MAIL))
				t->fullroom[r->pos]
					= r->msg->highmsg;
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR);
}
