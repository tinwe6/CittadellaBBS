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
* File : march.c                                                            *
*        Gestisce la marcia dell'utente fra le room.                        *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "cittaserver.h"
#include "blog.h"
#include "extract.h"
#include "file_messaggi.h"
#include "floors.h"
#include "march.h"
#include "mail.h"
#include "memstat.h"
#include "messaggi.h"
#include "room_ops.h"
#include "rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void cmd_goto(struct sessione *t, char *cmd);
static struct room * next_room_new(struct sessione *t, char mode);
static struct room * next_known_room(struct sessione *t, struct room *r);
static struct room * next_room_mode(struct sessione *t, char *name, int mode);
#ifdef USE_FLOORS
static struct room * next_room_new_f(struct sessione *t, char mode);
static struct room * next_known_room_f(struct sessione *t, struct room *r);
static struct room * next_floor_mode(struct sessione *t, char *name, int mode);
#endif

/* Restituisce il puntatore alla prossima room conosciuta dall'utente */
#define next_room(T) next_known_room((T), (T)->room)	
#ifdef USE_FLOORS
# define next_room_f(T) next_known_room_f((T), (T)->room)	
#endif
/***************************************************************************/
/*
 * GOTO: vai alla prossima room conosciuta.
 *       Restituisce il nome della nuova room, il numero di messaggi
 *       presenti e il numero di quelli ancora non letti.
 * mode: 0 - goto -> set all messages read.
 *       1 - skip -> leave fullroom unchanged.
 *       2 - abandon -> only set read the effectively read msgs.
 *       3 - Scan -> next known room, even if there are no new messages.
 *       4 - Home -> Torna alla Lobby) senza modificare il fullroom.
 *       5 - Mail -> Vai nella Mailroom abbandonando la room
 *       6 - Home -> Vai in Lobby) abbandonando la room
 *       7 - My Blog -> Vai a casa abbandonando la room
 *       (Nei modi 0, 1 e 2 va nelle room con nuovi messaggi)
 * nome: nome della room o del floor di destinazione. Se (null), va nella
 *       prossima room o prossimo floor.
 * tipo: 0 - room
 *       1 - floor
 *       2 - Blog
 * 
 * Sintassi: "GOTO mode|nome|tipo"
 * Restituisce: "%d Nomeroom>|NumMsg|NewMsg|Newgen|RoomFlags|New|Zap"
 *              Newgen=1 se la room e' di nuova generazione, 0 altrimenti.
 *              New e` il numero di room con messaggi da leggere,
 *              Zap e` il numero di room zappate.
 *              New e Zap vengono calcolati e inviati solo se l'utente e`
 *              finito nella Baseroom (lobby) altrimenti sono settati a 0.
 */
