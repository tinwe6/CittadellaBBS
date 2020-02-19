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
* File : friends.c                                                          *
*        funzioni per la gestione della friend-list.                        *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cittaclient.h"
#include "cml.h"
#include "colors.h"
#include "conn.h"
#include "extract.h"
#include "friends.h"
#include "inkey.h"
#include "prompt.h"
#include "tabc.h"
#include "utility.h"

struct name_list * friend_list;
struct name_list * enemy_list;

/***************************************************************************/
/***************************************************************************/
void carica_amici(void)
{
	char aaa[LBUF], name[MAXLEN_UTNAME];
	long id;

	friend_list = nl_create("friend-list");
	enemy_list = nl_create("enemy-list");

	serv_puts("FRDG");
	serv_gets(aaa);

	if (aaa[0] == '2') {
		while (serv_gets(aaa), strcmp(aaa, "000")) {
		        /* i = extract_int(&aaa[4], 0); */
			id = extract_long(&aaa[4], 1);
			extractn(name, &aaa[4], 2, MAXLEN_UTNAME);
			nl_insert(friend_list, name, id);
		}

                if (serverinfo.server_vcode < ((3 << 8) | 2))
                        return; /* Old server, no enemy-list support */

		while (serv_gets(aaa), strcmp(aaa, "000")) {
		        /* i = extract_int(&aaa[4], 0); */
			id = extract_long(&aaa[4], 1);
			extractn(name, &aaa[4], 2, MAXLEN_UTNAME);
			nl_insert(enemy_list, name, id);
		}
	}
}

static bool salva_amici(void)
{
	char buf[LBUF], buf1[LBUF];
	int f;

	strcpy(buf, "FRDP");
	for (f = 0; f < nl_num(friend_list); f++) {
		sprintf(buf1, "|%ld", nl_getid(friend_list, f));
		strcat(buf, buf1);
	}
	for ( ; f < NL_SIZE; f++)
		strcat(buf, "|-1");
	for (f = 0; f < nl_num(enemy_list); f++) {
		sprintf(buf1, "|%ld", nl_getid(enemy_list, f));
		strcat(buf, buf1);
	}
	for ( ; f < NL_SIZE; f++)
		strcat(buf, "|-1");
	serv_puts(buf);
	serv_gets(buf);

	return (buf[0] == '2');
}

/*
 * Visualizza il contenuto della lista.
 */
void vedi_amici(struct name_list * lista)
{
	int i, n;

	setcolor(C_FRIENDLIST);

	putchar('\n');
	n = nl_num(lista);

	if (n == 0) {
		cml_printf(_("Non ci sono utenti nella <b>%s</b>.\n"),
			   nl_name(lista));
		return;
	} else if (n == 1)
		cml_printf(_("C'&egrave; un utente nella <b>%s</b>:\n"),
                           nl_name(lista));
	else
		cml_printf(_("Ci sono <b>%d</b> utenti nella <b>%s</b>:\n"), n,
			   nl_name(lista));

	if (n % 2 == 0) {
		for (i = 0; (2 * i) < n; i++)
			cml_printf("%2d. <b>%-30s</b> %2d. <b>%s</b>\n", i+1,
                                   nl_get(lista, i), i + n / 2 + 1,
                                   nl_get(lista, i + n / 2));
	} else {
		for (i = 0; (2 * i) < n-1; i++)
			cml_printf("%2d. <b>%-30s</b> %2d. <b>%s</b>\n", i+1,
			       nl_get(lista, i), i + n / 2 + 1 + 1,
			       nl_get(lista, i + n / 2 + 1));

		cml_printf("%2d. <b>%-30s</b>\n", n / 2 + 1,
		       nl_get(lista, n / 2));
	}
	IFNEHAK;
}

