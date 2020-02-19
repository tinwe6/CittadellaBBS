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
* File : floors.c                                                           *
*        Funzioni per creare, cancellare, configurare, gestire i floor.     *
****************************************************************************/
#include "config.h"

#ifdef USE_FLOORS

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "cittaserver.h"
#include "floors.h"
#include "memstat.h"
#include "utility.h"

/* Variabili globali */
struct floor_list floor_list;

/* Prototipi delle funzioni pubbliche in floors.c */
void floor_load_data(void);
void floor_save_data(void);
long floor_create(char *name, long fmnum, long maxmsg, char type);
void floor_delete(struct floor *fl);
void floor_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr);
void floor_change(struct floor *f, struct room *r);
struct floor * floor_find(char *nome);
struct floor * floor_findp(char *nome);
struct floor * floor_findn(long num);
int floor_known(struct sessione *t, struct floor *f);
void floor_free_list(void);
void floor_add_room(struct floor *f, struct room *r);
void floor_remove_room(struct room *r);
void floor_post(long floor_num, long msgnum, time_t ptime);

/* Prototipi delle funzioni statiche in floors.c */
static void floor_save_room(struct floor *fl);
static void floor_load_room_lists(void);
static void floor_load_room(struct floor *fl);
static void floor_free(struct floor *fl);
static void floor_create_basic(void);

/***************************************************************************/
/*
 * Funzione per caricare i dati sui floor.
 */
void floor_load_data(void)
{
        FILE *fp;
        struct floor *fl, *punto;
        int hh;
	long pos = 0;

	log("FLOORS: Caricamento dati floors.");
        punto = NULL;
        floor_list.first = NULL;
        floor_list.last  = NULL;

        fp = fopen(FLOOR_DATA,"r");
        if (!fp) {
                log("SYSERR: no fdata: verra' creato successivamente.");
		floor_create_basic();
		return;
        }
        fseek(fp, 0L, 0);

        CREATE(fl, struct floor, 1, TYPE_FLOOR);
        CREATE(fl->data, struct floor_data, 1, TYPE_FLOOR_DATA);
        CREATE(fl->rooms, struct f_room_list, 1, TYPE_F_ROOM_LIST);
        hh = fread((struct floor_data *) (fl->data),
		   sizeof(struct floor_data), 1, fp);
        if (hh == 1) {
                floor_list.first = fl;
                floor_list.last = fl;
		fl->pos = pos++;
                fl->prev = NULL;
                fl->next = NULL;
		fl->rooms->prev = NULL;
		fl->rooms->next = NULL;
        } else {
                log("SYSTEM: floordata vuoto.");
		floor_create_basic();
		fclose(fp);
		Free(fl->data);
		Free(fl->rooms);
		Free(fl);
		return;
	}

        while (hh==1) {
                if (punto) {
                        punto->next = fl;
                        fl->prev = punto;
			fl->next = NULL;
			fl->pos = pos++;
			fl->rooms->prev = NULL;
			fl->rooms->next = NULL;
			floor_list.last = fl;
                }
                punto = fl;

		CREATE(fl, struct floor, 1, TYPE_FLOOR);
		CREATE(fl->data, struct floor_data, 1, TYPE_FLOOR_DATA);
		CREATE(fl->rooms, struct f_room_list, 1, TYPE_F_ROOM_LIST);
		hh = fread((struct floor_data *) (fl->data),
			   sizeof(struct floor_data), 1, fp);
        }
        fclose(fp);
	Free(fl->data);
	Free(fl->rooms);
	Free(fl);

	/* Carica le informazioni relative alle room nei floor */
	floor_load_room_lists();

	/* Se rimangono room senza floor, le mette nel Basic Floor (DA FARE) */
}

/*
 * Funzione per salvare la lista dei floor.
 */
void floor_save_data(void)
{
	FILE *fp;
	struct floor *punto;
	char buf[LBUF];
	int hh;

	sprintf(buf, "%s.bak", FLOOR_DATA);
	rename(FLOOR_DATA, buf);
	
	fp = fopen(FLOOR_DATA, "w");
        if (!fp) {
                log("SYSERR: Non posso aprire in scrittura floordata.");
                return;
        }
	for (punto = floor_list.first; punto; punto = punto->next) {
		floor_save_room(punto);
		hh = fwrite((struct floor_data *) (punto->data),
			    sizeof(struct floor_data), 1, fp);
		if (hh == 0)
			log("SYSERR: problemi scrittura struct floor_data");
	}
	fclose(fp);
}

