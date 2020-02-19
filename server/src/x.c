/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/*****************************************************************************
 * Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File: x.c                                                                 *
 *       comandi per la trasmissione di testi da client a server.            *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "x.h"
#include "banner.h"
#include "blog.h"
#include "email.h"
#include "extract.h"
#include "fileserver.h"
#include "floors.h"
#include "mail.h"
#include "memstat.h"
#include "messaggi.h"
#include "room_ops.h"
#include "urna-crea.h"
#include "urna-comm.h"
#include "urna-vota.h"
#include "utente.h"
#include "strutture.h"
#include "ut_rooms.h"
#include "utility.h"
#include "urna_errori.h"
#include "urna-servizi.h"


/*****************************************************************************
 *****************************************************************************/
/*
 * Inizia l'invio di un testo
 */
void init_txt(struct sessione *t)
{
        if ((t->text) != NULL)
		txt_free(&t->text);
        t->text = txt_create();
        cprintf(t, "%d\n", INVIA_TESTO);
}

/*
 * 
 */
void reset_text(struct sessione *t)
{
        if (t->text)
                txt_free(&t->text);
	t->text_com = 0;
	t->text_max = 0;
        t->riga = 0;
        t->stato = CON_COMANDI;
        t->occupato = 0;
}

/*
 * Riceve una riga del testo
 */
void cmd_text(struct sessione *t, char *txt)
{
        if ((t->utente) && (t->text_com) && (t->riga < t->text_max)) {
/*
	        if ((t->text_com == TXT_POST) 
		    || (t->text_com == TXT_MAIL)
		    || (t->text_com == TXT_MAILSYSOP)
		    || (t->text_com == TXT_MAILAIDE)
		    || (t->text_com == TXT_MAILRAIDE))
	                sprintf(txt+strlen(txt), "\n");
 */
		/* Sporco ma funziona... */
		if (*txt == '\0') {
			txt[0] = ' ';
			txt[1] = '\0';
		}
		txt_put(t->text, txt);
		t->riga++;
        } else
		cprintf(t, "%d\n", ERROR);
}

/*
 * Abort TeXT: interrompe l'invio di un testo da parte del client.
 */
void cmd_atxt(struct sessione *t)
{
	cprintf(t, "%d\n", OK);
        if (t->stato == CON_POST) {
	        t->room->lock = false;
                fs_cancel_all_bookings(t->utente->matricola);
        }
	t->rlock_timeout = 0;
        t->num_rcpt = 0;
	reset_text(t);
}

/****************************************************************************/
/*
 * Inizializza l'invio di un bug report.
 */
void cmd_bugb(struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_BUG_REPORT;
	t->text_max = MAXLINEEBUG;
	t->stato = CON_BUG_REP;
	t->occupato = 1;
}

/*
 * Fine Bug Report: se tutto e' andato bene lo invia.
 */
void cmd_buge(struct sessione *t)
{
        char subj[LBUF];
        int righe;

	if (t->text_com != TXT_BUG_REPORT)
                cprintf(t, "%d Bug report non inizializzato.\n", ERROR);
	else if ((righe = txt_len(t->text)) == 0)
                cprintf(t, "%d Nessun testo. Bug Report annullato.\n", ERROR);
	else {
                sprintf(subj, "Bug report da %s (%s)", t->utente->nome,
			t->utente->email);
		if (!send_email(t->text, NULL, subj, BBS_EMAIL, EMAIL_SINGLE)){
                        citta_logf("SYSERR: Bug Report of %s not sent.",
			     t->utente->nome);
			cprintf(t, "%d Problemi invio bug report.\n", ERROR);
		} else
			cprintf(t, "%d Bug Report inviato.\n", OK);
        }
	reset_text(t);
}

/****************************************************************************/
/*
 * Inizia la modifica del profile
 */
void cmd_prfb (struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_PROFILE;
	t->text_max = MAXLINEEPRFL;
	t->stato = CON_PROFILE;
	t->occupato = 1;
	t->riga = 0;
}

/*
 * PRoFile End: fine trasmissione del profile
 */
