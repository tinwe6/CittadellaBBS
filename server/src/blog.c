/*
 *  Copyright (C) 2004-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                      (C) M. Caldarelli *
*                                                                             *
* File: blog.c                                                                *
*       funzioni per la gestione delle room personali (blog)                  *
******************************************************************************/
#include "blog.h"

#ifdef USE_BLOG
#include <string.h>
#include <unistd.h>

#include "cittaserver.h"
#include "memstat.h"
#include "extract.h"
#include "file_messaggi.h"
#include "march.h"
#include "utility.h"

blog_t * blog_first;
blog_t * blog_last;

/* Prototipi funzioni statiche in questo file. */
static blog_t * blog_load(struct dati_ut *ut);
static void blog_save_data(blog_t *blog);
static blog_t * blog_cr8(struct dati_ut *ut);
static void blog_free(blog_t *blog);
static void blog_link(blog_t *blog);
static unsigned long blog_newmsg(blog_t *blog, long user,
                                 unsigned long *nummsg);
static dfr_t * dfr_create(void);
static void dfr_free(dfr_t *dfr);
static void dfr_load(dfr_t *dfr, long user);
static void dfr_init(dfr_t *dfr);
static void dfr_save(dfr_t *dfr, long user);
static void dfr_expand(dfr_t *dfr);
static void dfr_clean(dfr_t *dfr, long lowest);

/*****************************************************************************/
void blog_init(void)
{
        struct lista_ut *ut;

        blog_first = NULL;
	blog_last = NULL;

        /* File messaggi unico */
        dati_server.fm_blog = dati_server.fm_normal;
        dati_server.blog_nslot = BLOG_SLOTNUM;/*TODO Da eliminare dopo upgrade*/

	/* Se non esiste il file messaggi associato ai blog, lo crea */
	if (fm_find(dati_server.fm_blog) == NULL)
                citta_log("Blog: file messaggi non trovato. Problema serio: NOTIFICALO.");

        citta_log("Blog: Caricamento dati.");
        for (ut = lista_utenti; ut; ut = ut->prossimo) {
                ut->blog = blog_load(ut->dati);
                if (ut->blog)
                        blog_link(ut->blog);
        }
}

void blog_close(void)
{
	blog_t *blog, *next;

	citta_log("Salvataggio dati blog");
	blog_save();

	citta_log("Libero strutture dati blog");
	blog = blog_first;
	while (blog) {
                next = blog->next;
                blog_free(blog);
                blog = next;
        }
        citta_log("Fine liberazione blog");
}

/* Salva i dati di tutti i blog */
void blog_save(void)
{
        blog_t *blog;

        for (blog = blog_first; blog; blog = blog->next)
                blog_save_data(blog);
}

/* TRUE se l'utente ut puo' postare nella blog room r */
/* TODO: Aggiiungere check su dfr->flags */
bool blog_post(struct room *r, struct dati_ut *ut)
{
        struct dati_ut *utb;

        if (r->data->num == ut->matricola)
                return true;
        utb = trova_utente_n(r->data->num);
        if (utb->flags[7] & UT_BLOG_ALL) {
                if ((utb->flags[7] & UT_BLOG_ENEMIES)
                    && (is_enemy(ut, utb)))
                        return false;
                return true;
        }
        if ((utb->flags[7] & UT_BLOG_FRIENDS) && (is_friend(ut, utb)))
                return true;
        return false;
}

/* Trova i dati del blog associati all'utente con dati *ut */
/* Ritorna NULL se l'utente non ha attivato il blog        */
blog_t * blog_find(struct dati_ut *ut)
{
        blog_t * blog;

	if ((ut == NULL) || !(ut->flags[7] & UT_BLOG_ON))
                return NULL; /* L'utente non ha blog attivo */

	for (blog = blog_first; blog; blog = blog->next) {
                if (blog->user == ut)
	                return blog;
	}

        /* Prima volta che viene usato il blog di *ut: crea dati */
        return blog_cr8(ut);
}

/* Cambia il nome della blogroom se l'utente cambia nickname */
void blog_newnick(struct dati_ut *ut, char *newnick)
{
        blog_t *blog;

        blog = blog_find(ut);
        if (blog)
                strncpy(blog->room_data->name, newnick, ROOMNAMELEN);
}


/* Restituisce il numero di post presenti nel blog b */
/* TODO questo numerillo potrebbe venir generato al boot e tenuto nella
 * struttura blog_t             */
long blog_num_post(blog_t *b)
{
        long nummsg = 0;
        long lowest = fm_lowest(b->room_data->fmnum);
	int i;

	for (i = 0; i < b->room_data->maxmsg; i++)
		if (b->msg->num[i] >= lowest)
			nummsg++;
        return nummsg;
}