/* Edit Friend-list */
/* 'a' aggiungi, 'r' rimuovi, 's' salve ed esci, 'l' lascia perdere */
void edit_amici(struct name_list * nl)
{
	char buf[LBUF], nick[MAXLEN_UTNAME];
	char quit;
	int c, f, f1;

	prompt_curr = P_FRIEND;
	vedi_amici(nl);
	putchar('\n');
	quit = 0;
	while (!quit) {
                print_prompt();
		do
			c = inkey_sc(1);
		while (!index("arslx", c));
                setcolor(C_FRIEND_TEXT);
		switch (c) {
		case 'a':	/* Add friend */
			printf("Add\r");
			del_prompt();
			if (nl_num(nl) == NL_SIZE) {
				cml_printf(_("La lista &egrave; piena, non puoi aggiungere altri utenti.\n"));
                                break;
                        }
                        get_username(_("Nome dell'utente da aggiungere: "), nick);
                        if (nl_in(nl, nick) != -1) {
                                cml_printf(_("L'utente %s &egrave; gi&agrave; nella lista!\n\n"), nick);
                                break;
                        }
                        if (!strncmp(nick, nome, MAXLEN_UTNAME)) {
                                cml_printf(_("Non puoi aggiungere il tuo nome alle liste di utenti!\n\n"));
                                break;
                        }
                        if ((nl == friend_list) && (is_enemy(nick))) {
                                cml_printf(_("%s era un tuo nemico! Lo sposto tra gli amici.\n"), nick);
                                nl_remove(enemy_list, nick);
                        } else if ((nl == enemy_list) && (is_friend(nick))) {
                                cml_printf(_("%s era un tuo amico! Lo sposto tra i nemici.\n"), nick);
                                nl_remove(friend_list, nick);
                        }

                        /* Chiede al server il # matricola */
                        serv_putf("GMTR %s", nick);
                        serv_gets(buf);
                        if (buf[0] == '2') {
                                nl_insert(nl, nick,
                                          strtol(buf+4, NULL, 10));
                                vedi_amici(nl);
                                putchar('\n');
                        } else
                                printf(_("L'utente %s non esiste.\n\n"), nick);
			break;

		case 'r':	/* Remove friend */
			printf("Remove\r");
			del_prompt();
			f = new_int(_("Numero dell'utente da rimuovere: "));
			if (nl_get(nl, f - 1)) {
				cml_printf(_("Ok, %s &egrave; stato rimosso dalla lista.\n"), nl_get(nl, f-1));
				nl_removen(nl, f - 1);
				vedi_amici(nl);
				putchar('\n');
			} else
				printf(_("Numero non valido.\n\n"));
			break;

		case 's':	/* Save friend-list and quit */
			printf("Save\r");
			del_prompt();
			if (salva_amici())
				cml_printf(_("Ok, la %s &egrave; stata aggiornata.\n"), nl_name(nl));
			else
				printf(_("Non hai il livello necessario per modificare la lista!\n"));
			quit = 1;
			break;

		case 'l':	/* Quit */
			/* NOTA: QUI DOVREBBE RIPRISTINARE LA VECCHIA LISTA! */
			printf("Leave\r");
			del_prompt();
			quit = 1;
			break;

		case 'x':
                        printf("Xchange\r");
			del_prompt();
			printf(_("Scambia due utenti.\n"));
			f = new_int(_("Primo utente: "));
			f1 = new_int(_("Secondo utente: "));
			if (nl_swap(nl, f-1, f1-1) == NL_OK) {
				printf(_("Ok, utenti scambiati.\n"));
				vedi_amici(nl);
			} else
				printf(_("Numeri utente non validi.\n"));
			putchar('\n');
			break;
		}
	}
}

/* Risponde TRUE se str e` nella friend-list */
bool is_friend(const char *str)
{
	return ((nl_in(friend_list, str) < 0) ? false : true);
}


/* Risponde TRUE se str e` nella enemy-list */
bool is_enemy(const char *str)
{
	return ((nl_in(enemy_list, str) < 0) ? false : true);
}

/* Restituisce un puntatore all'amico #n, NULL se non c'e` */
const char * get_friend(int num)
{
	return nl_get(friend_list, num);
}

/* Restituisce un puntatore al nemico #n, NULL se non c'e` */
const char * get_enemy(int num)
{
	return nl_get(enemy_list, num);
}
