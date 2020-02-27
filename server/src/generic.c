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
* File: generic.c                                                           *
*       Comandi generici dal client                                         *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cittaserver.h"
#include "compress.h"
#include "generic.h"
#include "extract.h"
#include "file_flags.h"
#include "memstat.h"
#include "rooms.h"
#include "march.h"
#include "socket_citta.h"
#include "tabc_flags.h"
#include "ut_rooms.h"
#include "utility.h"
#include "versione.h"
#include "x.h"
#ifdef USE_FLOORS
#include "floors.h"
#endif

/* Directories where the messages from the server and help files are kept */
static const char *stdmsg_dir[] = {MESSAGGI_DIR, HELP_DIR};

/*Prototipi delle funzioni in questo file */
void cmd_info(struct sessione *t, char *cmd);
void cmd_hwho(struct sessione *t);
void cmd_time(struct sessione *t);
void legge_file(struct sessione *t,char *nome);
void cmd_help(struct sessione *t);
void cmd_lssh(struct sessione *t, char *cmd);
void cmd_ltrm(struct sessione *t, char *lock);
void cmd_msgi(struct sessione *t, char *buf);
void cmd_sdwn(struct sessione *t, char *buf);
void cmd_tabc(struct sessione *t, char *cmd);
void cmd_edng(struct sessione *t, char *buf);
void cmd_upgs(struct sessione *t);
void cmd_upgr(struct sessione *t);
void cmd_rcst(struct sessione *t);
static void legge_file_idx(struct sessione *t, const char *dir, int num);

/******************************************************************************
******************************************************************************/
/*
 * Lista connessioni: Nome utente + Host/Doing + Ora Login + Room + Chat
 */
void cmd_hwho(struct sessione *t)
{
        char nm[LBUF], rm[ROOMNAMELEN], host[LBUF];
        struct sessione *punto;
        int num_ospiti, known = 0;

        cprintf(t, "%d Lista connessioni (HWHO)\n", SEGUE_LISTA);

        num_ospiti = 0;
        for (punto = lista_sessioni; punto; punto = punto->prossima) {
		if (t->logged_in)
			known = room_known(t, punto->room);
		if (punto->logged_in) {
			switch (punto->stato) {
			case CON_COMANDI:
                                if ((punto->idle.min!=0)||(punto->idle.ore!=0))
                                        sprintf(nm,"%s.", punto->utente->nome);
                                else
                                        strcpy(nm, punto->utente->nome);
                                break;
			case CON_PROFILE:
                                sprintf(nm, "%s#", punto->utente->nome);
                                break;
			case CON_POST:
				if ((punto->room->data->flags & RF_ANONYMOUS)
				    ||(punto->room->data->flags&RF_ASKANONYM)
				    || (known == 0))
					sprintf(nm,"%s",punto->utente->nome);
				else
					sprintf(nm,"%s+",punto->utente->nome);
                                break;
			case CON_XMSG:
			case CON_BROAD:
			case CON_CHAT:
                                sprintf(nm, "%s&", punto->utente->nome);
                                break;
			case CON_CONF:
                                sprintf(nm, "%s^", punto->utente->nome);
                                break;
			case CON_REG:
                                sprintf(nm, "%s_", punto->utente->nome);
                                break;
			case CON_REFERENDUM:
			case CON_REF_PARM:
                                sprintf(nm, "%s?", punto->utente->nome);
                                break;
			case CON_BUG_REP:
                                sprintf(nm, "%s*", punto->utente->nome);
                                break;
			case CON_UPLOAD:
                                sprintf(nm, "%s>", punto->utente->nome);
                                break;
			case CON_ROOM_EDIT:
			case CON_ROOM_INFO:
			case CON_FLOOR_EDIT:
			case CON_FLOOR_INFO:
			case CON_EUSR:
			case CON_BANNER:
			case CON_NEWS:
                                sprintf(nm, "%s%%", punto->utente->nome);
                                break;
			case CON_LOCK:
                                sprintf(nm, "(%s)", punto->utente->nome);
                                break;
			case CON_SUBSHELL:
                                sprintf(nm, "{%s}", punto->utente->nome);
                                break;
			default:
                                sprintf(nm, "%s", punto->utente->nome);
                                break;
                        }
			strcpy(rm, punto->room->data->name);
                } else {
                        /* TODO: leave the text to the client */
                        strcpy(nm, "(login in corso)");
			strcpy(rm, "");
		}
                if (strncmp(nm, "Ospite", 6)) {
			if (!known) {
                                /* TODO: leave the text to the client */
				strcpy(rm, "[ Boh...                ]");
			} else if (punto->room->data->type == ROOM_DYNAMICAL) {
			        sprintf(rm, ":%-17s:",
                                        punto->room->data->name);
                        }
			if (punto->doing)
				sprintf(host, "[%s", punto->doing);
			else
				sprintf(host, "%s", punto->host);

			cprintf(t, "%d %d|%s||%ld|%s|%d\n", OK,
				punto->socket_descr, nm, punto->ora_login,
				rm, punto->canale_chat);
			cprintf(t, "%d %s\n", OK, host);
                } else
                        num_ospiti++;
        }
	cprintf(t, "000\n");

        /* Invia al client il numero di ospiti presenti */
        cprintf(t, "%d %d\n", OK, num_ospiti);
}