void cmd_prfe (struct sessione *t)
{
        int a, righe;
        FILE *fp;
        char filename[LBUF];

	if (t->text_com != TXT_PROFILE) {
                cprintf(t, "%d Profile non inizializzato.\n",
			ERROR+WRONG_STATE);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d Nessun testo. Edit profile annullato.\n",
			ERROR+ARG_VUOTO);
	} else {
                sprintf(filename,"%s/%ld", PROFILE_PATH, t->utente->matricola);
                fp = fopen(filename, "w");
                if (fp != NULL) {
			txt_rewind(t->text);
			for (a = 0; a < righe; a++)
				fprintf(fp, "%s\n", txt_get(t->text));
                        fclose(fp);
                cprintf(t, "%d Profile aggiornato.\n", OK);
                } else {
                        citta_log("SYSERR: Errore in scrittura del profile.");
			cprintf(t, "%d Errore scrittura.\n", ERROR);
		}
        }
	reset_text(t);
}

/****************************************************************************/
/*
 * Inizia la modifica delle info sulla room corrente
 */
void cmd_rieb (struct sessione *t)
{
	if (t->utente &&
	    ((t->utente->livello >= LVL_AIDE)
	     || (t->utente->livello >= LVL_ROOMAIDE &&
		 t->utente->matricola == t->room->data->master)
	     || ((t->room->data->type != ROOM_DYNAMICAL)
                 && (t->room_flags[t->room->pos] & UTR_ROOMHELPER)))) {
                init_txt(t);
		t->text_com = TXT_ROOMINFO;
		t->text_max = MAXLINEEROOMINFO;
                t->stato = CON_ROOM_INFO;
                t->occupato = 1;
                t->riga = 0;
        } else
                cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
}

/*
 * Room Info Edit End: fine trasmissione delle Info.
 */
void cmd_riee (struct sessione *t)
{
        int a, righe;
        FILE *fp;
        char filename[LBUF];

	if (t->text_com != TXT_ROOMINFO) {
                cprintf(t, "%d Room Info non inizializzate.\n",
			ERROR+WRONG_STATE);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d Nessun testo. Edit info annullato.\n",
			ERROR+ARG_VUOTO);
	} else {
                if (t->room->data->flags & RF_BLOG)
                        sprintf(filename, "%s%ld", FILE_BLOGINFO, t->room->data->num);
                else
                        sprintf(filename, "%s/%ld", ROOMS_INFO, t->room->data->num);
		fp = fopen(filename, "w");
                if (fp != NULL) {
			txt_rewind(t->text);
			for (a = 0; a < righe; a++)
				fprintf(fp, "%s\n", txt_get(t->text));
                        fclose(fp);
                        /* TODO: utr_setf per i blog */
                        if (t->room->data->flags & RF_BLOG)
                                dfr_setf_all(t->room->msg->dfr, UTR_NEWGEN);
                        else
                                utr_setf_all(t->room->pos, UTR_NEWGEN);
                        if (t->room->data->flags & RF_BLOG)
                                citta_logf("BLOG #%ld: Info edited by [%s].",
                                      t->room->data->num, t->utente->nome);
                        else
                                citta_logf("ROOM #%ld: Info edited by [%s].",
                                      t->room->data->num, t->utente->nome);
			time(&(t->room->data->mtime));
			cprintf(t, "%d Room Info aggiornato.\n", OK);
                } else {
                        citta_log("SYSERR: Errore in scrittura delle room info.");
			cprintf(t, "%d Errore scrittura.\n", ERROR);
		}
        }
	reset_text(t);
}

/****************************************************************************/
/*
 * Inizia la modifica del file del login banner
 */
void cmd_lbeb (struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_BANNERS;
	t->text_max = MAXLINEEBANNERS;
	t->stato = CON_BANNER;
	t->occupato = 1;
	t->riga = 0;
}

/*
 * Login Banner Edit End: fine trasmissione delle login banner.
 */
