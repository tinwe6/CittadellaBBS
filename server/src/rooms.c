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
* File : rooms.c                                                            *
*        Funzioni per la gestione delle room della bbs.                     *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "cestino.h"
#include "cittaserver.h"
#include "fileserver.h"
#include "rooms.h"
#include "mail.h"
#include "memstat.h"
#include "file_messaggi.h"
#include "ut_rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in rooms.c */
void room_load_data(void);
static void room_load_msglists(void);
void room_save_data(void);
static void room_save_msglist(struct room *room);
void room_free_list(void);
void room_free(struct room *punto);
void room_delete(struct room *room);
struct room * room_find(char *nome);
struct room * room_findp(struct sessione *t, char *nome);
long room_create(char *name, long fmnum, long maxmsg, char type);
void room_create_basic(void);
int room_known(struct sessione *t, struct room *r);
int room_guess(struct sessione *t, struct room *r, char *nome);
struct room * room_findn(long num);
void room_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr);
int room_swap(long rnum1, long rnum2);
void room_empty(struct room *r, struct sessione *t);

void msg_load_data(struct msg_list *msg, char *filename, long maxmsg);
void msg_save_data(struct msg_list *msg, char *filename, long maxmsg);
void msg_free(struct msg_list *msg);
void msg_resize(struct room *room, long newsize);
void msg_scroll(struct msg_list *msg, long nslots);
long msg_pos(struct room *room, long msgnum);
long msg_loc(struct room *room, long msgnum);
void msg_del(struct room *room, long msgnum);
void msg_move(struct room *dest, struct room *source, long msgnum);
void msg_copy(struct room *dest, struct room *source, long msgnum);

/* Definizione delle variabili globali */
struct room_list lista_room;

/***************************************************************************/
/*
 * Funzione per caricare i dati sulle room e creare la lista.
 * Se non trova il file dei dati delle room oppure questo e'
 * vuoto, crea la lista di default, con le room di default (Lobby>, etc.)
 */
void room_load_data(void)
{
        FILE *fp;
        struct room *rl, *punto;
        int hh;
	long pos = 0;
	
        punto = NULL;
        lista_room.first = NULL;
        lista_room.last  = NULL;

        fp = fopen(ROOMS_DATA,"r");
        if (!fp) {
                citta_log("SYSERR: no rdata: verra' creato successivamente.");
		room_create_basic();
		return;
        }
        fseek(fp, 0L, 0);

        CREATE(rl, struct room, 1, TYPE_ROOM);
        CREATE(rl->data, struct room_data, 1, TYPE_ROOM_DATA);
        hh = fread((struct room_data *) (rl->data),
		   sizeof(struct room_data), 1, fp);
        if (hh == 1) {
                lista_room.first = rl;
                lista_room.last = rl;
		rl->pos = pos++;
		rl->lock = false;
                rl->prev = NULL;
                rl->next = NULL;
        } else {
                citta_log("SYSTEM: rdata vuoto.");
		room_create_basic();
		fclose(fp);
		Free(rl->data);
		Free(rl);
		return;
	}

        while (hh == 1) {
                if (punto) {
                        punto->next = rl;
                        rl->prev = punto;
			rl->next = NULL;
			rl->pos = pos++;
			rl->lock = false;
			lista_room.last = rl;
                }
                punto = rl;

                CREATE(rl, struct room, 1, TYPE_ROOM);
                CREATE(rl->data, struct room_data, 1, TYPE_ROOM_DATA);
                hh = fread((struct room_data *) (rl->data),
                           sizeof(struct room_data), 1, fp);
        }
        fclose(fp);
	Free(rl->data);
        Free(rl);

	/* Carica le informazioni relative ai messaggi nelle room */
	room_load_msglists();
}