/*
 * Diciamo al client chi siamo
 */
void cmd_info(struct sessione *t, char *cmd)
{
        char rhost[LBUF], compress[LBUF];
        struct sessione *punto;
        int ut_conn = 0;

        /* TODO questa va eliminata: mettere contatore globale! */
        for (punto = lista_sessioni; punto; punto = punto->prossima)
                ut_conn++;

        if (cmd[0] == 'r') { /* e' una connessione remota */
                /* verifico che la connessione e' effettivamente remota */
		/* DA VEDERE!!! */
                extract(rhost, cmd, 1);
                /*if (!strcmp(t->host, citta_nodo) ||
                    !strcmp(t->host, "localhost") ||
                    !strcmp(t->host, "localhost.localdomain")) {*/
                dati_server.remote_cl++;
                strcpy(t->host, rhost);
                /*} else {
                        citta_logf("SECURE: finta connessione remota da [%s].",rhost);
                        t->stato = CON_CHIUSA;
                        cprintf(t, "%d\n", ERROR);
                        return;
                }*/
		/* dati_server.remote_cl++; */
                compress[0] = 0; /* nessuna compressione in remoto */
        } else if (cmd[0] == 'l') {/* e' una connessione locale */
                dati_server.local_cl++;
                extract(compress, cmd, 1);
        } else {
                cprintf(t, "%d\n", ERROR);
                return;
        }
        compress_check(t, compress);
        cprintf(t, "%d %s|%d|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%s\n", OK,
		citta_soft, SERVER_VERSION_CODE, citta_nodo, citta_dove,
		CLIENT_VERSION_CODE, NCANALICHAT, MAXLINEEBUG, MAXLINEEBX,
		MAXLINEENEWS, MAXLINEEPOST, MAXLINEEPRFL, MAXLINEEROOMINFO,
		SERVER_FLAGS, t->compressione);
        compress_set(t);
}

/*
 * Invia ora e data al client
 */
void cmd_time(struct sessione *t)
{
        long ora;

        ora = time(0);
        cprintf(t, "%d|%ld\n", OK, ora);
}

/*
 *
 */
void legge_file(struct sessione *t, char *nome)
{
        FILE *fp;
        char buf[LBUF];

        /* Apre file */
        fp = fopen(nome, "r");
        if (fp == NULL) {
                citta_logf("SYSERR: Errore apertura file [%s] da [%s]", nome,
		     t->utente->nome);
                cprintf(t, "%d Errore in apertura file.\n", ERROR);
                return;
        }

        /* Legge file e mette in coda */
        cprintf(t, "%d Legge file %s\n", SEGUE_LISTA, nome);
        while (fgets(buf, LBUF, fp) != NULL)
                cprintf(t, "%d %s", OK, buf);

        /* Chiude file */
        fclose(fp);

        cprintf(t, "000\n");
}

