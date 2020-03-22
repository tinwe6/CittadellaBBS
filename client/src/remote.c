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
* File : remote.c                                                           *
*        funzioni specifiche per il client remoto                           *
****************************************************************************/
#ifndef LOCAL

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
/* #include <utmp.h> : Uso utmpx.h invece di utmp.h perche' le info */
/* dicono che e' equivalente ma standard e quindi piu' portabile.   */
/* Verificare che e' la scelta giusta.                              */
#include "text.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "cml.h"
#include "macro.h"
#include "remote.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef MACOSX
#ifndef UTMPLIB
#define UTMPLIB
#endif
#endif

#ifdef UTMPLIB
#include <utmp.h>
#else
#include <utmpx.h>
#endif

/* ping.science.unitn.it */
char BBS_EDITOR[] = "/home/bbs/Cittadella/citta/client/bin/BBSeditor";

/***************************************************************************
***************************************************************************/
#ifdef LOGIN_PORT
/* Gets the remote hostname associated to the address ipaddr and returns it
 * as a malloc'ed string. The resulting string is empty if the hostname was
 * not found. Must be free()'d                                             */
char * get_hostname(char *ip_addr)
{
	struct sockaddr_in addr;
	char * hostname = (char *)malloc(NI_MAXHOST*sizeof(char));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip_addr, &(addr.sin_addr));
	if (getnameinfo((struct sockaddr *)&addr, sizeof(addr),
			hostname, NI_MAXHOST, NULL, 0, 0) != 0) {
		hostname[0] = 0;
	}
	printf("Remote host name '%s'\n", hostname);
	return hostname;
}

#else /* LOGIN_PORT */

void find_remote_host(char *rhost)
{
#ifdef UTMPLIB
        struct utmp *utente, ut_line;
#else
        struct utmpx *utente, ut_line;
#endif
        /* pid_t cittapid; */
        char *ttyn;

	utente = NULL;
        rhost[0] = 0;
        /* cittapid = getpid(); */
        ttyn = ttyname(STDOUT_FILENO);
        if (ttyn != NULL) {
                strcpy(ut_line.ut_line, &ttyn[5]);
#ifdef UTMPLIB
#ifndef MACOSX
                setutent();
                utente = getutline(&ut_line);
#endif
#else
                setutxent();
                utente = getutxline(&ut_line);
#endif
                if (utente)
                        strcpy(rhost,utente->ut_host);
#ifdef UTMPLIB
#ifndef MACOSX
                endutent();
#endif
#else
                endutxent();
#endif
	}
}

/* Questa e' una versione brutta e lenta del comando precedente, ma */
/* che funziona anche se utmpx.h fa pasticci.                       */
void old_find_remote_host(char *rhost)
{
        /* questo va in ident_client e viene eseguito solo se il client */
        /* e' locale.                                                   */
        FILE *syspipe;
        char *ttyn;
        char ttybuf[256];
        int ii;
        char *token;

        /* determina il tty associato al client e 'greppa' l'host */
        if (isatty(STDOUT_FILENO)) {
                ttyn = ttyname(STDOUT_FILENO);
                if (ttyn != NULL) {
                        sprintf(ttybuf, "who | grep %s\n", &ttyn[5]);
                        if ((syspipe = popen(ttybuf, "r"))) {
                                ii = 0;
                                while (ttybuf[ii] = fgetc(syspipe),
                                       ttybuf[ii++] != EOF);
                                ttybuf[ii - 1] = 0;
                                pclose(syspipe);
                                /* Estrae l'host */
                                strtok(ttybuf, " ");
                                for(ii = 0; ii < 4; ii++, strtok(NULL, " "));
                                token = strtok(NULL, " ");
                                if (token) {
                                        strcpy(rhost, &token[1]);
                                        rhost[strlen(rhost) - 2] = 0;
                                } else
                                        rhost[0] = 0;
                        }
#ifdef DEBUG
                        else
                                printf("popen failed.\n");
#endif
                }
#ifdef DEBUG
                else
                        printf("\nNon riesco a determinare il ttyname "
                               "associato a cittaclient.\n");
#endif
        }
}

#endif /* LOGIN_PORT */

void msg_dump(struct text *txt, int autolog)
{
        /* parameters only used by the local counterpart of this function. */
        IGNORE_UNUSED_PARAMETER(txt);
        IGNORE_UNUSED_PARAMETER(autolog);

	cml_print(_(
		    " *** Questa opzione &egrave; disponibile solo con il client locale.\n"
		    ));
}

#endif /* remote.c */