/* Carica la lista dei post nella room */
static void room_load_msglists(void)
{
        struct room *punto;
	/* struct room *dump; */
	struct msg_list *msg;
	char filename[LBUF];
	long maxmsg;

	/* dump = room_find(dump_room); */
	for (punto = lista_room.first; punto; punto = punto->next) {
		if (punto->data->flags & RF_MAIL)
			punto->msg = NULL;
		else {
			/* Crea i vettori */
			maxmsg = punto->data->maxmsg;
                        /* NON USO FM MULTIPLI
			if (punto == dump)
			        msg = msg_create(maxmsg, TRUE);
			else
			        msg = msg_create(maxmsg, FALSE);
                        */
                        msg = msg_create(maxmsg);
			punto->msg = msg;

			sprintf(filename, "%s%ld", FILE_MSGDATA,
				punto->data->num);
			msg_load_data(punto->msg, filename, maxmsg);

			/* TODO QUESTE ANDRANNO TOLTE,
                           dopo la CONVERSIONE */
			msg->highmsg = punto->data->highmsg;
			msg->highloc = punto->data->highloc;
		}
	}
}

/*
 * Funzione per salvare la lista di room.
 */
void room_save_data(void)
{
	FILE *fp;
	struct room *punto;
	char buf[LBUF];
	int hh;
	
	citta_log("Salvataggio dati room.");
	sprintf(buf, "%s.bak", ROOMS_DATA);
	rename(ROOMS_DATA, buf);
	
	fp = fopen(ROOMS_DATA, "w");
        if (!fp) {
                citta_log("SYSERR: Non posso aprire in scrittura roomdata.");
                return;
        }
	for (punto = lista_room.first; punto; punto = punto->next) {
	        room_save_msglist(punto);
		hh = fwrite((struct room_data *) (punto->data),
			    sizeof(struct room_data), 1, fp);
		if (hh == 0)
			citta_log("SYSERR: problemi scrittura struct room_data");
	}
	fclose(fp);
}

/* Salva la lista dei post nella room */
static void room_save_msglist(struct room *room)
{
	char filename[LBUF];
	
	if (room->data->flags & RF_MAIL)
		return;
	sprintf(filename, "%s%ld", FILE_MSGDATA, room->data->num);
	msg_save_data(room->msg, filename, room->data->maxmsg);

	/* TODO queste righe vanno eliminate dopo la conversione */
	room->data->highmsg = room->msg->highmsg;
	room->data->highloc = room->msg->highloc;
}

/*
 * Libera la memoria delle strutture room
 */
void room_free_list(void)
{
	struct room *punto, *prossimo;

	for (punto = lista_room.first; punto; punto = prossimo) {
		prossimo = punto->next;
		room_free(punto);
	}
	lista_room.first = NULL;
	lista_room.last = NULL;
}

/*
 * Libera la memoria di una room
 */
void room_free(struct room *punto)
{
	msg_free(punto->msg);
	Free(punto->data);
	Free(punto);
}

/*
 * Funzione per eliminare una room.
 */
void room_delete(struct room *room)
{
	struct room *prev, *next, *tmp;
	long i;

	room_empty(room, NULL);
	prev = room->prev;
	next = room->next;
	if (prev == NULL)
		lista_room.first = next;
	else
		prev->next = next;
	if (next == NULL)
		lista_room.last = prev;
	else
		next->prev = prev;
#ifdef USE_FLOORS
	floor_remove_room(room);
#endif
	utr_rmslot_all(room->pos);
	/* Sistema room->pos per riallinearsi con gli utr */
	for (i=0, tmp = lista_room.first; tmp->next; i++, tmp = tmp->next)
		tmp->pos = i;

	room_free(room);

	dati_server.room_curr--;
}

/*
 * Restituisce il puntatore alla struttura room della room 'nome'
 */
struct room * room_find(char *nome)
{
	struct room *punto;

	for (punto = lista_room.first; punto; punto = punto->next)
		if (!strcmp(punto->data->name, nome))
			return punto;
	return NULL;	
}

/*
 * Room find parziale: il nome puo' essere incompleto.
 * Restituisce il puntatore alla struttura room della room 'nome'
 */
struct room * room_findp(struct sessione *t, char *nome)
{
	struct room *punto;
	size_t n;
	