/*
 * Legge il file numero 'num' dalla directory 'dir'.
 * L'indice dei file sta in 'dir'/indice.
 * Se 'num' e' negativo, invia l'indice al client.
 */
static void legge_file_idx(struct sessione *t, const char *dir, int num)
{
        FILE *fp;
        char buf[LBUF], filename[LBUF];
	int i;

	if (num >= 0) { /* Cerca il nome del file nell'indice */
		sprintf(filename, "%s/%s", dir, STDFILE_INDEX);
		fp = fopen(filename, "r");
		if (fp == NULL) {
			citta_logf(
                             "SYSERR: Errore apertura indice [%s] da [%s]",
			     filename, t->utente->nome);
			cprintf(t, "%d Errore in apertura file.\n", ERROR);
			return;
		}
		buf[0] = 0;
		i = 0;
		while ((fgets(buf, LBUF, fp) != NULL) && (i++ < num));
		fclose(fp);
		extract(filename, buf, 0);
		sprintf(buf, "%s/%s", dir, filename);
	} else { /* Invia l'indice */
		sprintf(buf, "%s/%s", dir, STDFILE_INDEX);
        }

        /* Legge file e mette in coda */
        fp = fopen(buf, "r");
        if (fp == NULL) {
                citta_logf("SYSERR: Errore apertura file [%s] da [%s]", buf,
		     t->utente->nome);
                cprintf(t, "%d Errore in apertura file.\n", ERROR);
                return;
        }
        cprintf(t, "%d\n", SEGUE_LISTA);
        while (fgets(buf, LBUF, fp) != NULL)
                cprintf(t, "%d %s", OK, buf);
        fclose(fp);
        cprintf(t, "000\n");
}

/*
 * Invia al client il file di help
 */
void cmd_help(struct sessione *t)
{
        legge_file(t, FILE_HELP);
}

/*
 * Invia al client i messaggi standard e i file di help.
 * I files vengono trovati attraverso un indice.
 */

void cmd_msgi(struct sessione *t, char *buf)
{
        int msgtype, id;

        msgtype = extract_int(buf, 0);
        id = extract_int(buf, 1);

        if (msgtype >= 0 && msgtype < STDMSG_COUNT) {
                legge_file_idx(t, stdmsg_dir[msgtype], id);
        } else {
		cprintf(t, "%d\n", ERROR);
        }
}

/*
 * Comando per sysop per eseguire uno shutdown del server dopo un certo
 * numero di minuti.
 *
 * Sintassi: "SDWN delay|reboot"
 *           delay  : tempo prima dello shutdown
 *                    (delay < 0 per interrompere lo shutdown)
 *           reboot : se 0 fa un 'touch .killscript'
 */
void cmd_sdwn(struct sessione *t, char *buf)
{
        char tmp[LBUF];
        size_t tmplen;
        int tempo;
        struct sessione *s;

	tempo = extract_int(buf, 0);
        if (tempo < 0) {  /* Interrompe lo shutdown */
                if (citta_shutdown) {
			citta_reboot = true;
                        citta_shutdown = 0;
                        citta_sdwntimer = 0;
                        citta_logf("AIDE: Shutdown cancellato da [%s].",
                                t->utente->nome);
                        sprintf(tmp, "%d\n", SDWC);
                        tmplen = strlen(tmp);
                        for (s = lista_sessioni; s; s = s->prossima) {
                                if (s != t) {
                                        metti_in_coda(&s->output, tmp, tmplen);
					s->bytes_out += tmplen;
				}
                        }
                }
        } else {
		if (extract_int(buf, 1) == 0)
			citta_reboot = false;
                citta_shutdown = 1;
                citta_sdwntimer = tempo * (60*FREQUENZA) + 1;
                citta_logf("AIDE: Shutdown tra %d min di [%s].", tempo,
		     t->utente->nome);
        }
        cprintf(t, "%d\n", OK);
}

