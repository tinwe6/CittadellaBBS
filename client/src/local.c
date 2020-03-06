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
* File : local.c                                                            *
*        funzioni specifiche per il client locale                           *
****************************************************************************/
#ifdef LOCAL

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "cittacfg.h"
#include "cittaclient.h"
#include "conn.h"
#include "local.h"
#include "terminale.h"
#include "user_flags.h"
#include "utility.h"

/*Prototipi delle funzioni in questo file */
void subshell (void);
void finger(void);
void msg_dump(struct text *txt, int autolog);

/****************************************************************************/
/****************************************************************************/
/* Apre una subshell */
void subshell (void)
{
	int shell_pid, shell_exit, b, i;
	char buf[LBUF];

	fflush(stdout);
	serv_puts("LSSH 1");
	serv_gets(buf);
	if (buf[0] != '2') {
		printf(_("Non puoi entrare nella subshell.\n"));
		return;
	}
	shell_pid = fork();
	if (shell_pid == 0){
		reset_term();
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		execlp(SHELL, SHELL, NULL);
		exit(0);
	} else {
		if (shell_pid>0) {
			i=0;
			do {
				/* manda segnali keepalive al server */
				us_sleep(100000); /* 1/10 di secondo */
				i++;
				if (i == 600) {   /* Un minuto circa */
					serv_puts("NOOP");
					i = 0;
				}
				shell_exit = 0;
				b = waitpid(shell_pid, &shell_exit,
					    WNOHANG);
			} while((b != shell_pid) && (b >= 0));
		}
		shell_pid = (-1);
		term_save();
		term_mode();
		serv_puts("LSSH 0");
		serv_gets(buf);
		if (buf[0] != '2')
			printf(_("*** Problemi con il server.\n"));
	}
}

/* TODO Do we still need a finger command? */
/* Finger a un host */
void finger(void)
{
	int s;
	unsigned short porta = 79;
	struct sockaddr_in address;
	struct hostent *hp;
	char datainput[LBUF];
	char host[40];
	char user[MAXLEN_UTNAME];

	new_str_m(_("Nome dell'host : "), host, 39);
	new_str_m(_("Nome dell'utente : "), user, MAXLEN_UTNAME - 1);
	printf("\n");

	/* Creazione del socket */

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror("socket()");
		return;
	}

	/* Risoluzione dell'ip numerico del server */

	if ((hp = gethostbyname(host)) == NULL) {
		perror("gethostbyname()");
		return;
	}

	/* Inizializzazione della struttura sockaddr_in */

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(porta);
	memcpy(&sa.sin_addr, hp->h_addr_list[0], hp->h_length);

	/* Connessione */

	if (connect(s, (struct sockaddr *) &address, sizeof(address)) == -1) {
		perror("connect()");
		return;
	}

	/* Invio e ricezione dati finger */

	write(s, user, strlen(user));
	write(s, "\n", 1);

	while (read(s, datainput, 80) != 0) {
		printf("%s", datainput);
		*datainput = 0;
	}

	putchar('\n');

	/* Chiusura del socket */

	close(s);
}

/*
 * Appende il messaggio in fondo al file di dump.
 */
void msg_dump(struct text *txt, int autolog)
{
	FILE *fp;
	char *str, *filename;

	if (txt == NULL) {
		printf(_("Messaggio vuoto.\n"));
		return;
	}

	if (rflags & RF_MAIL)
		filename = LOG_FILE_MAIL ? LOG_FILE_MAIL : DUMP_FILE;
	else
		filename = DUMP_FILE;
	if ((fp = fopen(filename, "a+")) == NULL) {
		printf(_("Errore apertura file [%s].\n"), filename);
		return;
	}
	txt_rewind(txt);
	while ((str = txt_get(txt)))
		fprintf(fp, "%s\n", str);
	fprintf(fp, "\n                                    o---O---o\n\n");
	fclose(fp);
	if (!autolog)
		printf(_("*** Messaggio salvato.\n"));
}

#endif /* local.c */
