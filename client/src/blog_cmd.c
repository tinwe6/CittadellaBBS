/*
 *  Copyright (C) 2004-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : blog_cmd.c                                                         *
*        Comandi generici del client                                        *
****************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cittaclient.h"
#include "cittacfg.h"
#include "blog_cmd.h"
#include "ansicol.h"
#include "cml.h"
//#include "colors.h"
#include "comandi.h"
#include "conn.h"
#include "configurazione.h"
#include "cterminfo.h"
#include "extract.h"
#include "friends.h"
#include "room_cmd.h" /* for blog_display_pre[] */
#include "tabc.h"
#include "utility.h"

/* prototipi delle funzioni in questo file */


/***************************************************************************/
/***************************************************************************/
void blog_configure(void)
{
        uint8_t flag;
        bool update = false;

	carica_userconfig(1);
	push_color();

	flag = edit_uc(7);
	if (flag == uflags[7]) {
	        cml_printf(_("\nNessuna modifica.\n"));
                save_userconfig(false);
		pull_color();
		return;
	}
	printf(_("\nVuoi mantenere le modifiche (s/n)? "));
	if (si_no() == 's') {
		update = true;
		uflags[7] = flag;
	}
	update_cmode();
	if (save_userconfig(update) && update)
		cml_print(_("\nOk, la tua configurazione &egrave; stata aggiornata.\n"));
	else if (update) {
		setcolor(C_ERROR);
		cml_print(_("*** Errore: La tua configurazione non &egrave; stata aggionata.\n"));
	}
	pull_color();
}

/* Lista utenti con un blog attivo. */
void blog_list(void)
{
        char name[MAXLEN_UTNAME], last_poster[MAXLEN_UTNAME], aaa[LBUF];
        long pt, num, newmsg;
        struct tm *tmst;
	int riga;
	int fine = 0;

	setcolor(C_BLOGLIST);
        cml_printf(_("\n<b>         Nome Utente       # msg       Ultimo Post         Ora     Data</b>\n"
                          "  ------------------------ ----- ------------------------ ----- ----------\n"));
        serv_puts("BLGL");
        serv_gets(aaa);
	riga = 4;
/* probabilmente c'e' un "server smetti di mandare messaggi", ma non ricordo */
        if (aaa[0] == '2') {
                while (serv_gets(aaa), strcmp(aaa, "000")) {
			if (fine == 1)
				continue;

                        if (riga == NRIGHE) {
				fine = hit_but_char('q');
				if(fine == 1)
				  continue;
				else
                                  riga = 1;
                        }
			riga++;
                        extractn(name, aaa+4, 0, MAXLEN_UTNAME);
                        extractn(last_poster, aaa+4, 1, MAXLEN_UTNAME);
                        pt = extract_long(aaa+4, 2);
                        num = extract_long(aaa+4, 3);
                        newmsg = extract_long(aaa+4, 4);
                        /* param 5 is nummsg, but we do no use it */
			/* nummsg = extract_long(aaa+4, 5); */
                        if (newmsg) {
                                setcolor(C_BLOGNEW);
                                printf("N ");
                        } else
                                printf("  ");
                        if (!strncmp(name, nome, MAXLEN_UTNAME))
                                setcolor(C_ME);
                        else if (is_friend(name))
                                setcolor(C_FRIEND);
                        else if (is_enemy(name))
                                setcolor(C_ENEMY);
                        else
                                setcolor(C_USER);
                        printf("%-24s  ", name);
                        setcolor(C_BLOGNUM);
                        cml_printf("<b>%3ld</b>", newmsg, num);
                        if (last_poster[0] == 0) {
                                putchar('\n');
                                continue;
                        }
                        cml_printf("  %-24s ", last_poster);
                        tmst = localtime(&pt);
                        setcolor(C_BLOGTIME);
                        cml_printf("%02d<b>:</b>%02d ", tmst->tm_hour,
                                   tmst->tm_min);
                        setcolor(C_BLOGDATE);
			cml_printf("<b>%02d</b>/<b>%02d</b>/<b>%4d\n",
				   tmst->tm_mday, tmst->tm_mon+1,
				   1900+tmst->tm_year);
                }
	}
	IFNEHAK;
}

void blog_enter_message(void)
{


}

/* TODO Unificare con room_goto() */
void blog_goto(void)
{
	char buf[LBUF], blogname[LBUF];
	long msg_num, msg_new, room_new, room_zap;

	get_blogname("A casa di chi vuoi andare? ", blogname);
	if (blogname[0] == 0)
	        return;
	serv_putf("GOTO 0|%s|%d", blogname, GOTO_BLOG);
	serv_gets(buf);
	if (buf[0] == '2') {
                barflag = 1;
		sprintf(current_room, "</b>%s<b>%s", blog_display_pre,
			blogname);
		*room_type = ':';
		msg_num = extract_long(buf+4, 2);
		msg_new = extract_long(buf+4, 3);
		utrflags = extract_long(buf+4, 4);
		rflags = extract_long(buf+4, 5);
		room_new = extract_long(buf+4, 6);
		room_zap = extract_long(buf+4, 7);
		if (!msg_new)
			barflag = 0;
                putchar('\n');
		cml_printf((msg_new == 1) ? _("<b>%s</b>%s - <b>%ld</b> nuovo su <b>%ld</b> messaggi.\n") : _("<b>%s</b>%s - <b>%ld</b> nuovi su <b>%ld</b> messaggi.\n"), current_room, room_type, msg_new, msg_num);
                if (room_new || room_zap) { /* Altre info in Lobby */
                        if (room_zap == 1)
                                cml_printf(_("         Hai <b>%ld</b> room con messaggi non letti e <b>1</b> room dimenticata.\n"), room_new);
                        else
                                cml_printf(_("         Hai <b>%ld</b> room con messaggi non letti e <b>%ld</b> room dimenticate.\n"), room_new, room_zap);
                }
        } else {
		if (buf[1] == '7')
		        cml_printf(_("La casa di %s &egrave; chiusa in questo momento.\n"), blogname);
		else
		        printf(sesso ? _("Non sei autorizzata ad accedere alle case degli utenti.\n") : _("Non sei autorizzato ad accedere alle case degli utenti.\n"));
	}
}


