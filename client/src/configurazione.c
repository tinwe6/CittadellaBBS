/*
 *  Copyright (C) 1999-2004 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : configurazione.c                                                   *
*        Routine di configurazione utente e client.                         *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cittaclient.h"
#include "cittacfg.h"
#include "cml.h"
#include "configurazione.h"
#include "conn.h"
#include "extract.h"
#include "inkey.h"
#include "macro.h"
#include "server_flags.h"
#include "user_flags.h"
#include "utility.h"
#include "ansicol.h"

/* prototipi delle funzioni in questo file */
void carica_userconfig(int mode);
void edit_userconfig(void);
void vedi_userconfig(void);
bool save_userconfig(bool update);
bool toggle_uflag(int slot, long flag);
void update_cmode(void);

/***************************************************************************/
/***************************************************************************/
/*
 * Carica la configurazione dell'utente.
 * Se mode = 1 lo mette in stato di configurazione.
 */
void carica_userconfig(int mode)
{
	char buf[LBUF];
	int i;

	serv_putf("CFGG %d", mode);
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(_("\nNon riesco a ottenere la tua configurazione.\n"));
		return;
	}

	for (i = 0; i < 8; i++)
		uflags[i] = (char) extract_int(&buf[4], i);
	for (i = 0; i < 8; i++)
		sflags[i] = (char) extract_int(&buf[4], i + 8);
	sesso = sflags[0] & SUT_SEX;
}

/*
 * Visualizza la configurazione utente.
 */
