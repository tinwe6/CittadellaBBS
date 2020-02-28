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
* File : generic_cmd.c                                                      *
*        Comandi generici del client                                        *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ansicol.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "comandi.h"
#include "configurazione.h"
#include "cml.h"
#include "generic_cmd.h"
#include "conn.h"
#include "edit.h"
#include "extract.h"
#include "file_flags.h"
#include "friends.h"
#include "pager.h"
#include "signals.h"
#include "tabc.h"
#include "text.h"
#include "user_flags.h"
#include "macro.h"
#include "utility.h"
#include "versione.h"

/* prototipi delle funzioni in questo file */
void list_host(void);
void ora_data(void);
void lock_term(void);
void bug_report(void);
void biff(void);
void leggi_file(Stdmsg_type type, int id);
void help(void);
void Help(void);
void leggi_news(void);
void edit_doing(void);
void clear_doing(void);
void edit_term(void);
void upgrade_client(int vcode);
void toggle_debug_mode(void);

/***************************************************************************/
/***************************************************************************/
/* Elimina i vari suffissi e prefissi dal nome utente come inviato da HWHO */
const char hwho_prefix[] = "({";
const char hwho_suffix[] = ".#+&^_?*%)}";
static void strip_username(char * name, const char * str)
{
        size_t len;
        int flag = 0;

        len = strlen(str);

        if (index(hwho_prefix, str[0])) {
                strncpy(name, str+1, MAXLEN_UTNAME);
                flag++;
        } else
                strncpy(name, str, MAXLEN_UTNAME);
        if (index(hwho_suffix, str[len-1]))
                name[len-flag-1] = 0;
}

/*
 * <W>ho: lista utenti connessi al server con relativi host e altre info.
 */
void list_host(void)
{
        char aaa[LBUF];
        int chat;
        char nm[MAXLEN_UTNAME+2]; /* +2 per i simboli che indicano lo stato */
        char name[MAXLEN_UTNAME];
        char *hst;
	char room[ROOMNAMELEN];
        long ora;
        struct tm *tmst;
        int num_ospiti;
        int num_utenti = 0;
        int num_lin = 0;

	setcolor(C_WHO);
        cml_printf(_("\n<b>Login        Nome Utente               Room               Da Host/[Doing]</b>\n"
                 "----- ------------------------- ------------------- ----------------------------\n"));
        serv_puts("HWHO");
        serv_gets(aaa);
        if (aaa[0] == '2') {
                while (serv_gets(aaa), strcmp(aaa, "000")) {
		        /* fd = extract_int(aaa+4, 0); */
                        extractn(nm, aaa+4, 1, MAXLEN_UTNAME+2);
                        ora = extract_long(aaa+4, 3);
                        extractn(room, aaa+4, 4, ROOMNAMELEN);
                        chat = extract_int(aaa+4, 5);
                        tmst = localtime(&ora);
			/* prendi host/doing */
			serv_gets(aaa);
			hst = strdup(aaa+4);
                        setcolor(C_WHO_TIME);
                        printf("%02d:%02d ", tmst->tm_hour, tmst->tm_min);

                        strip_username(name, nm);
                        if (!strncmp(name, nome, MAXLEN_UTNAME))
                                setcolor(C_ME);
                        else if (is_friend(name))
                                setcolor(C_FRIEND);
                        else if (is_enemy(name))
                                setcolor(C_ENEMY);
                        else
                                setcolor(C_USER);
                        /* qui erano 25 caratteri, controllare */
                        printf("%-24s", nm);

                        /* controlla se (login in corso) */
                        if (strcmp(nm, "(login in corso)") == 0)
				num_lin++;
                        else
				num_utenti++;

                        if (chat) {
				setcolor(C_WHO_TIME);
				printf("C ");
                        } else
				printf("  ");
                        setcolor(C_ROOM);
                        printf("%-19s%s ", room, hstop);
                        setcolor(C_HOST);
                        if (hst[0] == '[') {
				int i;
				putchar('[');
				setcolor(C_DOING);
				i = cml_nprintf(MAXLEN_DOING-1, "%s", hst+1);
				i = MAXLEN_DOING - i - 2;
				setcolor(C_HOST);
				while (i-- > 0)
					putchar(' ');
				putchar(']');
                        } else {
				if (strlen(hst)> 26)
					hst[27] = '\0';
				printf("%-27s", hst);
                        }
                        Free(hst);
                        putchar('\n');
                }

                /* Prende il numero degli ospiti */
                serv_gets(aaa);
                num_ospiti = extract_int(aaa+4, 0);

                /* Stampa il numero degli uto^Henti ...*/
                setcolor(COL_GREEN);
		putchar('\n');
                if (num_utenti + num_ospiti == 1) {
			printf(_("Dai che fra un po' arrivano! :)"));
                } else {
		  if (num_utenti == 1) {
		        /* ... il numero degli ospiti ...*/
			if (num_ospiti == 1)
			  cml_printf(_("Sono connessi <b>1</b> utente e <b>1</b> ospite."));
			else
			  cml_printf(_("Sono connessi <b>1</b> utente e <b>%d</b> ospiti."),
				     num_ospiti);
		  } else {
                        if (num_ospiti == 0)
			  cml_printf(_("Sono connessi <b>%d</b> utenti."),
				       num_utenti);
			else if (num_ospiti == 1)
			  cml_printf(_("Sono connessi <b>%d</b> utenti e <b>1</b> ospite."),
				       num_utenti);
			else
			  cml_printf(_("Sono connessi <b>%d</b> utenti e <b>%d</b> ospiti."),
				       num_utenti, num_ospiti);
		  }
                }
		putchar('\n');

                /* ... e il numero delle vittime del lag. */
                if(num_lin)
			cml_printf(_("<b>%d</b> login in corso.\n"), num_lin);

        }
	IFNEHAK;
}