void cmd_goto(struct sessione *t, char *cmd)
{
	char name[LBUF];
	int mode, tipo;
	long nummsg = 0, newmsg = 0, lowest, fullroom, i, num_new, num_zap;
	long *msg;
        unsigned long utrflags;
	struct room *nr, *punto;
        blog_t *blog;
#ifdef USE_FLOORS
	struct floor *nf;
#endif

	mode = extract_int(cmd, 0);
	tipo = extract_int(cmd, 2);

        if (mode == 7) {
                tipo = GOTO_BLOG;
                strcpy(name, t->utente->nome);
        } else
                extract(name, cmd, 1);

	/* Trova la prossima room */
        switch (tipo) {
        case GOTO_ROOM:
		nr = next_room_mode(t, name, mode);
                break;
        case GOTO_FLOOR:
#ifdef USE_FLOORS
		nr = next_floor_mode(t, name, mode);
#else
                nr = NULL;
#endif
                break;
        case GOTO_BLOG:
                blog = blog_find(trova_utente(name));
                if (!blog) {
                        cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
                        return;
                }
                break;
        }

	if ((tipo != GOTO_BLOG) && (nr == NULL))
		return;

        /* Aggiorna fullroom dell'utente TODO : unificare */
        if ((t->room->data->type != ROOM_DYNAMICAL)
            || (t->room->data->flags & RF_BLOG)) {
                if (t->room->data->type != ROOM_DYNAMICAL)
                        fullroom = t->fullroom[t->room->pos];
                else
                        fullroom = dfr_getfullroom(t->room->msg->dfr,
                                                   t->utente->matricola);
                switch (mode) {
                case 1: /* skip */
                        fullroom = t->newmsg;
                case 4: /* Goto Baseroom */
                        break;
                case 2: /* abandon */
                case 5: /* Mail    */
                case 6: /* Home    */
                case 7: /* My Blog */
                        if (t->lastread > fullroom)
                                fullroom = t->lastread;
                        break;
                default: /* goto */
                        fullroom = get_highmsg(t);
                }
                if (t->room->data->type != ROOM_DYNAMICAL)
                        t->fullroom[t->room->pos] = fullroom;
                else
                        dfr_setfullroom(t->room->msg->dfr, t->utente->matricola,
                                        fullroom);
	}

	/* Se sono in una room virtuale, prima la elimino */
	kill_virtual_room(t);

        if (tipo == GOTO_BLOG) {
                cr8_virtual_room(t, name, -1, dati_server.fm_blog, RF_BLOG,
                                 blog->room_data);
                nr = t->room;
                t->room->msg = blog->msg;
                fullroom = dfr_getfullroom(blog->msg->dfr,t->utente->matricola);
                utrflags = dfr_getflags(blog->msg->dfr,t->utente->matricola);
        } else { /* Room normale */
                t->room = nr;
                t->lastread = 0;
#ifdef USE_FLOORS
                nf = floor_findn(nr->floor_num);
                t->floor = nf;
#endif
                fullroom = t->fullroom[nr->pos];
                utrflags = t->room_flags[t->room->pos];
        }

	PUT_USERMAIL(t); /* TODO risistemare e eliminare il PUT_USERMAIL */
        if (t->room->msg == NULL) {
                citta_logf("SYSERR 3: No msg data, [goto %s] for %s.", cmd,
                      t->utente->nome);
                cprintf(t, "%d SYSERR 3\n", ERROR);
                return;
        }

	lowest = fm_lowest(nr->data->fmnum);
	msg = nr->msg->num;
	t->newmsg = fullroom;

	for (i = 0; i < nr->data->maxmsg; i++)
		if (msg[i] >= lowest) {
			nummsg++;
			if (msg[i] > fullroom)
				newmsg++;
		}

        /* Se vado in lobby, aggiunge numero room con messaggi non letti */
        /* e numero di room zappate.                                     */
        num_new = 0;
        num_zap = 0;
        if (nr == lista_room.first) {
                for (punto = lista_room.first; punto; punto = punto->next)
                        switch (room_known(t, punto)){
                        case 1:
                                if (new_msg(t, punto))
                                        num_new++;
                                break;
                        case 2:
                                if (!(punto->data->flags & RF_GUESS))
                                        num_zap++;
                                break;
                        }
        }

#ifdef USE_FLOORS
	cprintf(t, "%d %s|%c|%ld|%ld|%ld|%ld|%ld|%ld|%s\n", OK, nr->data->name,
		room_type(nr), nummsg, newmsg, utrflags, nr->data->flags,
                num_new, num_zap, nf->data->name);
#else
	cprintf(t, "%d %s|%c|%ld|%ld|%ld|%ld|%ld|%ld\n", OK, nr->data->name,
		room_type(nr), nummsg, newmsg, utrflags, nr->data->flags,
                num_new, num_zap);
#endif

        if (t->room->data->type != ROOM_DYNAMICAL) {
                t->room_flags[t->room->pos] &= ~UTR_ZAPPED;
                t->room_flags[t->room->pos] |= UTR_KNOWN;
        } else if (t->room->data->flags & RF_BLOG) {
                dfr_set_flag(t->room->msg->dfr, t->utente->matricola,
                             UTR_KNOWN);
                dfr_clear_flag(t->room->msg->dfr, t->utente->matricola,
                               UTR_ZAPPED); /* TODO non serve per i blog */
        }

	REMOVE_USERMAIL(t);
}

