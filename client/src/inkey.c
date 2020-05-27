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
* File : inkey.c                                                            *
*        gestione input da utente                                           *
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "cittaclient.h"
#include "cmd_da_server.h"
#include "conn.h"
#include "edit.h"
#include "inkey.h"
#include "messaggi.h"
#include "prompt.h"
#include "signals.h"
#include "cterminfo.h"
#include "editor.h"

static int read_key(void);
static int getmeta(char *buf);

/*****************************************************************************/

/*
 * Prende un carattere in input dall'utente.
 * Mentre aspetta che l'utente prema un tasto, ascolta ed elabora
 * i comandi provenienti dal server.
 * Se 'esegue_non_urgenti' e' true, allora elabora anche i comandi non
 * urgenti, altrimenti solo quelli urgenti.
 */

#define INKEY_TIMEOUT_MS 33
#define MSEC_TO_USEC 1000
#define INKEY_TIMEOUT_USEC (INKEY_TIMEOUT_MS*MSEC_TO_USEC)

#define dwrite(str) write(STDOUT_FILENO, str, strlen(str))

int inkey_sc(bool exec_all_cmds)
{
        fflush(stdout);

	bool window_changed = false;
	struct timeval *timeout_ptr = NULL;
	struct timeval timeout;
	bool fast_mode = false;

        for (;;) {
                /* Controlliamo se il client ha input */
		fd_set input_set;
                for (;;) {
			esegui_segnali(&window_changed, &fast_mode);
			if (window_changed) {
				return Key_Window_Changed;
			}

                        FD_ZERO(&input_set);
                        FD_SET(0, &input_set);         /* Input dall'utente */
                        FD_SET(serv_sock, &input_set); /* Input dal server  */
                        timeout = (struct timeval){
				.tv_sec = 0,
				.tv_usec = INKEY_TIMEOUT_USEC,
			};
			timeout_ptr = fast_mode ? &timeout : NULL;
			int ret = select(serv_sock + 1, &input_set, NULL, NULL,
					 timeout_ptr);
			if (ret == -1) {
                                if (errno == EINTR) {
                                        /* Incoming signal */
                                } else {
                                        perror("Select");
                                        exit(1);
                                }
                        } else if (ret > 0) {
				break;
			}
		}

		/* Check incoming server commands */
                if (FD_ISSET(serv_sock, &input_set)) {
#ifdef HAVE_CTI
			if (prompt_curr == P_EDITOR) {
				window_push(0, gl_Editor_Pos - 1);
				cti_mv(0, gl_Editor_Pos - 1);
				scroll_up();
				//fflush(stdout);
			}
#endif
                        do {
				if (elabora_input() & CMD_POST_TIMEOUT) {
					return Key_Timeout;
				}
				if (exec_all_cmds) {
					esegue_comandi(0);
				}
                        } while (server_input_waiting);
			if (prompt_curr == P_EDITOR) {
#ifdef HAVE_CTI
				window_pop();
				cti_mv(gl_Editor_Hcurs, gl_Editor_Vcurs);
#endif
			} else {
				print_prompt();
			}
			fflush(stdout);
		}

		/* Check user input */
                if (FD_ISSET(0, &input_set)) {
			int key = read_key();
			return key;
                }
        }
}

/*
 * Funzione utilizzata dal pager per l'attesa di input.
 * Prende un carattere in input dall'utente o una riga di testo.
 * Mentre aspetta che l'utente prema un tasto, ascolta ed elabora
 * i comandi provenienti dal server.
 * Se (esegue) allora elabora tutti i comandi, altrimenti solo quelli
 * urgenti.
 * Restituisce un intero con i bit INKEY_KEY e INKEY_SERVER settati
 * se c'e' stato input da tastiera o da server rispettivamente.
 * Se e' stato premuto un tasto, questo viene memorizzato in *key,
 * se c'e' input da server si trova nella stringa *str.
 * Puo' capitare che ci sia input contemporaneo, nel qual caso i due bit
 * sono settati e i risultati si trovano in str e *key.
 */
int inkey_pager(int esegue, char **str, int *key)
{
        fflush(stdout);
	*key = 0;

	bool window_changed = false;
	struct timeval *timeout_ptr = NULL;
	struct timeval timeout;
	bool fast_mode = false;

        int ret_code = 0;
        for (;;) {
		/* Controlliamo se il client ha input */
		fd_set input_set;
		for (;;) {
			esegui_segnali(&window_changed, &fast_mode);
			if (window_changed) {
                                *key = Key_Window_Changed;
				return INKEY_WINCH;
			}

			FD_ZERO(&input_set);
			FD_SET(0, &input_set);         /* Input dall'utente */
			FD_SET(serv_sock, &input_set); /* Input dal server  */
			timeout = (struct timeval){
				.tv_sec = 0,
				.tv_usec = INKEY_TIMEOUT_USEC,
			};
			timeout_ptr = fast_mode ? &timeout : NULL;
			int ret = select(serv_sock + 1, &input_set, NULL, NULL,
					 timeout_ptr);
			if (ret == -1) {
				if (errno == EINTR) {
					/* Incoming signal */
				} else {
					perror("Select");
					exit(1);
				}
			} else if (ret > 0) {
				break;
			}
		}

		/* Check incoming server commands */
                if (FD_ISSET(serv_sock, &input_set)) {
			switch (elabora_input()) {
			case NO_CMD:
				ret_code |= INKEY_SERVER;
				*str = serv_fetch();
				break;
			case CMD_IN_CODA:
				if (!(esegue && esegue_comandi(0))) {
					ret_code |= INKEY_QUEUE;
					break;
				}
			case CMD_ESEGUITO:
/*				if (prompt_curr != P_EDITOR)  */
				ret_code |= INKEY_EXEC;
				print_prompt();
				fflush(stdout);
			}
                }

                if (FD_ISSET(0, &input_set)) {
			*key = read_key();
			ret_code |= INKEY_KEY;
                }

                if (ret_code & INKEY_EXIT) {
                        break;
                }
        }
        return ret_code;
}