/****************************************************************************/
/* Invia la lista degli utenti con un blog e la data dell'ultimo post */
void cmd_blgl(struct sessione *t)
{
        blog_t *blog;
        const char *last_poster;
        struct dati_ut *ut;
        unsigned long newmsg, nummsg;

        cprintf(t, "%d Lista blog attivi\n", SEGUE_LISTA);

        for (blog = blog_first; blog; blog = blog->next) {
                ut = trova_utente_n(blog->msg->last_poster);
                if (ut)
                        last_poster = ut->nome;
                else
                        last_poster = empty_string;
                newmsg = blog_newmsg(blog, t->utente->matricola, &nummsg);
                cprintf(t, "%d %s|%s|%ld|%ld|%ld|%ld\n", OK, blog->user->nome,
                        last_poster, blog->room_data->ptime,
                        blog->msg->highloc, newmsg, nummsg);
        }
	cprintf(t, "000\n");
}

/****************************************************************************/
/* Aggiungi un blog in fondo alla lista */
static void blog_link(blog_t *blog)
{
        if (blog_first)
                blog_last->next = blog;
        else
                blog_first = blog;
        blog_last = blog;
        blog->next = NULL;
}

/* Carica i dati per il blog dell'utente ut, restituisce puntatore alla
 * lista dei messaggi nel blog                                          */
static blog_t * blog_load(struct dati_ut *ut)
{
	blog_t *blog;
	char filename[LBUF];
        FILE *fp;
        size_t hh;

	if ((ut == NULL) || !(ut->flags[7] & UT_BLOG_ON))
                return NULL; /* L'utente non ha blog attivo */

	CREATE(blog, blog_t, 1, TYPE_BLOG);

	sprintf(filename, "%s%ld", FILE_BLOGROOM, ut->matricola);
        fp = fopen(filename, "r");
        if (!fp) {
                Free(blog);
                blog = blog_cr8(ut);
                return blog;
        }

        CREATE(blog->room_data, struct room_data, 1, TYPE_ROOM_DATA);
        hh = fread((struct room_data *) blog->room_data,
                   sizeof(struct room_data), 1, fp);
        fclose(fp);
        if (hh != 1) {
                Free(blog->room_data);
                Free(blog);
                blog = blog_cr8(ut);
                return blog;
        }
	blog->msg = msg_create(dati_server.blog_nslot);
	sprintf(filename, "%s%ld", FILE_BLOG, ut->matricola);
	msg_load_data(blog->msg, filename, dati_server.blog_nslot);

        blog->msg->dfr = dfr_create();
        dfr_load(blog->msg->dfr, ut->matricola);
	blog->user = ut;
        blog->dirty = false;
        blog_link(blog);

	return blog;
}

/* Crea strutture blog per utente *ut */
static blog_t * blog_cr8(struct dati_ut *ut)
{
	blog_t *blog;

        if (ut == NULL)
                return NULL;

        citta_logf("Blog: Creo struttura dati blog per [%s]", ut->nome);

	CREATE(blog, blog_t, 1, TYPE_BLOG);
	blog->msg = msg_create(dati_server.blog_nslot);
        blog->msg->dfr = dfr_create();
	blog->user = ut;

        /* Create room data */
        CREATE(blog->room_data, struct room_data, 1, TYPE_ROOM_DATA);
	blog->room_data->num = ut->matricola;
        strncpy(blog->room_data->name, ut->nome, ROOMNAMELEN);
        blog->room_data->type = ROOM_DYNAMICAL;
        blog->room_data->rlvl = LVL_NORMALE;
        blog->room_data->wlvl = LVL_NORMALE;
        blog->room_data->fmnum = dati_server.fm_blog;
	blog->room_data->maxmsg = dati_server.blog_nslot;
        time(&(blog->room_data->ctime));
        blog->room_data->mtime = 0;
        blog->room_data->ptime = 0;
        blog->room_data->flags = RF_DEFAULT | RF_BLOG;
        blog->room_data->def_utr = UTR_DEFAULT;
        blog->room_data->master = ut->matricola; /* owner is RA */
        blog->room_data->highmsg = 0;
        blog->room_data->highloc = 0;

        blog->dirty = true;
        blog_link(blog);
	return blog;
}