/*
 * Chiede al server l'ora e la data.
 */
void ora_data(void)
{
        char buf[LBUF];
        long ora;

        serv_puts("TIME");
        serv_gets(buf);
        if (buf[0] == '2') {
                ora = extract_int(buf, 1);
		putchar('\n');
		stampa_ora(ora);
		putchar('\n');
        }
	IFNEHAK;
}

/*
 * Lock del terminale finche' non viene reimmessa la password.
 */
void lock_term(void)
{
        char buf[LBUF], pwd[MAXLEN_PASSWORD];

        serv_puts("LTRM 1");
        serv_gets(buf);
	if (buf[0] != '2') {
		printf(_("\nNon hai il livello minimo necessario per lockare il terminale.\n"));
		return;
	}
	segnali_ign_sigtstp();
        while (1) {
                new_str_m(_("Inserisci la password: "), pwd,
			  -(MAXLEN_PASSWORD - 1));
                serv_putf("LTRM 0|%s", pwd);
                serv_gets(buf);
		if (buf[0] == '2') {
			segnali_acc_sigtstp();
			return;
		}
		if (buf[1] == '9') {
			cml_print(_("*** Errore... il terminale non &egrave; lockato.\n"));
			segnali_acc_sigtstp();
			return;
		}
		if (buf[1] == '3')
			printf(_("*** Password errata!\n\n"));
        }
}

/*
 * Invia un bug report ai sysop.
 */
void bug_report(void)
{
	struct text *txt;
        char buf[LBUF];

        leggi_file(STDMSG_MESSAGGI, STDMSGID_BUG_REPORT);
        serv_puts("BUGB");
        serv_gets(buf);
        if (buf[0] != '2') {
                printf(sesso ? _("\nNon sei autorizzata ad inviare un Bug Report.\n") :  _("\nNon sei autorizzato ad inviare un Bug Report.\n"));
                return;
        }
        putchar('\n');

	txt = txt_create();
	if (get_text(txt, serverinfo.maxlineebug, 79, 1) > 0)
		send_text(txt);
	txt_free(&txt);

        serv_puts("BUGE");
        serv_gets(buf);
        printf("%s\n", &buf[4]);
}

/*
 * Controlla se c'e' nuova posta.
 */
void biff(void)
{
	char buf[LBUF];
	long mail;

	serv_putf("BIFF %s", nome);
	serv_gets(buf);
	if (buf[0] == '2') {
		mail = extract_long(buf+4, 0);
		if (mail == 0)
			printf(_("Nessun nuovo mail.\n"));
		else if (mail == 1)
			printf(_("Hai un nuovo mail.\n"));
		else
			printf(_("Hai %ld nuovi mail.\n"), mail);
	} else
		printf(_("Non hai una casella della posta.\n"));
	IFNEHAK;
}

/*
 * Legge un file dal server utilizzando l'indice
 */