	if (nome[0] == 0)
		return NULL;
	if (isdigit(nome[0])) {
	        return room_findn(atol(nome));
	}
	for (punto = lista_room.first; punto; punto = punto->next)
		if (!strcmp(punto->data->name, nome))
			return punto;
	n = strlen(nome);
	for (punto = lista_room.first; punto; punto = punto->next)
		if ((!strncmp(punto->data->name, nome, n))
		    && (room_known(t, punto) == 1))
			return punto;
	return NULL;
}

/*
 * Restituisce il puntatore alla struttura room della room #num.
 */
struct room * room_findn(long num)
{
	struct room *punto;

	for (punto = lista_room.first; punto; punto = punto->next)
		if (punto->data->num == num)
			return punto;
	return NULL;	
}

/*
 * Funzione per creare una nuova room.
 * Restituisce la matricola della room, o -1 se c'e' stato un problema;
 */
long room_create(char *name, long fmnum, long maxmsg, char type)
{
	struct room *room;
	
	if (fmnum <= 0)
		fmnum = dati_server.fm_normal;
	else if (fm_find(fmnum) == NULL)
		return -1;

	if (maxmsg <= 0)
		maxmsg = DFLT_MSG_PER_ROOM;
	
	CREATE(room, struct room, 1, TYPE_ROOM);
	CREATE(room->data, struct room_data, 1, TYPE_ROOM_DATA);
	room->msg = msg_create(maxmsg);
	strncpy(room->data->name, name, ROOMNAMELEN);
	room->data->num = ++(dati_server.room_num);
	room->data->type = type;
	room->data->rlvl = LVL_OSPITE;
	room->data->wlvl = LVL_NORMALE;
	room->data->fmnum = fmnum;
	room->data->maxmsg = maxmsg;
	room->data->highmsg = 0;
	room->data->highloc = 0;
	time(&(room->data->ctime));
	room->data->mtime = 0;
	room->data->ptime = 0;
	room->data->flags = RF_DEFAULT;
	room->data->def_utr = UTR_DEFAULT;
	room->data->master = -1;
		
	room->next = NULL;
	if (lista_room.first) {
		(lista_room.last)->next = room;
		room->prev = lista_room.last;
		room->pos = (lista_room.last)->pos + 1;
		room->lock = false;
		lista_room.last = room;
	} else {
		lista_room.first = room;
		lista_room.last = room;
		room->prev = NULL;
		room->pos = 0;
		room->lock = false;
	}
	(dati_server.room_curr)++;
	if (dati_server.room_curr > dati_server.utr_nslot)
		utr_resize_all(UTR_NSLOTADD);
	utr_chval_all(room->pos, 0L, room->data->def_utr, 0L);
	return room->data->num;
}

/*
 * 
 */
void room_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr)
{
	struct room *r;
	
	r = room_findn(num);
	if (r == NULL)
		return;
	r->data->rlvl = rlvl;
	r->data->wlvl = wlvl;
	r->data->flags = flags;
	r->data->def_utr = def_utr;
}

/*
 * Crea le room di base (Lobby>, etc...) se non esistono
 */
void room_create_basic(void)
{
	long num;
	
	num = room_create(lobby, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_OSPITE, LVL_AIDE, RF_BASE_DEF, UTR_DEFAULT);

	num = room_create(sysop_room, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_SYSOP, LVL_SYSOP, RF_BASE_DEF, UTR_DEFAULT);

	num = room_create(aide_room, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_AIDE, LVL_AIDE, RF_BASE_DEF, UTR_DEFAULT);

	num = room_create(ra_room, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_ROOMAIDE, LVL_ROOMAIDE, RF_BASE_DEF, UTR_DEFAULT);

	num = room_create(mail_room, dati_server.fm_mail, MAIL_SLOTNUM,
                          ROOM_BASIC);
	room_edit(num, LVL_NORMALE, LVL_NORMALE, RF_BASE_DEF | RF_MAIL,
		  UTR_DEFAULT);
	dati_server.mailroom = num;

	num = room_create(twit_room, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_SYSOP, LVL_SYSOP+1, RF_BASE_DEF, UTR_DEFAULT);

	num = room_create(dump_room, dati_server.fm_basic, 0, ROOM_BASIC);
	room_edit(num, LVL_SYSOP, LVL_SYSOP+1, RF_BASE_DEF, UTR_DEFAULT);
}

