/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File : mail.c                                                             *
*        Funzioni per la gestione dei mail.                                 *
****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mail.h"
#include "cittaserver.h"
#include "memstat.h"
#include "user_flags.h"
#include "utility.h"
#include "ut_rooms.h"

/* Variabili globali */
long mailroompos; /* Posizione della mailroom nella lista delle room */

/* Prototipi funzioni in questo file */
void mail_load(struct sessione *t);
void mail_load1(struct dati_ut *utente, struct msg_list *mail_list);
void mail_save(struct sessione *t);
void mail_save1(struct dati_ut *utente, struct msg_list *mail_list);
void mail_check(void);
struct msg_list * mail_alloc(void);
void mail_free(struct msg_list *mail);
void mail_add(long user, long msgnum, long msgpos);
bool mail_access(char *nome);
int mail_new(struct sessione *t);
int mail_new_name(char *name);
void cmd_biff(struct sessione *t, char *name);
static FILE * mail_fopen(long user, const char *mode);
static void mail_fname(char *filename, long user);

/**************************************************************************/
/*
 * Legge i dati sui mail dell'utente (t->utente)
 */
void mail_load(struct sessione *t)
{
	mail_load1(t->utente, t->mail);
}

void mail_load1(struct dati_ut *utente, struct msg_list *msg)
{
	char filename[LBUF];
	long *tmp;

	if (dati_server.mail_nslot == 0)
		return;

	if ((utente == NULL) || (utente->livello < LVL_NORMALE))
		return;

	mail_fname(filename, utente->matricola);
	msg_load_data(msg, filename, dati_server.mail_nslot);

	/* ATTENZIONE mail sono salvati nell'ordine (num,pos,local)
           mentre room in ordine (pos,num,local) *sigh* */
	tmp = msg->pos;
	msg->pos = msg->num;
	msg->num = tmp;
	/* TODO: Convertire le strutture per averle nell'ordine giusto! */
}

/*
 * Salva i dati sui mail dell'utente (t->utente)
 */
void mail_save(struct sessione *t)
{
	mail_save1(t->utente, t->mail);
}

void mail_save1(struct dati_ut *utente, struct msg_list *msg)
{
	char filename[LBUF];
	long *tmp;

	if ((utente == NULL) || (utente->livello < LVL_NORMALE)
	    || (msg == NULL))
		return;

	if (dati_server.mail_nslot == 0)
		return;

	mail_fname(filename, utente->matricola);

	/* ATTENZIONE mail sono salvati nell'ordine (num,pos,local)
           mentre room in ordine (pos,num,local) *sigh* */
	tmp = msg->pos;
	msg->pos = msg->num;
	msg->num = tmp;
	/* TODO: Convertire le strutture per averle nell'ordine giusto! */

	msg_save_data(msg, filename, dati_server.mail_nslot);

	/* Rimetto a posto */
	tmp = msg->pos;
	msg->pos = msg->num;
	msg->num = tmp;
}

/*
 * Controlla l'esistenza dei file UTR per tutti gli utenti della lista
 * utenti; se il file non esiste, lo crea di default.
 */
void mail_check(void)
{
	FILE *fp;
	struct lista_ut *ut;
	long nslot, *zero_vec, zero = 0;

	citta_log("Controlla i dati mail utenti.");
	nslot = dati_server.mail_nslot;
	CREATE(zero_vec, long, nslot, TYPE_LONG);

	for (ut = lista_utenti; ut; ut = ut->prossimo) {
		fp = mail_fopen(ut->dati->matricola, "r");
		if (fp == NULL) {
			fp = mail_fopen(ut->dati->matricola, "w");
			fwrite(zero_vec, sizeof(long), nslot, fp);
			fwrite(zero_vec, sizeof(long), nslot, fp);
			fwrite(zero_vec, sizeof(long), nslot, fp);
			fwrite(&zero, sizeof(long), 1, fp);
			fwrite(&zero, sizeof(long), 1, fp);
		}
		fclose(fp);
	}
	Free(zero_vec);
}

/*
 * Alloca la memoria per i dati utr.
 */
struct msg_list * mail_alloc(void)
{
        struct msg_list *mail;

	mail = msg_create(dati_server.mail_nslot);
	return mail;
}

/* 
 * Funzione per liberare la memoria allocata per le slot degli utenti
 */
void mail_free(struct msg_list *mail)
{
	Free(mail->pos);
	Free(mail->num);
	Free(mail->local);
	Free(mail);
}

/****************************************************************************/
/*
 * Aggiunge in fondo alla lista dei mail dell'utente #user il mail
 * in msgnum, facendo scrollare i vecchi mail.
 */