void cmd_lbee (struct sessione *t)
{
	if (t->text_com != TXT_BANNERS)
                cprintf(t, "%d Testo LB non inizializzato.\n", ERROR);
	else if (txt_len(t->text) == 0)
                cprintf(t, "%d Nessun testo. Edit LB annullato.\n", ERROR);
	else {
                cprintf(t, "%d Login Banners aggiornati.\n", OK);
		save_banners(t);
        }
	reset_text(t);
}

/****************************************************************************/
#ifdef USE_REFERENDUM
/*
 * Dice se l'utente puo` iniziare un referendum o no
 */
void cmd_scpu (struct sessione *t)
{
        if (!(((t->utente->livello >= t->room->data->wlvl)
	    && (t->room->data->flags & RF_SONDAGGI)) || IS_RH (t))) {
		cprintf (t, "%d\n", URNA_NON_AIDE);
		return;
	}
	if (ustat.attive >= MAX_REFERENDUM) {
		cprintf (t, "%d\n", TROPPI_SOND);
		return;
	}
	cprintf (t, "%d\n", OK);
}

/*
 * Inizia l'invio di un nuovo sondaggio
 */
void cmd_srnb (struct sessione *t, char *cmd)
{
        int modo;
        modo=extract_int(cmd,0);

        if ((modo == 1) && (t->utente->livello < LVL_AIDE))
                u_err(t, URNA_NON_AIDE);

        if (!(((t->utente->livello >= t->room->data->wlvl)
	    && (t->room->data->flags & RF_SONDAGGI)) || IS_RH (t))) {
		u_err(t, URNA_NON_AIDE);
		return;
	}
		if(init_urna(t)){
		u_err(t, TROPPI_SOND);
		return;
	}

	t->parm_com=PARM_SONDAGGIO;
	t->stato = CON_REF_PARM;
	t->occupato = 1;
	cprintf(t, "%d\n", INVIA_PARAMETRI);
	return;
}

/*
 * Termina l'invio di un nuovo sondaggio.
 * o ne interrompe uno inizializzato (-1)
 */
void cmd_srne (struct sessione *t, char *cmd)
{
	long num;
	char lettera[3];
	if(extract_int(cmd,0)==-1){

		if(t->parm_com!=PARM_SONDAGGIO||t->stato!=CON_REF_PARM){
			cprintf(t, "%d Sondaggio non inizializzato.\n", ERROR);
			return;
		}

		t->parm_com=0;
		t->stato = CON_COMANDI; 
		t->occupato = 0;
		parm_free(t);
		cprintf(t,"%d\n",OK);
		return;
	}
	
	lettera[0]=0;

	if(t->parm_com!=PARM_SONDAGGIO||t->stato!=CON_REF_PARM){
		cprintf(t, "%d Sondaggio non inizializzato.\n", ERROR);
		return;
	}

	num = rs_new(t, cmd, lettera);

		if (num != 0)
			cprintf(t, "%d %ld|%s\n", OK, num,lettera);
		else
				cprintf(t,"%d",ERROR);
		/* l'errore e` gestito in rs_new */
	t->parm_com=0;
	t->stato = CON_COMANDI; 
	t->occupato = 0;
	/* 
	 * libera i parametri
	 */
	parm_free(t);
}

void cmd_sris (struct sessione *t,char *cmd)
{
	int num;
	num=extract_int(cmd,0);

	/*
	 * errore gestito in
	 * rs_inizia_voto
	 */
	if(rs_inizia_voto(t,num)){
			return;
	}

	t->parm_com=PARM_VOTO;
	t->stato = CON_REF_VOTO;
	t->occupato = 1;
	cprintf(t, "%d\n", INVIA_PARAMETRI);
	return;
}