/*
 * Funzione per scambiare di posto a due room nella lista.
 * Restituisce 1 se tutto ok, altrimenti 0.
 */
int room_swap(long rnum1, long rnum2)
{
	struct room *r1, *r2, *rtmp;
	long tmp;
	
	if (rnum1 == rnum2)
		return 1;
	if ((r1 = room_findn(rnum1)) == NULL)
		return 0;
	if ((r2 = room_findn(rnum2)) == NULL)
		return 0;
	
	utr_swap_all(r1->pos, r2->pos);

	tmp = r1->pos;
	r1->pos = r2->pos;
	r2->pos = tmp;
	
	if (lista_room.first == r1)
		lista_room.first = r2;
	else {
		if (lista_room.first == r2)
			lista_room.first = r1;
	}
	if (lista_room.last == r1)
		lista_room.last = r2;
	else {
		if (lista_room.last == r2)
			lista_room.last = r1;
	}
	if (r1->next == r2) {
		if (r1->prev)
			r1->prev->next = r2;
		if (r2->next)
			r2->next->prev = r1;
		r1->next = r2->next;
		r2->prev = r1->prev;
		r2->next = r1;
		r1->prev = r2;
	} else if (r2->next == r1) {
		if (r2->prev)
			r2->prev->next = r1;
		if (r1->next)
			r1->next->prev = r2;
		r2->next = r1->next;
		r1->prev = r2->prev;
		r2->prev = r1;
		r1->next = r2;
	} else {
		if (r1->prev)
			r1->prev->next = r2;
		if (r2->prev)
			r2->prev->next = r1;
		if (r1->next)
			r1->next->prev = r2;
		if (r2->next)
			r2->next->prev = r1;
		rtmp = r1->prev;
		r1->prev = r2->prev;
		r2->prev = rtmp;
		rtmp = r1->next;
		r1->next = r2->next;
		r2->next = rtmp;
	}
	return 1;
}

/*
 * Testa se l'utente (t->utente) conosce la room r.
 * Restituisce 0 se non la conosce, 1 se la conosce, 2 se la conosce
 * ma e' zappata.
 */
int room_known(struct sessione *t, struct room *r)
{
	long i;

	if (r == NULL)
	        return 0; /* La room non esiste */
	if (r->data->num == 1)
		return 1; /* La Lobby) va sempre bene! */
	if (t->utente->livello == LVL_SYSOP)
		return 1;
	if ((t->utente->livello >= LVL_AIDE)
	    && (t->utente->livello >= r->data->rlvl))
		return 1;
	
	/* Livello minimo per l'accesso */
	if (t->utente->livello < r->data->rlvl)
		return 0;

	i = r->pos;
	
	if (!(r->data->flags & RF_ENABLED)
	    && !(t->room_flags[i] & UTR_ROOMAIDE)
	    && !(t->room_flags[i] & UTR_ROOMHELPER))
		return 0; /* La room non e` attiva */

	if (t->room_flags[i] & UTR_KICKOUT)
		return 0;

	if (t->room_flags[i] & UTR_KNOWN) {
		if ((r->data->flags & RF_INVITO) &&
		    !(t->room_flags[i] & UTR_INVITED))
			return 0; /* L'utente non ha l'invito */
		if (t->room_flags[i] & UTR_ZAPPED)
			return 2; /* Room zappato */
		else
			return 1; /* Ok! conosce la room. **/
	}
	return 0;
}

/*
 * Testa se l'utente (t->utente) conosce la room r.
 * Restituisce 0 se non la conosce, 1 se la conosce, 2 se la conosce
 * ma e' zappata.
 */
int room_guess(struct sessione *t, struct room *r, char *nome)
{
/*	long i; */

/*	if ((r->data->flags & RF_GUESS)  */
	if ((t->utente->livello >= r->data->rlvl)
	    && (!strcmp(r->data->name, nome))) {
		if (t->room_flags[r->pos] & UTR_KICKOUT)
			return 0;
		return 1;
	}
	return 0;
}

