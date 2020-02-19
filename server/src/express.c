/*
 *  Copyright (C) 1999-2006 by Marco Caldarelli and Riccardo Vianello
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
* File: x.c                                                                 *
*       comandi di broadcast, express message e chat.                       *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "x.h"
#include "extract.h"
#include "memstat.h"
#include "utente.h"
#include "strutture.h"
#include "ut_rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void cmd_brdc(struct sessione *t);
void cmd_bend(struct sessione *t);
void citta_broadcast(char *msg);
void cmd_xmsg(struct sessione *t);
void cmd_xend(struct sessione *t);
void cmd_xinm(struct sessione *t, char *cmd);
void cmd_cmsg(struct sessione *t);
void cmd_cend(struct sessione *t);
void cmd_casc(struct sessione *t, char *cc);
void cmd_clas(struct sessione *t);
void cmd_cwho(struct sessione *t);
void chat_timeout(struct sessione *t);

/* Prototipi funzioni statiche in questo file */
static void bx_header(struct sessione *t, char *header, int cmd);
static void broadcast(struct sessione *t, char *buf);
static void send_chat(struct sessione *t, char canale, char *buf);
static bool check_dest(struct sessione *t, char *a_chi);
static bool send_multix(struct sessione *t, int righe);

/*****************************************************************************
 *****************************************************************************/
/*
 * Genera l'header per Broadcast e Xmsg
 */
static void bx_header(struct sessione *t, char *header, int cmd)
{
        time_t ora;
        struct tm *tmst;

	time(&ora);
	tmst = localtime(&ora);
	sprintf(header, "%d %s|%d|%d\n", cmd, t->utente->nome,
		tmst->tm_hour, tmst->tm_min);
}

/*
 * Invia un broadcast a tutti gli utenti
 */
static void broadcast(struct sessione *t, char *buf)
{
	struct sessione *punto;
	size_t buflen;

        buflen = strlen(buf);
	for (punto = lista_sessioni; punto; punto = punto->prossima)
		if ((punto->utente) && (punto != t)) {
			metti_in_coda(&punto->output, buf, buflen);
			punto->bytes_out += buflen;
		}
}

/*
 * Invia una stringa a tutti gli utenti nella chat
 */
static void send_chat(struct sessione *t, char canale, char *buf)
{
	struct sessione *punto;
	size_t buflen;
	
        buflen = strlen(buf);
        for (punto = lista_sessioni; punto; punto = punto->prossima)
		if ((punto->utente)&&(punto!=t)&&(punto->canale_chat==canale)){
			metti_in_coda(&punto->output, buf, buflen);
			punto->bytes_out += buflen;
		}
}
/****************************************************************************/
/*
 * Inizializza l'invio di un broadcast
 */
void cmd_brdc(struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_BROADCAST;
	t->text_max = MAXLINEEBX;
	t->stato = CON_BROAD;
	t->occupato = 1;
}

/*
 * Fine broadcast: se tutto e' andato bene lo invia.
 */
void cmd_bend(struct sessione *t)
{
        char buf[LBUF], header[LBUF];
        int a, righe;

	if (t->text_com != TXT_BROADCAST) {
                cprintf(t, "%d\n", ERROR+X_TXT_NON_INIT);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d\n", ERROR+X_EMPTY);
        } else {
                cprintf(t, "%d\n", OK);
		bx_header(t, header, BHDR);
		broadcast(t, header);
		txt_rewind(t->text);
		for (a = 0; a < righe; a++) {
			sprintf(buf,"%d %s\n", BTXT, txt_get(t->text));
			broadcast(t, buf);
		}
		sprintf(buf, "%d\n", BEND);
		broadcast(t, buf);
		dati_server.broadcast++;
	}
	reset_text(t);
}

/*
 * Invia un broadcast a tutti gli utenti da parte di Cittadella/UX.
 * Il broadcast e' lungo una riga, in msg.
 */