/* Crea una room virtuale 'name' per la sessione t, con n messaggi.
 * Se fmnum e' -1, il fmnum viene messo nella msg_list.
 * Se n e' -1, la msg_list non viene allocata.                       */
void cr8_virtual_room(struct sessione *t, char *name, long n, long fmnum,
		      long rflags, struct room_data *rd)
{
        struct room * room;

	/* TODO: fmnum not used (we are not using multiple msg files) */
	IGNORE_UNUSED_PARAMETER(fmnum);

	/* Creo ed inizializzo la room. */
	CREATE(room, struct room, 1, TYPE_ROOM);
        if (rd)
                room->data = rd;
        else {
                CREATE(room->data, struct room_data, 1, TYPE_ROOM_DATA);
                room->data->num = 0;
                strncpy(room->data->name, name, ROOMNAMELEN);
                room->data->type = ROOM_DYNAMICAL;
                room->data->rlvl = LVL_NORMALE;
                room->data->wlvl = LVL_NORMALE;
                /* NON USO FM multipli 
                   room->data->fmnum = fmnum;
                */
                room->data->fmnum = dati_server.fm_basic;
                room->data->maxmsg = n;
                time(&(room->data->ctime));
                room->data->mtime = 0;
                room->data->ptime = 0;
                room->data->flags = RF_DEFAULT | RF_DYNROOM | rflags;
                room->data->def_utr = UTR_DEFAULT;
                room->data->master = -1;

                /* highmsg e highloc vanno inizializzate successivamente (se usate) */
                room->data->highmsg = 0;
                room->data->highloc = 0;
        }

	/* genero dinamicamente la msg list se necessario */
	if (n != -1) {
                /* NON USO FM multipli 
	        room->msg = msg_create(n, (fmnum == -1));
                */
	        room->msg = msg_create(n);
		room->data->flags |= RF_DYNMSG;
	} else
	        room->msg = NULL;

	/* linko la room */
	room->next = t->room->next;
	room->prev = t->room;
	t->room = room;
}

void kill_virtual_room(struct sessione *t)
{
	struct room *vr;

        if (t->room->data->type == ROOM_DYNAMICAL) {
                vr = t->room;
                t->room = vr->prev;
                if (vr->data->flags & RF_DYNMSG)
                        msg_free(vr->msg);
                if (vr->data->flags & RF_DYNROOM)
                        Free(vr->data);
                Free(vr);
        }
}


/****************************************************************************/
/*
 * Restituisce il puntatore alla prossima room conosciuta con messaggi nuovi.
 * mode: 0 - Prossima room nella lista con messaggi nuovi
 *       1 - Prima room della lista con messaggi nuovi
 */
static struct room * next_room_new(struct sessione *t, char mode)
{
	struct room *nr;
	long i = 0;

	if (t->room == NULL)
		return lista_room.first;
	if (mode)
		nr = lista_room.first;
	else
		nr = t->room;
	do {
		if (nr->next)
			nr = nr->next;
		else
			return lista_room.first;
		i++;
	} while (((room_known(t, nr) != 1) || (new_msg(t, nr) == 0))
		 && (i < dati_server.room_curr));
	if (i < dati_server.room_curr)
		return nr;
	else
		return lista_room.first;
}

/*
 * Restituisce il puntatore alla room conosciuta successiva a r.
 */
static struct room * next_known_room(struct sessione *t, struct room *r)
{
	struct room *nr;

	if (r == NULL)
		return (lista_room.first);
	nr = r;
	do {
		if (nr->next)
			nr = nr->next;
		else
			return lista_room.first;
	} while (room_known(t, nr) != 1);
	return nr;
}

static struct room * next_room_mode(struct sessione *t, char *name, int mode)
{
	struct room *nr;

