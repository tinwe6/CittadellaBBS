/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: socket_citta.c                                                        *
*       gestione dei socket di Cittadella                                     *
******************************************************************************/
#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h> /* for inet_aton() */

#include "socket_citta.h"
#include "cittaserver.h"
#include "comm.h"
#include "compress.h"
#include "macro.h"
#include "memstat.h"
#include "sysconfig.h"
#include "utility.h"
#include "versione.h"

int iniz_socket(int porta)
{
	char nomehost[MAX_NOMEHOST+1];
	const char *ipstr = "127.0.0.1";
	struct sockaddr_in sa;
	struct hostent *hp = NULL;
	struct in_addr ip;
	int s;
  	int opt = 1;

	if (gethostname(nomehost, sizeof nomehost) == -1) {
	        Perror("gethostname");
	} else {
	        hp = gethostbyname(nomehost);
		if (hp == NULL) {
		        citta_logf("gethostbyname: %s", hstrerror(h_errno));
		}
	}

	if (hp == NULL) {
		if (!inet_aton(ipstr, &ip)) {
		        citta_logf("can't parse IP address %s", ipstr);
			exit(1);
		}
		hp = gethostbyaddr((const void *)&ip, sizeof ip, AF_INET);
		if (hp == NULL) {
		  citta_logf("gethostbyaddr: %s", hstrerror(h_errno));
		  exit(1);
		}
	}
	citta_logf("name associated with %s: %s", ipstr, hp->h_name);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = hp->h_addrtype;
	sa.sin_port   = htons(porta);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		Perror("Iniz-socket");
		exit(1);
	}

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt)) < 0) {
		Perror("setsockopt");
		exit(1);
	}

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		Perror("bind");
		close(s);
		exit(1);
	}

	listen(s, 5);

	return s;
}

/*
 * Apre la nuova connessione e restituisce in host l'hostname di provenienza
 * della connessione.
 */
int nuovo_descr(int s, char *host)
{
	char buf2[LBUF];
	int desc;
	struct sessione *punto;
	int i;
	int socket_connessi, socket_attivi;
	socklen_t size;
	struct sockaddr_in sock;
	struct hostent *da_dove;

	if ( (desc = nuova_conn(s)) < 0)
		return desc;

	/* ATTENZIONE : QUESTO NON ANDREBBE CALCOLATO OGNI VOLTA MA
	                MANTENUTO IN VAR GLOBALI AGGIORNATE         */

	socket_connessi = socket_attivi = 0;
	for (punto = lista_sessioni; punto; punto = punto->prossima) {
		socket_connessi++;
		if (punto->logged_in)
			socket_attivi++;
	}

	/* Controllo sul numero massimo di connessioni supportabili */

	if (socket_connessi >= max_num_desc) {
		sprintf(buf2, "%d Massimo numero di connessioni raggiunto.\n",
			ERROR);
		scrivi_a_desc(desc, buf2);
		close(desc);
		return 0;
	} else {
		if (desc > maxdesc)
			maxdesc = desc;
		sprintf(buf2, "%d Cittadella/UX %s -  Welcome!\n", OK,
			SERVER_RELEASE);
		scrivi_a_desc(desc, buf2);
	}

	/* Vanno prese le info relative al sito di provenienza */

	size = sizeof(sock);
	if (getpeername(desc, (struct sockaddr *) &sock, &size) < 0) {
		Perror("getpeername");
		host[0] = '\0';
	} else if (nameserver_lento ||
		   !(da_dove = gethostbyaddr((char *)&sock.sin_addr,
				  sizeof(sock.sin_addr), AF_INET))) {
		if (!nameserver_lento)
			Perror("gethostbyaddr");
		i = sock.sin_addr.s_addr;
		sprintf(host, "%d.%d.%d.%d", (i & 0x000000FF),
			(i & 0x0000FF00) >> 8, (i & 0x00FF0000) >> 16,
			(i & 0xFF000000) >> 24 );
	} else {
		/* SOSTITUIRE '49' CON APPOSITA COSTANTE */
		strncpy(host, da_dove->h_name, 49);
		host[49] = '\0';
	}

	/* Si controlla se il sito e' bandito totalmente */

	/* Se e' previsto uno shutdown avverte e chiude la connessione */

	citta_logf("Nuova connessione da [%s]", host);

	return desc;
}

/*
 * Accetta la nuova connessione e restituisce il file descriptor allo stream
 * dei dati in ingresso
 */
int nuova_conn(int s)
{
	struct sockaddr_in isa;
	socklen_t i;
	int t;

	i = sizeof(isa);
	getsockname(s, (struct sockaddr *)&isa, &i);

	if ((t = accept(s,(struct sockaddr *)&isa, &i)) < 0) {
		Perror("accept");
		return(-1);
	}
	nonblock(t);
	return t;
}

/* Invia al socket desc una stringa di testo. Restituisce -1 in caso di
 * errore, oppure il numero di bytes inviati                              */
int scrivi_a_desc(int desc, char *txt)
{
	int questo_giro, totale;

	totale = strlen(txt);

	questo_giro = write(desc, txt, totale);
	if (questo_giro < 0) {
		Perror("Write su socket");
		return(-1);
	}

	if (questo_giro < totale)
		memmove(txt, txt+questo_giro, totale-questo_giro);
	else /* Svuoto il buffer, altrimenti rispedisce il tutto. */
		*txt = 0;

	return questo_giro;
}