/*
 * TAB Completion.
 * "TABC foo|n|mode" restituisce la n-esima ricorrenza di una parola
 * che inizia con "foo", dove la parola e' nell'insieme dettato dal mode:
 * mode = 0 : completa con i nomi degli utenti connessi
 * mode = 1 : completa con i nomi degli utenti in file_utenti
 * mode = 2 : completa con i nomi dei file di help (DA FARE)
 * mode = 3 : completa con i nomi delle room conosciute
 * mode = 4 : completa con i nomi delle room con nuovi messaggi
 * mode = 5 : completa con i nomi dei floor conosciuti.
 * mode = 6 : completa con i nomi degli utenti con un blog attivo.
 */
void cmd_tabc (struct sessione *t, char *cmd)
{
        char str[LBUF], completion[LBUF]="";
	char *stringa;
        int num, mode;
        struct sessione *sess;
        struct lista_ut *ut;
        struct room *room;
        int count = 0;
        size_t lung;
#ifdef USE_FLOORS
        struct floor *floor;
#endif

	extract(str, cmd, 0);
	num = extract_int(cmd, 1);
	mode = extract_int(cmd, 2);
	if ((mode == TC_MODE_ROOMKNOWN) && (str[0] == ':')) {
		mode = TC_MODE_FLOOR;
		stringa = str+1;
	} else
		stringa = str;
	lung = strlen(stringa);
	switch (mode) {
	 case TC_MODE_X:     /* completa con nomi utenti connessi */
	 case TC_MODE_EMOTE: /* completa con nomi utenti connessi */
		sess=lista_sessioni;
		while (sess) {
			if ((sess->utente) &&
			    (strlen(sess->utente->nome) > lung) &&
			    (!strncasecmp(sess->utente->nome, stringa,lung))) {
				count++;
				if (count == num+1) {
					strcpy(completion, sess->utente->nome);
					sess = NULL;
				} else
					sess = sess->prossima;
			} else
				sess = sess->prossima;
		}
		break;

	 case TC_MODE_USER: /* completa con nomi in file_utenti */
		ut = lista_utenti;
		while (ut) {
			if ((strlen(ut->dati->nome) > lung) &&
			    (!strncasecmp(ut->dati->nome, stringa, lung))) {
				count++;
				if (count == (num + 1)) {
					strcpy(completion, ut->dati->nome);
					ut = NULL;
				} else
					ut = ut->prossimo;
			} else
				ut = ut->prossimo;
		}
		break;

         case TC_MODE_MAIL: /* completa con nomi utenti con mailbox */
		ut = lista_utenti;
		while (ut) {
			if ((strlen(ut->dati->nome) > lung) &&
			    (!strncasecmp(ut->dati->nome, stringa, lung))
                            && (ut->dati->livello >= LVL_NORMALE)) {
				count++;
				if (count == (num + 1)) {
					strcpy(completion, ut->dati->nome);
					ut = NULL;
				} else
					ut = ut->prossimo;
			} else
				ut = ut->prossimo;
		}
		break;

	 case TC_MODE_ROOMKNOWN: /* completa con nomi room conosciute */
		room = lista_room.first;
		while (room) {
			if ((room_known(t, room) == 1) &&
			    (strlen(room->data->name) > lung) &&
			    (!strncasecmp(room->data->name, stringa, lung))) {
				count++;
				if (count == num+1) {
					strcpy(completion, room->data->name);
					room = NULL;
				} else
					room = room->next;
			} else
				room = room->next;
		}
		break;

#ifdef USE_FLOORS
	 case TC_MODE_FLOOR: /* completa con nomi dei floor conosciuti */
		floor = floor_list.first;
		while (floor) {
			if ((floor_known(t, floor) == 1) &&
			    (strlen(floor->data->name) > lung) &&
			    (!strncasecmp(floor->data->name, stringa, lung))) {
				count++;
				if (count == num+1) {
					strcpy(completion, floor->data->name);
					floor = NULL;
				} else
					floor = floor->next;
			} else
				floor = floor->next;
		}
		break;
#endif

#ifdef USE_BLOG
	 case TC_MODE_BLOG: /* completa con nomi utenti con blog attivo */
		ut = lista_utenti;
		while (ut) {
			if ((strlen(ut->dati->nome) > lung) &&
			    (!strncasecmp(ut->dati->nome, stringa, lung))
			    && (ut->dati->flags[7] & UT_BLOG_ON)) {
				count++;
				if (count == (num + 1)) {
					strcpy(completion, ut->dati->nome);
					ut = NULL;
				} else
					ut = ut->prossimo;
			} else
				ut = ut->prossimo;
		}
		break;
#endif

	 default:
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
		return;
	}
	if (strlen(completion)) {
		if (str[0] == ':')
			cprintf(t, "%d :%s\n", OK, completion);
		else
			cprintf(t, "%d %s\n", OK, completion);
	} else
		cprintf(t, "%d\n", ERROR);
}


