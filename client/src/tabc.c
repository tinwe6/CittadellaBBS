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
* File : tabc.c                                                             *
*        routines per Tab Completion dei nomi (utenti, room, etc.)          *
****************************************************************************/
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cittaclient.h"
#include "tabc.h"
#include "ansicol.h"
#include "cml.h"
#include "conn.h"
#include "errore.h"
#include "friends.h"
#include "inkey.h"
#include "macro.h"
#include "utility.h"
#include "generic_cmd.h"

/* Prototipi delle funzioni statiche in questo file */
static int tcgetline(char *str, int max, char maiuscole, int del);
static int tc_newstring(char *str, int len, int mode, int del);
static int tcnewprompt_def(const char *prompt, char *def, char *str, int len,
                           int mode);
static bool cerca_completamento(char *cmp, char *buf, int num, int mode);
static bool check_recipient(struct name_list *nl, char *caio, int status);
static void recipient_err(const char *format, ...);

/**************************************************************************
 * 
 * Funzioni per input da tastiera con tab completion.
 * 
 * mode = 0 : completa con i nomi degli utenti connessi
 * mode = 1 : completa con i nomi degli utenti in file_utenti
 * mode = 2 : completa con i nomi dei file di help (DA FARE)
 * mode = 3 : completa con i nomi delle room conosciute
 * mode = 4 : completa con i nomi delle room con nuovi messaggi
 *
 **************************************************************************/
/*
 * Funzione getline con gestione del tasto tab.
 *
 * Valore di uscita: TC_ENTER    se esco con enter
 *                   TC_CONTINUE se esco con ','
 *                   TC_TAB      se esco con tab
 *           inoltre TC_MODIF e' settato se vi sono modifiche.
 *
 * Cancella 'del' caratteri dopo il primo tasto premuto.
 */
static int tcgetline(char *str, int max, char maiuscole, int del)
{
        int a, b;
        int flag = 0;

        if (max == 0)
                return TC_ERROR;

        if (max < 0)
                max = (0 - max);

        printf("%s", str);
        do {
                a = 0;
                a = inkey_sc(0);
                if (del) {
                        putnspace(del);
                        delnchar(del);
                        del = 0;
                }
                if ((isascii(a) && isprint(a)) || (a == Key_BS)
		    || (a == Key_TAB) || (a == Ctrl('X'))) {
                        b = strlen(str);
                        switch(a) {
                        case Key_BS:
                                if (b != 0) {
                                        delchar();
                                        str[b - 1] = '\0';
                                        flag = TC_MODIF;
                                }
                                break;
                        case Key_TAB:
                                if (b != 0) {
                                        delnchar(b);
                                        return (TC_TAB | flag);
                                }
                                break;
			case ',':
				putchar(',');
				return TC_CONTINUE;
			case Ctrl('X'):
				return TC_ABORT;
                        default:
                                if (b < max) {
                                        if (maiuscole
                                            && ((b==0) || (str[b - 1] == ' '))
                                            && (islower(a)))
                                                a = toupper(a);
                                        putchar(a);
                                        str[b] = a;
                                        str[b+1] = '\0';
                                        flag = TC_MODIF;
                                }
                                break;
                        }
                }
        } while ((a != 13) && (a != 10));

	putchar(' ');

        return TC_ENTER;
}

/*
 * tc_newstring()  -  prompt for a string with no existing value
 *                    with tab completion on values given by mode.
 */
static int tc_newstring(char *str, int len, int mode, int del)
{
        int ret;
        int matches = 0; /* numero di corrispondenze */
        char buf[LBUF];
        char cmp[LBUF];
  
        str[0] = 0;
        do {
		ret = tcgetline(str, len, 1, del);
                switch (ret) {
		case TC_ABORT:
			return ret;
                case TC_TAB:
                        if (cerca_completamento(cmp, buf, matches, mode)) {
                                strcpy(str, cmp);
                                matches++;
                        } else {
                                strcpy(str, buf);
                                matches = 0;
                        }
                        break;
                case (TC_TAB | TC_MODIF):
                        strcpy(buf, str);
                        matches = 0;
                        if (cerca_completamento(cmp, buf, matches, mode)) {
                                strcpy(str, cmp);
                                matches++;
                        }
                        break;
                }
                del = 0;
        } while ((ret != TC_ENTER) && (ret != TC_CONTINUE));

        return ret;
}

/*
 * tcnewprompt()  -  prompt for a string with no existing value,
 *                   showing a prompt and accepting tab completion.
 */
int tcnewprompt(const char *prompt, char *str, int len, int mode)
{
        printf("%s", prompt);
        return tc_newstring(str, len, mode, 79 - strlen(prompt));
}

/*
 * tcnewprompt_def()  -  prompt for a string with default value,
 *                       showing a prompt and accepting tab completion.
 */
