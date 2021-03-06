/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
 * File : utente_client.c                                                    *
 *        funzioni di gestione degli utenti per il client.                   *
 ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "cittacfg.h"
#include "ansicol.h"
#include "cittaclient.h"
#include "cml.h"
#include "conn.h"
#include "cterminfo.h"
#include "edit.h"
#include "editor.h"
#include "errore.h"
#include "extract.h"
#include "friends.h"
#include "generic_cmd.h"
#include "inkey.h"
#include "macro.h"
#include "string_utils.h"
#include "tabc.h"
#include "user_flags.h"
#include "utente_client.h"
#include "utility.h"

char *titolo[] = { NOME_DA_BUTTARE, NOME_OSPITE, NOME_NONVAL, NOME_ROMPIBALLE,
		   NOME_NORMALE, NOME_HH, NOME_ROOMAIDE, NOME_FLOORAIDE,
		   NOME_AIDE, NOME_MASTER, NOME_SYSOP, NOME_CITTA };

/***************************************************************************/
/***************************************************************************/
void lista_utenti(void)
{
	char aaa[LBUF];
	int call, post, mail, xmsg, chat;
	char nm[40];
	struct tm *tmst;
	long last_call;
	int lvl, riga;

	setcolor(C_USERLIST);
	cml_printf(_(
"\n<b>"
"Nome                      Lvl Last Call   Calls   Post   Mail   Xmsg   Chat"
"</b>\n"
"------------------------- --- ---------- ------- ------ ------ ------ ------"
"\n"
		     ));
        riga = 4;
        serv_puts("RUSR");
        serv_gets(aaa);
        if (aaa[0] == '2')
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        riga++;
                        if (riga == NRIGHE) {
				hit_any_key();
				riga = 1;
                        }
                        extractn(nm, aaa+4, 0, 40);
                        call = extract_int(aaa+4, 1);
                        post = extract_int(aaa+4, 2);
                        last_call = extract_long(aaa+4, 3);
                        tmst = localtime(&last_call);
                        lvl = extract_int(aaa+4, 4);
                        xmsg = extract_int(aaa+4, 5);
                        mail = extract_int(aaa+4, 6);
                        chat = extract_int(aaa+4, 7);
                        if (!strncmp(nm, nome, MAXLEN_UTNAME))
                                setcolor(C_ME);
                        else if (is_friend(nm))
                                setcolor(C_FRIEND);
                        else if (is_enemy(nm))
                                setcolor(C_ENEMY);
                        else
                                setcolor(C_USER);
                        printf("%-25s ", nm);
                        setcolor(C_USERLIST_LVL);
                        printf("%3d ", lvl);
                        setcolor(C_USERLIST_LAST);
                        cml_printf("%02d</b>/<b>%02d</b>/<b>%4d ",
                                   tmst->tm_mday, tmst->tm_mon+1,
                                   1900+tmst->tm_year);
                        setcolor(C_USERLIST_CALLS);
                        printf("%6d ", call);
                        setcolor(C_USERLIST_POST);
                        printf("%6d ", post);
                        setcolor(C_USERLIST_MAIL);
                        printf("%6d ", mail);
                        setcolor(C_USERLIST_XMSG);
                        printf("%6d ", xmsg);
                        setcolor(C_USERLIST_CHAT);
                        printf("%6d\n", chat);
                }
        IFNEHAK
                }

typedef struct {
	char nome_reale[MAXLEN_RNAME]; /* Nome 'Real-life'             */
	char via[MAXLEN_VIA];          /* Street address               */
	char citta[MAXLEN_CITTA];      /* Municipality                 */
	char stato[MAXLEN_STATO];      /* State or province            */
	char cap[MAXLEN_CAP];          /* Codice di avviamento postale */
	char tel[MAXLEN_TEL];          /* Numero di telefono           */
	char email[MAXLEN_EMAIL];      /* Indirizzo E-mail             */
	char url[MAXLEN_URL];          /* Home page URL                */
	int sex;
} registration_data;

/* Avverte il server che vogliamo editare la registrazione */
static void enter_registration_state(void)
{
	char buf[LBUF];

	serv_puts("BREG");
	serv_gets(buf); /* this can't fail */
	assert(buf[0] == '2');
}

/* Exits the CON_REG state without saving the registration data.        */
/* Returns false is something went wrong (session not in CON_REG state, */
/* or user not yet registered and must provvide the data).              */
static bool abandon_registration(void)
{
	char buf[LBUF];
	serv_putf("RGST 0");
	serv_gets(buf);
	return buf[0] == '2';
}

/* Retrieves the user's registration data from the server. The user session
 * must be in the CON_REG state, or else the request will fail.             */
static registration_data get_registration_data(void)
{
	registration_data data = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, 0};
	char buf[LBUF];

	serv_puts("GREG");
	serv_gets(buf);
	if (buf[0] == '2') {
		extractn(data.nome_reale, buf+4, 0, MAXLEN_RNAME);
		extractn(data.via, buf+4, 1, MAXLEN_VIA);
		extractn(data.citta, buf+4, 2, MAXLEN_CITTA);
		extractn(data.cap, buf+4, 3, MAXLEN_CAP);
		extractn(data.stato, buf+4, 4, MAXLEN_STATO);
		extractn(data.tel, buf+4, 5, MAXLEN_TEL);
		extractn(data.email, buf+4, 6, MAXLEN_EMAIL);
		extractn(data.url, buf+4, 7, MAXLEN_URL);
		data.sex = extract_int(buf+4, 8);
		sesso = data.sex;
	} else {
		printf(_(
"Errore: Non &egrave; stato possibile recuperare i dati di registrazione.\n"
			 ));
	}
	return data;
}

