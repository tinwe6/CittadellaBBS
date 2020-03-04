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
#include "macro.h"
#include "user_flags.h"
#include "utility.h"

enum {
        USER_NOT_YET_VALIDATED,
        USER_IS_VALIDATED,
};

enum {
        LOGIN_FAILED,
        LOGIN_WRONG_NAME,
        LOGIN_OSPITE,
        LOGIN_NUOVO,
        LOGIN_NONVAL,
        LOGIN_APPENA_VALIDATO,
        LOGIN_VALIDATO,
};

/* Prototipi funzioni in questo file */
int login(void);
static int login_new_user (bool is_first_user);
static int login_ospite(void);
static int login_user(int user_is_validated);
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
        char buf[LBUF];
        int ok = LOGIN_FAILED;
        bool has_other_connection, is_guest, is_new_user, is_first_user,
                is_validated;
        bool first_go = true, has_default_name = false;

        if (USER_NAME && (USER_NAME[0] != '\0')) {
		strncpy(nome, client_cfg.name, MAXLEN_UTNAME);
		nome[MAXLEN_UTNAME - 1] = '\0';
	} else {
		nome[0] = '\0';
		if (USER_PWD && (USER_PWD[0] != '\0')) {
			Free(USER_PWD);
			USER_PWD = strdup("");
		}
	}

        do {
                if (first_go && nome[0] != '\0') {
                        first_go = false;
			printf(_("\nNome    : %s\n"), nome);
                } else {
                        first_go = false;
                        printf(_(
"\nInserire il nome che si vuole utilizzare presso la bbs oppure 'Ospite' \n"
"nel caso si voglia solo dare un'occhiata ('Esci' chiude la connessione).\n"
                                 ));
                        do {
                                if (has_default_name) {
                                        new_str_def_M(_("\nNome"), nome,
                                                      MAXLEN_UTNAME - 1);
                                } else {
                                        new_str_M(_("\nNome    : "), nome,
                                                  MAXLEN_UTNAME - 1);
                                }
                        } while (*nome == 0);

                        if ((!strcmp(nome,"Esci"))||(!strcmp(nome,"Off"))) {
                                pulisci_ed_esci(SHOW_EXIT_BANNER);
                        }
                }
                serv_putf("USER %s", nome);
                serv_gets(buf);

                if (buf[0] != '2') {
                        /* this should never happen... */
                        cml_printf(_(
"\n*** Si &egrave; prodotto un errore... ricomincia!\n"
                                     ));
                        nome[0] = '\0';
                        continue;
                }

                /*--8<-------------------------------------------------*/
                /* TODO this is for compatibility with the old login   */
                /* protocol and must be eliminated before next release */
                if (serverinfo.legacy) {
                        switch (buf[2]) {
                        case '0': /* nuovo utente */
                                ok = login_new_user(is_first_user);
                                break;
                        case '1': /* ospite */
                                ok = login_ospite();
                                break;
                        case '2': /* utente non validato */
                                ok = login_user(USER_NOT_YET_VALIDATED);
                                break;
                        case '3': /* utente validato */
                                ok = login_user(USER_IS_VALIDATED);
                                break;
                        }
                        if (ok == LOGIN_FAILED) {
                                nome[0] = 0;
                        }
                        continue;
                }
                /*--8<-------------------------------------------------*/

                has_other_connection = extract_bool(buf+4, 0);
                is_guest = extract_bool(buf+4, 1);
                is_new_user = extract_bool(buf+4, 2);
                is_first_user = extract_bool(buf+4, 3);
                is_validated = extract_bool(buf+4, 4);

                if (has_other_connection) {
                        cml_printf(_(
"\n*** L'utente %s &egrave; gi&agrave; collegato.\n"
                                     ), nome);
                        printf(_(
"\nVuoi continuare il login ed eliminare l'altra sessione? (s/n) "
                                 ));
                        if (si_no() == 'n') {
                                pulisci_ed_esci(SHOW_EXIT_BANNER);
                        }
                }

                if (is_new_user) {
                        ok = login_new_user(is_first_user);
                } else if (is_guest) {
                        ok = login_ospite();
                } else if (is_validated) {
                        ok = login_user(USER_IS_VALIDATED);
                } else {
                        ok = login_user(USER_NOT_YET_VALIDATED);
                }

                has_default_name = (ok != LOGIN_WRONG_NAME);

        } while (ok == LOGIN_FAILED || ok == LOGIN_WRONG_NAME);

	return ok;
}

/*
 * Displays the messages on data protection and privacy, and asks the user
 * to accept the terms. Returns true if (s)he does, false otherwise.
 */