void mail_add(long user, long msgnum, long msgpos)
{
	struct sessione *s;
	FILE *fp;
	long nslot, *v1, *v2, *v3, highmsg, highloc, i;

	nslot = dati_server.mail_nslot;
	s = collegato_n(user);
	if (s) {
	        msg_scroll(s->mail, nslot);
		s->mail->num[nslot-1] = msgnum;
		s->mail->pos[nslot-1] = msgpos;
		s->mail->local[nslot-1] = (s->mail->highloc)++;
		s->mail->highmsg = msgnum;
	} else {
		if ((fp = mail_fopen(user, "r")) == NULL)
			return;
		CREATE(v1, long, nslot, TYPE_LONG);
		CREATE(v2, long, nslot, TYPE_LONG);
		CREATE(v3, long, nslot, TYPE_LONG);
		fread(v1, sizeof(long), nslot, fp);
		fread(v2, sizeof(long), nslot, fp);
		fread(v3, sizeof(long), nslot, fp);
		fread(&highmsg, sizeof(long), 1, fp);
		fread(&highloc, sizeof(long), 1, fp);
		fclose(fp);
		for (i = 0; i < nslot-1; i++) {
			v1[i] = v1[i+1];
			v2[i] = v2[i+1];
			v3[i] = v3[i+1];
		}
		v1[nslot-1] = msgnum;
		v2[nslot-1] = msgpos;
		v3[nslot-1] = (highloc)++;
		highmsg = msgnum;
		
		if ((fp = mail_fopen(user, "w")) == NULL)
			return;
		fwrite(v1, sizeof(long), nslot, fp);
		fwrite(v2, sizeof(long), nslot, fp);
		fwrite(v3, sizeof(long), nslot, fp);
		fwrite(&highmsg, sizeof(long), 1, fp);
		fwrite(&highloc, sizeof(long), 1, fp);
		fclose(fp);
	}
}

/*
 * Trova la posizione del mail #msgnum.
 * Restituisce -1L se il mail non viene trovato
 */
long mail_pos(struct sessione *t, long msgnum)
{
	long i;

	for (i = 0; i < dati_server.mail_nslot; i++)
		if (t->mail->num[i] == msgnum)
			return t->mail->pos[i];
	return -1L;
}

/*
 * Restituisce 1 se l'utente nome ha una mailbox, altrimenti 0.
 */
bool mail_access(char *nome)
{
	struct dati_ut *ut;
	struct room *r;

	ut = trova_utente(nome);
	r = room_findn(dati_server.mailroom);

	return (ut && r && (ut->livello >= r->data->rlvl));
}

/*
 * Restituisce 1 se l'utente *ut ha una mailbox, altrimenti 0.
 */
bool mail_access_ut(struct dati_ut * ut)
{
	struct room *r = room_findn(dati_server.mailroom);
	
	return (ut && r && (ut->livello >= r->data->rlvl));
}

/*
 * Restituisce il numero di mail nuovi.
 */
int mail_new(struct sessione *t)
{
	struct room *r;
	long msgmax, fullroom, i, newmail = 0;

	r = room_findn(dati_server.mailroom);
	msgmax = r->data->maxmsg;
	fullroom = t->fullroom[r->pos];

	for (i = 0; i < msgmax; i++)
		if (t->mail->num[i] > fullroom)
				newmail++;
	return newmail;
}

/*
 * Restituisce il numero di mail nuovi dell'utente 'name'.
 */
int mail_new_name(char *name)
{
	struct sessione *s;
	struct room *r;
	FILE *fp;
	long msgmax, fullroom, i, user, *mvec, newmail = 0;

	r = room_findn(dati_server.mailroom);
	msgmax = r->data->maxmsg;
	if ((s = collegato(name))) {
		fullroom = s->fullroom[r->pos];
		for (i = 0; i < msgmax; i++)
			if (s->mail->num[i] > fullroom)
				newmail++;
	} else {
		user = trova_utente(name)->matricola;
		if ((fp = mail_fopen(user, "r")) == NULL)
			return -1;
		CREATE(mvec, long, msgmax, TYPE_LONG);	
		fread(mvec, sizeof(long), msgmax, fp);
		fclose(fp);
		fullroom = utr_get_fullroom(user, r->pos);
		for (i = 0; i < msgmax; i++)
			if (mvec[i] > fullroom)
				newmail++;
		Free(mvec);
	}
	return newmail;
}

void mail_link(struct sessione *t)
{
  //  cr8_virtual_room(t, "Mail", dati_server.mail_nslot, )
}

/*
 * Invia al client il numero di nuovi mail.
 */
void cmd_biff(struct sessione *t, char *name)
{
	if (mail_access(name))
		cprintf(t, "%d %d\n", OK, mail_new_name(name));
	else
		cprintf(t, "%d\n", ERROR);
}

/***************************************************************************/
/*
 * Apre il file UTR dell'utente #user in modo 'mode' ("r", "w", etc...)
 */
static FILE * mail_fopen(long user, const char *mode)
{
	FILE *fp;
	char filename[LBUF];

	sprintf(filename, "%s%ld", FILE_MAIL, user);
	fp = fopen(filename, mode);
	return fp;
}

static void mail_fname(char *filename, long user)
{
	sprintf(filename, "%s%ld", FILE_MAIL, user);
}

