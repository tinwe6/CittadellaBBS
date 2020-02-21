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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : login.c                                                            *
*        Procedura di login.                                                *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cittaclient.h"
#include "cittacfg.h"
#include "ansicol.h"
#include "cml.h"
#include "configurazione.h"
#include "conn.h"
#include "extract.h"
#include "generic_cmd.h"
#include "login.h"
#include "user_flags.h"
#include "utility.h"
#include "macro.h"

/* Prototipi funzioni in questo file */
int login(void);
static int login_new_user (void);
static int login_ospite(void);
static int login_non_val(void);
static int login_validato(void);
void chiedi_valkey(void);
static bool auto_validazione(void);
void login_banner(void);
void user_config(int type);

/****************************************************************************
****************************************************************************/
/* 
 * Procedura di login
 */
int login(void)
{
        char buf[LBUF], def[LBUF] = "";
        int ok = LOGIN_FAILED;

	if (USER_NAME && (USER_NAME[0] != '\0')) {
		strncpy(nome, client_cfg.name, MAXLEN_UTNAME);
		nome[MAXLEN_UTNAME-1] = '\0';
	} else {
		nome[0] = '\0';
		if (USER_PWD && (USER_PWD[0]!='\0')) {
			Free(USER_PWD);
			USER_PWD = strdup("");
		}
	}

        while(ok == LOGIN_FAILED) {
                if (*nome == 0) {
                        printf(_("\nInserire il nome che si vuole utilizzare presso la bbs oppure 'Ospite' \n"
				 "nel caso si voglia solo dare un'occhiata ('Esci' chiude la connessione).\n"));
                        do {
			        new_str_def_M(_("\nNome"), nome,
					      MAXLEN_UTNAME - 1);
                        } while (*nome == 0);

                        if ((!strcmp(nome,"Esci"))||(!strcmp(nome,"Off")))
                                pulisci_ed_esci();
                } else
			printf(_("\nNome    : %s\n"), nome);
                serv_putf("USER %s", nome);
                serv_gets(buf);

                if (extract_int(buf+4, 0)) {
                                cml_printf(_("\n*** L'utente %s &egrave; gi&agrave; collegato.\n"), nome);
                                printf(_("\nVuoi continuare il login ed eliminare l'altra sessione? (s/n) "));
                                if (si_no() == 'n')
                                        pulisci_ed_esci();
                }

                switch (buf[2]) {
                case '0': /* nuovo utente */
                        ok = login_new_user();
                        break;
                case '1': /* ospite */
                        ok = login_ospite();
                        break;
                case '2': /* utente non validato */
                        ok = login_non_val();
                        break;
                case '3': /* utente validato */
                        ok = login_validato();
                        break;
                }
                if (ok == 0) {
			strcpy(def, nome);
                        nome[0] = 0;
		}
	}
	return ok;
}

/*
 * Procedura di login per un nuovo utente.
 * Consiste nella conferma del nome, definizione di una password 
 * e procedura di validazione. Restituisce 1 se e' andato tutto bene.
 */
static int login_new_user (void)
{
        char buf[LBUF], passwd[MAXLEN_PASSWORD], passwd1[MAXLEN_PASSWORD];
        char loop;
	bool primo_utente = false;
        
        printf(_("\nRecord non esistente. Nuovo utente? (s/n) "));
        if (si_no()=='n') {
                printf(_("\nAllora da capo...\n"));
                return LOGIN_FAILED;
        }
        leggi_file(0, 1);  /* bruttinick */
        
        /* Verifica il nome */
        cml_printf(_("\nIl nome che hai digitato &egrave; \"<b>%s</b>\".\n"
		     "&Egrave; questo il nome che vuoi utilizzare? (s/n) "),
		   nome);
        if (si_no() == 'n') {
                printf(_("\nAllora da capo...\n"));
                return LOGIN_FAILED;
        }
        /* Chiede una password */
        putchar('\n');
        leggi_file(0, 6);
        loop = 0;
        while (loop == 0) {
                do {
                        new_str_m(_("Inserisci la password che vuoi usare: "),
                                  passwd, - (MAXLEN_PASSWORD - 1));
                        if (!strlen(passwd))
                                cml_print(_("La password non pu&oacute; essere vuota!\n"));
                } while (strlen(passwd) == 0);
                new_str_m(_("Reimmetti la password che vuoi usare: "),
                          passwd1, -(MAXLEN_PASSWORD - 1));
                if (!strcmp(passwd, passwd1))
                        loop = 1;
                else
                        printf(_("\nLe password non coincidono. Riprova.\n"));
        }
        serv_putf("USR1 %s|%s", nome, passwd);
        serv_gets(buf);
        if (buf[0] != '2')
                pulisci_ed_esci();
	if (buf[1] == '1') {
		primo_utente = true;
		cml_printf("\n\nCongratulazioni!!\n"
"Sei il primo utente di questa BBS, e sar&agrave; tuo compito gestirla.\n"
"Hai automaticamente il livello 'Sysop', che ti rende onnipotente.\n\n"
"Questo non ti risparmia per&ograve; dal compito di dare il buon esempio\n"
"agli altri utenti, perci&ograve; passiamo comunque alla fase di\n"
"registrazione, anche se non hai bisogno di validarti... :)\n\n");
		hit_any_key();
	}
        leggi_file(0, 3);
        putchar('\n');
        hit_any_key();
        putchar('\n');
        registrazione(true);
        /* configurazione(); */
	if (primo_utente)
		return LOGIN_NUOVO;
	serv_puts("GVAL");
        serv_gets(buf);
        if (buf[0] != '2')
                /* L'utente deve registrarsi prima di richiedere la valkey! */
                pulisci_ed_esci();
        return LOGIN_NUOVO;
}

