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
* File : fm_cmd.c                                                           *
*        Comandi per gestire i file messaggi da client.                     *
****************************************************************************/
#include "config.h"
#include <stdio.h>
#ifdef USE_STRING_TEXT
#include "argz.h"
#endif
#include "cittaserver.h"
#include "fm_cmd.h"
#include "extract.h"
#include "file_messaggi.h"
#include "messaggi.h"
#include "rooms.h"
#include "text.h"
#include "utility.h"

/* Prototipi funzioni in questo file */
void cmd_fmcr(struct sessione *t, char *buf);
void cmd_fmdp(struct sessione *t, char *buf);
void cmd_fmhd(struct sessione *t, char *buf);
void cmd_fmri(struct sessione *t, char *buf);
void cmd_fmrm(struct sessione *t, char *buf);
void cmd_fmrp(struct sessione *t, char *buf);
void cmd_fmxp(struct sessione *t, char *buf);

/****************************************************************************
****************************************************************************/
/*
 * Restituisce la lista di TUTTI gli headers del file messaggi richiesto
 * (SYSOP ONLY)
 */
void cmd_fmhd(struct sessione *t, char *buf)
{
	struct text *txt;
	long fmnum;
	char err, *riga;

	fmnum = extract_long(buf, 0);
	txt = txt_create();
	if ((err=fm_headers(fmnum, txt))==0) {
		cprintf(t, "%d\n", SEGUE_LISTA);
		txt_rewind(txt);
		while (riga = txt_get(txt), riga)
			cprintf(t, "%d %s\n", OK, riga);
		cprintf(t, "000\n");
	} else {
		cprintf(t, "%d Error code %d.\n", ERROR, err);
	}
	txt_free(&txt);
}

/*
 * Crea nuovo File Messaggi
 */
void cmd_fmcr(struct sessione *t, char *buf)
{
	long fmnum, fmlen;
	char desc[LBUF];
	struct text *txt;
	struct room *r;

	fmlen = extract_long(buf, 0);
	extract(desc, buf, 1);
	fmnum = fm_create(fmlen, desc);
	if ((r = room_find(sysop_room))) {
		txt = txt_create();
		txt_putf(txt, "Il file messaggi #%ld e' stato creato da %s.\n",
			 fmnum, t->utente->nome);
		citta_post(r, "Creazione File Messaggi.", txt);
		txt_free(&txt);
	}
	cprintf(t, "%d %ld\n", OK, fmnum);
}

/*
 * Elimina File Messaggi
 */
void cmd_fmrm(struct sessione *t, char *buf)
{
	long fmnum;
	struct text *txt;
	struct room *r;

	/* Controlla che il FM non e' usato da nessuna room (TODO) */
	fmnum = extract_long(buf, 0);
	if (fm_delete(fmnum)) {
		if ((r = room_find(sysop_room))) {
			txt = txt_create();
			txt_putf(txt, "Il file messaggi #%ld e' stato eliminato da %s.\n",
				 fmnum, t->utente->nome);
			citta_post(r, "Cancellazione File Messaggi.", txt);
			txt_free(&txt);
		}
		cprintf(t, "%d FM deleted\n", OK);
	} else
		cprintf(t, "%d Could not delete FM\n", ERROR);
}

/*
 * Invia le info sui file message al client.
 */
void cmd_fmri(struct sessione *t, char *buf)
{
	struct fm_list *punto;
	long fmnum;

	if (lista_fm == NULL) {
		cprintf(t, "%d Non sono presenti file messaggi.\n", ERROR);
		return;
	}
	fmnum = extract_long(buf, 0);
	cprintf(t, "%d\n", SEGUE_LISTA);
	for (punto=lista_fm; punto; punto=punto->next)
		if ((fmnum==0)||((punto->fm).num==fmnum))
			cprintf(t, "%d %ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%d|%s\n", OK,
				(punto->fm).num, (punto->fm).size,
				(punto->fm).lowest, (punto->fm).highest,
				(punto->fm).posizione,
				(punto->fm).nmsg, (punto->fm).ndel,
				(punto->fm).flags, (punto->fm).ctime,
				(punto->fm).fmck, (punto->fm).desc);
	cprintf(t, "000\n");
}

/*
 * Read Post #num in position pos from file message #fmnum.
 * Call: "FMRP fmnum|num|pos"
 */
void cmd_fmrp(struct sessione *t, char *buf)
{
#ifdef USE_STRING_TEXT
	char *txt;
	size_t len;
#else
	struct text *txt;
#endif
	long fmnum, num, pos;
	char header[LBUF], err, *riga;

	fmnum = extract_long(buf, 0);
	num = extract_long(buf, 1);
	pos = extract_long(buf, 2);
#ifdef USE_STRING_TEXT
	if ((err=fm_get(fmnum, num, pos, &txt, &len, header)) == 0) {
#else
		txt = txt_create();
		if ((err=fm_get(fmnum, num, pos, txt, header)) == 0) {
#endif
			cprintf(t, "%d\n", SEGUE_LISTA);
			cprintf(t, "%d %s\n", OK, header);
#ifdef USE_STRING_TEXT
			for (riga=txt; riga; riga = argz_next(txt, len, riga))
				cprintf(t, "%d %s\n", OK, riga);
#else
			txt_rewind(txt);
			while (riga = txt_get(txt), riga)
				cprintf(t, "%d %s\n", OK, riga);
#endif
			cprintf(t, "000\n");
		} else {
			cprintf(t, "%d Error code %d.\n", ERROR, err);
		}
#ifndef USE_STRING_TEXT
		txt_free(&txt);
#endif
}

/*
 * Delete Post #num in position pos from file message #fmnum.
 * Call: "FMRP fmnum|num|pos"
 */
void cmd_fmdp(struct sessione *t, char *buf)
{
	long fmnum, num, pos;
	char err;

	fmnum = extract_long(buf, 0);
	num = extract_long(buf, 1);
	pos = extract_long(buf, 2);
	if ((err=fm_delmsg(fmnum, num, pos))==0)
		cprintf(t, "%d Message Deleted.\n", OK);
	else
		cprintf(t, "%d Error code %d.\n", ERROR, err);
}

/*
 * Espande il file messaggi.
 */
void cmd_fmxp(struct sessione *t, char *buf)
{
	long fmnum, newsize;
	struct text *txt;
	struct room *r;

	fmnum = extract_long(buf, 0);
	newsize = extract_long(buf, 1);
	if (fm_expand(fmnum, newsize))
		cprintf(t, "%d\n", ERROR);
	else {
		if ((r = room_find(sysop_room))) {
			txt = txt_create();
			txt_putf(txt, "Il file messaggi #%ld e' stato espanso a %ld bytes da %s.\n",
				 fmnum, newsize, t->utente->nome);
			citta_post(r,"Espansione File Messaggi.", txt);
			txt_free(&txt);
		}
		cprintf(t, "%d\n", OK);
	}
}