static bool ask_accept_terms()
{
        putchar('\n');
        leggi_file(STDMSG_MESSAGGI, STDMSGID_DATA_PROTECTION);
        putchar('\n');
        leggi_file(STDMSG_MESSAGGI, STDMSGID_PRIVACY);

        cml_printf(_("\nAccetti queste condizioni? (s/n) "));
        return (si_no() == 's');
}


/*
 * Procedura di login per un nuovo utente.
 * Consiste nella conferma del nome, creazione di una password,
 * accettazione dei termini (GDPR) e procedura di validazione.
 * Restituisce 1 se e' andato tutto bene.
 */
static int login_new_user (bool is_first_user)
{
        char buf[LBUF], passwd[MAXLEN_PASSWORD], passwd1[MAXLEN_PASSWORD];
	bool terms_accepted = false;

        printf(_("\nRecord non esistente. Nuovo utente? (s/n) "));
        if (si_no()=='n') {
                printf(_("\nAllora da capo...\n"));
                return LOGIN_WRONG_NAME;
        }
        leggi_file(STDMSG_MESSAGGI, STDMSGID_BAD_NICKS);

        /* Verifica il nome */
        cml_printf(_("\nIl nome che hai digitato &egrave; \"<b>%s</b>\".\n"
		     "&Egrave; questo il nome che vuoi utilizzare? (s/n) "),
		   nome);
        if (si_no() == 'n') {
                printf(_("\nAllora da capo...\n"));
                return LOGIN_WRONG_NAME;
        }
        /* Chiede una password */
        putchar('\n');
        leggi_file(STDMSG_MESSAGGI, STDMSGID_CHANGE_PWD);

        while (true) {
                while (true) {
                        new_str_m(_("Inserisci la password che vuoi usare: "),
                                  passwd, - (MAXLEN_PASSWORD - 1));
                        if (strlen(passwd) == 0) {
                                cml_print(_(
"\nLa password non pu&oacute; essere vuota!\n"
                                            ));
                        } else {
                                break;
                        }
                }
                new_str_m(_("Reimmetti la password che vuoi usare: "),
                          passwd1, -(MAXLEN_PASSWORD - 1));
                if (!strcmp(passwd, passwd1)) {
                        break;
                } else {
                        printf(_("\nLe password non coincidono. Riprova.\n"));
                }
        }

        if (is_first_user) {
		cml_printf(_(
"\n\nCongratulazioni!!\n"
"Sei il primo utente di questa BBS, e sar&agrave; tuo compito gestirla.\n"
"Hai automaticamente il livello 'Sysop', che ti rende onnipotente.\n\n"
"Questo non ti risparmia per&ograve; dal compito di dare il buon esempio\n"
"agli altri utenti, perci&ograve; passiamo comunque alla fase di\n"
"registrazione, anche se non hai bisogno di validarti... :)\n\n"
                             ));
		hit_any_key();
        }

        /* Ask to accept terms */
        terms_accepted = ask_accept_terms();
        if (!terms_accepted) {
                printf(_(
"\nSiamo spiacenti ma allora non puoi registrarti a Cittadella BBS...\n"
"Puoi comunque collegarti come Ospite per dare un'occhiata.\n"
                         ));
                return LOGIN_FAILED;
        }

        /* completa il login */
        serv_putf("USR1 %s|%s", nome, passwd);
        serv_gets(buf);
        /* The server should always reply OK here! */
        if (buf[0] != '2') {
                pulisci_ed_esci(SHOW_EXIT_BANNER);
        }
        assert((buf[1] == '1') == is_first_user);
        assert(is_first_user == extract_bool(buf + 4, 1));
        assert(!extract_bool(buf + 4, 2));
        assert(!extract_bool(buf + 4, 3));

        /* Send the user consent for data processing to the server */
        serv_putf("GCST %d", terms_accepted);
        serv_gets(buf);
        assert(buf[0] == '2');
        assert(extract_bool(buf + 4, 0));

        /* Registration */
        leggi_file(STDMSG_MESSAGGI, STDMSGID_REGISTRATION);
        putchar('\n');
        hit_any_key();
        putchar('\n');
        registrazione(true);

        /* Ask for a validation key (unless it's the first user) */
	if (!is_first_user) {
                serv_puts("GVAL");
                serv_gets(buf);
                if (buf[0] != '2') {
                        /* L'utente deve registrarsi prima di richiedere */
                        /* la valkey! () */
                        pulisci_ed_esci(SHOW_EXIT_BANNER);
                }
        }
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
        serv_putf("USR1 %s|", nome);
        serv_gets(buf);

        if (buf[0] != '2') {
                pulisci_ed_esci(SHOW_EXIT_BANNER);
        }
        return LOGIN_OSPITE;
}

/*
 * Procedura di login per gli utenti non validati.
 * Restituisce il risultato del login.
 */
