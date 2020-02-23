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
* File : ut_rooms.c                                                         *
*        Gestione delle strutture dati dinamiche degli utenti.              *
****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cittaserver.h"
#include "memstat.h"
#include "ut_rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void utr_load(struct sessione *t);
void utr_load1(struct dati_ut *utente, long *fullroom, long *room_flags,
	       long *room_gen);
void utr_save(struct sessione *t);
void utr_check(void);
void utr_alloc(struct sessione *t);
void utr_alloc1(long **fullroom, long **room_flags, long **room_gen);
void utr_free(struct sessione *t);
void utr_setf(long user, long pos, long flag);
void utr_resetf(long user, long pos, long flag);
int utr_togglef(long user, long pos, long flag);
void utr_setf_all(long pos, long flag);
void utr_resetf_all(long pos, long flag);
void utr_resetf_all_lvl(long pos, long flag, int lvl);
void utr_swap(long user, long pos1, long pos2);
void utr_swap_all(long pos1, long pos2);
void utr_rmslot(long user, long slot);
void utr_rmslot_all(long slot);
void utr_resize(long user, long n, long oldn);
void utr_resize_all(long nslots);
void utr_chval(long user, long pos, long fr, long rf, long rg);
void utr_chval_all(long pos, long fr, long rf, long rg);
long utr_getf(long user, long pos);
long utr_get_fullroom(long user, long pos);
static FILE * utr_fopen(long user, const char *mode);
static void utr_default(struct sessione *t);
static void utr_default1(long *fullroom, long *room_flags, long *room_gen);

/* Variabile globale */
char FILE_UTR[256] = "./lib/utenti/utr_data/utr";

/**************************************************************************/
/*
 * Legge i dati sulle room dell'utente (t->utente)
 */
void utr_load(struct sessione *t)
{
	FILE *fp;
	long nslot;

	nslot = dati_server.utr_nslot;
	if (nslot == 0)
		return;

	if ((t->utente == NULL) || (t->utente->livello < LVL_NON_VALIDATO)) {
		utr_default(t);
		return;
	}

	fp = utr_fopen(t->utente->matricola, "r");
	if (!fp) {
		citta_logf("UTR_DATA assente per utente %ld.", t->utente->matricola);
		utr_default(t);
		return;
	}
	fread(t->fullroom, sizeof(long), nslot, fp);
	fread(t->room_flags, sizeof(long), nslot, fp);
	fread(t->room_gen, sizeof(long), nslot, fp);
	fclose(fp);
}

/* Interfaccia alternativa per il POP3 */
void utr_load1(struct dati_ut *utente, long *fullroom, long *room_flags,
	       long *room_gen)
{
	FILE *fp;
	long nslot;

	nslot = dati_server.utr_nslot;
	if (nslot == 0)
		return;

	if ((utente == NULL) || (utente->livello < LVL_NON_VALIDATO)) {
		utr_default1(fullroom, room_flags, room_gen);
		return;
	}

	fp = utr_fopen(utente->matricola, "r");
	if (!fp) {
		citta_logf("UTR_DATA assente per utente %ld.", utente->matricola);
		utr_default1(fullroom, room_flags, room_gen);
		return;
	}
	fread(fullroom, sizeof(long), nslot, fp);
	fread(room_flags, sizeof(long), nslot, fp);
	fread(room_gen, sizeof(long), nslot, fp);
	fclose(fp);
}

/*
 * Controlla l'esistenza dei file UTR per tutti gli utenti della lista
 * utenti; se il file non esiste, lo crea di default.
 */