void vedi_userconfig(void)
{
	carica_userconfig(0);

	push_color();
	setcolor(C_CONFIG_H);
	printf(_("\nVariabili generiche:\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Utente Esperto               : %s\n"),
	       print_si_no(uflags[0] & UT_ESPERTO));
	cml_printf(_(" Usa l'editor esterno         : %s\n"),
	       print_si_no(uflags[0] & UT_EDITOR));
	cml_printf(_(" Usa il vecchio editor interno: %s\n"),
	       print_si_no(uflags[0] & UT_OLD_EDT));
	cml_printf(_(" Line Editor per Xmsg         : %s\n"),
	       print_si_no(uflags[0] & UT_LINE_EDT_X));

	setcolor(C_CONFIG_H);
	printf(_("\nApparenza:\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Niente prompt dopo i messaggi: %s\n"),
	       print_si_no(uflags[1] & UT_NOPROMPT));
	cml_printf(_(" Usa prompt che spariscono    : %s\n"),
	       print_si_no(uflags[1] & UT_DISAPPEAR));
	cml_printf(_(" Pausa a ogni schermata       : %s\n"),
	       print_si_no(uflags[1] & UT_PAGINATOR));
	cml_printf(_(" Beep abilitato               : %s\n"),
	       print_si_no(uflags[1] & UT_BELL));
	cml_printf(_(" Usa caratteri ASCII estesi   : %s\n"),
	       print_si_no(uflags[1] & UT_ISO_8859));
	cml_printf(_(" Visualizza i colori ANSI     : %s\n"),
	       print_si_no(uflags[1] & UT_ANSI));
	if (!(uflags[1] & UT_ANSI))
		cml_printf(_(" Usa il grassetto             : %s\n"),
		       print_si_no(uflags[1] & UT_HYPERTERM));

	setcolor(C_CONFIG_H);
	printf(_("\nComunicazione (Xmsg e Notifiche):\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Accetto Xmsg                  : %s\n"),
	       print_si_no(uflags[3] & UT_X));
	cml_printf(_(" Accetto Xmsg dagli amici      : %s\n"),
	       print_si_no(uflags[3] & UT_XFRIEND));
	cml_printf(_(" Accetto messaggi nel pager    : %s\n"),
	       print_si_no(uflags[3] & UT_XPOST));
	cml_printf(_(" Follow Up automatico Xmsg     : %s\n"),
	       print_si_no(uflags[3] & UT_FOLLOWUP));
	cml_printf(_(" Notifica l'arrivo dei mail    : %s\n"),
	       print_si_no(uflags[3] & UT_NMAIL));
	cml_printf(_(" Notifica i LogIn/Out          : %s\n"),
	       print_si_no(uflags[3] & UT_LION));
	cml_printf(_(" Not. i LogIn/Out degli amici  : %s\n"),
	       print_si_no(uflags[3] & UT_LIONF));
	cml_printf(_(" Salva nel diario gli X inviati: %s\n"),
	       print_si_no(uflags[3] & UT_MYXCC));

	putchar('\n');
	hit_any_key();

	setcolor(C_CONFIG_H);
	printf(_("Scansione room e lettura messaggi:\n"));
	setcolor(C_CONFIG_B);
	if (SERVER_FLOORS)
		cml_printf(_(" Organizza le room in floors           : %s\n"),
		       print_si_no(uflags[4] & UT_FLOOR));
	cml_printf(_(" Va nella prima room con msg nuovi     : %s\n"),
	       print_si_no(uflags[4] & UT_ROOMSCAN));
	cml_printf(_(" Room incolonnate nella lista room note: %s\n"),
	       print_si_no(uflags[4] & UT_KRCOL));
	cml_printf(_(" Ultimo letto prima dei nuovi messaggi : %s\n"),
	       print_si_no(uflags[4] & UT_LASTOLD));
	cml_printf(_(" Ristampa il messaggio dopo Reply      : %s\n"),
	       print_si_no(uflags[4] & UT_AGAINREP));
	cml_printf(_(" Notifica nuovo post nella room        : %s\n"),
	       print_si_no(uflags[4] & UT_NPOST));

	setcolor(C_CONFIG_H);
	printf(_("\nMail:\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Carbon copy dei mail inviati          : %s\n"),
	       print_si_no(uflags[5] & UT_MYMAILCC));
	cml_printf(_(" Ricevo Newsletter BBS via Email       : %s\n"),
	       print_si_no(uflags[5] & UT_NEWSLETTER));

	setcolor(C_CONFIG_H);
	printf(_("\nBlog (Casa):\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Blog personale attivato               : %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_ON));
	cml_printf(_(" Tutti possono scrivere sul mio blog   : %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_ALL));
	cml_printf(_(" Gli amici possono scrivere sul blog   : %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_FRIENDS));
	cml_printf(_(" I nemici non possono scrivere sul blog: %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_ENEMIES));
	cml_printf(_(" Blog accessibile da cittaweb          : %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_WEB));
        /* TODO : Implementare!
	cml_printf(_(" Finisci il giro delle room a casa mia : %s\n"),
	       print_si_no(uflags[7] & UT_BLOG_LOBBY));
        */

	setcolor(C_CONFIG_H);
	printf(_("\nDati personali visibili a tutti:\n"));
	setcolor(C_CONFIG_B);
	cml_printf(_(" Nome reale : %s\n"), print_si_no(uflags[2] & UT_VNOME));
	cml_printf(_(" Indirizzo  : %s\n"), print_si_no(uflags[2] & UT_VADDR));
	cml_printf(_(" N. telefono: %s\n"), print_si_no(uflags[2] & UT_VTEL));
	cml_printf(_(" Email      : %s\n"), print_si_no(uflags[2] &UT_VEMAIL));
	cml_printf(_(" Home page  : %s\n"), print_si_no(uflags[2] & UT_VURL));
	cml_printf(_(" Sesso      : %s\n"), print_si_no(uflags[2] & UT_VSEX));
	cml_printf(_(" Profile personalizzato: %s\n"),
	       print_si_no(uflags[2] & UT_VPRFL));
	cml_printf(_(" Visibili agli amici   : %s\n"),
	       print_si_no(uflags[2] & UT_VFRIEND));

	/*  cml_print(_(": %s\n"),print_si_no(uflags[0]|UT_)); */

	pull_color();
}

char edit_uc(char mode)
{
	char flag;

	switch (mode) {
	case 0:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nVariabili generiche:\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Utente esperto      "),
				  IS_SET(uflags[0], UT_ESPERTO)))
			flag |= UT_ESPERTO;
		if (new_si_no_def(_(" Usa l'editor esterno"),
				  IS_SET(uflags[0], UT_EDITOR)))
				flag |= UT_EDITOR;
		if (new_si_no_def(_(" Usa il vecchio editor esterno (per telnet windows)"),
				  IS_SET(uflags[0], UT_OLD_EDT)))
		        flag |= UT_OLD_EDT;
		if (new_si_no_def(_(" Line Editor per Xmsg"),
				  IS_SET(uflags[0], UT_LINE_EDT_X)))
			flag |= UT_LINE_EDT_X;
		break;
	case 1:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nApparenza:\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Niente prompt dopo i messaggi"),
				  IS_SET(uflags[1], UT_NOPROMPT)))
			flag |= UT_NOPROMPT;
		if (new_si_no_def(_(" Usa prompt che spariscono    "),
				  IS_SET(uflags[1], UT_DISAPPEAR)))
			flag |= UT_DISAPPEAR;
		if (new_si_no_def(_(" Pausa a ogni schermata       "),
				  IS_SET(uflags[1], UT_PAGINATOR)))
			flag |= UT_PAGINATOR;
		if (new_si_no_def(_(" Beep abilitato               "),
				  IS_SET(uflags[1], UT_BELL)))
			flag |= UT_BELL;
		if (new_si_no_def(_(" Usa caratteri ASCII estesi   "),
				  IS_SET(uflags[1], UT_ISO_8859)))
			flag |= UT_ISO_8859;
		if (new_si_no_def(_(" Visualizza i colori ANSI     "),
				  IS_SET(uflags[1], UT_ANSI)))
			flag |= UT_ANSI;
		if (!(uflags[1] & UT_ANSI) &&
		    new_si_no_def(_(" Usa il grassetto             "),
				  IS_SET(uflags[1], UT_HYPERTERM)))
			flag |= UT_HYPERTERM;
		break;
	case 3:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nComunicazione (Xmsg e Notifiche):\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Accetto Xmsg                  "),
				  IS_SET(uflags[3], UT_X)))
			flag |= UT_X;
		if (new_si_no_def(_(" Accetto Xmsg dagli amici      "),
				  IS_SET(uflags[3], UT_XFRIEND)))
			flag |= UT_XFRIEND;
		if (new_si_no_def(_(" Accetto messaggi nel pager    "),
				  IS_SET(uflags[3], UT_XPOST)))
			flag |= UT_XPOST;
		if (new_si_no_def(_(" Follow Up automatico Xmsg     "),
				  IS_SET(uflags[3], UT_FOLLOWUP)))
			flag |= UT_FOLLOWUP;
		if (new_si_no_def(_(" Notifica l'arrivo dei mail    "),
				  IS_SET(uflags[3], UT_NMAIL)))
			flag |= UT_NMAIL;
		if (new_si_no_def(_(" Notifica i LogIn/Out          "),
				  IS_SET(uflags[3], UT_LION)))
			flag |= UT_LION;
		if (!(flag & UT_LION) && 
		    new_si_no_def(_(" Not. i LogIn/Out degli amici  "),
				  IS_SET(uflags[3], UT_LIONF)))
			flag |= UT_LIONF;
		if (!(flag & UT_MYXCC) && 
		    new_si_no_def(_(" Salva nel diario gli X inviati"),
				  IS_SET(uflags[3], UT_MYXCC)))
			flag |= UT_MYXCC;
		break;
	case 4:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nScansione room e lettura messaggi:\n"));
		setcolor(C_CONFIG_B);
		if (SERVER_FLOORS) {
			if (new_si_no_def(_(" Organizza le room in floors           "),
					  IS_SET(uflags[4], UT_FLOOR)))
				flag |= UT_FLOOR;
		}
		if (new_si_no_def(_(" Va nella prima room con msg nuovi     "),
				  IS_SET(uflags[4], UT_ROOMSCAN)))
			flag |= UT_ROOMSCAN;
		if (new_si_no_def(_(" Room incolonnate nella lista room note"),
				  IS_SET(uflags[4], UT_KRCOL)))
			flag |= UT_KRCOL;
		if (new_si_no_def(_(" Ultimo letto prima dei nuovi messaggi "),
				  IS_SET(uflags[4], UT_LASTOLD)))
			flag |= UT_LASTOLD;
		if (new_si_no_def(_(" Ristampa il messaggio dopo un Reply   "),
				  IS_SET(uflags[4], UT_AGAINREP)))
			flag |= UT_AGAINREP;
		if (new_si_no_def(_(" Notifica nuovo post nella room        "),
				  IS_SET(uflags[4], UT_NPOST)))
			flag |= UT_NPOST;
		break;
	case 5:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nMail:\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Carbon copy dei mail inviati          "),
				  IS_SET(uflags[5], UT_MYMAILCC)))
			flag |= UT_MYMAILCC;
		if (new_si_no_def(_(" Ricevo Newsletter BBS via Email       "),
				  IS_SET(uflags[5], UT_NEWSLETTER)))
			flag |= UT_NEWSLETTER;
		break;
	case 7:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nBlog:\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Blog personale attivato               "),
				  IS_SET(uflags[7], UT_BLOG_ON))) {
			flag |= UT_BLOG_ON;
			if (new_si_no_def(_(" Tutti possono scrivere sul mio blog   "),
					  IS_SET(uflags[7], UT_BLOG_ALL)))
			  flag |= UT_BLOG_ALL;
			if (new_si_no_def(_(" Gli amici possono scrivere sul blog   "),
					  IS_SET(uflags[7], UT_BLOG_FRIENDS)))
			  flag |= UT_BLOG_FRIENDS;
			if (new_si_no_def(_(" I nemici non possono scrivere sul blog"),
					  IS_SET(uflags[7], UT_BLOG_ENEMIES)))
			  flag |= UT_BLOG_ENEMIES;
			if (new_si_no_def(_(" Blog accessibile da cittaweb          "),
					  IS_SET(uflags[7], UT_BLOG_WEB)))
			  flag |= UT_BLOG_WEB;
                        /* TODO Implementare!
			if (new_si_no_def(_(" Finisci il giro delle room a casa tua "),
					  uflags[7] & UT_BLOG_LOBBY))
			  flag |= UT_BLOG_LOBBY;
                        */
		}
		break;
	case 2:
		flag = 0;
		setcolor(C_CONFIG_H);
		printf(_("\nDati personali visibili a tutti:\n"));
		setcolor(C_CONFIG_B);
		if (new_si_no_def(_(" Nome reale "),
				  IS_SET(uflags[2], UT_VNOME)))
			flag |= UT_VNOME;
		if (new_si_no_def(_(" Indirizzo  "),
				  IS_SET(uflags[2], UT_VADDR)))
			flag |= UT_VADDR;
		if (new_si_no_def(_(" N. telefono"),
				  IS_SET(uflags[2], UT_VTEL)))
			flag |= UT_VTEL;
		if (new_si_no_def(_(" Email      "),
				  IS_SET(uflags[2], UT_VEMAIL)))
			flag |= UT_VEMAIL;
		if (new_si_no_def(_(" Home page  "),
				  IS_SET(uflags[2], UT_VURL)))
			flag |= UT_VURL;
		if (new_si_no_def(_(" Sesso      "),
				  IS_SET(uflags[2], UT_VSEX)))
			flag |= UT_VSEX;
		if (new_si_no_def(_(" Profile personalizzato"),
				  IS_SET(uflags[2], UT_VPRFL)))
			flag |= UT_VPRFL;
		if (new_si_no_def(_(" Visibili agli amici   "),
				  IS_SET(uflags[2], UT_VFRIEND)))
			flag |= UT_VFRIEND;
		break;
	}
	return flag;
}