static int login_user(int user_is_validated)
{
        char buf[LBUF], passwd[MAXLEN_PASSWORD];
	bool auto_login = false;
        bool is_registered, terms_accepted = false;

	if (USER_PWD && (USER_PWD[0] != '\0')) {
		strncpy(passwd, USER_PWD, MAXLEN_PASSWORD-1);
		passwd[MAXLEN_PASSWORD-1] = '\0';
		Free(USER_PWD);
		USER_PWD = strdup("");
		auto_login = true;
	} else {
		new_str_m(_("Password: "), passwd, -(MAXLEN_PASSWORD - 1));
        }

        serv_putf("USR1 %s|%s", nome, passwd);
        serv_gets(buf);
        terms_accepted = extract_bool(buf, 2);
        is_registered = extract_bool(buf + 4, 3);

        if (buf[0] != '2') {
                if (buf[1] == '3') {
                        printf(_("<< password errata >>\n"));
                } else {
                        pulisci_ed_esci(SHOW_EXIT_BANNER);
                }
                return LOGIN_FAILED;
        }
	if (auto_login) {
		printf(_("Password corretta.\n\n"));
		hit_any_key();
	}

        /* TODO before next release */
        if (serverinfo.legacy) {
                if (user_is_validated == USER_IS_VALIDATED) {
                        return LOGIN_VALIDATO;
                } else {
                        cml_printf(
"Hai un client troppo recente. Potrebbero succedere cose strane.. Good luck!\n"
                                   );
                }
        }

        /* controlla se i termini sono stati accettati */
        if (!terms_accepted) {
                do {
                        putchar('\n');
                        terms_accepted = ask_accept_terms();
                        if (terms_accepted) {
                                break;
                        }
                        cml_printf(sesso
                                   ? _(
"\nSei sicura di voler eliminare il tuo account su Cittadella BBS? (s/n) "
                                       )
                                   : _(
"\nSei sicuro di voler eliminare il tuo account su Cittadella BBS? (s/n) "
                                       ));
                } while(si_no() == 'n');

                /* Send the user consent for data processing to the server */
                serv_putf("GCST %d", terms_accepted);
                serv_gets(buf);
                assert(buf[0] == '2');

                if (!terms_accepted) {
                        cml_printf(_(
"\n<b>"
"I Sysop sono stati notificati della tua decisione e provvederanno ad\n"
"eliminare il tuo account nelle prossime 48 ore. Se nel frattempo dovessi\n"
"cambiare idea, puoi ricollegarti e accettare le condizioni.\n"
"</b>\n"
                                     ));
                        sleep(5);
                        pulisci_ed_esci(NO_EXIT_BANNER);
                }

                assert(extract_bool(buf + 4, 0) != is_registered);
        }

        /* If the user is not correctly registered, go through registration. */
        /* This shouldn't happen, some older users might need it though      */
        if (!is_registered) {
                leggi_file(STDMSG_MESSAGGI, STDMSGID_REGISTRATION);
                putchar('\n');
                hit_any_key();
                putchar('\n');
                registrazione(true);
        }

        if (user_is_validated == USER_IS_VALIDATED) {
                return LOGIN_VALIDATO;
        }

        /* Se ha la validation key esegue l'autovalidazione */
	printf(_(
            "\nHai ricevuto la chiave di validazione e vuoi validarti? (s/n) "
                 ));
	if (si_no()=='n') {
		printf(_(
"\nPotrai eseguire la validazione la prossima volta che ti colleghi.\n"
"Nel frattempo puoi visitare la BBS come utente non validato. Ricorda tuttavia"
"\n"
"che la validazione va eseguita entro 48 ore dalla prima connessione.\n"
                         ));
        } else if (auto_validazione()) {
		return LOGIN_APPENA_VALIDATO;
        }
	return LOGIN_NONVAL;
}