/* Spedisce la registrazione al server */
static void send_registration(registration_data *data)
{
	char buf[LBUF];

	serv_putf("RGST 1|%s|%s|%s|%s|%s|%s|%s|%s|%d", data->nome_reale,
		  data->via, data->citta, data->stato, data->cap, data->tel,
		  data->email, data->url, data->sex);
	serv_gets(buf);
	if (buf[0] == '2') {
		return;
	}
	bool name_invalid = extract_bool(buf+4, 1);
	bool email_invalid = extract_bool(buf+4, 2);
	bool email_taken = extract_bool(buf+4, 3);
	bool user_registered = extract_bool(buf+4, 4);

	if (name_invalid) {
		cml_printf(_(
		     "\nIl nome che hai inserito non &egrave; valido.\n\n"
			     ));
	}
	if (email_taken) {
		cml_printf(_(
"L'e-mail che hai fornito &egrave; gi%agrave; in uso e non possiamo "
"accettarlo:\n"
"Non &egrave; permesso avere pi&ugrave; di un account in questa BBS.\n\n"
			     ));
	} else if (email_invalid) {
		cml_printf(_(
		     "L'indirizzo e-mail fornito non &egrave; valido.\n"
			     ));
	}
	if (!(name_invalid || email_invalid || email_taken)) {
		printf(_(
"\n*** Problema con il server: registrazione non accettata.\n\n"
			 ));
	}
	if (!user_registered) {
		pulisci_ed_esci(NO_EXIT_BANNER);
	}
	hit_any_key();
}

/*
 * Modifica la registrazione dell'utente. Se nuovo = 1 l'utente e' nuovo
 * e la registrazione viene immessa per la prima volta.
 */
static void edit_registration_data(registration_data *data)
{
        int max_tries = 3;   /* max number of tries to enter name and email */
        int tries, c;
	bool new_reg = (data->nome_reale[0] == 0) || (data->email[0] == 0);

        printf(_("Registrazione (i campi con un asterisco sono obbligatori):\n\n"));

        tries = 0;
        if (new_reg) {
                while (true) {
                        tries++;
                        new_str_M(_("Nome e Cognome (*): "), data->nome_reale,
                                      MAXLEN_RNAME-1);
                        if (is_valid_full_name(data->nome_reale)) {
                                break;
                        } else {
                                if (tries < max_tries) {
                                        printf(_(
"\nDevi inserire il tuo nome reale, altrimenti non verrai validato.\n"
"Se vuoi solo dare un'occhiata entra come 'Ospite'.\n\n"
                                                 ));
                                        data->nome_reale[0] = 0;
                                } else {
                                        printf(_(
"\nTorna quando ti ricorderai come scrivere correttamente il tuo nome,\n"
"oppure entra come 'Ospite' per dare un'occhiata.\n"
                                                 ));
                                        pulisci_ed_esci(NO_EXIT_BANNER);
                                }
                                sleep(1);
                        }
                }
        } else {
                new_str_def_M(_("Nome e Cognome (*)"), data->nome_reale,
                              MAXLEN_RNAME - 1);
                if (!is_valid_full_name(data->nome_reale)) {
                        cml_print(_(
"\nIl nome che hai inserito non &egrave; valido. Devi inserire nome e cognome!"
"\n\n"
                                    ));
                }
        }
        printf(_("Sesso: (M/F) "));
        if (new_reg) {
                printf(": ");
        } else {
                printf("[%c]: ", data->sex ? 'F' : 'M');
        }
        c = 0;
        while (((c != 'm') && (c != 'M') && (c != 'f') && (c != 'F')
                && (c != 10) && (c != 13))
               || (((c == 10) || (c == 13)) && new_reg)) {
                c = inkey_sc(0);
        }
        if ((c == 'M') || (c == 'm')) {
		putchar('M');
                data->sex = 0;
		sesso = data->sex;
        } else if ((c == 'F') || (c == 'f')) {
		putchar('F');
                data->sex = 1;
		sesso = data->sex;
        }
	putchar('\n');

        new_str_def_M(_("Indirizzo"), data->via, MAXLEN_VIA - 1);
        new_str_def_M(_("Citt&agrave;"), data->citta, MAXLEN_CITTA - 1);
        new_str_def_m(_("Codice di avviamento postale"), data->cap,
		      MAXLEN_CAP - 1);
        new_str_def_M(_("Stato"), data->stato, MAXLEN_STATO - 1);
        new_str_def_m(_("Numero di telefono"), data->tel, MAXLEN_TEL-1);

        if (new_reg) {
                cml_print(_(
"\n Ora devi fornire il tuo indirizzo e-mail. "
" Verr&agrave; usato per inviarti\n"
" la chiave di validazione, senza la quale non potrai lasciare messaggi\n"
" e usufruire di tutti i servizi della BBS.\n"
" Se vuoi semplicemente entrare per dare un'occhiata, puoi comunque\n"
" collegarti con il nome 'Ospite'.\n\n"
                            ));
        }

        tries = 0;
	char new_email[MAXLEN_EMAIL];
        while (true) {
		strncpy(new_email, data->email, MAXLEN_EMAIL);
                tries++;
		if (*new_email) {
			new_str_def_m(_("Indirizzo e-mail (*)"), new_email,
				      MAXLEN_EMAIL - 1);
		} else {
			new_str_m(_("Indirizzo e-mail (*): "), new_email,
				  MAXLEN_EMAIL - 1);
		}
                if (is_valid_email(new_email)) {
                        break;
                } else {
                        if (tries < max_tries) {
                                cml_printf(_(
"\n"
"L'indirizzo e-mail fornito non &egrave; valido. Devi inserire un indirizzo\n"
"corretto per poter ricevere una chiave di validazione e registrarti alla BBS."
"\n\n"
                                             ));
                                sleep(1);
                        } else {
                                if (new_reg) {
                                        printf(_(
"\nCi dispiace, ma se non inserisci il tuo e-mail non puoi creare un tuo\n"
"account personale. Puoi comunque visitare la BBS come 'Ospite'.\n"
                                                 ));
                                        pulisci_ed_esci(NO_EXIT_BANNER);
					sleep(1);
                                } else {
                                        printf(_(
"\n"
"Ok, teniamo il vecchio e-mail visto che non riesci a digitare quello nuovo "
":-)\n\n"
                                                 ));
                                }
                        }
                }
        }
	strncpy(data->email, new_email, MAXLEN_EMAIL);

        new_str_def_m(_("URL della Home Page"), data->url, MAXLEN_URL - 1);
}

