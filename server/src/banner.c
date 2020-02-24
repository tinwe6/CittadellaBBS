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
* File : banner.c                                                           *
*        Funzioni per gestire i login banners di Cittadella/UX.             *
****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cittaserver.h"
#include "banner.h"
#include "extract.h"
#include "memstat.h"
#include "utility.h"
#include "macro.h"
#include "time.h"
#include "versione.h"

/* Variabili Globali */
long banner_num;
long * banner_hash = NULL;

/* Prototipi funzioni in questo file */
void hash_banners(void);
void banner_save_hash(long n, long *pos);
void banner_load_hash(void);
void cmd_lban(struct sessione *t, char *cmd);
void cmd_lbgt(struct sessione *t);
void save_banners(struct sessione *t);

/***************************************************************************/
void hash_banners(void)
{
	FILE *fp;
	char buf[LBUF];
	int n, i, c;
	long *pos;

	n = 0;
	fp = fopen(BANNER_FILE, "r");
	if (!fp)
		return; /* No banner file */

	while (fgets(buf, LBUF, fp) != NULL)
		if (!strcmp(buf, "%\n"))
			n++;
	if (!n) {
		fclose(fp);
		return;
	}
	n++;
	CREATE(pos, long, n, TYPE_LONG);
	fseek(fp, 0L, SEEK_SET);
	pos[0] = 0;
	i = 1;
	while(((c = getc(fp)) != EOF) && (i<n))
		if (c == '%') {
			if (getc(fp) == '\n')
				pos[i++] = ftell(fp);
		}
	fclose(fp);
	banner_save_hash(n, pos);
	Free(pos);
}

/*
 * Salva la hash table per i banners
 */
void banner_save_hash(long n, long *pos)
{
	FILE *fp;

	fp = fopen(BANNER_HASHFILE, "w");
	if (!fp)
		return;
	fwrite((long *) &n, sizeof(long), 1, fp);
	fwrite((long *) pos, sizeof(long), n, fp);
	/* TODO check that fwrite was successful */
	fclose(fp);
}

/*
 * Carica la hash table per i banners
 */
void banner_load_hash(void)
{
	FILE *fp;

	fp = fopen(BANNER_HASHFILE, "r");
	if (!fp)
		return;
	fread((long *) &banner_num, sizeof(long), 1, fp);
	if (banner_num > 0) {
		CREATE(banner_hash, long, banner_num, TYPE_LONG);
		fread((long *) banner_hash, sizeof(long), banner_num, fp);
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
			strcat(buf, "Non c'&egrave; nessuno connesso.");
			break;
		 case 1:
			strcat(buf, "C'&egrave; un utente connesso.");
			break;
		 default:
			sprintf(buf+strlen(buf),
				"Ci sono %d utenti connessi.", n);
		}
		cprintf(t, "%s\n", buf);
		cprintf(t, "%d   %s</b>\n", OK, citta_dove);
		fp = fopen(FILE_LOGO, "r");
		if (fp) {
			while (fgets(buf, LBUF, fp) != NULL)
				cprintf(t, "%d %s", OK, buf);
			fclose(fp);
		}
	} else {
                if (num_ut() == 1)
                        cprintf(t, "%d 1 utente connesso\n", OK);
                else
                        cprintf(t, "%d %d utenti connessi\n", OK, num_ut());
        }
	cprintf(t, "%d <b;fg=6>\n", OK);
	send_random_banner(t);
	if (!nobanner)
		cprintf(t, "%d \n", OK);
#else
	fp = fopen(FILE_BBS_CLOSED, "r");
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
        IGNORE_UNUSED_PARAMETER(cmd);
        cprintf(t, "%d Not yet implemented\n", ERROR);
}

/*
 * Ricrea la hash table per le login banners
 */
void cmd_lbch(struct sessione *t, char *cmd)
{
        IGNORE_UNUSED_PARAMETER(cmd);
        cprintf(t, "%d Not yet implemented\n", ERROR);
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
		citta_logf("LOGIN BANNERS: edited by [%s].", t->utente->nome);
	} else
		citta_log("SYSERR: Errore in scrittura delle room info.");
}

#if 0
void random_banner(char *str)
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
		printf("%s", buf);
	fclose(fp);
}

void main(void)
{
	printf("\nHashing Login Banner file: ");
	hash_banners();
	printf("OK!\n");
	banner_load_hash();
	printf("A random Banner:\n");
	random_banner(NULL);
	printf("\n");
}
#endif