/*
 * Funzione per editare la propria configurazione utente.
 */
void edit_userconfig(void)
{
        uint8_t flag[SIZE_FLAGS];
        bool update = false;
	int c, i;

	push_color();
	carica_userconfig(1);
	setcolor(C_CONFIG_H);
	printf(_("\nScegli:\n"));
	setcolor(C_CONFIG_B);
	printf(_(" 1. Variabili generiche\n"
		 " 2. Apparenza\n"
		 " 3. Comunicazione (Xmsg e Notifiche)\n"
		 " 4. Scansione room e lettura messaggi\n"
		 " 5. Mail.\n"
		 " 6. Blog.\n"
		 " 7. Dati personali visibili a tutti\n"
		 " 0. Tutto\n"));
	setcolor(C_CONFIG_H);
	printf(_("Cosa vuoi editare? "));
	c = 0;
	while (((c != 10) && (c < '0')) || (c > '7'))
		c = inkey_sc(0);
	printf("%c", c);
	if (c == 10) {
                save_userconfig(false);
		pull_color();
		return;
	}
	putchar('\n');
	for (i = 0; i < SIZE_FLAGS; i++)
		flag[i] = uflags[i];
	if ((c == '0') || (c == '1'))
		flag[0] = edit_uc(0);
	if ((c == '0') || (c == '2'))
		flag[1] = edit_uc(1);
	if ((c == '0') || (c == '3'))
		flag[3] = edit_uc(3);
	if ((c == '0') || (c == '4'))
		flag[4] = edit_uc(4);
	if ((c == '0') || (c == '5'))
		flag[5] = edit_uc(5);
	if ((c == '0') || (c == '6'))
		flag[7] = edit_uc(7);
	if ((c == '0') || (c == '7'))
		flag[2] = edit_uc(2);

	printf(_("\nVuoi mantenere le modifiche (s/n)? "));
	if (si_no() == 's') {
		update = true;
		for (i = 0; i < SIZE_FLAGS; i++)
			uflags[i] = flag[i];
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

/*
 * Invia al server la nuova configurazione utente.
 */
bool save_userconfig(bool update)
{
	char buf[LBUF];

        if (update)
                serv_putf("CFGP 1|%d|%d|%d|%d|%d|%d|%d|%d", uflags[0],
                          uflags[1], uflags[2], uflags[3], uflags[4],
                          uflags[5], uflags[6], uflags[7]);
        else
                serv_putf("CFGP 0");
	serv_gets(buf);
	return (buf[0] == '2') ? true : false;
}

/*
 * Toggle configuration bit. Returns the status of the bit.
 */
bool toggle_uflag(int slot, long flag)
{
	if ((slot < 0) || (slot > 7))
		return -1;
	TOGGLE_BIT(uflags[slot], flag);
	print_on_off(IS_SET(uflags[slot], flag));
	save_userconfig(true);
	return IS_SET(uflags[slot], flag);
}

/* Chiama color_mode() per aggiornare la conf plain/bold/ansi */
void update_cmode(void)
{
	if (uflags[1] & UT_ANSI)
		color_mode(CMODE_ANSI);
	else if (uflags[1] & UT_HYPERTERM)
		color_mode(CMODE_BOLD);
	else
		color_mode(CMODE_PLAIN);
	USE_COLORS = (uflags[1] & UT_ANSI) ? 1 : 0;
}