void citta_broadcast(char *msg)
{
	struct sessione *punto;
	char header[LBUF], buf[LBUF], bend[LBUF];
        time_t ora;
        struct tm *tmst;
	size_t headlen, buflen, bendlen, totlen;

	if ((!msg) || (msg[0] == '\0') || (msg[0] == '\n'))
		return;

	time(&ora);
	tmst = localtime(&ora);
	sprintf(header, "%d Cittadella/UX|%d|%d\n", BHDR,
		tmst->tm_hour, tmst->tm_min);
        headlen = strlen(header);
	sprintf(buf, "%d %s\n", BTXT, msg);
	buflen = strlen(buf);
	sprintf(bend, "%d\n", BEND);
	bendlen = strlen(bend);
	totlen = headlen + buflen + bendlen;

	for (punto = lista_sessioni; punto; punto = punto->prossima)
		if (punto->utente) {
			metti_in_coda(&punto->output, header, headlen);
			metti_in_coda(&punto->output, buf, buflen);
			metti_in_coda(&punto->output, bend, bendlen);
			punto->bytes_out += totlen;
		}
}

/****************************************************************************/
/*
 * Inizializza invio X-message
 */
void cmd_xmsg(struct sessione *t)
{
        if (t->utente->sflags[1] & SUT_NOX) {
                cprintf(t, "%d Non autorizzato.\n", ERROR+X_ACC_PROIBITO);
                return;
        }
        t->stato = CON_XMSG;
        t->occupato = 1;
        init_txt(t);
        t->text_com = TXT_XMSG;
        t->text_max = MAXLINEEBX;
}

/*
 * Aggiungi un destinatario per il multi-X.
 */
void cmd_dest(struct sessione *t, char *a_chi)
{
        check_dest(t, a_chi);
}

/*
 * Controlla se destinatario X e' ok, lo aggiunge alla lista e risponde
 * al client. Ritorna TRUE se il dest e' stato messo in lista.
 */
static bool check_dest(struct sessione *t, char *a_chi)
{
        struct sessione *sess;
	struct dati_ut *ut;

	if ((ut = trova_utente(a_chi)) == NULL)
                cprintf(t, "%d %s\n", ERROR+X_NO_SUCH_USER, a_chi);
	else if ((sess = collegato(a_chi)) == NULL)
                cprintf(t, "%d %s|%d\n", ERROR+X_UTENTE_NON_COLLEGATO, a_chi,
			SESSON(ut));
	else if (sess == t)
                cprintf(t, "%d\n", ERROR+X_SELF);
	else if (!is_enemy(t->utente, sess->utente) &&
		 ((sess->utente->flags[3] & UT_X) ||
		  ((sess->utente->flags[3] & UT_XFRIEND) &&
		   is_friend(t->utente, sess->utente)))) {
                if (t->num_rcpt < MAX_RCPT) {
                        t->rcpt[t->num_rcpt] = sess->utente->matricola;
                        t->num_rcpt++;
                }
		cprintf(t, "%d\n", OK);
                return true;
        } else
                cprintf(t, "%d %s\n", ERROR+X_RIFIUTA, a_chi);
        return false;
}

/*
 * Fine X-message: se tutto e' andato bene lo invia al destinatario.
 */
void cmd_xend(struct sessione *t)
{
        int righe;

	if (t->text_com != TXT_XMSG)
                cprintf(t, "%d\n", ERROR+X_TXT_NON_INIT);
	else if (t->num_rcpt == 0)
                cprintf(t, "%d\n", ERROR+X_NO_DEST);        
	else if ((righe = txt_len(t->text)) == 0)
                cprintf(t, "%d\n", ERROR+X_EMPTY);
	else {
		cprintf(t, "%d\n", SEGUE_LISTA);
                if (!send_multix(t, righe))
			return;
	}
        t->num_rcpt = 0;
	reset_text(t);
}

/*
 * Fine X-message: se tutto e' andato bene lo invia al destinatario.
 *                 Ritorna TRUE se sono stati mandati tutti gli X.
 */