int scrivi_a_desc_len(int desc, char *txt, size_t totale)
{
	int questo_giro;

	questo_giro = write(desc, txt, totale);
	if (questo_giro < 0) {
		Perror("Write su socket");
		return -1;
	}

	if ((size_t)questo_giro < totale) {
		memmove(txt, txt+questo_giro, totale-questo_giro);
                //citta_logf("SADL scritto %ld/%ld", questo_giro, totale);
        }

	return questo_giro;
}

int scrivi_a_desc_iobuf(int desc, struct iobuf *buf)
{
        size_t sent;

        sent = scrivi_a_desc_len(desc, buf->out, buf->olen);
        buf->olen -= sent;

        return sent;
}

int scrivi_a_client(struct sessione *t, char *testo)
{
        size_t len = strlen(testo);

#if ALLOW_COMPRESSION
    if (t->compressing)
            return compress_scrivi_a_client(t, testo, len);
#endif

    return scrivi_a_desc(t->socket_descr, testo);
}


size_t scrivi_a_client_iobuf(struct sessione *t, struct iobuf *buf)
{
        size_t sent;

#if ALLOW_COMPRESSION
        if (t->compressing)
                sent = compress_scrivi_a_client(t, buf->out, buf->olen);
        else
#endif
                sent = scrivi_a_desc_len(t->socket_descr, buf->out, buf->olen);
                //sent = scrivi_a_desc(t->socket_descr, buf->out);
        //citta_logf("SENT %ld", sent);
        if (sent > 0)
                buf->olen -= sent;

        return sent;
}


int scrivi_tutto_a_client(struct sessione *t, struct iobuf *buf)
{
        while (buf->olen) {
		scrivi_a_client_iobuf(t, buf);
        }
        return 0;
}


int elabora_input(struct sessione *t)
{
	int fino_a, questo_giro, inizio, i, k, flag;
        size_t buflen;
	char tmp[MAX_L_INPUT+2];
        char *buf;

	fino_a = 0;
	flag = 0;

        buf = t->iobuf.in;
        inizio = t->iobuf.ilen;

	/* Legge un po' di roba */
	if ( (questo_giro = read(t->socket_descr, buf+inizio+fino_a,
                                 MAX_STRINGA-(inizio+fino_a)-1)) > 0)
		fino_a += questo_giro;
	else if (questo_giro < 0) {
		if (errno != EWOULDBLOCK) {
			Perror("Read1 - ERRORE");
			return(-1);
		}
	} else {
		citta_log("SYSERR: Incontrato EOF in lettura su socket.");
		return -1;
	}

        buflen = inizio + fino_a;

        /* Se la trasmissione e' in binario la sbatte su file */
        if (t->fp) {
                int size, iii;

                if (buflen > t->binsize)
                        size = t->binsize;
                else
                        size = buflen;
                for (iii=0; iii< size; iii++)
                        fputc(buf[iii], t->fp);
                t->binsize -= size;
                if (t->binsize == 0) {
                        fclose(t->fp);
                        t->fp = NULL;
                }
                buflen -= size;
                if (buflen > 0)
                        memmove(buf, buf+size, buflen);
        }

	*(buf + buflen) = '\0';

	/* Se l'input non contiene nessun newline esce senza fare nulla */
	for (i = inizio; !ISNEWL(*(buf + i)); i++)
		if (!*(buf + i)) {
                        t->iobuf.ilen += buflen;
			return(0);
                }

	/* L'input contiene 1 o piu' newline. Elabora e mette in coda */
	tmp[0] = '\0';
	for (i = 0, k = 0; buf[i]; ) {
		if (!ISNEWL(*(buf + i)) &&
		    !(flag = (k >= (MAX_L_INPUT - 2)))) {
			if (*(buf + i) == '\b') {  /* backspace */
				if (k) /* c'e' qualcosa di cancellabile */
					k--;
			} else if (isascii(*(buf + i)) &&
				   isprint(*(buf + i))) {
				*(tmp + k) = *(buf + i);
				k++;
			} /* altrimenti carattere di cui non ci frega nulla */
			i++;
		} else {
			*(tmp + k) = '\0';
			if (tmp[0] != '\0') {      /* SEMPRE VERA ?? */
				metti_in_coda(&t->input, tmp, k);
				tmp[0] = '\0';
			}

			if (flag) /* linea troppo lunga. ignora il resto */
				for ( ; !ISNEWL(*(buf + i)); i++);

			/* trova la fine della linea */
			for ( ; ISNEWL(*(buf + i)); i++)
			        ;

			/* elimina la linea dall'input buffer */
                        memmove(buf, buf+i, buflen-i+1);
                        buflen -= i;

			k = 0;
			i = 0;
		}
	}
        t->iobuf.ilen = buflen;

	return 1;
}

#if defined(SVR4) || defined(LINUX)

void nonblock(int s)
{
	int  flags;
	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0) {
		Perror("Fatal error executing nonblock (cittaserver.c)");
		exit(1);
	}
}

#else

void nonblock(int s)
{
	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		Perror("Fatal error executing nonblock (cittaserver.c)");
		exit(1);
	}
}

#endif

/*
 * Cancella l'idle time della sessione t
 */
#if 0 /* unused */
static inline void clear_idle(struct sessione *t)
{
	t->idle.cicli = 0;
	t->idle.min = 0;
	t->idle.ore = 0;
	t->idle.warning = 0;
}
#endif
/***************************************************************************/