void new_registration(void)
{
	registration_data data = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, 0};

	for(;;) {
		edit_registration_data(&data);
		printf("\nI dati che hai inserito sono corretti (s/n)? ");
		if (si_no() == 's') {
			send_registration(&data);
			return;
		}
		putchar('\n');
	}
}

void modify_registration(void)
{
	registration_data data;

	enter_registration_state();
	data = get_registration_data();
	edit_registration_data(&data);
	printf(sesso
	       ? _("Sei sicura di voler mantenere le modifiche (s/n)? ")
	       : _("Sei sicuro di voler mantenere le modifiche (s/n)? ")
	       );
	if (si_no() == 's') {
		send_registration(&data);
	} else {
		abandon_registration();
	}
}

/*
 * Stampa il tempo trascorso, dove 'a' sono i cicli del server,
 * 'b' i minuti trascorsi, e 'c' le ore.
 * Il tutto viene stampato solo se la durata e' superiore a 'min' secondi.
 * Se 'str' non e' NULL, il testo e' preceduto da 'str'.
 * Restituisce 1 se ha stampato qualcosa, altrimenti 0.
 */
int durata(int a, int b, int c, int min, char *str)
{
        int FREQUENZA = 100;	/* ATTENZIONE: DA CAMBIARE */

        if ((3600 * c + 60 * b + a / FREQUENZA) > min) {
                if (str)
                        cml_printf("%s", str);
                if (c != 0) {
                        if (c == 1)
                                printf(_("1 ora "));
                        else
                                printf(_("%d ore "), c);
                        printf(_("%d min e %d sec.\n"), b, a / FREQUENZA);
                } else if (b != 0) {
                        printf(_("%d min e %d sec.\n"), b, a / FREQUENZA);
                } else
                        printf(_("%d sec.\n"), a / FREQUENZA);
                return 1;
        }
        return 0;
}