/*
 * Caccia tutti gli utenti nella room *r e li manda in Lobby), tranne
 * l'utente con sessione *t. Se t == NULL caccia tutti.
 * Sistema anche i puntatori next/prev delle room virtuali.
 */
void room_empty(struct room *r, struct sessione *t)
{
	struct sessione *s;
	
	for (s = lista_sessioni; s; s = s->prossima) {
		if ((s != t) && (s->room == r)) {
			s->room = lista_room.first;
                        cprintf(s, "%d\n", LOBD);
		} else if (s->room->data->type == ROOM_DYNAMICAL) {
		        if (s->room->prev == r)
			        s->room->prev = r->prev;
			if (s->room->next == r)
			        s->room->next = r->next;
		}
	}
}

/****************************************************************************/
/*
 * Crea una struttura msg in grado di contenere le informazioni su
 * maxmsg messaggi.
 * Se fmnum e' TRUE, crea anche un vettore per il file message
 */
struct msg_list * msg_create(long maxmsg)
{
	struct msg_list *msg;
	int i;

	CREATE(msg, struct msg_list, 1, TYPE_MSG_LIST);
	CREATE(msg->num, long, maxmsg, TYPE_LONG);
	CREATE(msg->pos, long, maxmsg, TYPE_LONG);
	CREATE(msg->local, long, maxmsg, TYPE_LONG);
        /* NON USO FM multipli
	if (fmnum)
	        CREATE(msg->fmnum, long, maxmsg, TYPE_LONG);
	else
	        msg->fmnum = NULL;
        */
	for (i = 0; i < maxmsg; i++) {
		msg->num[i] = -1L;
		msg->pos[i] = -1L;
		msg->local[i] = -1L;
        /* NON USO FM multipli
		if (msg->fmnum)
		        msg->fmnum[i] = -1L;
        */
	}
        msg->highmsg = 0;
        msg->highloc = 0;
        msg->last_poster = NO_USER;
	msg->dfr = NULL;
	msg->dirty = false;
	return msg;
}

/*
 * Carica le informazioni sui post nelle room
 */
void msg_load_data(struct msg_list *msg, char *filename, long maxmsg)
{
        FILE *fp;

	/* Legge da file i vettori */
	fp = fopen(filename, "r");
	if (!fp) {
	        citta_logf("SYSERR: non trovo msglist %s", filename);
		return; /* Da mettere a posto */
	}
	fread(msg->pos, sizeof(long), maxmsg, fp);
	fread(msg->num, sizeof(long), maxmsg, fp);
	fread(msg->local, sizeof(long), maxmsg, fp);
        /* NON USO FM multipli
	if (msg->fmnum)
	        fread(msg->fmnum, sizeof(long), maxmsg, fp);
        */
	fread(&msg->highmsg, sizeof(long), 1, fp);
	fread(&msg->highloc, sizeof(long), 1, fp);
	fread(&msg->last_poster, sizeof(long), 1, fp);
	fclose(fp);
	msg->dirty = false;
}

/*
 * Salva le informazioni sui post nelle room
 */
void msg_save_data(struct msg_list *msg, char *filename, long maxmsg)
{
	FILE *fp;
	char bak[LBUF];
	
	if (!msg->dirty)
	        return;
	sprintf(bak, "%s.bak", filename);
	rename(filename, bak);

	fp = fopen(filename, "w");
        if (!fp) {
		citta_logf("SYSERR: Cannot write msglist %s", filename);
		rename(bak, filename);
		return;
        }
	fwrite(msg->pos, sizeof(long), maxmsg, fp);
	fwrite(msg->num, sizeof(long), maxmsg, fp);
	fwrite(msg->local, sizeof(long), maxmsg, fp);
        /* NON USO FM multipli
	if (msg->fmnum)
	        fwrite(msg->fmnum, sizeof(long), maxmsg, fp);
        */
	/* TODO questi li salviamo, nella prossima versione li eliminiamo
	   dalla struttura room                                      */
	fwrite(&msg->highmsg, sizeof(long), 1, fp);
	fwrite(&msg->highloc, sizeof(long), 1, fp);
	fwrite(&msg->last_poster, sizeof(long), 1, fp);

	fclose(fp);
	unlink(bak);

	msg->dirty = false;
}