/*
 * Entro in una Subshell con il client locale
 */
void cmd_lssh(struct sessione *t, char *cmd)
{
	if (cmd[0] == '1') { /* Entro nella subshell */
		/* TODO fare controllo che il client non e' remoto */
		t->occupato = 1;
		t->stato = CON_SUBSHELL;
		cprintf(t, "%d\n", OK);
	} else if (cmd[0] == '0') { /* Esco dalla subshell */
		t->occupato = 0;
		t->stato = CON_COMANDI;
		cprintf(t, "%d\n", OK);
	} else
		cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
}

/*
 * Lock terminale utente (a=1: lock, a=0: unlock)
 */
void cmd_ltrm(struct sessione *t, char *lock)
{
	char pwd[LBUF];

        if (extract_int(lock, 0)) {
                t->occupato = 1;
                t->stato = CON_LOCK;
        } else {
		if (t->stato != CON_LOCK) {
			cprintf(t, "%d\n", ERROR+WRONG_STATE);
			return;
		}
		extract(pwd, lock, 1);
		if (check_password(pwd, t->utente->password) == 0) {
			cprintf(t, "%d\n", ERROR+PASSWORD);
			return;
		}
                t->occupato = 0;
                t->stato = CON_COMANDI;
        }
        cprintf(t, "%d\n", OK);
}

/*
 * Set/Modify doing.
 * Syntax: "EDNG mode|doing"
 *         mode = 0 disable, 1 enable.
 */
void cmd_edng(struct sessione *t, char *buf)
{
        int mode;

        mode = extract_int(buf, 0);
        if ((mode) && (buf[1] != '|')) {
		cprintf(t, "%d\n", ERROR);
		return;
	}
        if (t->doing != NULL)
                Free(t->doing);
        if (extract_int(buf, 0) == 0)
		t->doing = NULL;
	else
		t->doing = Strdup(buf+2);
	cprintf(t, "%d\n", OK);
}

/*
 * UPDate Client: Setta il flag SUT_UPDATE di tutti i client. In questo modo,
 * al primo collegamento con il client nuovo, possono passare nella
 * riconfigurazione delle nuove opzioni.
 */
void cmd_upgs(struct sessione *t)
{
	sut_set_all(0, SUT_UPGRADE);
	citta_logf("SUT_UPDATE set to all by [%s].", t->utente->nome);
	cprintf(t, "%d\n", OK);
}

/*
 * UPDate client Reset : Resets the SUT_UPDATE flag of the user.
 */
void cmd_upgr(struct sessione *t)
{
	t->utente->sflags[0] &= ~SUT_UPGRADE;
	cprintf(t, "%d\n", OK);
}

/*
 * Request ConSenT: clears the SUT_NEED_CONSTENT flag for all users, so
 * that the next time they connect they will have to accept the terms.
 * Sysops should use this command each time the terms are changed.
 */
void cmd_rcst(struct sessione *t)
{
	sut_set_all(0, SUT_NEED_CONSENT);
	citta_logf("SUT_NEED_CONSENT set for all users by [%s].",
                   t->utente->nome);
	cprintf(t, "%d\n", OK);
}