/*
 * Procedura di login per gli ospiti.
 * Setta ospite = true. Restituisce 1 se e' andato tutto bene.
 */
static int login_ospite(void)
{
        char buf[LBUF];

        ospite = true;
        serv_putf("USR1 %s", nome);
        serv_gets(buf);
  
        if (buf[0] != '2')
                pulisci_ed_esci();
        return LOGIN_OSPITE;
}

/*
 * Procedura di login per gli utenti non validati.
 * Restituisce 1 se e' andato tutto bene.
 */
static int login_non_val(void)
{
        char buf[LBUF];
        char passwd[MAXLEN_PASSWORD];
	bool auto_login = false;

	if (USER_PWD && (USER_PWD[0] != '\0')) {
		strncpy(passwd, USER_PWD, MAXLEN_PASSWORD-1);
		passwd[MAXLEN_PASSWORD-1] = '\0';
		Free(USER_PWD);
		USER_PWD = strdup("");
		auto_login = true;
	} else
		new_str_m(_("Password: "), passwd, -(MAXLEN_PASSWORD - 1));
        serv_putf("USR1 %s|%s", nome, passwd);
        serv_gets(buf);
  
        if (buf[0] != '2') {
                if (buf[1] == '3')
                        printf(_("<< password errata >>\n"));
                else
                        pulisci_ed_esci();
                return LOGIN_FAILED;
        }
	if (auto_login) {
		printf(_("Password corretta.\n\n"));
		hit_any_key();
	}
	/* Se ha la validation key esegue l'autovalidazione */
	printf(_("\nHai ricevuto la chiave di validazione e vuoi validarti? (s/n) "));
	if (si_no()=='n') 
		printf(_("\nPotrai eseguire la validazione la prossima volta che ti colleghi.\n"
			 "Nel frattempo puoi visitare la BBS come utente non validato. Ricorda tuttavia\n"
			 "che la validazione va eseguita entro 48 ore dalla prima connessione.\n"));
	else if (auto_validazione())
		return LOGIN_APPENA_VALIDATO;
	return LOGIN_NONVAL;
}

/*
 * Procedura di login per gli utenti validati. 
 * Verifica la password e restituisce 1 se e' esatta
 */
static int login_validato(void)
{
        char buf[LBUF], passwd[MAXLEN_PASSWORD];
	bool auto_login = false;

	if (USER_PWD && (USER_PWD[0] != '\0')) {
		strncpy(passwd, USER_PWD, MAXLEN_PASSWORD-1);
		passwd[MAXLEN_PASSWORD-1] = '\0';
		Free(USER_PWD);
		USER_PWD = strdup("");
		auto_login = true;
	} else
		new_str_m(_("Password: "), passwd, -(MAXLEN_PASSWORD - 1));

        serv_putf("USR1 %s|%s", nome, passwd);
        serv_gets(buf);
        if (buf[0] != '2') {
                if (buf[1] == '3')
                        printf(_("\n<< password errata >>\n"));
                else
                        pulisci_ed_esci();
                return LOGIN_FAILED;
        }
	if (auto_login) {
		printf(_("Password corretta.\n\n"));
		hit_any_key();
	}
        return LOGIN_VALIDATO;
}

void chiedi_valkey(void)
{
	char buf[LBUF];
	
	printf(sesso ? _("\nSei sicura di voler richiedere una nuova chiave di validazione? (s/n) ") : _("\nSei sicuro di voler richiedere una nuova chiave di validazione? (s/n) "));
	if (si_no()=='s') { 
		serv_puts("GVAL");
		serv_gets(buf);
		if (buf[0] != '2') {
			if (buf[1] == '2')
				printf(_("Prima devi compilare la registrazione...\n"));
			else
				cml_print(sesso ? _("Mi dispiace, ma sei gi&agrave; validata!!!\n") : _("Mi dispiace, ma sei gi&agrave; validato!!!\n"));
		} else
			cml_printf(_("Ok, una nuova chiave di validazione &egrave; stata generata e inviata!\n"));
	}
}

/*
 * Chiede due volte all'utente la chiave di validazione.
 * Se e' esatta, l'utente viene validatio dal server.
 */