/*
 * Funzione per creare un nuovo floor.
 * Restituisce la matricola del floor, o -1 se c'e' stato un problema;
 */
long floor_create(char *name, long fmnum, long maxmsg, char type)
{
	struct floor *fl;
	
	if (fmnum <= 0)
		fmnum = dati_server.fm_normal;
	if (fm_find(fmnum) == NULL)
		return -1;

	if (maxmsg <= 0)
		maxmsg = DFLT_MSG_PER_ROOM;
	
	CREATE(fl, struct floor, 1, TYPE_FLOOR);
	CREATE(fl->data, struct floor_data, 1, TYPE_FLOOR_DATA);
	fl->rooms = NULL;
	
	strncpy(fl->data->name, name, FLOORNAMELEN);
	fl->data->num = ++(dati_server.floor_num);
	fl->data->type = type;
	fl->data->rlvl = LVL_OSPITE;
	fl->data->wlvl = LVL_NORMALE;
	fl->data->fmnum = fmnum;
	fl->data->maxmsg = maxmsg;
	fl->data->highmsg = 0;
	fl->data->highloc = 0;
	fl->data->numpost = 0;
	fl->data->numroom = 0;
	time(&(fl->data->ctime));
	fl->data->mtime = 0;
	fl->data->ptime = 0;
	fl->data->flags = RF_DEFAULT;
	fl->data->master = -1;
		
	fl->next = NULL;
	if (floor_list.first) {
		(floor_list.last)->next = fl;
		fl->prev = floor_list.last;
		fl->pos = (floor_list.last)->pos + 1;
		floor_list.last = fl;
	} else {
		floor_list.first = fl;
		floor_list.last = fl;
		fl->prev = NULL;
		fl->pos = 0;
	}
	(dati_server.floor_curr)++;
	return fl->data->num;
}

/*
 * Funzione per eliminare un floor.
 * Se ci sono ancora delle room all'interno del floor, le sposta prima
 * nel Basic Floor.
 * Trasforma la Floor Lobby in una room normale: potra' cosi' essere
 * successivamente eliminata.
 * Non e' possibile eliminare il Basic Floor.
 */
void floor_delete(struct floor *fl)
{
	struct floor *prev, *next;
	struct f_room_list *frl, *next_frl;
	
	if ((fl == NULL) || (fl == floor_list.first))
		return;

	fl->rooms->room->data->flags &= ~RF_FLOOR;
	fl->rooms->room->data->type = ROOM_NORMAL;
	for (frl = fl->rooms; frl; frl = next_frl) {
		next_frl = frl->next;
		floor_change(floor_list.first, frl->room);
	}
	prev = fl->prev;
	next = fl->next;

	if (prev == NULL)
		floor_list.first = next;
	else
		prev->next = next;
	if (next == NULL)
		floor_list.last = prev;
	else
		next->prev = prev;
	floor_free(fl);
	dati_server.floor_curr--;
}

/* Edita il floor. */
void floor_edit(long num,long rlvl,long wlvl,unsigned long flags,long def_utr)
{
	struct floor *fl;

	if ( (fl = floor_findn(num)) == NULL)
		return;
	fl->data->rlvl = rlvl;
	fl->data->wlvl = wlvl;
	fl->data->flags = flags;
}

/* Floor change: sposta la room r nel floor f */
void floor_change(struct floor *f, struct room *r)
{
	if (f == NULL) 
		return;
	floor_remove_room(r);
	floor_add_room(f, r);
}

/*
 * Restituisce il puntatore alla struttura del floor 'nome'.
 */
struct floor * floor_find(char *nome)
{
	struct floor *punto;

	for (punto = floor_list.first; punto; punto = punto->next)
		if (!strcmp(punto->data->name, nome))
			return punto;
	return NULL;	
}

/*
 * Room find parziale: il nome puo' essere incompleto.
 * Restituisce il puntatore alla struttura room della room 'nome'
 */
struct floor * floor_findp(char *nome)
{
	struct floor *punto;
	long num;
	char *c;
	size_t n;
	
