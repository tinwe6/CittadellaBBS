/*
 *  Copyright (C) 2004 by Marco Caldarelli
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
* File: find.c                                                                *
*       funzioni per la ricerca delle parole nei messaggi                     *
******************************************************************************/
#include "config.h"
#include "argz.h"
#include "cache_post.h"
#include "extract.h"
#include "find.h"
#include "march.h"
#include "memstat.h"
#include "prefixtree.h"
#include "room_ops.h"
#include "utente.h"
#include "utility.h"

static ptree_t *find_tree;

/* Prototipi funzioni statiche */
static void find_index_str(const char *str, reference_t *ref);
static int find_ref_cmp(void *a, void *b);

/****************************************************************************/
/* Inizializza il motore di ricerca nei post e crea l'albero dei prefissi   */
void find_init(void)
{
        struct room * r;
        long nmsg = 0, nroom = 0;
        
	/* Inizializzazione strutture dati albero dei prefissi */
	find_tree = ptCreate(find_ref_cmp);

        /* Read all messages present in each room */
        for (r = lista_room.first; r; r = r->next) {
		if (!(r->data->flags & RF_MAIL) && (r->data->flags & RF_FIND)) {
                        nroom++;
                        nmsg += find_cr8index(r);
		}
	}
	citta_logf("FIND: %ld messaggi da %ld room indicizzati.", nmsg, nroom);
}

/* Chiude il motore di ricerca ed libera le risorse associate. */
void find_free(void)
{
        ptDeallocate(find_tree);
	Free(find_tree);
}

/* Indicizza i messaggi nella room r */
long find_cr8index(struct room *r)
{
        long lowest, i, nmsg = 0;

        lowest = fm_lowest(r->data->fmnum);
        for (i = 0; i < r->data->maxmsg; i++) {
                if (r->msg->num[i] >= lowest) {
                        find_insert(r, i);
                        nmsg++;
                }
        }
        return nmsg;
}

/* Parse message #i in fullroom of room r and insert it in the prefix tree */
void find_insert(struct room *r, long i)
{
	msg_ref_t msgref[1];
	reference_t *ref;
	char autore[MAXLEN_UTNAME];
	char subject[LBUF];
	char *txt;
	const char *riga;
	int err;
	size_t len;
	char *header;
	struct dati_ut * ut = NULL;

	err = cp_get(r->data->fmnum, r->msg->num[i], r->msg->pos[i], &txt,
		     &len, &header);

	if (err == 0) {
		msgref->fmnum = r->data->fmnum;
		msgref->msgnum = r->msg->num[i];
		msgref->msgpos = r->msg->pos[i];
		msgref->local = r->msg->local[i];
		msgref->room = r->data->num;
		msgref->flags = extract_long(header, 5);
		if (!(msgref->flags & MF_ANONYMOUS)) {
		        extract(autore, header, 2);
			ut = trova_utente(autore);
			msgref->autore = (ut) ?  ut->matricola : -1; 
		} else
		        msgref->autore = -1;

	        ref = refCreate(msgref, sizeof(msg_ref_t));

		extract(subject, header, 4);
		if (subject[0])
		        find_index_str(subject, ref);

		for (riga = txt; riga; riga = argz_next(txt, len, riga))
		        find_index_str(riga, ref);
		refClose(ref);
	}
}

/* Comando server per effettuare una ricerca. */
void cmd_find(struct sessione *t, char *str)
{
        reflist_t *risultato, *rl;
	bool exact = true;
	int n, i;

        if (str[strlen(str)-1] == '#') {
	        exact = false;
		str[strlen(str)-1] = 0;
        }

        risultato = ptLook4Word(find_tree, str, exact);
	n = rlLength(risultato);

	if ( n == 0 ) {
	        cprintf(t, "%d Nessun risultato\n", ERROR);
		return;
	}

	/* Se ero gia' in una room virtuale, prima la elimino */
	kill_virtual_room(t);

	cr8_virtual_room(t, "Risultati Ricerca", n, -1, RF_FIND, NULL);

	i = 0;
        for (rl = risultato; rl; rl = rlNext(rl)) {
    	        msg_ref_t *msgref;
	        /* Il risultato di una ricerca e' disponibile all'utente */
	        /* solamente se ha accesso di lettura del messaggio      */
		msgref = (msg_ref_t *)(refData(rl->ref));
	        if (room_known(t, room_findn(msgref->room))) {
		        t->room->msg->num[i] = msgref->msgnum;
			t->room->msg->pos[i] = msgref->msgpos;
			t->room->msg->local[i] = msgref->local;
                        i++;
                        /* NON USO FM multipli 
                           t->room->msg->fmnum[i] = msgref->fmnum;
                        */
		} else
		        n--;
	}

	if (n == 0) {
	        kill_virtual_room(t);
	        cprintf(t, "%d\n", ERROR);
		return;
	}

	cprintf(t, "%d %d|%s|%c|%d|%ld\n", OK, n, t->room->data->name,
		room_type(t->room), UTR_DEFAULT, t->room->data->flags);
}


/* Per comparare due riferimenti  VEDERE */
static int find_ref_cmp(void *a, void *b)
{
        return( (((msg_ref_t *)a)->msgnum) - (((msg_ref_t *)b)->msgnum) );
}

/* Inserisce nel prefix tree il riferimento ref per ogni parola
 *  nella stringa str.                                           */
static void find_index_str(const char *str, reference_t *ref)
{
	char word[LBUF]; /* TODO decidere una lungh massima word */
        const char *tmp;
	char *stripped;
	size_t wlen;

	stripped = strstrip(str);
	tmp = stripped;
	while (*tmp) {
	        tmp = get_word(word, tmp, &wlen);
		if (wlen >= MINLEN_WORD)
		        ptAddWord(find_tree, word, ref);
	}
	Free(stripped);
}