char profile(char *nome_def)
{
        char cmd[LBUF], buf[LBUF], aaa[LBUF], bbb[LBUF], ccc[LBUF];
        char nick[MAXLEN_UTNAME];
        int a, b, c, moons, weeks, giorni, ore, minuti;
        char connesso, lock;
        long tempo, mail, pc, tol, ora;
        int lvl, riga = 2, room, sex = 0;
        int myprofile;
        size_t lung;

        if (nome_def == NULL)
                get_username_def(_("Informazioni sull'utente"), last_profile,
                                 nick);
        else
                strcpy(nick, nome_def);

        if (strlen(nick) == 0)
                return 1;

        if (!strcmp(nick, "Ospite")) {
                cml_print(_("\n"
			    "'Ospite' &egrave; il nome di un account pubblico "
			    "per chi vuole visitare la BBS.\n"
                            ));
                return 0;
        }

        if (strlen(nick) > 0) {
                serv_putf("PRFL %s", nick);
                serv_gets(cmd);

                if (cmd[0] != '2') {
                        printf(_("\nL'utente '%s' non esiste.\n"), nick);
                        strcpy(last_profile, "");
                        return 2;	/* Utente sconosciuto */
                }
                strcpy(last_profile, nick);
                serv_gets(cmd);
                extractn(nick, cmd+4, 0, MAXLEN_UTNAME);
                lvl = extract_int(cmd+4, 1);
                connesso = extract_int(cmd+4, 2);
                putchar('\n');
                setcolor(C_USER);
                cml_printf("<b>%-40s</b>", nick);
                setcolor(C_PRFL);
                cml_printf("(<b>%s</b>)\n", titolo[lvl]);
                setcolor(C_PRFL_ADDR);
                while (serv_gets(cmd), strcmp(cmd, "000"))
                        switch (cmd[4]) {
                        case '1':
				myprofile = !(strcmp(nome, nick));
				printf(_("\nDati personali"));
				if (myprofile)
                                        printf(_(
" [i dati tra parentesi quadre non sono visibili agli altri]"
                                                 ));
				printf(":\n\n");
				riga++;
				extract(aaa, cmd+4, 1);
				extract(bbb, cmd+4, 9);
				if (strlen(aaa))
                                        printf("  ");
				if (myprofile && !(uflags[2] & UT_VSEX))
                                        printf("[");
				if (bbb[0] == 'M') {
                                        if (strlen(aaa))
                                                printf(_("Mr."));
                                        else
                                                cml_print(_("  &Egrave; un maschio.\n"));
				} else if (bbb[0] == 'F') {
                                        if (strlen(aaa))
                                                printf(_("Mrs."));
                                        else
                                                cml_print(_("  &Egrave; una femmina.\n"));
                                        sex = 1;
				}
				if (myprofile && !(uflags[2] & UT_VSEX)
				    && (uflags[2] & UT_VNOME))
                                        putchar(']');
				if (strlen(aaa)) {
                                        if (bbb[0] == 'M' || bbb[0] == 'F')
                                                putchar(' ');
                                        if (myprofile && !(uflags[2]&UT_VNOME)
                                            && (uflags[2] & UT_VSEX))
                                                putchar('[');
                                        printf("%s", aaa);
                                        if (myprofile && !(uflags[2]&UT_VNOME))
                                                putchar(']');
                                        putchar('\n');
                                        riga++;
				}
				extract(aaa, cmd+4, 2);
				if (strlen(aaa)) {
                                        printf("  ");
                                        if (myprofile && !(uflags[2]&UT_VADDR))
                                                printf("[%s]", aaa);
                                        else
                                                printf("%s", aaa);
                                        putchar('\n');
                                        riga++;
				}
				extract(aaa, cmd+4, 3);
				extract(bbb, cmd+4, 4);
				extract(ccc, cmd+4, 5);
				if (strlen(aaa) || strlen(bbb)
				    || strlen(ccc)) {
                                        printf("  ");
                                        if (myprofile && !(uflags[2]&UT_VADDR))
                                                printf("[%s - %s   (%s)]",
                                                       aaa, bbb, ccc);
                                        else
                                                printf("%s - %s   (%s)", aaa,
                                                       bbb, ccc);
                                        putchar('\n');
                                        riga++;
				}
				extract(aaa, cmd+4, 6);
				extract(bbb, cmd+4, 7);
				if (strlen(aaa)) {
                                        printf("  ");
                                        if (myprofile && !(uflags[2]&UT_VTEL))
                                                printf(_("[Tel: %s]"), aaa);
                                        else
                                                printf(_("Tel: %s"), aaa);
                                        printf("    ");
				}
				if (strlen(bbb)) {
                                        if (aaa[0] == 0)
                                                printf("  ");
                                        if (myprofile&&!(uflags[2]&UT_VEMAIL))
                                                printf(_("[E-mail: %s]"), bbb);
                                        else
                                                printf(_("E-mail: %s"), bbb);
				}
				if (strlen(aaa) || strlen(bbb))
                                        putchar('\n');
				riga++;
				extract(aaa, cmd+4, 8);
				if (strlen(aaa)) {
                                        printf("  ");
                                        if (myprofile && !(uflags[2]&UT_VURL))
                                                printf(_("[Home page URL: %s]"),aaa);
                                        else
                                                printf(_("Home page URL: %s"),
                                                       aaa);
                                        putchar('\n');
                                        riga++;
				}
				break;
                        case '2':
				printf(_("\nUtente numero: %d\n"),
				       extract_int(cmd+4, 1));
				cml_printf(_(
"Ha chiamato <b>%d</b> volte,\ne ha lasciato <b>%d</b> post, <b>%d</b> mail"
" e inviato <b>%d</b> X-msg <b>%d</b> chat-msg.\n"
                                             ), extract_int(cmd+4, 2),
                                           extract_int(cmd+4, 3),
                                           extract_int(cmd+4, 10),
                                           extract_int(cmd+4, 4),
                                           extract_int(cmd+4, 11)
                                           );
				if (extract_int(cmd+4, 12)) {
                                        cml_printf(
					    "La casa di %s &egrave; aperta.\n",
						   nick);
                                        riga++;
				}
				//putchar('\n');
				pc = extract_long(cmd+4, 5);
				printf(_("Primo collegamento : "));
				stampa_datall(pc);
				tol = extract_long(cmd+4, 6);
				tempo = extract_long(cmd+4, 9);
				ora = time(0);
				if (tempo)
                                        tol = tol + (ora - tempo);
				tempo = tol;
				moons = (int) (tempo / (4 * 7 * 24 * 60 * 60));
				tempo -= moons * 4 * 7 * 24 * 60 * 60;
				weeks = (int) (tempo / (7 * 24 * 60 * 60));
				tempo -= weeks * 7 * 24 * 60 * 60;
				giorni = (int) (tempo / (24 * 60 * 60));
				tempo -= giorni * 24 * 60 * 60;
				ore = (int) (tempo / 3600);
				tempo -= ore * 3600;
				minuti = (int) (tempo / 60);
				printf(_("\nHa trascorso "));
				if (moons == 1)
                                        printf(_("1 luna, "));
				else if (moons > 1)
                                        printf(_("%d lune, "), moons);
				if (weeks == 1)
                                        printf(_("1 settimana, "));
				else if (weeks > 1)
                                        printf(_("%d settimane, "), weeks);
				if (giorni == 1)
                                        printf(_("1 giorno, "));
				else if (giorni > 1)
                                        printf(_("%d giorni, "), giorni);
				if (ore == 1)
                                        printf(_("1 ora, "));
				else
                                        printf(_("%d ore, "), ore);
				if (minuti == 1)
                                        printf(_("e 1 minuto in linea "));
				else
                                        printf(_("%d minuti in linea "),
                                               minuti);
				cml_printf(_("(<b>%.1f</b>%% vita).\n"),
                                           (float)tol/(ora-pc)*100.0);
				riga += 7;
				if (!connesso) {
                                        tempo = extract_int(cmd+4, 7);
                                        extract(aaa, cmd+4, 8);
                                        printf(_(
					     "Ultimo collegamento da %s il "
                                                 ), aaa);
                                        stampa_datal(tempo);
                                        putchar('\n');
                                        riga++;
				}
				break;
                        case '3':
				room = 0;
				lung = 16;
				while (serv_gets(aaa), strcmp(aaa, "000")) {
                                        if (room == 0) {
                                                room++;
                                                cml_printf(_(
						    "<b>Room Aide di</b>  : "
                                                             ));
                                        }
                                        lung += strlen(aaa) - 2;
                                        if (lung > 79) {
                                                putchar('\n');
                                                lung = strlen(aaa) - 2;
                                                riga++;
                                        }
                                        printf("%s  ", aaa+4);
				}
				if (room) {
                                        putchar('\n');
                                        riga++;
				}
				break;
                        case '4':
				room = 0;
				lung = 16;
				while (serv_gets(aaa), strcmp(aaa, "000")) {
                                        if (room == 0) {
                                                room++;
                                                cml_printf(_(
						    "<b>Room Helper di</b>: "
                                                             ));
                                        }
                                        lung += strlen(aaa) - 2;
                                        if (lung > 79) {
                                                putchar('\n');
                                                lung = strlen(aaa) - 2;
                                                riga++;
                                        }
                                        printf("%s  ", aaa+4);
				}
				if (room) {
                                        putchar('\n');
                                        riga++;
				}
				break;
                        case '5':
				tempo = extract_int(cmd+4, 2);
				extract(aaa, cmd+4, 1);
				cml_printf(sex
                                           ? _("&Egrave; collegata da %s da ")
                                           : _("&Egrave; collegato da %s da "),
                                           aaa);
				stampa_data_smb(tempo);
				putchar('\n');
				riga++;
				a = extract_int(cmd+4, 4);
				b = extract_int(cmd+4, 5);
				c = extract_int(cmd+4, 6);
				if ((lock = extract_int(cmd+4, 7))) {
                                        printf(_(
				            "ma ha il terminale lockato da "
                                                 ));
                                        durata(a, b, c, 0, NULL);
                                        riga++;
				} else if (extract_int(cmd+4, 3) != 0) {
                                        cml_printf(sex
                                                   ? _(
"&Egrave; occupata in questo momento.\n"
                                                       )
                                                   : _(
"&Egrave; occupato in questo momento.\n"
                                                       ));
                                        riga++;
				} else if (durata(a, b, c, 10, _(
						  "&Egrave; idle da ")
                                                  ))
                                        riga++;
				break;
                        case '6':
                                if (cmd[0] == '2') {
                                        cml_printf(_(
"Ha allegato %lu files e scaricato %lu file.\n"
                                                     ),
                                                   extract_ulong(cmd+4, 4),
                                                   extract_ulong(cmd+4, 3)
                                                   );
                                        cml_printf(_(
"Sono presenti %lu suoi files, per un totale di %lu bytes.\n"),
                                                   extract_ulong(cmd+4, 2),
                                                   extract_ulong(cmd+4, 1)
                                                   );
                                        riga += 2;
                                }
                                break;
                        }
        }

        /* Controlla la posta */
        serv_putf("BIFF %s", nick);
        serv_gets(buf);
        if (buf[0] == '2') {
                mail = extract_long(buf+4, 0);
                if (mail == 0)
                        printf(_("Nessun nuovo mail.\n"));
                else if (mail == 1)
                        printf(_("Un nuovo mail.\n"));
                else
                        printf(_("%ld nuovi mail.\n"), mail);
        }
        putchar('\n');
        riga += 2;

        /* Carica il profile personalizzato */
        serv_putf("PRFG %s", nick);
        serv_gets(buf);
        if (buf[0] == '2') {
                setcolor(C_PRFL_PERS);
                while (serv_gets(buf), strcmp(buf, "000")) {
                        riga++;
                        if (riga == (NRIGHE - 1)) {
                                hit_any_key();
                                riga = 1;
                        }
                        cml_printf("%s\n", &buf[4]);
                }
        }
        IFNEHAK
                setcolor(C_DEFAULT);
        return 0;
}