	if (nome[0] == 0)
		return NULL;
	if (isdigit(nome[0])) {
		num = strtol(nome, &c, 10);
		if (*c == 0)
			return floor_findn(atol(nome));
	}
	for (punto = floor_list.first; punto; punto = punto->next)
		if (!strcmp(punto->data->name, nome))
			return punto;
	n = strlen(nome);
	for (punto = floor_list.first; punto; punto = punto->next)
		if (!strncmp(punto->data->name, nome, n))
			return punto;
	return NULL;
}

/*
 * Restituisce il puntatore alla struttura del floor numero 'num'.
 */
struct floor * floor_findn(long num)
{
	struct floor *punto;

	for (punto = floor_list.first; punto; punto = punto->next)
		if (punto->data->num == num)
			return punto;
	return NULL;	
}

/*
 * Libera la memoria delle strutture floor.
 */
void floor_free_list(void)
{
	struct floor *punto, *prossimo;

	for (punto = floor_list.first; punto; punto = prossimo) {
		prossimo = punto->next;
		floor_free(punto);
	}
	floor_list.first = NULL;
	floor_list.last = NULL;
	log("FLOORS: Liberata memoria dinamica dei floors.");
}

/*
 * Floor add room: aggiunge la nuova room r al floor.
 */
void floor_add_room(struct floor *f, struct room *r)
{
	struct f_room_list *frl, *punto;
	
	CREATE(frl, struct f_room_list, 1, TYPE_F_ROOM_LIST);
	frl->room = r;
	frl->next = NULL;
	if (f->rooms) {
		for (punto = f->rooms; punto->next; punto = punto->next);
		frl->prev = punto;
		punto->next = frl;
	} else {
		frl->prev = NULL;
		f->rooms = frl;
	}
	r->floor_num = f->data->num;
	r->f_march = frl;
	f->data->numroom++;
	time(&f->data->mtime);
}

/* Floor remove room: toglie la room r dal suo floor */
void floor_remove_room(struct room *r)
{
	struct floor *f;
	
	f = floor_findn(r->floor_num);
	if (r->f_march->prev)
		r->f_march->prev->next = r->f_march->next;
	else
		f->rooms = r->f_march->next;
	if (r->f_march->next)
		r->f_march->next->prev = r->f_march->prev;
	f->data->numroom--;
	time(&f->data->mtime);
	Free(r->f_march);
	r->f_march = NULL;
}

/***************************************************************************/
/*
 * Salva lista delle room del floor 'fl'.
 */
static void floor_save_room(struct floor *fl)
{
	FILE *fp;
	struct f_room_list *punto;
	char filename[LBUF], bak[LBUF];

	sprintf(filename, "%s%ld", FLOOR_ROOMLIST, fl->data->num);
	sprintf(bak, "%s.bak", filename);
	rename(filename, bak);
	
	fp = fopen(filename, "w");
        if (!fp) {
		logf("SYSERR: Cannot write room list for floor #%ld",
		     fl->data->num);
		rename(bak, filename);
		return;
        }
	for (punto = fl->rooms; punto; punto = punto->next)
		fwrite((long *) &(punto->room->data->num),
		       sizeof(long), 1, fp);
	fclose(fp);
	unlink(bak);
}

/* Carica le liste di room di tutti i floors. */
static void floor_load_room_lists(void)
{
	struct floor *fl;

	log("FLOORS: Carica lista rooms dei floors.");
	for (fl = floor_list.first; fl; fl = fl->next)
		floor_load_room(fl);
}

/* Carica la lista delle room nel floor 'fl'. */
static void floor_load_room(struct floor *fl)
{
	FILE *fp;
	struct f_room_list *punto, *prev;
	long room_num, num = 0;
	struct room *room;
	char filename[LBUF];
	int hh;
	
	sprintf(filename, "%s%ld", FLOOR_ROOMLIST, fl->data->num);
	
	fp = fopen(filename, "r");
        if (!fp) {
                logf("SYSERR: no room list for floor #%ld", fl->data->num);
		return;
        }
        fseek(fp, 0L, 0);

	CREATE(punto, struct f_room_list, 1, TYPE_F_ROOM_LIST);
	punto = fl->rooms;
	prev = NULL;
	room = NULL;
	do {
		hh = fread((long *) (&room_num), sizeof(long), 1, fp);
/* 		logf("Room #%ld", room_num); */
		if (hh == 1)
			room = room_findn(room_num);
		else {
			fclose(fp);
			return;
		}
	} while (room == NULL);

	while (hh == 1) {
	        room = room_findn(room_num);
		if (room) {
			punto->room = room;
			punto->prev = prev;
			punto->next = NULL;
			if (prev)
				prev->next = punto;
			prev = punto;
			/* Mette a posto i riferimenti al floor della room */
			room->floor_num = fl->data->num;
			room->f_march   = punto;
			num++;
		}
		CREATE(punto, struct f_room_list, 1, TYPE_F_ROOM_LIST);
		hh = fread((long *) (&room_num), sizeof(long), 1, fp);
/*		logf("Room #%ld", room_num); */
        }
        fclose(fp);
	Free(punto);
	if (num != fl->data->numroom) {
		logf("FLOOR [%s]: trovate %d room di %d.", fl->data->name,
		     num, fl->data->numroom);
		fl->data->numroom = num;
	}
}