void leggi_file(Stdmsg_type type, int id)
{
        char aaa[LBUF];
        int riga;

        serv_putf("MSGI %d|%d", type, id);
        serv_gets(aaa);
        if (aaa[0] == '2') {
                riga=0;
		/* Visualizza file con pager rudimentale */
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        riga++;
                        if (riga == NRIGHE) {
                                hit_any_key();
                                riga = 1;
                        }
                        cml_printf("%s", &aaa[4]);
			putchar('\n');
                }
        }
}

void leggi_file_pager(int type, int id)
{
        char aaa[LBUF];

        serv_putf("MSGI %d|%d", type, id);
        serv_gets(aaa);
        if (aaa[0] == '2')
		file_pager(NULL);
}

void help(void)
{
        char aaa[LBUF];

        serv_puts("HELP");
        serv_gets(aaa);
        if (aaa[0]=='2') {
		push_color();
		setcolor(C_HELP);
		file_pager(NULL);
		pull_color();
	}
	IFNEHAK;
}

/* Legge il file di Help */
void Help(void)
{
        char scelta;
        char aaa[LBUF], buf[LBUF], buf1[LBUF];
        int n, len;

        printf("\n\n");
        /* Carica la lista dei file */
        serv_puts("MSGI 1|-1");
        serv_gets(aaa);
        n=0;
	len = 0;
        if (aaa[0]=='2')
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        extract(buf, &aaa[4], 2);
                        sprintf(buf1, " (%2d) %s", n+1, buf);
			if (len == 0) {
				printf("%s", buf1);
				len = strlen(buf1);
			} else if ((len < 40) && (strlen(buf1) < 40)) {
				while(len++<40)
					putchar(' ');
				printf("%s\n", buf1);
				len = 0;
			} else {
				printf("\n%s", buf1);
				len = strlen(buf1);
			}
			n++;
                }
        printf("\n\n");
        scelta = new_int("Scelta: ");
	if (scelta < 1) return;
        if (scelta > n)
                printf(_("Il file di Help richiesto non esiste.\n"));
        else
                leggi_file_pager(1, scelta-1);
	IFNEHAK;
}

/* Legge il file delle news */
void leggi_news(void)
{
        putchar('\n');
	setcolor(C_NEWS);
        leggi_file(STDMSG_MESSAGGI, STDMSGID_NEWS);
}

/*
 * Edit the doing string of the user.
 */
void edit_doing(void)
{
	char *doing, buf[LBUF];

        printf("\nDoing [");
	push_color();
	setcolor(C_DOING);
	cml_print(DOING);
	pull_color();
	printf("]: ");
	setcolor(C_DOING);
        doing = getline_col(MAXLEN_DOING - 1, 0, 0);
        if (doing[0] != '\0') {
		if (DOING)
			Free(DOING);
		DOING = strdup(doing);
		serv_putf("EDNG 1|%s", doing);
		serv_gets(buf);
		if (buf[0]!='2')
			cml_printf(_(" *** Spiacente, si &egrave; presentato un'errore!"));
	}
	Free(doing);
}

void clear_doing(void)
{
	char buf[LBUF];

	if (DOING && (DOING[0]!='\0')) {
		serv_puts("EDNG 0");
		serv_gets(buf);
		Free(DOING);
		DOING = strdup("");
		cml_print(_("*** Ok, il tuo doing &egrave; stato cancellato.\n"));
	} else
		printf(_("*** Attualmente non hai il doing settato!\n"));
}

/* Modifica settaggi terminale */
void edit_term(void)
{
	NRIGHE = new_int_def(_("Numero righe del terminale"), NRIGHE);
}

/* Cerca un messaggio */
void cerca(void)
{
        char expression[LBUF], buf[LBUF];
	int num_risultati;
        char *ptr;

	new_str_m(_("Cerca: "), expression, MAXLEN_FIND);

        /* Trasforma in minuscole */
        for (ptr = expression; *ptr; ptr++)
                if ((*ptr >= 'A') && (*ptr <= 'Z'))
                        *ptr += 'a'-'A';

	serv_putf("FIND %s", expression);
	serv_gets(buf);
	if (buf[0]!='2') {
	        cml_printf(_(" *** Spiacente, non c'&egrave; nessun messaggio corrispondente.\n"));
		find_vroom = false;
	} else {
	        num_risultati = extract_int(buf+4, 0);
		find_vroom = true;
                barflag = 1;
		extractn(current_room, buf+4, 1, ROOMNAMELEN);
		extract(room_type, buf+4, 2);
		utrflags = extract_long(buf+4, 3);
		rflags = extract_long(buf+4, 4);
		cml_printf((num_risultati == 1) ? _("<b>%s</b>%s - <b>%ld</b> messaggio trovato.\n") : _("<b>%s</b>%s - <b>%ld</b> messaggi trovati.\n"), current_room, room_type, num_risultati);
	}
}