void utr_check(void)
{
	FILE *fp;
	struct lista_ut *ut;
	struct room *r;
	long nslot, *rf, *zero_vec;

	citta_log("UTR Check data");
	/* Genero UTR_Data di default */
	nslot = dati_server.utr_nslot;
	/*
	{
		int i = 0;
		for (r = lista_room.first; r; r = r->next) i++;
		citta_logf("nslots %d, i %d", nslot, i);
	}
	*/
	CREATE(zero_vec, long, nslot, TYPE_LONG);
	CREATE(rf, long, nslot, TYPE_LONG);
	for (r = lista_room.first; r; r = r->next)
		rf[r->pos] = (r->data)->def_utr;

	for (ut = lista_utenti; ut; ut = ut->prossimo) {
		fp = utr_fopen(ut->dati->matricola, "r");
		if (fp == NULL) {
			fp = utr_fopen(ut->dati->matricola, "w");
			fwrite(zero_vec, sizeof(long), nslot, fp);
			fwrite(rf, sizeof(long), nslot, fp);
			fwrite(zero_vec, sizeof(long), nslot, fp);
		}
		fclose(fp);
	}
	Free(zero_vec);
	Free(rf);
}

/*
 * Salva i dati sulle room dell'utente (t->utente)
 */
void utr_save(struct sessione *t)
{
	FILE *fp;
	char filename[LBUF], bak[LBUF];
	unsigned long nslot;
	unsigned int err=0;
	
	if ((t->utente == NULL) || (t->utente->livello < LVL_NON_VALIDATO))
		return;

	nslot = (unsigned long)dati_server.utr_nslot;
	if (nslot == 0)
		return;
	
	sprintf(filename, "%s%ld", FILE_UTR, t->utente->matricola);
	sprintf(bak, "%s.bak", filename);
	rename(filename, bak);
	
	fp = fopen(filename, "w");
	if (!fp) {
		citta_logf("UTR_DATA errore in scrittura utente #%ld.",
		     t->utente->matricola);
		rename(bak, filename);
		return;
	}
	err += (fwrite(t->fullroom, sizeof(long), nslot, fp) != nslot);
	err += (fwrite(t->room_flags, sizeof(long), nslot, fp) != nslot);
	err += (fwrite(t->room_gen, sizeof(long), nslot, fp) != nslot);
	fclose(fp);
	if(err){
		  citta_logf("UTR_DATA errore in scrittura del file utente #%ld.",
				                       t->utente->matricola);
		  rename(bak, filename);
		  return;
	}
	unlink(bak);
}

/*
 * Alloca la memoria per i dati utr.
 */
void utr_alloc(struct sessione *t)
{
	long nslot;

	nslot = dati_server.utr_nslot;
	CREATE(t->fullroom, long, nslot, TYPE_LONG);
	CREATE(t->room_flags, long, nslot, TYPE_LONG);
	CREATE(t->room_gen, long, nslot, TYPE_LONG);
}

void utr_alloc1(long **fullroom, long **room_flags, long **room_gen)
{
	long nslot;

	nslot = dati_server.utr_nslot;
	CREATE(*fullroom, long, nslot, TYPE_LONG);
	CREATE(*room_flags, long, nslot, TYPE_LONG);
	CREATE(*room_gen, long, nslot, TYPE_LONG);
}

/* 
 * Funzione per liberare la memoria allocata per le slot degli utenti
 */
void utr_free(struct sessione *t)
{
	Free(t->fullroom);
	Free(t->room_flags);
	Free(t->room_gen);
}

/*
 * Setta flag UTR per la room pos dell'utente #user.
 * Se l'utente e' collegato, modifica semplicemente il flag nel suo vettore
 * sessione->room_flags che verra' poi salvata sul suo file utr, altrimenti
 * modifica direttamente il file utr.
 */
void utr_setf(long user, long pos, long flag)
{
	struct sessione *s;
	FILE *fp;
	long l;
	
	s = collegato_n(user);
	if (s)
		s->room_flags[pos] |= flag;
	else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return;
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos), SEEK_SET);
		fread(&l, sizeof(long), 1, fp);
		l |= flag;
		fseek(fp, -sizeof(long), SEEK_CUR);
		fwrite(&l, sizeof(long), 1, fp);
		fclose(fp);
	}
}

