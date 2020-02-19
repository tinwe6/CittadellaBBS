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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : floor_ops.c                                                        *
*        Operazioni sui floor della BBS.                                    *
****************************************************************************/
#include "config.h"
#ifdef USE_FLOORS

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "cittaserver.h"
#include "messaggi.h"
#include "floors.h"
#include "floor_ops.h"
#include "extract.h"
#include "mail.h"
#include "text.h"
#include "utente.h"
#include "ut_rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void cmd_fnew(struct sessione *t, char *cmd);
void cmd_fdel(struct sessione *t, char *cmd);
void cmd_flst(struct sessione *t, char *cmd);
void cmd_fmvr(struct sessione *t, char *cmd);
void cmd_faid(struct sessione *t, char *arg);
void cmd_finf(struct sessione *t);
void cmd_fkrl(struct sessione *t, char *cmd);
void cmd_fkrm(struct sessione *t, char *cmd);
void cmd_fkra(struct sessione *t, char *cmd);

void cmd_fedt(struct sessione *t, char *cmd);

/***************************************************************************/
/*
 * Crea un nuovo floor. Viene automaticamente creata anche la room principale
 * del floor, la Floor_Lobby>.
 * 
 * Sintassi: "FNEW floorname|fmnum|maxmsg"
 * Restituisce: "OK floor_num|room_num|nome_room"
 *              floor_num : numero nuovo floor.
 *              room_num  : numero Floor_Lobby> del floor.
 *              nome_room : nome Floor_Lobby>.
 */
void cmd_fnew(struct sessione *t, char *cmd)
{
	char nome[LBUF], nome_room[LBUF];
	long fmnum, maxmsg, num, rnum;
	struct floor *floor;
	struct room *room;
	struct text *txt;
	
	extract(nome, cmd, 0);
	if (strlen(nome) == 0) {
		cprintf(t, "%d Serve il nome del floor.\n", ERROR);
		return;
	}
	if (floor_find(nome)) {
		cprintf(t, "%d Il floor %s esiste gia'.\n", ERROR,
			nome);
		return;
	}
	fmnum = extract_long(cmd, 1);
	maxmsg = extract_long(cmd, 2);
	num = floor_create(nome, fmnum, maxmsg, FLOOR_NORMAL);
	if (num != -1) {
		logf("FLOOR #%ld [%s] creato da [%s].", num, nome,
		     t->utente->nome);
		floor = floor_findn(num);
		/* Ora crea il Floor_Lobby> */
		sprintf(nome_room, "Floor Lobby %ld", num);
		rnum = room_create(nome_room,fmnum,maxmsg,ROOM_FLOOR);
		if (rnum != -1) {
			logf("ROOM #%ld [%s] creata da [%s].", rnum,
			     nome_room, t->utente->nome);
		} else {
			/* Cancellare il Floor *DA FARE* */
			cprintf(t, "%d\n", ERROR);
			return;
		}
		room = room_findn(rnum);
		room->data->flags = RF_FLOOR_DEF;
		room->data->mtime = room->data->ctime;
		floor_add_room(floor, room);
		if ( (room = room_find(aide_room))) {
			txt = txt_create();
			txt_putf(txt, "Floor [%s] creato da %s.\n",
				 nome, t->utente->nome);
			citta_post(room, "Nuovo floor.", txt);
			txt_free(&txt);
		}
		cprintf(t, "%d %ld|%ld|%s\n", OK, num, rnum,nome_room);
	} else
		cprintf(t, "%d\n", ERROR+NO_SUCH_FM);
}

/*
 * Elimina il floor corrente.
 */