/*
 * Libera memoria della lista messaggi
 */
void msg_free(struct msg_list *msg)
{
	if (!msg)
		return;
	if (msg->dirty) /* Questo non dovrebbe MAI succedere! */
	        citta_log("ERROR: freeing dirty msg_list!");
	Free(msg->num);
	Free(msg->pos);
	Free(msg->local);
        /* NON USO FM multipli
	if (msg->fmnum)
	        Free(msg->fmnum);
        */
	Free(msg);
}

/*
 * Ridimensiona il numero di messaggi nella room
 */
void msg_resize(struct room *room, long newsize)
{
	struct msg_list *msg;
	long i, j;

	if (newsize <= 0)
		return;

        /* NON USO FM multipli
	msg = msg_create(newsize, (room->msg->fmnum != NULL));
        */
	msg = msg_create(newsize);
	for(i = room->data->maxmsg-1, j = newsize-1;
	    (i >= 0) && (j >= 0); i--, j--) {
		msg->num[j] = room->msg->num[i];
		msg->pos[j] = room->msg->pos[i];
		msg->local[j] = room->msg->local[i];
        /* NON USO FM multipli
		if (room->msg->fmnum)
		        msg->fmnum[j] = room->msg->fmnum[i];
        */
	}
	msg->dirty = true;
	msg_free(room->msg);
	room->msg = msg;

#ifdef USE_CESTINO
	/* Sistema il dump_undelete se e` il cestino */
	if (!strcmp(room->data->name, dump_room))
		msg_dump_resize(room->data->maxmsg, newsize);
#endif

	room->data->maxmsg = newsize;
}

/*
 * Scrolla di uno i messaggi nella lista.
 */
void msg_scroll(struct msg_list *msg, long nslots)
{
	long i;

        fs_delete_file(msg->num[0]);
	for (i = 0; i < (nslots-1); i++) {
		msg->num[i] = msg->num[i+1];
		msg->pos[i] = msg->pos[i+1];
		msg->local[i] = msg->local[i+1];
        /* NON USO FM multipli
		if (msg->fmnum)
		        msg->fmnum[i] = msg->fmnum[i+1];
        */
	}
	msg->dirty = true;
}

/*
 * Trova la posizione del messaggio #msgnum nella room *room.
 * Restituisce -1L se il messaggio non viene trovato
 */
long msg_pos(struct room *room, long msgnum)
{
	long i;

	for (i = 0; i < room->data->maxmsg; i++)
		if (room->msg->num[i] == msgnum)
			return room->msg->pos[i];
	return -1L;
}

/*
 * Trova il local number del messaggio #msgnum nella room *room.
 * Restituisce -1L se il messaggio non viene trovato
 */
long msg_loc(struct room *room, long msgnum)
{
	long i;

	for (i = 0; i < room->data->maxmsg; i++)
		if (room->msg->num[i] == msgnum)
			return room->msg->local[i];
	return -1L;
}

/*
 * Cancella il messaggio msgnum nella room *room: lo elimina dalla
 * lista dei messaggi della room e lo cancella dal file messaggi.
 */
void msg_del(struct room *room, long msgnum)
{
        long msgpos, fmnum;
	long pos, i;

	for (i = 0; i < room->data->maxmsg; i++)
		if (room->msg->num[i] == msgnum) {
			pos = i;
			msgpos = room->msg->pos[i];
			/* locnum = room->msg->local[i]; */
        /* NON USO FM multipli
			if (room->data->fmnum == -1)
			        fmnum = room->msg->fmnum[i];
			else
			        fmnum = room->data->fmnum;
        */
                        fmnum = room->data->fmnum;
			for (i = pos; i > 0; i--) {
				room->msg->num[i] = room->msg->num[i-1];
				room->msg->pos[i] = room->msg->pos[i-1];
				room->msg->local[i] = room->msg->local[i-1];
        /* NON USO FM multipli
				if (room->msg->fmnum)
				  room->msg->fmnum[i] = room->msg->fmnum[i-1];
        */
			}
			room->msg->num[0] = -1L;
			room->msg->pos[0] = -1L;
			room->msg->local[0] = -1L;
        /* NON USO FM multipli
			if (room->msg->fmnum)
			        room->msg->fmnum[0] = -1L;
        */
			room->msg->dirty = true;
			if (!(room->data->flags & RF_MAIL))
				fm_delmsg(fmnum, msgnum, msgpos);

                        /* Elimino gli eventuali file associati */
                        fs_delete_file(msgnum);

			return;
		}
}