static bool auto_validazione(void)
{
        char buf[LBUF], valkey[16];
        char loop = 0;

        printf("\n");
        while (loop < 2) {
                new_str_m(_("Inserisci la chiave di validazione: "),valkey,-6);
                serv_putf("AVAL %s", valkey);
                serv_gets(buf);
                if (buf[0] != '2') {
                        /* validation key errata */
                        cml_print(_("La chiave di validazione &egrave; errata. "));
                        if (loop == 0) {
                                printf(_("Vuoi fare un altro tentativo? (s/n) "));
                                if (si_no() == 'n')
                                        loop = 2;
                        }
                } else {
                        printf(_("Chiave di validazione corretta: ora sei un utente registrato!\n\n"));
			return true;
                }
                loop++;
        }
	printf(_("Riprova la prossima volta che ti colleghi.\n"));
	return false;
}

/*
 * Chiede al server la login banner
 */
void login_banner(void)
{
        char buf[LBUF];

	serv_putf("LBAN %d", NO_BANNER);
        serv_gets(buf);
	if (buf[0]!='2')
		return;
	while (serv_gets(buf), strcmp(buf, "000")) {
		cml_print(buf+4);
		putchar('\n');
	}
	if (NO_BANNER)
		putchar('\n');
}

/* Configura il client per Ospiti e utenti nuovi. */
void user_config(int type)
{
	if (type == LOGIN_VALIDATO)
		return;
	
	setcolor(C_DEFAULT);

	if ((type == LOGIN_OSPITE) || (type == LOGIN_NUOVO)) {
		uflags[1] |= UT_ANSI;
		if (!new_si_no_def(_("\nIl tuo terminale &egrave; in grado di visualizzare i colori ansi?"), IS_SET(uflags[1], UT_ANSI))) {
			if (!(uflags[1] & UT_ANSI) &&
			    new_si_no_def(_("   Usa il grassetto             :"),
					  IS_SET(uflags[1], UT_HYPERTERM)))
				uflags[1] |= UT_HYPERTERM;
			else
				uflags[1] &= ~UT_HYPERTERM;
			update_cmode();
		} else {
			update_cmode();
			cml_print(_("\nTest dei colori:\n"
				    "  Colore del carattere : <b;fg=1>rosso <fg=2>verde <fg=3>giallo/marrone <fg=4>blu <fg=5>magenta <fg=6>azzurro <fg=7>bianco</b>\n"
				    "  Colore dello sfondo  : <b;bg=1>rosso<bg=0> <bg=2>verde<bg=0> <bg=3>giallo/marrone<bg=0> <bg=4>blu<bg=0> <bg=5>magenta<bg=0> <bg=6>azzurro<bg=0> <bg=7>bianco</b;bg=0>\n\n"));
			if (!new_si_no_def(_("Vedi correttamente i colori qui sopra?"), true)) {
				uflags[1] &= ~UT_ANSI;
				update_cmode();
			}
		}

		setcolor(C_DEFAULT);
		uflags[1] &= ~UT_ISO_8859;
		if (!new_si_no_def(_("\nIl tuo terminale pu&ograve; visualizzare i caratteri ISO-8859-1\n  (accenti, etc...)?"), true)) {
			uflags[1] &= ~UT_ISO_8859;
		} else {
			uflags[1] |= UT_ISO_8859;
			cml_print(_("\nTest degli accenti: <b>&agrave; &eacute; &egrave; &ecirc; &iacute; &icirc; &uacute;</b>\n"));
			if (!new_si_no_def(_("\nVedi correttamente le lettere accentate qui sopra"), true)) {
				uflags[1] &= ~UT_ISO_8859;
				update_cmode();
			}
		}

		setcolor(C_DEFAULT);
		uflags[0] &= ~UT_OLD_EDT;
		cml_printf(_(
"\nPer immettere i messaggi, &egrave; disponibile un editor con funzioni avanzate,\n"
"che purtroppo non funziona con il programma <b>telnet di Windows</b> e con i\n"
"sistemi operativi <b>Mac OS 9 o precedenti</b>. In questo caso dovrai usare\n"
"il vecchio editor.\n"
"NB: Si consiglia di utilizzare il programma puTTY per collegarsi da Windows.\n"));
		if (new_si_no_def(_("\nStai usando il telnet Windows o un Mac OS 9 o precedente?"), false)) {
		        uflags[0] |= UT_OLD_EDT;
		} else {
			uflags[0] &= ~UT_OLD_EDT;
		}
		cml_print(_("\nIn ogni caso potrai modificare in ogni momento la tua configurazione utente\nutilizzando digitando i tasti <b>.eu</b>\n"));
		save_userconfig(true);
}
	setcolor(C_DEFAULT);

	cml_printf("\nCongratulazioni! hai terminato la configurazione del tuo account!\n\n");
	hit_any_key();
	putchar('\n');

	/* Legge i file di benvenuto e introduzione. */
	switch(type) {
	case LOGIN_OSPITE:
		leggi_file(0, 10);
		break;
	case LOGIN_NUOVO:
		leggi_file(0, 11);
		break;
	case LOGIN_APPENA_VALIDATO:
		leggi_file(0, 12);
		break;
	default:
		return;
	}
	hit_any_key();
}