/* Entra/Esce dal modo debug */
void toggle_debug_mode(void)
{
        DEBUG_MODE = (DEBUG_MODE ? false : true);
        if (DEBUG_MODE)
                printf("On\n");
        else
                printf("Off\n");
}


/* Emette un segnale acustico */
void Beep(void)
{
        if (uflags[1] & UT_BELL)
                putchar('\a');
}


/* Questa funzione viene chiamata solo al primo collegamento dopo un
 * upgrade del client, e permette fare le riconfigurazioni necessarie. */
void upgrade_client(int vcode)
{
  /* Not used at the moment! */
  if (vcode > 10) {
    return;
  }
}

#if 0
/* Questa funzione viene chiamata solo al primo collegamento dopo un
 * upgrade del client, e permette fare le riconfigurazioni necessarie. */
void upgrade_client_02(void)
{
	char buf[LBUF];

	setcolor(CYAN);
	cml_printf(_("\n\n*** <u>Benvenut%c nella versione <b>%s</b> del cittaclient!!!</u>\n\n"), oa[sesso], CLIENT_RELEASE);
	setcolor(C_DEFAULT);
	cml_print(_(
"In questo nuovo client sono state introdotte delle nuove opzioni che\n"
"richiedono di essere configurate. Ora ti far&ograve; qualche domanda per\n"
"ottimizzare il client.\n\n"

"1) Ora il client pu&ograve; utilizzare dei caratteri colorati.\n"
"   Se il tuo terminale &egrave; in grado di gestire i colori (e li vuoi\n"
"   attivare), rispondi affermativamente alla seguente domanda.\n"
"   Alternativamente, puoi ancora usare la modalit&agrave; '<b>bold</b>'\n"
"   o la modalit&agrave; puro testo.\n\n")
);
	setcolor(L_BLUE);
	uflags[1] |= UT_ANSI;
	if (!new_si_no_def(_("   Visualizza i colori ANSI     :"),
			   IS_SET(uflags[1], UT_ANSI))) {
		if (!(uflags[1] & UT_ANSI) &&
		    new_si_no_def(_("   Usa il grassetto             :"),
				  IS_SET(uflags[1], UT_HYPERTERM)))
			uflags[1] |= UT_HYPERTERM;
		else
			uflags[1] &= ~UT_HYPERTERM;
	}
	update_cmode();

	setcolor(C_DEFAULT);
	cml_print(_("\n"
"2) Ora il client pu&ograve; trattare i caratteri accentati, e pi&uacute; in\n"
"   generale tutti i caratteri ISO-8859-1. Se il tuo terminale &egrave; in\n"
"   grado di visualizzarli (e li vuoi attivare), rispondi affermativamente\n"
"   alla seguente domanda. Alternativamente, se rispondi di no, tutti i\n"
"   caratteri speciali verranno tradotti in ascii puro: ad esempio la \n"
"   lettere accentate avranno l'accento che le segue.\n\n"
));
	setcolor(L_BLUE);
	uflags[1] |= UT_ISO_8859;
	if (!new_si_no_def(_("   Usa caratteri ASCII estesi   :"),
			   IS_SET(uflags[1], UT_ISO_8859)))
		uflags[1] &= ~UT_ISO_8859;

	save_userconfig(true);

	setcolor(C_DEFAULT);
	cml_print(_("\n"
"Benissimo! Il tuo client &egrave; stato riconfigurato!\n"
"Ti ricordo che in ogni momento puoi leggere la tua configurazione con il\n"
"comando \\<<b>.</b>> \\<<b>r</b>>ead \\<<b>c</b>>onfig, e modificarla con \\<<b>.</b>> \\<<b>e</b>>dit\\<<b>c</b>>onfig.\n"
"\nBuon divertimento! :-)\n\n"));

	hit_any_key();

	serv_puts("UPGR");
	serv_gets(buf);
}
#endif