void caccia_utente(void)
{
        char buf[LBUF];
        char nick[MAXLEN_UTNAME];

        printf(_("Immetti 'Ospite' per cacciare tutti gli ospiti presenti.\n")
	       );
        tcnewprompt(_("Nome dell'utente da cacciare: "),
		    nick, MAXLEN_UTNAME, 0);
        putchar('\n');

        if (nick[0] != 0) {
                serv_putf("CUSR %s", nick);
                serv_gets(buf);

                if (buf[0] != '2')
                        printf(sesso
                               ? _(
"\nNon sei autorizzata a cacciare altri utenti.\n"
                                   )
                               : _(
"\nNon sei autorizzato a cacciare altri utenti.\n"
                                   ));
                else if (buf[4] == '0')
                        cml_printf(_(
"L'utente %s non &egrave; connesso in questo momento.\n"
                                     ), nick);
        }
}

void elimina_utente(void)
{
        char buf[LBUF];
        char nick[MAXLEN_UTNAME];

        get_username(_("Nome dell'utente da eliminare: "), nick);

        if (nick[0] != 0) {
                serv_putf("KUSR %s", nick);
                serv_gets(buf);

                if (buf[0] != '2')
                        printf(sesso
                               ? _(
"\nNon sei autorizzata a cacciare altri utenti.\n"
                                   )
                               : _(
"\nNon sei autorizzato a cacciare altri utenti.\n"
                                   )
                               );
                else if (buf[4] == '0')
                        printf(_("L'utente %s non esiste.\n"), nick);
                else
                        cml_printf(_(
"L'utente %s &egrave; stato eliminato definitivamente.\n"
                                     ), nick);
        }
}

