/*
 *  Copyright (C) 1999 - 2003 by Marco Caldarelli and Riccardo Vianello
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
* File : patacche.c                                                         *
*        Gestione dei distintivi assegnati agli utenti.                     *
****************************************************************************/
#include "config.h"

#ifdef USE_BADGES

#include "cittaserver.h"
#include "banner.h"
#include "extract.h"
#include "memstat.h"
#include "utility.h"
#include "macro.h"
#include "time.h"
#include "versione.h"

static long badge_num;      /* Numero di badge presenti                     */
static long badge_slots;    /* Numero di slot per badge allocati in memoria */
static struct badge * badges; /* Puntatore alla lista di badge              */


/* Prototipi funzioni in questo file */

/***************************************************************************/
void hash_banners(void)
{
	FILE *fp;
	char buf[LBUF];
	int n, i, c;
	long *pos;
	
	n = 0;
	fp = fopen(BADGES_FILE, "r");
	if (!fp)
		return; /* No badges file */


	while (fgets(buf, LBUF, fp) != NULL)
		if (!strcmp(buf, "%\n"))
			n++;

	if (!n) {
		fclose(fp);
		return;
	}
	n += 5; /* Lascio spazio per aggiungere qualche altro distintivo */

	badge_slots = n;
	badge_num = 0;

	CREATE(badges, struct badge, badge_slots, TYPE_BADGE);
	fseek(fp, 0L, SEEK_SET);

	i = 0;
	while(((c = getc(fp)) != EOF) && (c!='%'));
	fgets(buf, )
	while(((c=getc(fp)) != EOF) && (i<n))
		if (c=='%')
			pos[i++] = ftell(fp)+1;
	fclose(fp);
	Free(pos);
}

/*
 * Salva la hash table per i banners
 */
void banner_save_hash(long n, long *pos)
{
	FILE *fp;
	int hh;
	
	fp = fopen(BANNER_HASHFILE, "w");
	if (!fp)
		return;
	hh = fwrite((long *) &n, sizeof(long), 1, fp);
	hh = fwrite((long *) pos, sizeof(long), n, fp);
	fclose(fp);
}

/*
 * Carica la hash table per i banners
 */
void banner_load_hash(void)
{
	FILE *fp;
	long hh;
	
	fp = fopen(BANNER_HASHFILE, "r");
	if (!fp)
		return;
	hh = fread((long *) &banner_num, sizeof(long), 1, fp);
	if (banner_num>0) {
		CREATE(banner_hash, long, banner_num, TYPE_LONG);
		hh = fread((long *) banner_hash, sizeof(long), banner_num,
			   fp);
	}
	fclose(fp);
}

void send_random_banner(struct sessione *t)
{
	char buf[LBUF];
	FILE *fp;
	int bnum;
	
	srand(time(0));
	bnum = (int) ((double)banner_num * rand() / (RAND_MAX+1.0));
	fp = fopen(BANNER_FILE, "r");
	if (!fp)
		return; /* No banner file */
	fseek(fp, banner_hash[bnum], SEEK_SET);
	while ((fgets(buf, LBUF, fp) != NULL)&&(strcmp(buf, "%\n")))
		cprintf(t, "%d   %s", OK, buf);
	fclose(fp);
}

int num_ut(void)
{
	struct sessione *p;
	int n = 0;
	
	for (p = lista_sessioni; p; p = p->prossima)
		if (p->utente)
			n++;
	return n;
}

void cmd_lban(struct sessione *t, char *cmd)
{
        FILE *fp;
        char buf[LBUF];
	int nobanner, n;
	
	if (t->logged_in) {
		cprintf(t, "%d\n", ERROR);
		return;
	}
	nobanner = extract_int(cmd, 0);
	n = num_ut();
	
	cprintf(t, "%d\n", SEGUE_LISTA);
#ifndef ONLY_BANNER
	if (!nobanner) {
		cprintf(t, "%d \n", OK);
		cprintf(t, "%d \n", OK);
		sprintf(buf, "%d   <b;fg=4>%s %-30s ", OK, citta_soft,
			SERVER_RELEASE);
		switch (n) {
		 case 0:
			strcat(buf, "Non c'&egrave; nessuno connesso.\n");
			break;
		 case 1:
			strcat(buf, "C'&egrave; un utente connesso.\n");
			break;
		 default:
			sprintf(buf+strlen(buf),
				"Ci sono %d utenti connessi.\n", n);
		}
		metti_in_coda(&t->output, buf);
		cprintf(t, "%d   %s</b>\n", OK, citta_dove);
		fp = fopen(LOGO_FILE, "r");
		if (fp) {
			while (fgets(buf, LBUF, fp) != NULL)
				cprintf(t, "%d %s", OK, buf);
			fclose(fp);
		}
	} else
		cprintf(t, "%d %d utent%s conness%s\n", OK, num_ut(),
                (num_ut() == 1 ? "e" : "i"),
                (num_ut() == 1 ? "o" : "i"));
	cprintf(t, "%d <b;fg=6>\n", OK);
	send_random_banner(t);
	if (!nobanner)
		cprintf(t, "%d \n", OK);
#else
	fp = fopen(BBS_CLOSED_FILE, "r");
	if (fp) {
		while (fgets(buf, LBUF, fp) != NULL)
			cprintf(t, "%d %s", OK, buf);
		fclose(fp);
	}
	t->stato = CON_CHIUSA;
#endif
	cprintf(t, "000\n");
}

/*
 * Invia il file delle login banner.
 */
void cmd_lbgt(struct sessione *t)
{
	FILE *fp;
        char buf[LBUF];

#ifndef USE_INTERPRETE
	if ((t->utente == NULL) || (t->utente->livello < MINLVL_BANNER)) {
		cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
		return;
	}	
#endif
	if ((fp = fopen(BANNER_FILE, "r")) == NULL) {
		cprintf(t, "%d\n", ERROR);
		return;
	}
	cprintf(t, "%d\n", SEGUE_LISTA);
	while (fgets(buf, LBUF, fp) != NULL)
		cprintf(t, "%d %s", OK, buf);
	fclose(fp);
	cprintf(t, "000\n");
}

/*
 * Aggiunge una login banner al file delle banner
 */
void cmd_lbad(struct sessione *t, char *cmd)
{

}

/*
 * Ricrea la hash table per le login banners
 */
void cmd_lbch(struct sessione *t, char *cmd)
{

}

void save_banners(struct sessione *t)
{
	long righe, a;
	FILE *fp;
	
	fp = fopen(BANNER_FILE, "w");
	if (fp != NULL) {
		righe = txt_len(t->text);
		txt_rewind(t->text);
		for (a = 0; a < righe; a++)
			fprintf(fp, "%s\n", txt_get(t->text));
		fclose(fp);
		hash_banners();
		Free(banner_hash);
		banner_load_hash();
		logf("LOGIN BANNERS: edited by [%s].", t->utente->nome);
	} else
		log("SYSERR: Errore in scrittura delle room info.");
}

#endif /* USE_BADGES */