/*
 * Resetta flag UTR per la room pos dell'utente #user.
 */
void utr_resetf(long user, long pos, long flag)
{
	struct sessione *s;
	FILE *fp;
	long l;
	
	s = collegato_n(user);
	if (s)
		s->room_flags[pos] &= ~flag;
	else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return;
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos), SEEK_SET);
		fread(&l, sizeof(long), 1, fp);
		l &= ~flag;
		fseek(fp, -sizeof(long), SEEK_CUR);
		fwrite(&l, sizeof(long), 1, fp);
		fclose(fp);	
	}
}

/*
 * Toggle flag UTR per la room pos dell'utente #user.
 * Restituisce il nuovo valore del flag (-1 se problemi).
 */
int utr_togglef(long user, long pos, long flag)
{
	struct sessione *s;
	FILE *fp;
	long l;
	
	s = collegato_n(user);
	if (s) {
		s->room_flags[pos] =  s->room_flags[pos] ^ flag;
		return ((s->room_flags[pos] & flag) ? 1 : 0);
	}
	fp = utr_fopen(user, "r+");
	if (!fp)
		return -1;
	fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos), SEEK_SET);
	fread(&l, sizeof(long), 1, fp);
	l = l ^ flag;
	fseek(fp, -sizeof(long), SEEK_CUR);
	fwrite(&l, sizeof(long), 1, fp);
	fclose(fp);
	return ((l & flag) ? 1 : 0);
}

/*
 * Setta il flag 'flag' nella room 'pos' a tutti gli utenti.
 */
void utr_setf_all(long pos, long flag)
{
	struct lista_ut *ut;
	
	for (ut = lista_utenti; ut; ut = ut->prossimo)
		utr_setf(ut->dati->matricola, pos, flag);
}

/*
 * Resetta il flag 'flag' nella room 'pos' a tutti gli utenti.
 */
void utr_resetf_all(long pos, long flag)
{
	struct lista_ut *ut;
	
	for (ut = lista_utenti; ut; ut = ut->prossimo)
		utr_resetf(ut->dati->matricola, pos, flag);
}

/*
 * Resetta il flag 'flag' nella room 'pos' a tutti gli utenti di 
 * livello inferiore a lvl.
 */
void utr_resetf_all_lvl(long pos, long flag, int lvl)
{
	struct lista_ut *ut;
	
	for (ut = lista_utenti; ut; ut = ut->prossimo)
		if (ut->dati->livello < lvl)
			utr_resetf(ut->dati->matricola, pos, flag);
}

/*
 * Scambia i dati UTR per le room in posizione pos1 e pos2 dell'utente #user.
 */
void utr_swap(long user, long pos1, long pos2)
{
	struct sessione *s;
	FILE *fp;
	long tmp1, tmp2;
	
	if (pos1 == pos2)
		return;
	
	s = collegato_n(user);
	if (s) {
		tmp1 = s->fullroom[pos1];
		s->fullroom[pos1] = s->fullroom[pos2];
		s->fullroom[pos2] = tmp1;
		tmp1 = s->room_flags[pos1];
		s->room_flags[pos1] = s->room_flags[pos2];
		s->room_flags[pos2] = tmp1;
	} else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return;

		fseek(fp, sizeof(long)*pos1, SEEK_SET);
		fread(&tmp1, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*pos2, SEEK_SET);
		fread(&tmp2, sizeof(long), 1, fp);
		fseek(fp, -sizeof(long), SEEK_CUR);
		fwrite(&tmp1, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*pos1, SEEK_SET);
		fwrite(&tmp2, sizeof(long), 1, fp);

		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos1), SEEK_SET);
		fread(&tmp1, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos2), SEEK_SET);
		fread(&tmp2, sizeof(long), 1, fp);
		fseek(fp, -sizeof(long), SEEK_CUR);
		fwrite(&tmp1, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos1), SEEK_SET);
		fwrite(&tmp2, sizeof(long), 1, fp);

		fclose(fp);
	}
}