/*
 * Edit User: Comando per aide, per modificare tutti i dati
 *            corrispondenti a un determinato utente.
 */
void edit_user(void)
{
        char buf[LBUF];
        char nick[MAXLEN_UTNAME] = "";
        char newnick[MAXLEN_UTNAME] = "";
        char nome_reale[MAXLEN_RNAME] = ""; /* Nome 'Real-life'      */
        char via[MAXLEN_VIA] = "";	    /* Street address        */
        char citta[MAXLEN_CITTA] = "";	    /* Municipality          */
        char stato[MAXLEN_STATO] = "";	    /* State or province     */
        char cap[MAXLEN_CAP] = "";   /* Codice di avviamento postale */
        char tel[MAXLEN_TEL] = "";          /* Numero di telefono    */
        char email[MAXLEN_EMAIL] = "";      /* Indirizzo E-mail      */
        char url[MAXLEN_URL] = "";          /* Home page URL         */
        char valkey[MAXLEN_VALKEY] = "";    /* Validation key        */
        /*	char sex;  */
        int lvl, newlvl, val, secpmsg, msgph, newmsgph, divieti, flags;

        get_username(_("Dati sull'utente: "), nick);
        if (nick[0] == 0)
                return;

        /* Chiede al server la registrazione attuale */
        serv_putf("GUSR %s", nick);
        serv_gets(buf);
        if (buf[0] != '2') {
                if (buf[1]=='5') {
                        printf(_("L'utente %s non esiste.\n"), nick);
                } else {
                        printf(sesso
                               ? _(
"Non sei autorizzata a modificare i dati personali di altri utenti.\n"
                                   )
                               : _(
"Non sei autorizzato a modificare i dati personali di altri utenti.\n"
                                   ));
                }
                return;
        }

        extractn(nome_reale, buf+4, 0, MAXLEN_RNAME);
        extractn(via, buf+4, 1, MAXLEN_VIA);
        extractn(citta, buf+4, 2, MAXLEN_CITTA);
        extractn(cap, buf+4, 3, MAXLEN_CAP);
        extractn(stato, buf+4, 4, MAXLEN_STATO);
        extractn(tel, buf+4, 5, MAXLEN_TEL);
        extractn(email, buf+4, 6, MAXLEN_EMAIL);
        extractn(url, buf+4, 7, MAXLEN_URL);
        lvl = extract_int(buf+4, 8);
        extractn(valkey, buf+4, 9, MAXLEN_VALKEY);
 	secpmsg = extract_int(buf+4, 10);
        divieti = extract_int(buf+4, 11);

 	if (secpmsg == 0)
                msgph = 0;
        else
                msgph = (int) ((3600/STEP_POST) / secpmsg);

        setcolor(C_CONFIG_H);
        printf(_("\nRegistrazione:\n"));
        setcolor(C_CONFIG_B);
        new_str_def_M(_(" Nome e Cognome (VERI)"), nome_reale, MAXLEN_RNAME-1);
        new_str_def_M(_(" Indirizzo"), via, MAXLEN_VIA - 1);
        new_str_def_M(_(" Citt&agrave;"), citta, MAXLEN_CITTA - 1);
        new_str_def_m(_(" Codice di avviamento postale"), cap, MAXLEN_CAP - 1);
        new_str_def_M(_(" Stato"), stato, MAXLEN_STATO - 1);
        new_str_def_m(_(" Numero di telefono"), tel, MAXLEN_TEL - 1);
        new_str_def_m(_(" Indirizzo e-mail"), email, MAXLEN_EMAIL - 1);
        new_str_def_m(_(" URL della Home Page"), url, MAXLEN_URL - 1);

        setcolor(C_CONFIG_H);
        cml_printf(_("\nPropriet&agrave; utente:\n"));
        setcolor(C_CONFIG_B);
        newlvl = -1;
        while (newlvl < 0) {
                newlvl = new_int_def(_(" Livello di accesso dell'utente"),
                                     lvl);
                if (newlvl > livello) {
                        printf(_(
" *** Non puoi assegnare un livello superiore al tuo!!\n"
                                 ));
                        newlvl = -1;
                } else
                        lvl = newlvl;
        }

        /* il server salva secpmsg (1= STEP_POST secondi per messaggio) */
        newmsgph = 1000;
 	while (newmsgph < 0 || newmsgph > ((3600*STEP_POST / 256) - 1)) {
                newmsgph = new_int_def(_(
			       " Numero di messaggi per ora (0 per illimitati)"
                                         ), msgph);
                msgph = newmsgph;
 	}
        if (msgph == 0) {
                secpmsg = 0;
        } else {
                secpmsg = (int)(3600.0 / (msgph*STEP_POST) + .5);
        }
        if (secpmsg > 255) {
                secpmsg = 255;
        }

        /* validazione */
        if (valkey[0] != 0) {
                cml_print(_(
		    " L'utente non &egrave; validato. Vuoi validarlo? (s/n) "
                            ));
                val = (si_no() == 's') ? 1 : 0;
        }

        /* Nickname */
        printf(_(" Vuoi modificare il nickname dell'utente? "));
        if (si_no() == 's') {
                new_str_M(_(" Nuovo nickname: "), newnick, MAXLEN_UTNAME - 1);
                if (!strncmp(newnick, nick, MAXLEN_UTNAME))
                        cml_printf(_(
" <b>Ricordati di avvertire l'utente che il suo nick &egrave cambiato!!</b>\n"
                                     ));
        }
        putchar('\n');

        /* Edita i divieti per l'utente */
        flags = 0;
        setcolor(C_CONFIG_H);
        printf(_("\nDivieti:\n"));
        setcolor(C_CONFIG_B);
        if (new_si_no_def(_(" Divieto di inviare X      "),
                          IS_SET(divieti, SUT_NOX)))
                flags |= SUT_NOX;
        if (new_si_no_def(_(" Divieto di usare la chat  "),
                          IS_SET(divieti, SUT_NOCHAT)))
                flags |= SUT_NOCHAT;
        if (new_si_no_def(_(" Divieto di inserire post  "),
                          IS_SET(divieti, SUT_NOPOST)))
                flags |= SUT_NOPOST;
        if (new_si_no_def(_(" Divieto di inviare mail   "),
                          IS_SET(divieti, SUT_NOMAIL)))
                flags |= SUT_NOMAIL;
        if (new_si_no_def(_(" Divieto di postare anonimo"),
                          IS_SET(divieti, SUT_NOANONYM)))
                flags |= SUT_NOANONYM;
        putchar('\n');

        setcolor(C_CONFIG_H);
        printf(sesso
	       ? _("Sei sicura di voler mantenere le modifiche? ")
               : _("Sei sicuro di voler mantenere le modifiche? "));

        if (si_no() == 's') {
                /* Spedisce i nuovi dati al server */
                serv_putf("EUSR %s|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d|%d|%d|%s",
                          nick, nome_reale, via, citta, stato, cap, tel,
                          email, url, lvl, val, secpmsg, flags, newnick);
                serv_gets(buf);
                if (buf[0] == '2')
                        printf(_("I dati di %s sono stati modificati!"), nick);
                else if (buf[1] == '2')
                        printf(_(
"Non puoi modificare i dati di un utente di livello superiore al tuo!\n"
                                 ));
                else if (buf[1] == '4')
                        printf(_(
"Non puoi assegnare un livello superiore al tuo!\n"
                                 ));
                else if (buf[1] == '5')
                        printf(_("L'utente %s non esiste!\n"), nick);
        } else {
                serv_puts("EUSR");
                serv_gets(buf);
        }
        putchar('\n');
}