/*
 * Sposta un post.
 * TODO: se sposto tra room in fm diversi va spostato tutto il post
 */
void msg_move(struct room *dest, struct room *source, long msgnum)
{
	long msgpos, locnum;
	long pos, i;

	for (i = 0; i < source->data->maxmsg; i++)
		if (source->msg->num[i] == msgnum) {
			pos = i;
			msgpos = source->msg->pos[i];
			locnum = source->msg->local[i];
			for (i = pos; i > 0; i--) {
				source->msg->num[i] = source->msg->num[i-1];
				source->msg->pos[i] = source->msg->pos[i-1];
				source->msg->local[i] = source->msg->local[i-1];
			}
			source->msg->num[0] = -1L;
			source->msg->pos[0] = -1L;
			source->msg->local[0] = -1L;
			source->msg->dirty = true;
			i = 1;
			do {
				dest->msg->num[i-1]= dest->msg->num[i];
				dest->msg->pos[i-1]= dest->msg->pos[i];
				dest->msg->local[i-1]= dest->msg->local[i];
				i++;
			} while ((i < dest->data->maxmsg)
                                 && (dest->msg->num[i] < msgnum));
			dest->msg->num[i-1]= msgnum;
			dest->msg->pos[i-1]= msgpos;
			dest->msg->local[i-1]= locnum;
			dest->msg->dirty = true;
			return;
		}
}

/*
 * Copia un post.
 * TODO: se copio tra room in fm diversi va spostato tutto il post
 */
void msg_copy(struct room *dest, struct room *source, long msgnum)
{
	long msgpos, locnum;
	long i;

	for (i = 0; i < source->data->maxmsg; i++)
		if (source->msg->num[i] == msgnum) {
			msgpos = source->msg->pos[i];
			locnum = source->msg->local[i];
			i = 1;
			do {
				dest->msg->num[i-1]= dest->msg->num[i];
				dest->msg->pos[i-1]= dest->msg->pos[i];
				dest->msg->local[i-1]= dest->msg->local[i];
				i++;
			} while ((i < dest->data->maxmsg)
                                 && (dest->msg->num[i] < msgnum));
			dest->msg->num[i-1]= msgnum;
			dest->msg->pos[i-1]= msgpos;
			dest->msg->local[i-1]= locnum;
			dest->msg->dirty = true;
                        fs_addcopy(msgnum);
			return;
		}
}

/***************************************************************************/
long get_highloc(struct sessione *t)
{
        return get_highloc_room(t, t->room);
}

long get_highloc_room(struct sessione *t, struct room *r)
{
        if (r->data->flags & RF_MAIL)
                return t->mail->highloc;
        return r->msg->highloc;
}

void set_highloc(struct sessione *t, long highloc)
{
        if (t->room->data->flags & RF_MAIL)
                t->mail->highloc = highloc;
        else
                t->room->msg->highloc = highloc;
}

long inc_highloc(struct sessione *t)
{
        if (t->room->data->flags & RF_MAIL)
                return (t->mail->highloc)++;
        return (t->room->msg->highloc)++;
}

long get_highmsg(struct sessione *t)
{
        return get_highmsg_room(t, t->room);
}

long get_highmsg_room(struct sessione *t, struct room *r)
{
        if (r->data->flags & RF_MAIL)
                return t->mail->highmsg;
        return r->msg->highmsg;
}

void set_highmsg(struct sessione *t, long highmsg)
{
        if (t->room->data->flags & RF_MAIL)
                t->mail->highmsg = highmsg;
        else
                t->room->msg->highmsg = highmsg;
}