void cmd_sren (struct sessione *t, char *cmd){

	if(extract_int(cmd,0)==-1){

		if(t->parm_com!=PARM_VOTO||t->stato!=CON_REF_VOTO){
			cprintf(t, "%d Non stavi votando.\n", ERROR);
			return;
		}
		t->parm_com=0;
		t->stato = CON_COMANDI; 
		t->occupato = 0;
		parm_free(t);
		del_votante(-1,t->utente->matricola);

		cprintf(t,"%d\n",OK);
		return;
	}
	
	if((t->parm_com!=PARM_VOTO)||(t->stato!=CON_REF_VOTO)){
			cprintf(t, "%d Non stavi votando.\n", ERROR);
		return;
	}

	/* 
	 * l'errore e` gestito in unra_vota
	 */
		urna_vota(t,cmd);
		t->parm_com=0;
		t->stato = CON_COMANDI; 
		t->occupato = 0;

		parm_free(t);
		return;
}
#endif /* USE_REFERENDUM */

/****************************************************************************/
/*
 * Inizia la modifica delle news.
 */
void cmd_nwsb(struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_NEWS;
	t->text_max = MAXLINEENEWS;
	t->stato = CON_NEWS;
	t->occupato = 1;
	t->riga = 0;
}

/*
 * Fine trasmissione delle News.
 */
void cmd_nwse(struct sessione *t)
{
        int a, righe;
        FILE *fp;

	if (t->text_com != TXT_NEWS) {
                cprintf(t, "%d News non inizializzate.\n", ERROR+WRONG_STATE);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d Nessun testo. Edit news annullato.\n",
			ERROR+ARG_VUOTO);
	} else {
		fp = fopen(FILE_NEWS, "w");
                if (fp != NULL) {
			txt_rewind(t->text);
			for (a = 0; a < righe; a++)
				fprintf(fp, "%s\n", txt_get(t->text));
			citta_logf("NEWS edited by [%s].", t->utente->nome);
			/* Segnala nuove news agli utenti */
			sut_set_all(0, SUT_NEWS);
			cprintf(t, "%d News aggiornate.\n", OK);
                } else {
                        citta_log("SYSERR: Errore in scrittura delle news.");
			cprintf(t, "%d Problemi scrittura.\n", ERROR);
		}
		fclose(fp);
        }
	reset_text(t);
}

/****************************************************************************/
#ifdef USE_FLOORS
/*
 * Inizia la modifica delle info del floor corrente.
 */
void cmd_fieb (struct sessione *t)
{
	if (t->utente &&
	    ((t->utente->livello >= LVL_AIDE)
	     || (t->utente->livello >= LVL_FLOORAIDE &&
		 t->utente->matricola == t->floor->data->master)
/*	     || (t->room_flags[t->room->pos] & UTR_ROOMHELPER) */
	     )) {
                init_txt(t);
		t->text_com = TXT_FLOORINFO;
		t->text_max = MAXLINEEFLOORINFO;
                t->stato = CON_FLOOR_INFO;
                t->occupato = 1;
                t->riga = 0;
        } else
                cprintf(t, "%d Livello insufficiente.\n", ERROR+ACC_PROIBITO);
}

/*
 * Floor Info Edit End: fine trasmissione delle Info.
 */
void cmd_fiee (struct sessione *t)
{
        int a, righe;
        FILE *fp;
        char filename[LBUF];

	if (t->text_com != TXT_FLOORINFO) {
                cprintf(t, "%d Floor Info non inizializzate.\n", ERROR);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d Nessun testo. Edit info annullato.\n", ERROR);
	} else {
                cprintf(t, "%d Floor Info aggiornato.\n", OK);

                sprintf(filename, "%s/%ld", FLOOR_INFO, t->floor->data->num);
		fp = fopen(filename, "w");
                if (fp != NULL) {
			txt_rewind(t->text);
			for (a = 0; a < righe; a++)
				fprintf(fp, "%s\n", txt_get(t->text));
                        fclose(fp);
/*			utr_setf_all((t->room)->pos, UTR_NEWGEN); */
			citta_logf("FLOOR #%ld [%s]: Info edited by [%s].",
			     t->floor->data->num, t->floor->data->name,
			     t->utente->nome);
			time(&(t->floor->data->mtime));
                } else
                        citta_log("SYSERR: Errore in scrittura delle floor info.");
        }
	reset_text(t);
}
#endif /* USE_FLOORS */