/*
 * Libera la memoria allocata al floor fl.
 */
static void floor_free(struct floor *fl)
{
	struct f_room_list *punto, *next;
	
	for (punto = fl->rooms; punto; punto = next) {
		next = punto->next;
		Free(punto);
	}
	Free(fl->data);
	Free(fl);
}

/*
 * Crea il floor di base e ci piazza tutte le room.
 */
static void floor_create_basic(void)
{
	struct f_room_list *punto, *prev;
	struct floor *fl;
	struct room *room;
	long num;

	log("FLOORS: Crea il floor di base.");
	num = floor_create(FLOOR_NAME_BASIC, dati_server.fm_basic,
			   0, FLOOR_BASIC);
	floor_edit(num, LVL_OSPITE, LVL_AIDE, RF_DEFAULT, UTR_DEFAULT);

	log("FLOORS: Inserisce tutte le room nel floor di base.");
	fl = floor_list.first;
	CREATE(punto, struct f_room_list, 1, TYPE_F_ROOM_LIST);
	fl->rooms = punto;
	prev = NULL;
	for (room = lista_room.first; room; room = room->next) {
		punto->room = room;
		punto->prev = prev;
		punto->next = NULL;
		if (prev)
			prev->next = punto;
		prev = punto;
		room->floor_num = fl->data->num;
		room->f_march = punto;
		fl->data->numroom++;
		CREATE(punto, struct f_room_list, 1, TYPE_F_ROOM_LIST);
	}
	Free(punto);
}

/*
 * Testa se l'utente (t->utente) conosce il floor f.
 * Restituisce 0 se non lo conosce, 1 se lo conosce, 2 se lo conosce
 * ma e' zappato. (NOTA: Per ora non e' possibile zappare i floors)
 */
int floor_known(struct sessione *t, struct floor *f)
{
/*	long i; */
	
	if (f == floor_list.first)
		return 1;
	if (t->utente->livello == LVL_SYSOP)
		return 1;
	if (t->utente->livello >= LVL_AIDE
	    && f->data->rlvl <= LVL_AIDE)
		return 1;

	if (t->utente->livello < f->data->rlvl)
		return 0;
	else
		return 1;

/* Per quando ci saranno i floor flags....
	i = r->pos;
	if (t->room_flags[i] & UTR_KICKOUT)
		return 0;

	if (t->room_flags[i] & UTR_KNOWN) {
		if ((r->data->flags & RF_INVITO) &&
		    !(t->room_flags[i] & UTR_INVITED))
			return 0;
		if (t->room_flags[i] & UTR_ZAPPED)
			return 2;
		else
			return 1;
	}
 */
	return 0;
}

/*
 * Aggiorna i dati sull'ultimo post lasciato in una room del floor.
 * (Numero totale di post, ora e data ultimo post, numero ultimo post.
 */
void floor_post(long floor_num, long msgnum, time_t ptime)
{
	struct floor *floor;

	if ( (floor = floor_findn(floor_num))) {
		floor->data->ptime = ptime;
		floor->data->numpost++;
		floor->data->highmsg = msgnum;
	}
}

/************** FIN QUI E' STATO IMPLEMENTATO TUTTO *************************/
/*
 * Funzione per scambiare di posto a due room nella lista.
 * Restituisce 1 se tutto ok, altrimenti 0.
 */
/*
int floor_swap(long rnum1, long rnum2)
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
	
	if (floor_list.first == r1)
		floor_list.first = r2;
	else {
		if (floor_list.first == r2)
			floor_list.first = r1;
	}
	if (floor_list.last == r1)
		floor_list.last = r2;
	else {
		if (floor_list.last == r2)
			floor_list.last = r1;
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
*/
		
#endif