/*
 * Scambia i dati UTR corrispondenti alle room in pos1 e pos2 a tutti gli
 * utenti nella lista_utenti.
 */
void utr_swap_all(long pos1, long pos2)
{
	struct lista_ut *ut;
	
	if (pos1 == pos2)
		return;
	for (ut = lista_utenti; ut; ut = ut->prossimo)
		utr_swap(ut->dati->matricola, pos1, pos2);
}

/*
 * Elimina lo slot #slot dai vettori di slot dell'utente #user, e scrolla
 * gli slot successivi.
 */
void utr_rmslot(long user, long slot)
{
	struct sessione *s;
	FILE *fp;
	long nslot, *v1, *v2, *v3, i;

	nslot = dati_server.utr_nslot;
	s = collegato_n(user);
	if (s) {
		for (i = slot; i < nslot-1; i++) {
			s->fullroom[i] = s->fullroom[i+1];
			s->room_flags[i] = s->room_flags[i+1];
			s->room_gen[i] = s->room_gen[i+1];
		}
		s->fullroom[nslot-1] = 0L;
		s->room_flags[nslot-1] = 0L;
		s->room_gen[nslot-1] = 0L;
	} else {
		if ((fp = utr_fopen(user, "r")) == NULL)
			return;
		CREATE(v1, long, nslot, TYPE_LONG);
		CREATE(v2, long, nslot, TYPE_LONG);
		CREATE(v3, long, nslot, TYPE_LONG);
		fread(v1, sizeof(long), nslot, fp);
		fread(v2, sizeof(long), nslot, fp);
		fread(v3, sizeof(long), nslot, fp);
		fclose(fp);
		for (i = slot; i < nslot-1; i++) {
			v1[i] = v1[i+1];
			v2[i] = v2[i+1];
			v3[i] = v3[i+1];
		}
		v1[nslot-1] = 0L;
		v2[nslot-1] = 0L;
		v3[nslot-1] = 0L;
		if ((fp = utr_fopen(user, "w")) == NULL)
			return;
		fwrite(v1, sizeof(long), nslot, fp);
		fwrite(v2, sizeof(long), nslot, fp);
		fwrite(v3, sizeof(long), nslot, fp);
		fclose(fp);
	}
}

/*
 * Elimina lo slot #slot dai vettori di slot degli utenti, e scrolla
 * gli slot successivi.
 */
void utr_rmslot_all(long slot)
{
	struct lista_ut *ut;
	
	if (slot < dati_server.room_curr) {
		for (ut = lista_utenti; ut; ut = ut->prossimo)
			utr_rmslot(ut->dati->matricola, slot);
	} else
		citta_log("Tentativo di eliminare slot inesistente.");
}

/*
 * Porta da oldn a n il numero di slot UTR dell'utente #user.
 */
void utr_resize(long user, long n, long oldn)
{
	struct sessione *s;
	FILE *fp;
	long *v1, *v2, *v3;

	s = collegato_n(user);
	if (s) {
		RECREATE(s->fullroom, long, n);
		RECREATE(s->room_flags, long, n);
		RECREATE(s->room_gen, long, n);
	} else {
		if ((fp = utr_fopen(user, "r")) == NULL)
			return;
		CREATE(v1, long, n, TYPE_LONG);
		CREATE(v2, long, n, TYPE_LONG);
		CREATE(v3, long, n, TYPE_LONG);
		fread(v1, sizeof(long), oldn, fp);
		fread(v2, sizeof(long), oldn, fp);
		fread(v3, sizeof(long), oldn, fp);
		fclose(fp);
		if ((fp = utr_fopen(user, "w")) == NULL)
			return;
		fwrite(v1, sizeof(long), n, fp);
		fwrite(v2, sizeof(long), n, fp);
		fwrite(v3, sizeof(long), n, fp);
		fclose(fp);
	}
}