/* Modifica la password */
void chpwd(void)
{
        char buf[LBUF];
        char oldpwd[MAXLEN_PASSWORD], newpwd[MAXLEN_PASSWORD],
                newpwd1[MAXLEN_PASSWORD];

        putchar('\n');
        leggi_file(STDMSG_MESSAGGI, STDMSGID_CHANGE_PWD);
        /* Verifica la vecchia password */
        new_str_m(_("Inserisci la vecchia password: "), oldpwd,
                  -(MAXLEN_PASSWORD - 1));
        serv_putf("PWDC %s", oldpwd);
        serv_gets(buf);
        if (buf[0] == '2') {
                /* Chiede la nuova password */
                do {
                        new_str_m(_("Inserisci la nuova password: "), newpwd,
                                  -(MAXLEN_PASSWORD - 1));
                        if (!strlen(newpwd))
                                cml_print(_(
"La password non pu&oacute; essere vuota!\n"
                                            ));
                } while (strlen(newpwd) == 0);
                new_str_m(_("Reimmetti la nuova password: "), newpwd1,
                          -(MAXLEN_PASSWORD - 1));
                if (!strcmp(newpwd, newpwd1)) {
                        serv_putf("PWDN %s|%s", oldpwd, newpwd);
                        serv_gets(buf);
                        if (buf[0] != '2')
                                cml_print(_(
"\nLa password non &egrave; stata cambiata.\n"
                                            ));
                        else
                                cml_print(_(
"\nOk! La password &egrave; stata aggiornata.\n"
                                            ));
                } else
                        printf(_("\nLe password non coincidono.\n"));
        } else
                cml_print(_("\nLa password &egrave; errata.\n"));
}