	if (name[0] == 0)
		switch (mode) {
		 case 1:
		 case 2:
#ifdef USE_FLOORS
			if (t->utente->flags[4] & UT_FLOOR)
				nr = next_room_new_f(t, 0);
			else
#endif
				nr = next_room_new(t, 0);
			break;
		 case 3:
#ifdef USE_FLOORS
			if (t->utente->flags[4] & UT_FLOOR)
				nr = next_room_f(t);
			else
#endif
				nr = next_room(t);
			break;
		 case 4:
                 case 6:
			nr = lista_room.first;
			break;
		 case 5:
			nr = room_findn(dati_server.mailroom);
			break;
		 default:
#ifdef USE_FLOORS
			if (t->utente->flags[4] & UT_FLOOR)
				nr = next_room_new_f(t, t->utente->flags[4]
						     & UT_ROOMSCAN);
			else
#endif
				nr = next_room_new(t, t->utente->flags[4]
						   & UT_ROOMSCAN);
		}
	else {
		if ((nr = room_findp(t, name)) == NULL) {
			cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
			return NULL;
		}
		switch(room_known(t, nr)) {
		 case 1:
			break;
		 case 2:
			if (room_guess(t, nr, name))
				break;
		 default:
			cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
			return NULL;
		}
	}
	return nr;
}

#ifdef USE_FLOORS
/* 
 * Prossima room seguendo la marcia dei floor.
 * Restituisce il puntatore alla prossima room conosciuta con messaggi nuovi.
 * mode: 0 - Prossima room nella lista con messaggi nuovi
 *       1 - Prima room della lista con messaggi nuovi
 */
static struct room * next_room_new_f(struct sessione *t, char mode)
{
	struct room *nr;
	struct floor *nf;
	long i = 0;

	if (t->room == NULL)
		return lista_room.first;
	if (mode) {
		nr = lista_room.first;
		nf = floor_list.first;
	} else {
		nr = t->room;
		nf = t->floor;
	}
	do {
		if (nr->f_march->next)
			nr = nr->f_march->next->room;
		else {
			if (nf->next) {
				nf = nf->next;
				nr = nf->rooms->room;
			} else
				return lista_room.first;
		}
		i++;
	} while (((room_known(t, nr) != 1) || (new_msg(t, nr) == 0))
		 && (i < dati_server.room_curr));
	if (i < dati_server.room_curr)
		return nr;
	else
		return lista_room.first;
}

/*
 * Restituisce il puntatore alla room conosciuta successiva a r, seguendo
 * la marcia indicata dai floor.
 */
static struct room * next_known_room_f(struct sessione *t, struct room *r)
{
	struct room *nr;
	struct floor *nf;
	
	if (r == NULL)
		return (lista_room.first);
	nr = r;
	nf = t->floor;
	do {
		if (nr->f_march->next)
			nr = nr->f_march->next->room;
		else {
			if (nf->next) {
				nf = nf->next;
				nr = nf->rooms->room;
			} else
				return lista_room.first;
		}
	} while (room_known(t, nr) != 1);
	return nr;
}

static struct room * next_floor_mode(struct sessione *t, char *name, int mode)
{
	struct room *nr;

	if (name[0] == 0)
		switch (mode) {
		 case 1:
		 case 2:
			nr = next_room_new_f(t, 0);
			break;
		 case 3:
			nr = next_room_f(t);
			break;
		 case 4:
			nr = lista_room.first;
			break;
		 case 5:
			nr = room_findn(dati_server.mailroom);
			break;
		 default:
			nr = next_room_new_f(t, t->utente->flags[4]
					     & UT_ROOMSCAN);
		}
	else {
		if ((nr = room_findp(t, name)) == NULL) {
			cprintf(t, "%d\n", ERROR+NO_SUCH_ROOM);
			return NULL;
		}
		switch(room_known(t, nr)) {
		 case 1:
			break;
		 case 2:
			if (room_guess(t, nr, name))
				break;
		 default:
			cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
			return NULL;
		}
	}
	return nr;
}
#endif /* USE_FLOORS */