/*
 * Aggiunge un numero pari a nslots slots al vettore di slot degli utenti
 * Attenzione: per ora nslots > 0...
 */
void utr_resize_all(long nslots)
{
	struct lista_ut *ut;
	long n, oldn;
	
	if (nslots < 0)
		return;

	oldn = dati_server.utr_nslot;
	dati_server.utr_nslot += nslots;
	n = dati_server.utr_nslot;

	for (ut = lista_utenti; ut; ut = ut->prossimo)
		utr_resize(ut->dati->matricola, n, oldn);
}

/*
 * Inizializza le variabili UTR in pos 'pos' dell'utente #user
 * ai valori (fr, rf, rg).
 */
void utr_chval(long user, long pos, long fr, long rf, long rg)
{
	struct sessione *s;
	FILE *fp;
	
	s = collegato_n(user);
	if (s) {
		s->fullroom[pos] |= fr;
		s->room_flags[pos] |= rf;
		s->room_gen[pos] |= rg;
	} else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return;
		fseek(fp, sizeof(long)*pos, SEEK_SET);
		fwrite(&fr, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos), SEEK_SET);
		fwrite(&rf, sizeof(long), 1, fp);
		fseek(fp, sizeof(long)*(2*dati_server.utr_nslot+pos), SEEK_SET);
		fwrite(&rg, sizeof(long), 1, fp);
		fclose(fp);
	}
}


/*
 * Inizializza le variabili UTR in pos 'pos' ai valori (fr, rf, rg).
 */
void utr_chval_all(long pos, long fr, long rf, long rg)
{
	struct lista_ut *ut;
	
	for (ut = lista_utenti; ut; ut = ut->prossimo)
		utr_chval(ut->dati->matricola, pos, fr, rf, rg);
}

/*
 * Restituisce il flag UTR per la room pos dell'utente #user.
 */
long utr_getf(long user, long pos)
{
	struct sessione *s;
	FILE *fp;
	long l;
	
	s = collegato_n(user);
	if (s)
		l = s->room_flags[pos];
	else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return 0;
		fseek(fp, sizeof(long)*(dati_server.utr_nslot+pos), SEEK_SET);
		fread(&l, sizeof(long), 1, fp);
		fclose(fp);
	}
	return l;
}

/*
 * Restituisce il fullroom per la room pos dell'utente #user.
 */
long utr_get_fullroom(long user, long pos)
{
	struct sessione *s;
	FILE *fp;
	long l;

	s = collegato_n(user);
	if (s)
		l = s->room_flags[pos];
	else {
		fp = utr_fopen(user, "r+");
		if (!fp)
			return 0;
		fseek(fp, sizeof(long)*pos, SEEK_SET);
		fread(&l, sizeof(long), 1, fp);
		fclose(fp);
	}
	return l;
}


/****************************************************************************/
/*
 * Apre il file UTR dell'utente #user in modo 'mode' ("r", "w", etc...)
 */
static FILE * utr_fopen(long user, const char *mode)
{
	FILE *fp;
	char filename[LBUF];

	sprintf(filename, "%s%ld", FILE_UTR, user);
	fp = fopen(filename, mode);

	return fp;
}

/*
 * Dati UTR di default (per Ospiti, o in caso di difficolta'...)
 */
static void utr_default(struct sessione *t)
{
	utr_default1(t->fullroom, t->room_flags, t->room_gen);
}

/* Interfaccia alternativa per il POP3 */
static void utr_default1(long *fullroom, long *room_flags, long *room_gen)
{
	struct room *r;

	for (r = lista_room.first; r; r = r->next) {
		room_flags[r->pos] = (r->data)->def_utr;
		room_gen[r->pos] = 0L;
		fullroom[r->pos] = 0L;
	}
}

/****************************************************************************/