void cmd_fdel(struct sessione *t, char *cmd)
{
	struct floor *f;
	struct room *room;
	struct text *txt;

	f = t->floor;
	if (t->utente->livello >= f->data->rlvl) {
		t->room = lista_room.first;
		t->floor = floor_list.first;
		logf("FLOOR [%s] cancellato da [%s].", f->data->name,
		     t->utente->nome);
		if ( (room = room_find(aide_room))) {
			txt = txt_create();
			txt_putf(txt, "Il floor [%s] e' stato cancellato da %s.\n",
				 f->data->name, t->utente->nome);
			citta_post(room, "Cancellazione Room.", txt);
			txt_free(&txt);
		}
		floor_delete(f);
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Invia la lista dei floor con la loro configurazione.
 */
void cmd_flst(struct sessione *t, char *cmd)
{
	char buf[LBUF];
        struct floor *punto;
	struct dati_ut *fa;

	cprintf(t, "%d\n", SEGUE_LISTA);
	for (punto = floor_list.first; punto; punto = punto->next) {
		if (t->utente->livello >= punto->data->rlvl) {
			fa = trova_utente_n(punto->data->master);
			if (fa)
				strcpy(buf, fa->nome);
			else
				strcpy(buf, "");
			cprintf(t, "%d %ld|%s|%ld|%ld|%ld|%ld|%ld|%ld|%s\n",
				OK, punto->data->num, punto->data->name,
				punto->data->numroom, punto->data->numpost,
				punto->data->ptime, punto->data->flags,
				punto->data->rlvl, punto->data->wlvl, buf);
		}
	}
	cprintf(t, "000\n");
}

/*
 * Edita il floor corrente
 */
void cmd_fedt(struct sessione *t, char *cmd)
{
	char nome[LBUF];
	long fmask;
	struct floor *fl;
	struct floor_data *fd;

	fl = t->floor;
	if (IS_FH(t, fl)) {
		fd = fl->data;
		switch(cmd[0]) {
		 case '0': /* send config */
			cprintf(t, "%d %s|%d|%d|%d|%ld|%ld|%ld|%ld\n", OK,
				fd->name, fd->type, fd->rlvl, fd->wlvl,
				fd->fmnum, fd->maxmsg, fd->flags, fd->master);
			break;
		 case '1': /* modify configuration */
			extractn(nome, cmd, 1, FLOORNAMELEN);
			if (strlen(nome)) 
				strcpy(fd->name, nome);
			fd->rlvl = extract_int(cmd, 2);
			fd->wlvl = extract_int(cmd, 3);
			fmask = fd->flags & FF_RESERVED;
			fd->flags = fmask|(extract_long(cmd,4) & ~FF_RESERVED);
			time(&(fd->mtime));

                        /*
			utr_setf_all(t->room->pos, UTR_NEWGEN);
                        */

			/* Se e' una Guess Room, la devono dimenticare tutti */
			/*
			if (rd->flags & RF_GUESS) {
				utr_resetf_all_lvl(t->room->pos, UTR_KNOWN,
						   LVL_AIDE);
                                rd->def_utr &= ~UTR_KNOWN;
				room_empty(t->room, t);
			} else {
                                utr_setf_all(t->room->pos, UTR_KNOWN);
                                rd->def_utr |= UTR_KNOWN;
                        }
			*/
			logf("FLOOR #%ld edited by [%s].", fd->num,
			     t->utente->nome);
			cprintf(t, "%d\n", OK);
			break;
		}
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Floor: Move Room - Sposta la room corrente nel floor "floor_name"
 * Sintassi: "FMVR floor_name"
 */
void cmd_fmvr(struct sessione *t, char *cmd)
{
	struct sessione *punto;
	struct floor *f, *dest;
	struct room *room;
	struct text *txt;
	char floor_name[LBUF];

	f = t->floor;
	if (IS_FA(t, f)) {
		extract(floor_name, cmd, 0);
		dest = floor_find(floor_name);
		if (dest) {
			floor_change(dest, t->room);
			if ( (room = room_find(ra_room))) {
				txt = txt_create();
				txt_putf(txt, "Cambio floor per la room [%s].\n",
					 t->room->data->name);
				txt_putf(txt, "Spostata in [%s] da %s.\n",
					 dest->data->name, t->utente->nome);
				citta_post(room, "Spostamento Room.", txt);
				txt_free(&txt);
			}
			/* Aggiorna le sessioni degli utenti nella room. */
			room = t->room;
			for (punto = lista_sessioni; punto;
			                       punto = punto->prossima) {
				if (punto->room == room)
					punto->floor = dest;
			/* Qui andrebbe notificato il cambio di floor agli *
			 * utenti presenti nella room (*DA FARE*)          */
			}
			cprintf(t, "%d\n", OK);
		} else
			cprintf(t, "%d\n", ERROR);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Set Floor Aide for current floor.
 * MinLvl: Aide
 * Sintassi : "FAID name"
 *            name: Nome del Floor Aide.
 */
void cmd_faid(struct sessione *t, char *arg)
{
	char nome[LBUF];
	struct dati_ut *ut;
	struct room *room;
	struct text *txt;

	extract(nome, arg, 0);
	ut = trova_utente(nome);
	if (ut == NULL)
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
	else if (ut->livello < LVL_NORMALE)
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
	else {
		t->floor->data->master = ut->matricola;
		if (ut->livello < LVL_FLOORAIDE)
			ut->livello = LVL_FLOORAIDE;
		/* La prossima forse fa troppo lavoro... */
		/*
                  utr_resetf_all(t->room->pos, UTR_ROOMAIDE);
                  utr_setf(ut->matricola, t->room->pos, UTR_ROOMAIDE);
                */
		logf("FLOOR: [%s] nuovo FA di [%s]", ut->nome,
		     t->floor->data->name);
		if ((room = room_find(aide_room))) {
			txt = txt_create();
			txt_putf(txt, "%s e' il nuovo Floor Aide di [%s].\n",
				 ut->nome, t->room->data->name);
			citta_post(room, "Nuovo Floor Aide.", txt);
			txt_free(&txt);
		}
		cprintf(t, "%d\n", OK);
	}
}

/*
 * Read Floor INFo del floor corrente.
 */
void cmd_finf(struct sessione *t)
{
	FILE *fp;
        char filename[LBUF];
        char buf[LBUF], aaa[LBUF], faide[LBUF];
	struct floor_data *fd;
	struct dati_ut *fa;
	
/*
        t->room_flags[t->room->pos] &= ~UTR_NEWGEN;
*/
	fd = t->floor->data;
	fa = trova_utente_n(fd->master);
	if (fa)
		strcpy(faide, fa->nome);
	else
		strcpy(faide, "nessuno");
	sprintf(aaa, "%d|%d|%ld|%ld|%ld|%ld|%s|%s|%ld|%ld", fd->rlvl, fd->wlvl,
		fd->numpost, fd->ctime, fd->mtime, fd->ptime, faide,
		fd->name, fd->flags, fd->numroom);
	sprintf(filename, "%s/%ld", FLOOR_INFO, fd->num);
	if ((fp = fopen(filename, "r")) == NULL) {
		cprintf(t, "%d %s\n", OK, aaa);
		return;
	}
	cprintf(t, "%d %s\n", SEGUE_LISTA, aaa);
	
	while (fgets(buf, LBUF, fp) != NULL)
		cprintf(t, "%d %s\n", OK, buf);
	
	fclose(fp);
	cprintf(t, "000\n");
}

/*
 * Invia la lista di tutte le room conosciute organizzate per floor.
 * Sintassi : "FKRA mode"
 *  mode : 0 - Tutte le room conosciute (anche zappate)
 *         1 - con messaggi nuovi
 *         2 - senza messaggi nuovi
 *         3 - room zappate
 */
void cmd_fkra(struct sessione *t, char *cmd)
{
	int mode, zap;
	struct floor *f;
        struct f_room_list *punto;

	mode = extract_int(cmd, 0);
        cprintf(t, "%d\n", SEGUE_LISTA);

	for (f = floor_list.first; f; f = f->next) {
		cprintf(t, "%d 0|%s:|%ld\n", OK, f->data->name,
			f->data->num);
		for (punto = f->rooms; punto; punto = punto->next) {
			if ( (zap = room_known(t, punto->room)) > 0)
				cprintf(t, "%d 1|%s%c|%ld|%d|%d\n", OK,
					punto->room->data->name,
					room_type(punto->room),
					punto->room->data->num, zap-1,
					new_msg(t, punto->room));
		}
	}
	cprintf(t, "000\n");
}

/*
 * Invia la lista delle room conosciute nel floor corrente
 * Sintassi : "FKRL mode"
 *  mode : 0 - Tutte le room conosciute (anche zappate)
 *         1 - con messaggi nuovi
 *         2 - senza messaggi nuovi
 *         3 - room zappate
 */
void cmd_fkrl(struct sessione *t, char *cmd)
{
	int mode;
        struct f_room_list *punto;

	mode = extract_int(cmd, 0);
        cprintf(t, "%d\n", SEGUE_LISTA);

	switch (mode) {
	 case 0:
		for (punto = t->floor->rooms; punto; punto = punto->next)
			if (room_known(t, punto->room) > 0)
				cprintf(t, "%d %s%c|%ld\n", OK,
					punto->room->data->name,
					room_type(punto->room),
					punto->room->data->num);
		break;
	 case 1:
		for (punto = t->floor->rooms; punto; punto = punto->next)
			if ((room_known(t, punto->room) == 1) 
			    && new_msg(t, punto->room))
				cprintf(t, "%d %s%c|%ld\n", OK,
					punto->room->data->name,
					room_type(punto->room),
					punto->room->data->num);
		break;
	 case 2:
		for (punto = t->floor->rooms; punto; punto = punto->next)
			if ((room_known(t, punto->room) == 1)
			    && !new_msg(t, punto->room))
				cprintf(t, "%d %s%c|%ld\n", OK,
					punto->room->data->name,
					room_type(punto->room),
					punto->room->data->num);
		break;
	 case 3:
		for (punto = t->floor->rooms; punto; punto = punto->next)
			if ((room_known(t, punto->room) == 2)
			    && !(punto->room->data->flags & RF_GUESS))
				cprintf(t, "%d %s%c|%ld\n", OK,
					punto->room->data->name,
					room_type(punto->room),
					punto->room->data->num);
		break;
	}
	cprintf(t, "000\n");
}

/*
 * Invia la lista dei floor con room conosciute
 * Sintassi : "FKRM mode"
 *  mode : 0 - Tutti i floors conosciuti
 *         1 - con room che hanno messaggi nuovi
 *         2 - dove tutte le room sono senza messaggi nuovi
 */
void cmd_fkrm(struct sessione *t, char *cmd)
{
	int mode, flag;
	struct floor *f;
        struct f_room_list *punto;

	mode = extract_int(cmd, 0);
        cprintf(t, "%d\n", SEGUE_LISTA);

	switch (mode) {
	 case 0:
		for (f = floor_list.first; f; f = f->next) {
			flag = 0;
			punto = f->rooms;
			while (punto) {
				if (room_known(t, punto->room) > 0) {
					flag = 1;
					break;
				}
				punto = punto->next;
			}
			if (flag)
				cprintf(t, "%d %s:|%ld\n", OK, f->data->name,
					f->data->num);
		}
		break;
	 case 1:
		for (f = floor_list.first; f; f = f->next) {
			if (f != t->floor) {
				flag = 0;
				punto = f->rooms;
				while (punto) {
					if ((room_known(t, punto->room) == 1) 
					    && new_msg(t, punto->room)) {
						flag = 1;
						break;
					}
					punto = punto->next;
				}
				if (flag)
					cprintf(t, "%d %s:|%ld\n", OK,
						f->data->name, f->data->num);
			}
		}
		break;
	 case 2:
		for (f = floor_list.first; f; f = f->next) {
			if (f != t->floor) {
				flag = 0;
				punto = f->rooms;
				while (punto) {
					if ((room_known(t, punto->room) == 1)
					    && new_msg(t, punto->room)) {
						flag = 1;
						break;
					}
					punto = punto->next;
				}
				if (flag == 0)
					cprintf(t, "%d %s:|%ld\n", OK,
						f->data->name, f->data->num);
			}
		}
		break;
	}
	cprintf(t, "000\n");
}
/***************************************************************************/
/*
 * Zap current room.
 */
void cmd_fzap(struct sessione *t)
{
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
 * Da privilegi di room helper a un utente nella room corrente
 * Sintassi: "RNRH nome"
 *           dove 'nome' e' il nome nuovo Room Helper.
 */
void cmd_fnrh(struct sessione *t, char *buf)
{
	struct dati_ut *ut;
	char nome[LBUF];

	if (IS_RA(t)) {
		extract(nome, buf, 0);
		ut = trova_utente(nome);
		if (ut == NULL)
			cprintf(t, "%d\n", ERROR+NO_SUCH_USER);
		else if ((ut->livello < LVL_NORMALE)
			 || (ut->livello < t->room->data->rlvl))
			cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		else
			cprintf(t, "%d %d\n", OK,
				utr_togglef(ut->matricola, t->room->pos,
					    UTR_ROOMHELPER));
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Swap position of two rooms.
 * Sintassi: "RSWP roomname1|roomname2"
 */
void cmd_fswp(struct sessione *t, char *cmd)
{
	char buf[LBUF];
	struct room *r1, *r2;
	
	if ((t->utente) && (t->utente->livello >= MINLVL_SWAPROOM)) {
		extract(buf, cmd, 0);
		r1 = room_find(buf);
		if (r1==NULL) {
			cprintf(t, "%d %d\n", ERROR, 1);
			return;
		}
		extract(buf, cmd, 1);
		r2 = room_find(buf);
		if (r2==NULL) {   
			cprintf(t, "%d %d\n", ERROR, 2);
			return;
		}
		room_swap(r1->data->num, r2->data->num);
		logf("ROOM Swap [%s] e [%s] da %s.", r1->data->name,
		     r2->data->name, t->utente->nome);
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}
#endif /* USE_FLOORS */