static bool send_multix(struct sessione *t, int righe)
{
        char header[LBUF];
	bool status[MAX_RCPT];
        int inviati = 0, scollegati = 0;
        int i, j, a;
	size_t len;
	struct sessione *ricevente;
        struct dati_ut *ut;

        bx_header(t, header, XHDR);
        for (i = 0; i < t->num_rcpt; i++) {
                if ( (ricevente = collegato_n(t->rcpt[i])) == NULL) {
			status[i] = false;
                        if ( (ut = trova_utente_n(t->rcpt[i])) == NULL) {
                                cprintf(t, "%d %ld\n",
                                        ERROR+X_UTENTE_CANCELLATO, t->rcpt[i]);
                                t->rcpt[i] = -1;
                        } else {
                                cprintf(t, "%d %s|%d\n", ERROR+X_SCOLLEGATO,
                                        ut->nome, SESSON(ut));
                                scollegati++;
                        }
                } else {
			len = strlen(header);
			metti_in_coda(&ricevente->output, header, len);
                        ricevente->bytes_out += len;
                        /* Questo e' poco efficiente: migliorare! */
                        for (j = 0; j < t->num_rcpt; j++)
                                cprintf(ricevente, "%d %s\n", DEST,
                                        trova_utente_n(t->rcpt[j])->nome);
                        txt_rewind(t->text);
                        for (a = 0; a < righe; a++)
                                cprintf(ricevente, "%d %s\n", XTXT,
                                        txt_get(t->text));
                        cprintf(ricevente, "%d\n", XEND);

                        dati_server.X++;
                        inviati++;

                        if (ricevente->occupato)
                                cprintf(t, "%d %s|%d\n", ERROR+X_OCCUPATO,
                                        ricevente->utente->nome,
                                        SESSON(ricevente->utente));
                        else
                                cprintf(t, "%d\n", OK);
			status[i] = true;
                }
        }

	/* Toglie dalla lista gli utenti che hanno ricevuto l'X. */
        for (i = 0; i < t->num_rcpt; i++)
		if (status[i])
			t->rcpt[i] = -1;

        if (inviati > 0)
                t->utente->x_msg++;

        cprintf(t, "000 %d|%d\n", inviati, scollegati);

	if (scollegati)
		return false;

	return true;
}

/*
 * X IN Mail: Si chiama questa funzione quando il destinatario si e'
 *            scollegato durante la scrittura dell'X.
 * Sintassi : "XINM mode" : mode = 1 invia l'X in mail, 0 lascia perdere.
 */
void cmd_xinm(struct sessione *t, char *cmd)
{
	struct dati_ut *caio;
        int i;
        int inviati = 0;
	bool err = true;

	if (t->text_com != TXT_XMSG) {
		cprintf(t, "%d\n", ERROR);
        }

	if (extract_int(cmd,0) == 0){
		err = false;
		inviati=0;
	} else {
                for (i = 0; i < t->num_rcpt; i++) {
                        if ((t->rcpt[i] != -1) &&
                            ( (caio = trova_utente_n(t->rcpt[i])) != NULL)) {
                                if (send_mail(caio, t->utente, "[eXpress message arrivato mentre ti scollegavi]", t->text)) {
                                        err = false;
                                        inviati++;
                                }
                        }
                }
        }

        if (err)
		cprintf(t, "%d\n", ERROR);
        else
                cprintf(t, "%d %d\n", OK, inviati);

        t->num_rcpt = 0;
	reset_text(t);
}

/****************************************************************************/
/*
 *  Inizializza invio messaggio in chat
 */
void cmd_cmsg(struct sessione *t)
{
	if (t->canale_chat != 0) {
                init_txt(t);
		t->text_com = TXT_CHAT;
		t->text_max = MAXLINEEBX;
                t->stato = CON_CHAT;
                t->occupato = 1;
		t->chat_timeout = 0;
        } else
                cprintf(t, "%d Non sei connesso alla chat.\n", ERROR);
}

/*
 * Fine messaggio in chat: se tutto e' andato bene lo invia.
 */
