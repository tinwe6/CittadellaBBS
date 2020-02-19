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
#include "cti.h"
#include "edit.h"
#include "inkey.h"
#include "messaggi.h"
#include "prompt.h"
#include "signals.h"
#include "cterminfo.h"
#include "editor.h"

/* Prototipi funzioni in questo file */
int inkey_sc(int esegue);
int inkey_pager(int esegue, char *str, int *c);
int inkey_elenco(const char *elenco);
int inkey_elenco_def(const char *elenco, int def);
static int getmeta(char *buf);

/*****************************************************************************/
/*
 * Prende un carattere in input dall'utente.
 * Mentre aspetta che l'utente prema un tasto, ascolta ed elabora
 * i comandi provenienti dal server.
 * Se (esegue) allora elabora tutti i comandi, altrimenti solo quelli
 * urgenti.
 */
int inkey_sc(int esegue)
{
        int a = 0, b;
        fd_set input_set;
        struct timeval tv;
        char buf[LBUF];

        fflush(stdout);

        do {
                /* Controlliamo se il client ha input */
                do {
                        FD_ZERO(&input_set);
                        FD_SET(0, &input_set);         /* Input dall'utente */
                        FD_SET(serv_sock, &input_set); /* Input dal server  */
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
			b = select(serv_sock + 1, &input_set, NULL, NULL, &tv);
			if (b == -1) {
                                if (errno == EINTR)
                                        esegui_segnali();
                                else {
                                        perror("Select");
                                        exit(1);
                                }
                        }
		} while ((b <= 0));

                if (FD_ISSET(serv_sock, &input_set)) {
                        /* Ho input dal server */
#ifdef HAVE_CTI
			if (prompt_curr == P_EDITOR) {
				cti_scroll_reg(0, Editor_Pos - 1);
				cti_mv(0, Editor_Pos - 1);
				scroll_up();
				fflush(stdout);
			}
#endif
                        do
			if ((elabora_input() == CMD_ESEGUITO)
			    || (esegue && esegue_comandi(0))) {
				if (post_timeout)
					return 0;
				if (prompt_curr != P_EDITOR)
					print_prompt();
#ifdef HAVE_CTI
				else {
					if (Editor_Win)
						cti_scroll_reg(Editor_Pos+1, NRIGHE-1);
					else
						cti_scroll_reg(0, NRIGHE-1);
					cti_mv(Editor_Hcurs, Editor_Vcurs);
				}
#endif
				fflush(stdout);
			}
                        while (server_input_waiting);
                }

                if (FD_ISSET(0, &input_set)) {
                        /* Ho input dall'utente */
                        read(0, buf, 1);
                        a = (unsigned char)buf[0];
			if (a == Key_ESC)
				a = getmeta(buf);
			else if (a == Key_DEL)
				a = Key_BS;
                }

        } while (a == 0);

        return a;
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
 * Se e' stato premuto un tasto, questo viene memorizzato in *c,
 * se c'e' input da server si trova nella stringa *str.
 * Puo' capitare che ci sia input contemporaneo, nel qual caso i due bit
 * sono settati e i risultati si trovano in str e *c.
 */
int inkey_pager(int esegue, char *str, int *c)
{
        int b, ret = 0;
	char buf[LBUF];
        fd_set input_set;
        struct timeval tv;

        fflush(stdout);
	*c = 0;

        do {
                /* Controlliamo se il client ha input */
                do {
                        FD_ZERO(&input_set);
                        FD_SET(0, &input_set);         /* Input dall'utente */
                        FD_SET(serv_sock, &input_set); /* Input dal server  */
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
			b = select(serv_sock + 1, &input_set, NULL, NULL, &tv);
			if (b == -1) {
                                if (errno == EINTR)
                                        esegui_segnali();
                                else {
                                        perror("Select");
                                        exit(1);
                                }
                        }
		} while (b <= 0);

                if (FD_ISSET(serv_sock, &input_set)) {
                        /* Ho input dal server */
			switch (elabora_input()) {
			case NO_CMD:
				ret |= INKEY_SERVER;
				break;
			case CMD_IN_CODA:
				if (!(esegue && esegue_comandi(0))) {
					ret |= INKEY_QUEUE;
					break;
				}
			case CMD_ESEGUITO:
/*				if (prompt_curr != P_EDITOR)  */
				ret |= INKEY_EXEC;
				print_prompt();
				fflush(stdout);
			}
                }
		
                if (FD_ISSET(0, &input_set)) {
                        /* Ho input dall'utente */
                        read(0, buf, 1);
                        *c = (unsigned char)buf[0];
			if (*c == Key_ESC)
				*c = getmeta(buf);
			else if (*c == Key_DEL)
				*c = Key_BS;
			ret |= INKEY_KEY;
                }

        } while (!(ret & INKEY_EXIT));

        return ret;
}

/*
 * Funzione per la cattura delle sequenze di escape per i caratteri
 * speciali.
 */
static int getmeta(char *buf)
{
	read(STDIN_FILENO, buf+1, 1);
#ifdef HAVE_CTI
	switch(buf[1]) {
	case 27:
		return 0;
	case 79: /* Alt-O */
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
		if (buf[2] <= 'S' && buf[2] >= 'P')
			return Key_F(buf[2] - 79);
		break;
	case 91: /* Alt-[ */
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
		case '[':
			read(STDIN_FILENO, buf+3, 1);
			if (buf[3] >= 'A' && buf[3] <= 'E')
				return Key_F(buf[3] - 64);
			break;
		case '1':
			read(STDIN_FILENO, buf+3, 1);
			if (buf[3] >= '1' && buf[3] <= '5') {
				read(STDIN_FILENO, buf+3, 1);
				return Key_F(buf[3] - 48);
			}
			if (buf[3] >= '7' && buf[3] <= '9') {
				read(STDIN_FILENO, buf+3, 1);
				return Key_F(buf[3] - 49);
			}
			if (buf[3] == 126)
				return Key_HOME;
			break;
		case '2':
			read(STDIN_FILENO, buf+3, 1);
			if (buf[3] == 126)
				return Key_INS;
			break;
		case '3':
			read(STDIN_FILENO, buf+3, 1);
			if (buf[3] == 126)
				return Key_DEL;
			break;
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
#if 0
	if (key_npage && !strncmp(buf, key_npage, strlen(key_npage)))
		return Key_PAGEDOWN;
	
	if (key_ppage && !strncmp(buf, key_ppage, strlen(key_ppage)))
		return Key_PAGEUP;
		
	if (key_up && !strncmp(buf, key_up, strlen(key_up)))
		return Key_UP;
		
	if (key_down && !strncmp(buf, key_down, strlen(key_down)))
		return Key_DOWN;
		
	if (key_right && !strncmp(buf, key_right, strlen(key_right)))
		return Key_RIGHT;
		
	if (key_left && !strncmp(buf, key_left, strlen(key_left)))
		return Key_LEFT;
		
	if (key_home && !strncmp(buf, key_home, strlen(key_home)))
		return Key_HOME;
	
	if (key_end && !strncmp(buf, key_end, strlen(key_end)))
		return Key_END;
		
	if (key_f1 && !strncmp(buf, key_f1, strlen(key_f1)))
		return Key_F(1);

	if (key_f2 && !strncmp(buf, key_f2, strlen(key_f2)))
		return Key_F(2);
		
	if (key_f3 && !strncmp(buf, key_f3, strlen(key_f3)))
		return Key_F(3);

	if (key_f4 && !strncmp(buf, key_f4, strlen(key_f4)))
		return Key_F(4);
		
	if (key_f5 && !strncmp(buf, key_f5, strlen(key_f5)))
		return Key_F(5);
		
	if (key_f6 && !strncmp(buf, key_f6, strlen(key_f6)))
		return Key_F(6);
	
	if (key_f7 && !strncmp(buf, key_f7, strlen(key_f7)))
		return Key_F(7);
		
	if (key_f8 && !strncmp(buf, key_f8, strlen(key_f8)))
		return Key_F(8);
		
	if (key_ic && !strncmp(buf, key_ic, strlen(key_ic)))
		return Key_INS;

#endif
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