static void blog_save_data(blog_t *blog)
{
        char filename[LBUF], backup[LBUF];
        FILE *fp;
        int hh;

        /* TODO rendere pienamente funzionante il blog->dirty */
        //if (!blog->dirty)
        //      return;

        sprintf(filename, "%s%ld", FILE_BLOG, blog->user->matricola);
        msg_save_data(blog->msg, filename, dati_server.blog_nslot);
        dfr_save(blog->msg->dfr, blog->user->matricola);

        sprintf(filename, "%s%ld", FILE_BLOGROOM, blog->user->matricola);
        sprintf(backup, "%s.bak", filename);
        rename(filename, backup);

        fp = fopen(filename, "w");
        if (!fp) {
                citta_logf("SYSERR: Non posso aprire in scrittura %s", filename);
                rename(backup, filename);
                return;
        }
        hh = fwrite((struct room_data *)(blog->room_data),
                    sizeof(struct room_data), 1, fp);
        if (hh == 0)
                citta_logf("SYSERR: Problemi di scrittura %s", filename);
        fclose(fp);
        unlink(backup);

        blog->dirty = false;
}

static void blog_free(blog_t *blog)
{
        
        dfr_free(blog->msg->dfr);
        msg_free(blog->msg);
        Free(blog->room_data);
	Free(blog);
}

/* TODO unificare con analoga routine in march.c */
static unsigned long blog_newmsg(blog_t *blog, long user,
                                 unsigned long *nummsg)
{
        unsigned long i, newmsg = 0;
        long lowest, fullroom;

        *nummsg = 0;
        fullroom = dfr_getfullroom(blog->msg->dfr, user);
	lowest = fm_lowest(blog->room_data->fmnum);

	for (i = 0; i < blog->room_data->maxmsg; i++)
		if (blog->msg->num[i] >= lowest) {
			(*nummsg)++;
			if (blog->msg->num[i] > fullroom)
				newmsg++;
		}

        return newmsg;
}

/****************************************************************************/
#define DFR_ALLOC_STEP 2

void dfr_setfullroom(dfr_t *dfr, long user, long msgnum)
{
        int i;
  
        if (!dfr)
                return;

	dfr->dirty = true;
	for (i = 0; i < dfr->num; i++) {
	        if (dfr->user[i] == user) {
		        dfr->fr[i] = msgnum;
			return;
		} else if (dfr->fr[i] > user) {
		        dfr_expand(dfr);
		        memmove((dfr->user)+i+1, (dfr->user)+i, dfr->num-i);
		        memmove((dfr->fr)+i+1, (dfr->fr)+i, dfr->num-i);
		        memmove((dfr->flags)+i+1, (dfr->flags)+i, dfr->num-i);
			dfr->user[i] = user;
			dfr->fr[i] = msgnum;
			dfr->flags[i] = UTR_DEFAULT;
			(dfr->num)++;
			return;
		}
	}
	dfr_expand(dfr);
	dfr->user[dfr->num] = user;
	dfr->fr[dfr->num] = msgnum;
	dfr->flags[dfr->num] = UTR_DEFAULT;
	(dfr->num)++;
}

void dfr_setflags(dfr_t *dfr, long user, unsigned long flags)
{
        int i;
  
        if (!dfr)
                return;

	dfr->dirty = true;
	for (i = 0; i < dfr->num; i++) {
	        if (dfr->user[i] == user) {
		        dfr->flags[i] = flags;
			return;
		} else if (dfr->fr[i] > user) {
		        dfr_expand(dfr);
		        memmove((dfr->user)+i+1, (dfr->user)+i, dfr->num-i);
		        memmove((dfr->fr)+i+1, (dfr->fr)+i, dfr->num-i);
		        memmove((dfr->flags)+i+1, (dfr->flags)+i, dfr->num-i);
			dfr->user[i] = user;
			dfr->fr[i] = -1L;
			dfr->flags[i] = flags;
			(dfr->num)++;
			return;
		}
	}
	dfr_expand(dfr);
	dfr->user[dfr->num] = user;
	dfr->fr[dfr->num] = -1L;
	dfr->flags[dfr->num] = flags;
	(dfr->num)++;
}

void dfr_set_flag(dfr_t *dfr, long user, unsigned long flag)
{
        int i;
  
        if (!dfr)
                return;

	dfr->dirty = true;
	for (i = 0; i < dfr->num; i++) {
	        if (dfr->user[i] == user) {
		        dfr->flags[i] |= flag;
			return;
		}
	}
        dfr_setflags(dfr, user, UTR_DEFAULT | flag);
}

void dfr_clear_flag(dfr_t *dfr, long user, unsigned long flag)
{
        int i;
  
        if (!dfr)
                return;

	dfr->dirty = true;
	for (i = 0; i < dfr->num; i++) {
	        if (dfr->user[i] == user) {
		        dfr->flags[i] &= ~flag;
			return;
		}
	}
        dfr_setflags(dfr, user, UTR_DEFAULT | flag);
}

void dfr_setf_all(dfr_t *dfr, unsigned long flags)
{
        int i;
  
        if (!dfr)
                return;

	dfr->dirty = true;
	for (i = 0; i < dfr->num; i++)
                dfr->flags[i] |= flags;
}