static int tcnewprompt_def(const char *prompt, char *def, char *str, int len,
                           int mode)
{
	int ret, n;

        n = printf("%s [%s]: ", prompt, def);
        ret = tc_newstring(str, len, mode, 79 - n);
        if (str[0] == 0)
                strcpy(str, def);
	return ret;
}

/*
 * get_username() -  prompt for a username with no existing value,
 *                   showing a prompt and accepting tab completion.
 */
int get_username(const char *prompt, char *str)
{
        int ret;

        printf("%s", prompt);
        ret = tc_newstring(str, MAXLEN_UTNAME, TC_MODE_USER, 0);
        putchar('\n');
        return ret;
}

/*
 * get_username_def() -  prompt for a username with default value,
 *                       showing a prompt and accepting tab completion.
 */
int get_username_def(const char *prompt, char *def, char *str)
{
        char buf[LBUF];
	int ret;

        sprintf(buf, "%s [%s]: ", prompt, def);
        ret = get_username(buf, str);
        if (str[0] == 0)
                strcpy(str, def);
	return ret;
}


/*
 * get_blogname() -  prompt for a user with blog with no existing value,
 *                   showing a prompt and accepting tab completion.
 */
int get_blogname(const char *prompt, char *str)
{
        int ret;

        printf("%s", prompt);
        ret = tc_newstring(str, MAXLEN_UTNAME, TC_MODE_BLOG, 0);
        putchar('\n');
        return ret;
}

/*
 * get_username_def() -  prompt for a user with blog and with default value,
 *                       showing a prompt and accepting tab completion.
 */
int get_blogname_def(const char *prompt, char *def, char *str)
{
        char buf[LBUF];
	int ret;

        sprintf(buf, "%s [%s]: ", prompt, def);
        ret = get_blogname(buf, str);
        if (str[0] == 0)
                strcpy(str, def);
	return ret;
}

/*
 * get_roomname() -  prompt for a roomname with no existing value,
 *                   showing a prompt and accepting tab completion.
 * Se blog == TRUE completa anche per le blog rooms.
 */
int get_roomname(const char *prompt, char *str, bool blog)
{
        int ret;
        int mode;

        if (blog)
                mode = TC_MODE_ROOMBLOG;
        else
                mode = TC_MODE_ROOMKNOWN;
        printf("%s", prompt);
        ret = tc_newstring(str, ROOMNAMELEN, mode, 0);
        putchar('\n');
        return ret;
}

/*
 * get_floorname() -  prompt for a floorname with no existing value,
 *                   showing a prompt and accepting tab completion.
 */
int get_floorname(const char *prompt, char *str)
{
        int ret;

        printf("%s", prompt);
        ret = tc_newstring(str, FLOORNAMELEN, TC_MODE_FLOOR, 0);
        putchar('\n');
        return ret;
}

/*
 * cerca completamento
 */
static bool cerca_completamento(char *cmp, char *buf, int num, int mode)
{
        char cmd[LBUF];
        int offset = 0;

        if (mode == TC_MODE_ROOMBLOG ) {
                if (buf[0] == ':') { /* Cerca nei blog */
                        mode = TC_MODE_BLOG;
                        cmp[0] = ':';
                        offset++;
                } else
                        mode = TC_MODE_ROOMKNOWN;
        }
        serv_putf("TABC %s|%d|%d", buf+offset, num, mode);
        serv_gets(cmd);

        if (cmd[0] == '2') {

                strcpy(cmp+offset, &cmd[4]);
                return true;
        } else
                return false;
}

/*
 * Chiede il nome del destinatario, con tab-completion e friend-list.
 * mode = 0 : Xmsg; 1 : mail; 2 : emote
 * Traduce &num e ^num rispettivamente con friend e enemy.
 * return true se ok, false se problema.
 */
bool get_recipient(char *caio, int mode)
{
	const char * str1[] = {"Messaggio per", "Mail per", "Emote per"};
	const char * str2[] = {"\nNon puoi mandare Xmsg a te stess%c.\n",
			       "\nNon puoi mandare mail a te stess%c.\n",
			       "\nNon puoi mandare emote a te stess%c.\n"};
	const char * str3[] = {"EXpress-message per %s.\n", "Mail per %s.\n",
			       "Emote per %s.\n"};
	const char * str;

	switch (mode) {
	case 0:
	case 2:
		tcnewprompt_def(str1[mode], last_x, caio, MAXLEN_UTNAME, mode);
		break;
	case 1:
		tcnewprompt_def(str1[mode], last_mail, caio, MAXLEN_UTNAME,
				mode);
		break;
	default:
		return false;
	}
	if (caio[0] == '\0')
		return false;
	/* se la stringa inizia con & e' un amico */
	if (caio[0] == '&') {
		str = nl_get(friend_list, strtol(&caio[1], NULL, 10) - 1);
		if (str) {
			strcpy(caio, str);
			printf(str3[mode], caio);
		} else {
			printf("Numero amico non valido.\n");
			return false;
		}
	} else if (caio[0] == '^') {
		str = nl_get(enemy_list, strtol(&caio[1], NULL, 10) - 1);
		if (str) {
			strcpy(caio, str);
			printf(str3[mode], caio);
		} else {
			printf("Numero nemico non valido.\n");
			return false;
		}
	}
	if (!strncmp(caio, nome, MAXLEN_UTNAME)) {
		printf(str2[mode], oa[sesso]);
		strcpy(caio, "");
		return false;
	} else
		strcpy(last_x, caio);
	return true;
}