/* Modifica la password di un utente (Aide Only) */
void chupwd(void)
{
        char buf[LBUF], nick[MAXLEN_UTNAME];
        char mypwd[MAXLEN_PASSWORD], newpwd[MAXLEN_PASSWORD];

        putchar('\n');
        leggi_file(STDMSG_MESSAGGI, STDMSGID_CHANGE_PWD);
        /* Chiede la password dell'aide prima di procedere */
        new_str_m(_("Inserisci la tua password: "),mypwd,-(MAXLEN_PASSWORD-1));
        serv_putf("PWDC %s", mypwd);
        serv_gets(buf);

        if (buf[0] == '2') {
                get_username(_("Cambia la password dell'utente : "), nick);
                if (strlen(nick) == 0)
                        return;	/* Input nullo */

                if (!strcmp(nick, "Ospite")) {
                        printf(_(
"\nAll'utente `Ospite' non viene associata nessuna password.\n"
                                 ));
                        return;
                }

                /* Chiede la nuova password */
                serv_putf("PWDU %s|%s|%s", mypwd, nick, newpwd);
                serv_gets(buf);
                if (buf[0] != '2') {
                        switch (buf[1]) {
                        case '0':
                                printf(_(
"\nCi sono stati problemi per inviare l'e-mail."
                                         ));
                                break;
                        case '2':
                                printf(sesso
                                       ? _(
"\nNon sei autorizzata a cambiare la password degli altri utenti."
                                           )
                                       : _(
"\nNon sei autorizzata a cambiare la password degli altri utenti."
                                           ));
                                break;
                        case '3':
                                cml_print(_(
				    "\nLa tua password &egrave; errata."));
                                break;
                        case '5':
                                printf(_("\nL'utente [%s] non esiste."), nick);
                                break;
                        }
                        cml_printf(_(
"\nLa password di [%s] non &egrave; stata cambiata.\n"
                                     ), nick);
                } else
                        cml_printf(_(
"\nOk! La nuova password di [%s] &egrave; stata generata.\n"
                                     ), nick);
        } else
                cml_print(_("La password &egrave; errata.\n"));
}

void enter_profile(void)
{
        struct text *txt = NULL;
        char buf[LBUF];
        char *filename, *str;
        int fd;
        int riga, ext_editor;
        FILE *fp;

        ext_editor = ((uflags[0] & UT_EDITOR) && (EDITOR != NULL)) ? 1 : 0;
        if (ext_editor) {
                /* Apro un file temporaneo per il profile */
                filename = astrcat(TMPDIR, TEMP_EDIT_TEMPLATE);
                fd = mkstemp(filename);
                close(fd);
                chmod(filename, 0660);
                fp = fopen(filename, "w");
        }
        txt = txt_create();

        /* Chiede al server se un profile e' gia' presente per l'utente */
        serv_putf("PRFG %s", nome);
        serv_gets(buf);

        if (buf[0] == '1') {
                if (buf[1] == '6') {	/* Il profile e' nuovo */
                        cml_print(_(
"\nOra puoi inserire un breve testo di presentazione,\n"
"che apparir&agrave; in fondo al tuo profile.\n"
                                    ));
                } else {	/* Errore */
                        printf(_("\n*** Problemi con il server.\n"));
                        if (ext_editor)
                                fclose(fp);
                        txt_free(&txt);
                        return;
                }
        } else {    /* Il profile esiste: lo prendo per modificarlo */
                if (ext_editor) {
                        while (serv_gets(buf), strcmp(buf, "000"))
                                fprintf(fp, "%s\n", &buf[4]);
                        fclose(fp);
                } else {
                        while (serv_gets(buf), strcmp(buf, "000"))
                                txt_put(txt, &buf[4]);
                }
        }

        serv_puts("PRFB");
        serv_gets(buf);

        if (buf[0] != '2') {
                printf(_("\nProblemi con il server.\n"));
                txt_free(&txt);
                return;
        }
        putchar('\n');

        if (ext_editor) {
                edit_file(filename);
                fp = fopen(filename, "r");
                if (fp == NULL) {
                        printf(_("\nProblemi di I/O\n"));
                        serv_puts("ATXT");
                        serv_gets(buf);
                        txt_free(&txt);
                        return;
                }
                riga = 0;
                while ((fgets(buf, LBUF, fp) != NULL)
                       && (riga < serverinfo.maxlineeprfl)) {
                        txt_put(txt, buf);
                        riga++;
                }
                fclose(fp);
                unlink(filename);
                free(filename);
        } else {
                get_text_full(txt, serverinfo.maxlineeprfl, 79, false,
                              C_PRFL_PERS, NULL);
        }

        printf(sesso
               ? _("\nSei sicura di voler mantenere le modifiche (s/n)? ")
               : _("\nSei sicuro di voler mantenere le modifiche (s/n)? ")
               );
        if (si_no() == 's') {
                txt_rewind(txt);
                while ( (str = txt_get(txt))) {
                        serv_putf("TEXT %s", str);
                }
                serv_puts("PRFE");
                serv_gets(buf);
                print_err_edit_profile(buf);
        } else {
                serv_puts("ATXT");
                serv_gets(buf);
        }
        txt_free(&txt);
}