void chiedi_valkey(void)
{
	char buf[LBUF];

	printf(sesso
               ? _(
"\nSei sicura di voler richiedere una nuova chiave di validazione? (s/n) "
                   )
               : _(
"\nSei sicuro di voler richiedere una nuova chiave di validazione? (s/n) "
                   ));
	if (si_no()=='s') {
		serv_puts("GVAL");
		serv_gets(buf);
		if (buf[0] != '2') {
			if (buf[1] == '2') {
				printf(_(
"Prima devi compilare la registrazione...\n"
                                         ));
			} else {
				cml_print(sesso
                                          ? _(
"Mi dispiace, ma sei gi&agrave; validata!!!\n"
                                              )
                                          : _(
"Mi dispiace, ma sei gi&agrave; validato!!!\n"
                                              ));
                        }
		} else {
			cml_printf(_(
"Ok, una nuova chiave di validazione &egrave; stata generata e inviata!\n"
                                     ));
                }
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
                new_str_m(_("Inserisci la chiave di validazione: "),
                          valkey, -6);
                serv_putf("AVAL %s", valkey);
                serv_gets(buf);
                if (buf[0] != '2') {
                        /* validation key errata */
                        cml_print(_(
                                "La chiave di validazione &egrave; errata. "
                                    ));
                        if (loop == 0) {
                                printf(_(
                                    "Vuoi fare un altro tentativo? (s/n) "
                                       ));
                                if (si_no() == 'n')
                                        loop = 2;
                        }
                } else {
                        printf(_(
"Chiave di validazione corretta: ora sei un utente registrato!\n\n")
                               );
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
		if (!new_si_no_def(_(
"\nIl tuo terminale &egrave; in grado di visualizzare i colori ansi?"
                                     ), IS_SET(uflags[1], UT_ANSI))) {
			if (!(uflags[1] & UT_ANSI) &&
			    new_si_no_def(_("   Usa il grassetto             :"),
					  IS_SET(uflags[1], UT_HYPERTERM)))
				uflags[1] |= UT_HYPERTERM;
			else
				uflags[1] &= ~UT_HYPERTERM;
			update_cmode();
		} else {
			update_cmode();
			cml_print(_(
"\nTest dei colori:\n"
"  Colore del carattere : <b;fg=1>rosso <fg=2>verde <fg=3>giallo/marrone <fg=4>blu <fg=5>magenta <fg=6>azzurro <fg=7>bianco</b>\n"
"  Colore dello sfondo  : <b;bg=1>rosso<bg=0> <bg=2>verde<bg=0> <bg=3>giallo/marrone<bg=0> <bg=4>blu<bg=0> <bg=5>magenta<bg=0> <bg=6>azzurro<bg=0> <bg=7>bianco</b;bg=0>\n\n"
                                    ));
                        if (!new_si_no_def(_("Vedi correttamente i colori qui sopra?"), true)) {
				uflags[1] &= ~UT_ANSI;
				update_cmode();
			}
		}

		setcolor(C_DEFAULT);
		uflags[1] &= ~UT_ISO_8859;
		if (!new_si_no_def(_(
"\nIl tuo terminale pu&ograve; visualizzare i caratteri ISO-8859-1\n"
"  (accenti, etc...)?"
                                     ), true)) {
			uflags[1] &= ~UT_ISO_8859;
		} else {
			uflags[1] |= UT_ISO_8859;
			cml_print(_(
"\nTest degli accenti: <b>&agrave; &eacute; &egrave; &ecirc; &iacute; &icirc; &uacute;</b>\n"
                                    ));
                        if (!new_si_no_def(_(
"\nVedi correttamente le lettere accentate qui sopra"
                                             ), true)) {
				uflags[1] &= ~UT_ISO_8859;
				update_cmode();
			}
		}

		setcolor(C_DEFAULT);
		uflags[0] &= ~UT_OLD_EDT;
		cml_printf(_(
"\n"
"Per immettere i messaggi, &egrave; disponibile un editor con funzioni avanzate,\n"
"che purtroppo non funziona con il programma <b>telnet di Windows</b> e con i\n"
"sistemi operativi <b>Mac OS 9 o precedenti</b>. In questo caso dovrai usare\n"
"il vecchio editor.\n"
"NB: Si consiglia di utilizzare il programma puTTY per collegarsi da Windows.\n"
                             ));
		if (new_si_no_def(_("\nStai usando il telnet Windows o un Mac OS 9 o precedente?"), false)) {
		        uflags[0] |= UT_OLD_EDT;
		} else {
			uflags[0] &= ~UT_OLD_EDT;
		}
		cml_print(_(
"\nIn ogni caso potrai modificare in ogni momento la tua configurazione utente"
"\nutilizzando digitando i tasti <b>.eu</b>\n")
                          );
		save_userconfig(true);
}
	setcolor(C_DEFAULT);

	cml_printf(
"\nCongratulazioni! hai terminato la configurazione del tuo account!\n\n"
                   );
	hit_any_key();
	putchar('\n');

	/* Legge i file di benvenuto e introduzione. */
	switch(type) {
	case LOGIN_OSPITE:
                leggi_file(STDMSG_MESSAGGI, STDMSGID_INTRO_GUEST);
		break;
	case LOGIN_NUOVO:
                leggi_file(STDMSG_MESSAGGI, STDMSGID_INTRO_NEW);
		break;
	case LOGIN_APPENA_VALIDATO:
                leggi_file(STDMSG_MESSAGGI, STDMSGID_INTRO_JUST_VALIDATED);
		break;
	default:
		return;
	}
	hit_any_key();
}