static int read_key(void)
{
	char buf[32];

	read(STDIN_FILENO, buf, 1);
	int key = (unsigned char)buf[0];
	if (key == Key_ESC) {
		key = getmeta(buf);
	} else if (key == Key_DEL) {
		key = Key_BS;
	}
	return key;
}

/*
 * Funzione per la cattura delle sequenze di escape per i caratteri  speciali.
 * NOTE: see http://aperiodic.net/phil/archives/Geekery/term-function-keys.html
 *       for the function key escape codes (not fully implemented yet).
 */
static int getmeta(char *buf)
{
	read(STDIN_FILENO, buf+1, 1);

#ifdef HAVE_CTI
	switch(buf[1]) {
	case 27:
		return 0;
	case 79: /* ^[O */
		read(STDIN_FILENO, buf+2, 1);
		switch(buf[2]) {
		case 'A':
		case 'a':
			return(Key_UP);
		case 'B':
		case 'b':
			return(Key_DOWN);
		case 'C':
		case 'c':
			return(Key_RIGHT);
		case 'D':
		case 'd':
			return(Key_LEFT);
		}
		/* ^[OP ^[OQ ^[OR ^[OS for F1-F4 (vt100) */
		if ('P' <= buf[2] && buf[2] <= 'S')
			return Key_F(buf[2] - 'O');
		break;
	case 91: /* ^[[ */
		read(STDIN_FILENO, buf+2, 1);
		switch(buf[2]) {
		case 'A':
		case 'a':
			return Key_UP;
		case 'B':
		case 'b':
			return Key_DOWN;
		case 'C':
		case 'c':
			return Key_RIGHT;
		case 'D':
		case 'd':
			return Key_LEFT;
		case '4':
		case '8':
			read(STDIN_FILENO, buf+3, 1);
		case 'F':
			return Key_END;
		case '7':
			read(STDIN_FILENO, buf+3, 1);
		case 'H':
			return Key_HOME;
		case '1': /* ^[[1 */
		case '2': /* ^[[2 */
		case '3': /* ^[[3 */
			read(STDIN_FILENO, buf+3, 1);
			if (buf[3] == '~') {
				switch(buf[2]) {
				case '1': return Key_HOME;
				case '2': return Key_INS;
				case '3': return Key_DEL;
				}
				return 0;
			}

			read(STDIN_FILENO, buf+4, 1);
			if (buf[4] == '~') {
				int code = (buf[2] - '0')*10 + (buf[3] - '0');
				switch(code) {
				case 11: return Key_F(1);
				case 12: return Key_F(2);
				case 13: return Key_F(3);
				case 14: return Key_F(4);
				case 15: return Key_F(5);
				case 17: return Key_F(6);
				case 18: return Key_F(7);
				case 19: return Key_F(8);
				case 20: return Key_F(9);
				case 21: return Key_F(10);
				case 23: return Key_F(11);
				case 24: return Key_F(12);
				case 25: return Key_F(13);
				case 26: return Key_F(14);
				case 28: return Key_F(15);
				case 29: return Key_F(16);
				case 31: return Key_F(17);
				case 32: return Key_F(18);
				case 33: return Key_F(19);
				case 34: return Key_F(20);
				}
			}
			return 0;
		case '5':
			read(STDIN_FILENO, buf+3, 1);
		case 'V':
		case 'I':
			return Key_PAGEUP;
		case '6':
			read(STDIN_FILENO, buf+3, 1);
		case 'U':
		case 'G':
			return Key_PAGEDOWN;
		}
		break;
	}
#endif

	return META(buf[1]);
}

/* Aspetta un carattere in un elenco di caratteri
 * 0 non è accettabile
 */
int inkey_elenco(const char *elenco)
{
	int i;

	i = 0;
	while(!index(elenco, i) || (i == 0)) {
		i = inkey_sc(0);
		//tolower(i);
	}
        return(i);
}

/*
 * Aspetta un carattere in un elenco di caratteri
 * 0 non è accettabile
 * se si preme \013 o \010
 * torna def.
 */

int inkey_elenco_def(const char *elenco, int def)
{
	int i;

	i = 0;
	while (!index(elenco, i) || (i == 0)) {
		i = inkey_sc(0);
		if ((i == 13) || (i == 10)){
                        i=tolower(def);
                        break;
		}
		//tolower(i);
		printf("%c",i);
	}
        return(i);
}