/*
 * Legge le info del blog corrente.
 * Se det!=0, da informazioni piu' dettagliate.
 */
void blog_info(int det)
{
	char buf[LBUF], raide[LBUF], rname[LBUF];
	int  riga = 4;
	long local, ct, mt, pt, flags;

	serv_puts("RINF");
	serv_gets(buf);
	if (buf[0] != '2') {
		printf(_(" *** Problemi con il server.\n"));
		return;
	}
	/* rlvl = extract_int(buf+4, 0); */
	/* wlvl = extract_int(buf+4, 1); */
	local = extract_long(buf+4, 2);
	ct = extract_long(buf+4, 3);
	mt = extract_long(buf+4, 4);
	pt = extract_long(buf+4, 5);
	extract(raide, buf+4, 6);
	extract(rname, buf+4, 7);
      	flags = extract_long(buf+4, 8);

	setcolor(C_ROOM_INFO);
	cml_printf(_("\n Blog di '<b>%s</b>', creato il "), rname);
	stampa_datal(ct);
        //	cml_printf(_(".\n Room Aide: <b>%s</b> - Livello minimo di accesso <b>%d</b>, per scrivere <b>%d</b>.\n"), raide, rlvl, wlvl);
	if (det) {
                if (flags & RF_INVITO) {
                        cml_print(_(" &Egrave; una room ad invito.\n"));
			riga++;
		}
		if (flags & RF_GUESS) {
                        cml_print(_(" &Egrave; una guess room.\n"));
			riga++;
		}
                if (flags & RF_ANONYMOUS) {
                        printf(_(" I post di questa room sono anonimi.\n"));
			riga++;
		} else if (flags & RF_ASKANONYM) {
                        cml_print(_(" &Egrave; possibile lasciare messaggi anonimi in questa room.\n"));
			riga++;
		}
                if (flags & RF_SUBJECT)
                        printf(_(" I subject sono abilitati"));
                else
                        printf(_(" I subject non sono abilitati"));
                if (flags & RF_REPLY)
                        printf(_(" e i Reply sono abilitati.\n"));
                else
                        printf(_(" e i Reply sono disabilitati.\n"));
                if (flags & RF_POSTLOCK) {
                        cml_printf(_(" Un solo utente alla volta pu&ograve; scrivere un messaggio.\n"));
			if (flags & RF_L_TIMEOUT)
				cml_printf(_(" C'&egrave; un tempo limite per scrivere i post.\n"));
		}
		if (flags & RF_SONDAGGI)
			printf(_(" Tutti gli utenti possono postare messaggi in questa room.\n"));
		if (flags & RF_FIND)
			cml_printf(_(" &Egrave possibile ricercare parole in questa room.\n"));
		if (flags & RF_CITTAWEB)
			cml_print(_(" Questa room &egrave; accessibile via web.\n"));
		if (mt == 0)
			printf(_(" Questa room non ha subito modifiche dalla sua creazione.\n"));
		else {
			printf(_(" Ultima modifica della room il "));
			stampa_datal(mt);
			putchar('\n');
		}
		riga += 2;
		if (pt) {
			printf(_(" Ultimo post lasciato nella room il "));
			stampa_datal(pt);
			putchar('\n');
			riga++;
		}
	}
	if (local) {
                switch(local) {
                case 0:
			cml_print(_(" Nessun messaggio &egrave; stato lasciato in questa room.\n\n"));
			break;
                case 1:
			cml_print(_(" &Egrave; stato lasciato un messaggio dalla sua creazione.\n\n"));
			break;
                default:
			cml_printf(_(" Sono stati lasciati <b>%ld</b> messaggi dalla sua creazione.\n\n"), local);
                }
		riga++;
	}
	if (buf[1] == '1') {
		riga += 2;
		//cml_print(_(" <b>Descrizione della room:</b>\n\n"));
		while (serv_gets(buf), strcmp(buf, "000")) {
			riga++;
                        if (riga == (NRIGHE - 1)) {
                                if (uflags[1] & UT_PAGINATOR)
                                        hit_any_key();
                                riga = 1;
                        }
			cml_printf("%s\n", buf+4);
		}
	} else
		cml_print(_(" Non &egrave; attualmente presente una descrizione di questa blog.\n\n"));
        setcolor(C_DEFAULT);
	IFNEHAK;
}