/*
 * Chiede il nome del destinatario, con tab-completion e friend-list.
 * mode = 0 : Xmsg; 1 : mail; 2 : emote
 * return 1 ok, 0 problema.
 */
struct name_list * get_multi_recipient(int mode)
{
	const char * str1[] = {"Messaggio per", "Mail per", "Emote per"};
	struct name_list * nl;
	char caio[MAXLEN_UTNAME], prompt2[LBUF], format[LBUF];
	const char *prompt;
        int ret = TC_ERROR, i;
	size_t prompt_len;

	if ((mode < 0) || (mode > 2))
		return NULL;

	nl = nl_create("Recipients");

	prompt = str1[mode];
	caio[0] = 0;
	do {
		putchar('\r');
		switch (mode) {
		case TC_MODE_X:
		case TC_MODE_EMOTE:
			ret = tcnewprompt_def(prompt, last_x, caio,
                                              MAXLEN_UTNAME, mode);
			prompt_len = strlen(prompt) + strlen(last_x)+3;
			break;
		case TC_MODE_MAIL:
			ret = tcnewprompt_def(prompt, last_mail, caio,
                                              MAXLEN_UTNAME, mode);
			prompt_len = strlen(prompt) + strlen(last_mail)+3;
			break;
		}
		if (ret == TC_ABORT) {
			nl_free(&nl);
			return NULL;
		}
	} while (check_recipient(nl, caio, ret));

	i = 1;
	while (ret == TC_CONTINUE) {
		i++;
		sprintf(format, "%%%lus#%%d: ", prompt_len-2);
		sprintf(prompt2, format, "", i);
		do {
			putchar('\r');
			ret = tcnewprompt(prompt2, caio, MAXLEN_UTNAME, mode);
			if (ret == TC_ABORT) {
				nl_free(&nl);
				return NULL;
			}
		} while (check_recipient(nl, caio, ret));
	}

	if (nl_num(nl) == 0)
                nl_free(&nl);
        else
                strcpy(last_x, nl_get(nl, 0));

	return nl;
}

static bool check_recipient(struct name_list *nl, char *caio, int status)
{     
        char buf[LBUF];
	const char *fr;
        int amico;

	if (caio[0] == 0) {
                if (status != TC_CONTINUE) {
			putchar('\r');
			putnspace(78);
			putchar('\r');
                        return false;
		}
		delchar();
		recipient_err(_("Devi inserire il nome di un utente!"));
                return true;
        }

	if (!strncmp(caio, nome, MAXLEN_UTNAME)) {
		delnchar(strlen(caio)+1);
		recipient_err(_("Non puoi mandarlo a te stesso!"));
                Beep();
		return true;
	}

	if (caio[0] == '&') {
		amico = strtol(&caio[1], NULL, 10);
		fr = get_friend(amico - 1);
                delnchar(strlen(caio)+1);
		if (fr) {
			strncpy(caio, fr, MAXLEN_UTNAME);
			printf("%s ", caio);
		} else {
			recipient_err("Non c'&egrave; l'amico #<b>%d</b> nella lista!", amico);
			Beep();
			return true;
		}
	} else if (caio[0] == '^') {
		amico = strtol(&caio[1], NULL, 10);
		fr = get_enemy(amico - 1);
                delnchar(strlen(caio)+1);
		if (fr) {
			strncpy(caio, fr, MAXLEN_UTNAME);
			printf("%s ", caio);
		} else {
			recipient_err("Non c'&egrave; il nemico #<b>%d</b> nella lista!", amico);
			Beep();
			return true;
		}
	}

        serv_putf("DEST %s", caio);
        serv_gets(buf);
        if (buf[0] != '2') {
		delnchar(strlen(caio)+1);
                push_color();
                setcolor(C_ERROR);
                print_err_destx(buf);
                pull_color();
                Beep();
                return true;
        }

	if (nl_insert(nl, caio, 0) == NL_OK) {
		putchar('\n');
		return false;
	}

	delnchar(strlen(caio)+1);
	recipient_err("L'utente <b>%s</b> &egrave; gi&agrave; nella lista!", caio);
	Beep();
	return true;
}

static void recipient_err(const char *format, ...)
{
        va_list ap;

        push_color();
        setcolor(C_ERROR);
	va_start(ap, format);
        cml_printf(format, ap);
	va_end(ap);
        pull_color();
}