long dfr_getfullroom(dfr_t *dfr, long user)
{
        int i;

        if (!dfr)
                return -1L;

	for (i = 0; i < (dfr->num); i ++)
	        if (dfr->user[i] == user)
		        return dfr->fr[i];
	return -1L;
}

unsigned long dfr_getflags(dfr_t *dfr, long user)
{
        int i;

        if (!dfr)
                return -1L;

	for (i = 0; i < (dfr->num); i ++)
	        if (dfr->user[i] == user)
		        return dfr->flags[i];
	return -1L;
}

/*************************************************************************/
static dfr_t * dfr_create(void)
{
        dfr_t *dfr;

	CREATE(dfr, dfr_t, 1, TYPE_DFR);
	dfr->num = 0;
	dfr->alloc = 0;
        dfr->user = NULL;
        dfr->fr = NULL;
        dfr->flags = NULL;
        dfr->dirty = false;

	return dfr;
}

/* Libera memoria associata al dfr */
static void dfr_free(dfr_t *dfr)
{
        if (dfr->dirty) /* Non dovrebbe accadere mai!! */
                citta_log("SYSERR: Freeing dirty DFR!");
        Free(dfr->user);
        Free(dfr->fr);
        Free(dfr->flags);
        Free(dfr);
}

/* Carica il dfr associato al blog dell'utente user */
static void dfr_load(dfr_t *dfr, long user)
{
        char filename[LBUF];
	FILE *fp;

	sprintf(filename, "%s%ld", FILE_BLOGDFR, user);

	/* Legge da file i vettori */
	fp = fopen(filename, "r");
	if (!fp) {
	        citta_logf("SYSERR: non trovo dfr %s, verra' creato.", filename);
                dfr_init(dfr);
		return;
	}
	fread(&dfr->num, sizeof(long), 1, fp);
	if (dfr->num > 0) {
	        CREATE(dfr->user, long, dfr->num, TYPE_LONG);
		fread(dfr->user, sizeof(long), dfr->num, fp);
	        CREATE(dfr->fr, long, dfr->num, TYPE_LONG);
		fread(dfr->fr, sizeof(long), dfr->num, fp);
	        CREATE(dfr->flags, unsigned long, dfr->num, TYPE_LONG);
		fread(dfr->flags, sizeof(unsigned long), dfr->num, fp);
	} else {
                dfr_init(dfr);
                return;
        }
	fclose(fp);
        dfr->alloc = dfr->num;
	dfr->dirty = false;
}

static void dfr_init(dfr_t *dfr)
{
        CREATE(dfr->user, long, DFR_ALLOC_STEP, TYPE_LONG);
        CREATE(dfr->fr, long, DFR_ALLOC_STEP, TYPE_LONG);
        CREATE(dfr->flags, unsigned long, DFR_ALLOC_STEP, TYPE_LONG);
        dfr->alloc = DFR_ALLOC_STEP;
        dfr->num = 0;
        dfr->dirty = false;
}

static void dfr_save(dfr_t *dfr, long user)
{
        FILE *fp;
        char filename[LBUF], bak[LBUF];

	if (!dfr->dirty)
	        return;

        dfr_clean(dfr, fm_lowest(dati_server.fm_blog));

	sprintf(filename, "%s%ld", FILE_BLOGDFR, user);
	sprintf(bak, "%s.bak", filename);
	rename(filename, bak);

	fp = fopen(filename, "w");
        if (!fp) {
		citta_logf("SYSERR: Cannot write dfr %s", filename);
		rename(bak, filename);
		return;
        }
	fwrite(&dfr->num, sizeof(long), 1, fp);
	fwrite(dfr->user, sizeof(long), dfr->num, fp);
	fwrite(dfr->fr, sizeof(long), dfr->num, fp);
	fwrite(dfr->flags, sizeof(unsigned long), dfr->num, fp);

	fclose(fp);
	unlink(bak);

	dfr->dirty = false;
}

static void dfr_expand(dfr_t *dfr)
{
        if (dfr->num == dfr->alloc) {
	        dfr->alloc += DFR_ALLOC_STEP;
		RECREATE(dfr->user, long, dfr->alloc);
		RECREATE(dfr->fr, long, dfr->alloc);
		RECREATE(dfr->flags, unsigned long, dfr->alloc);
	}
}

/* Elimina lo slot di un utente inesistente */
/* TODO : da scrivere! */
static void dfr_clean(dfr_t *dfr, long lowest)
{
        /*
        int i;

	for (i = 0; i < dfr->num; i += 2) {
	        if (dfr->fr[i+1] <= lowest) {
		        memmove(dfr->fr+i, dfr->fr+i+2, dfr->num*2-i-2);
			(dfr->num)--;
			dfr->dirty = TRUE;
			i -= 2;
		}
	}
        */
}

#endif /* USE_BLOG */