void cmd_cend(struct sessione *t)
{
        char buf[LBUF], *str;
        char canale;
        int a, righe;

        canale = t->canale_chat;

	if (t->text_com != TXT_CHAT)
                cprintf(t, "%d Messaggio in chat non inizializzato.\n", ERROR);
	else if (canale == 0)
                cprintf(t, "%d Non sei connesso alla chat.\n", ERROR);
	else if ((righe = txt_len(t->text)) == 0)
                cprintf(t, "%d Nessun testo. Chat msg annullato.\n", ERROR);
	else {
		sprintf(buf, "%d %s\n", CHDR, t->utente->nome);
		send_chat(t, canale, buf);
		txt_rewind(t->text);
		for (a = 0; a < righe; a++) {
			asprintf(&str, "%d %s\n", CTXT, txt_get(t->text));
			send_chat(t, canale, str);
			free(str);
		}
		sprintf(buf, "%d Fine chat msg.\n", CEND);
		send_chat(t, canale, buf);
                cprintf(t, "%d Messaggio inviato in chat.\n", OK);

		/* Aggiorna statistiche */
		t->utente->chat++;
		dati_server.chat++;
	}
	reset_text(t);
	t->chat_timeout = 0;
}

/*
 * Mette l'utente in ascolto del canale chat numero cc e lo notifica
 * agli altri utenti presenti nel canale.
 */
void cmd_casc(struct sessione *t, char *cc)
{
        char buf[LBUF];
        char canale;

        if (t->utente->sflags[1] & SUT_NOCHAT) {
                cprintf(t, "%d Non autorizzato.\n", ERROR+ACC_PROIBITO);
                return;
        }

        canale = extract_int(cc,0);
        if (canale > NCANALICHAT) {
                cprintf(t, "%d Canale non attivato.\n", ERROR+ARG_NON_VAL);
                return;
        }

        if (!((canale<NCANALICHAT) || (canale==NCANALICHAT &&
			     t->utente->livello>=MINLVL_SYSOPCHAT))) {
                cprintf(t, "%d Accesso proibito alla SYSOPCHAT.\n", ERROR);
                return;
        }

        cprintf(t, "%d Ok, sei connesso al canale %d.\n",OK,canale);

        /* Se gia' connesso a quel canale: nessuna notifica. */
        if (t->canale_chat == canale)
                return;

        /* Notifica uscita dalla chat */
        if (t->canale_chat) {
                sprintf(buf, "%d %c%s\n", LGOC, SESSO(t->utente),
                        t->utente->nome);
                send_chat(t, t->canale_chat, buf);
        }

        /* Notifica l'entrata in chat */
        sprintf(buf, "%d %c%s\n", LGIC, SESSO(t->utente), t->utente->nome);
	send_chat(t, canale, buf);

        t->canale_chat = canale;
	t->chat_timeout = 0;
}

/*
 * Abbandona il canale chat corrente e notifica l'uscita agli altri
 * utenti presenti nel canale.
 */
void cmd_clas(struct sessione *t)
{
        char buf[LBUF];
  
        if (t->canale_chat == 0) {
                cprintf(t, "%d Non sei connesso alla chat.\n", ERROR);
                return;
        }
        cprintf(t, "%d Ok, hai lasciato la chat.\n", OK);
        /* Notifica uscita dalla chat */
        sprintf(buf, "%d %c%s\n", LGOC, SESSO(t->utente), t->utente->nome);
	send_chat(t, t->canale_chat, buf);
        t->canale_chat = 0;
}

/*
 * Invia la lista degli utenti collegati a un canale della chat.
 */
void cmd_cwho(struct sessione *t)
{
        struct sessione *punto;

        cprintf(t, "%d Lista utenti in chat (CWHO)\n", SEGUE_LISTA);
        for (punto = lista_sessioni; punto; punto = punto->prossima)
                if ((punto->utente) && (punto->canale_chat != 0)
                    && (punto->logged_in))
                        cprintf(t, "%d %d|%s\n", OK, punto->canale_chat,
                                punto->utente->nome);
        cprintf(t, "000\n");
}

/*
 * Caccia l'utente per timeout dalla chat e notifica.
 */
void chat_timeout(struct sessione *t)
{
        char buf[LBUF];
  
        cprintf(t, "%d chat timeout.\n", COUT);

        /* Notifica uscita dalla chat */
        sprintf(buf, "%d %c%s\n", LOIC, SESSO(t->utente), t->utente->nome);
	send_chat(t, t->canale_chat, buf);
        t->canale_chat = 0;
}

/****************************************************************************/
