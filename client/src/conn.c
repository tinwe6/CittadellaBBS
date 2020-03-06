/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
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
* File : conn.c                                                             *
*        Gestione sockets                                                   *
****************************************************************************/
#ifdef LINUX
#define HAS_GETOPT_LONG
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <pwd.h>
#include <ctype.h>
#ifdef HAS_GETOPT_LONG
#include <getopt.h>
#endif

#include "cittaclient.h"
#include "cittacfg.h"
#include "conn.h"
#include "cmd_da_server.h"
#include "decompress.h"
#include "macro.h"
#include "signals.h"
#include "utility.h"
#include "versione.h"

/* initial stretchy buffer length */
#define TMPBUF_LEN 256

#ifndef LOCAL
# ifdef LOGIN_PORT
char remote_key[LBUF];  /* Chiave per entrare come connessione remota */
char remote_host[LBUF]; /* Host dal quale si collega l'utente         */
# endif
#endif

const char *str_conn_closed = "\rConnection closed by foreign host.\n";
static struct coda_testo server_in;

/* buffer in entrata */
static char static_data [ 4096 ];
static struct serv_buffer static_buf = { static_data, 0, 0, sizeof(static_data) };

/* prototipi delle funzioni in questo file */
void parse_opt(int argc, char **argv, char **rcfile, bool *no_rc);
void crea_connessione(char *host, unsigned int port);
int conn_server(char *h, int p);
void timeout(int signum);
void serv_gets(char *strbuf);
void serv_puts(const char *string);
void serv_putf(char *format, ...);
void serv_read(char *buf, int bytes);
void serv_write(const char *buf, int bytes, bool progressbar);
static int serv_buffer_has_input(void);
int elabora_input(void);

bool prendi_da_coda(struct coda_testo *coda, char **dest);
bool prendi_da_coda_t(struct coda_testo *coda, char **dest, int mode);
void metti_in_coda(struct coda_testo *coda, char *txt);

/* variabili globali di conn.c */
int serv_sock;  /* file descr del socket di connessione al server */
int server_input_waiting; /* TRUE se dopo un elabora_input() rimane altro */
                          /* input da server da elaborare                 */

/****************************************************************************
****************************************************************************/
/* Parses command line options.
 * If an alternative rc file is indicated, its path is dinamicaly allocated
 * and put in *rcfile
 */
void parse_opt(int argc, char **argv, char **rcfile, bool *no_rc)
{
#define HELP_MSG \
"Usage: cittaclient [OPTION]...\n"                                       \
"Client application to connect to a Cittadella/UX BBS server.\n\n"       \
"  -c         --compress    enforce compressed transmission\n"           \
"  -C         --nocompress  do not compress the transmission\n"          \
"  -d         --debug       debug mode\n"                                \
"  -F <file>  --rcfile      specify an alternate configuration file\n"   \
"  -H         --help        display this help and exit\n"                \
"  -h <host>  --host        specify host name\n"                         \
"  -l         --localhost   connect to localhost\n"                      \
"  -n         --no-banner   skip the login/logout banners\n"             \
"  -N         --norc        do not read config file and use default settings\n" \
"  -p <port>  --port        specify a port (server must be on the same port)\n" \
"  -u <name>  --username    log in using this user name\n"               \
"  -v         --version     output version information and exit\n"       \
"\n"

#ifdef HAS_GETOPT_LONG
	static struct option long_options[] = {
		{"compress",    0, 0, 'c'},
		{"nocompress",  0, 0, 'C'},
		{"debug",       0, 0, 'd'},
		{"help",        0, 0, 'H'},
		{"host",        1, 0, 'h'},
		{"localhost",   0, 0, 'l'},
		{"norc",        0, 0, 'N'},
		{"no-banner",   0, 0, 'n'},
		{"port",        1, 0, 'p'},
		{"rcfile",      1, 0, 'F'},
		{"username",    1, 0, 'u'},
		{"version",     0, 0, 'v'},
#ifndef LOCAL
# ifdef LOGINPORT
		{"remote",      1, 0, 'r'},
		{"remote_host", 1, 0, 'R'},
# endif
# endif
		{0, 0, 0, 0}
	};
	int option_index = 0;
#endif /* HAS_GETOPT_LONG */

	char buf[LBUF];
        int porta;
        int c;

	*no_rc = false;
	*rcfile = NULL;


        while (1) {
#ifdef HAS_GETOPT_LONG
		c = getopt_long(argc, argv, "cCdF:Hh:lnNp:u:v", long_options,
				&option_index);
#else
		c = getopt(argc, argv, "cCdF:Hh:lnNp:u:v");
#endif /* HAS_GETOPT_LONG */
		if (c == -1)
			break;
                switch (c) {
                case 'c': /* compress */
			cfg_add_opt("COMPRESS \"z\"");
                        break;

                case 'C': /* no compression */
			cfg_add_opt("COMPRESS \"\"");
                        break;

                case 'd': /* debug */
                        DEBUG_MODE = true;
                        break;

                case 'h': /* host name */
                        if (optarg) {
                                sprintf(buf, "HOST \"%s\"", optarg);
				cfg_add_opt(buf);
			}
                        break;

                case 'F': /* rcfile */
                        if (optarg)
                                *rcfile = Strdup(optarg);
                        break;

                case 'H': /* help */
                        printf(HELP_MSG);
                        exit(0);
                        break;

                case 'l': /* localhost */
			cfg_add_opt("HOST \"localhost\"");
                        break;

                case 'n': /* nobanner */
			cfg_add_opt("NO_BANNER");
                        break;

                case 'N': /* norc */
			*no_rc = true;
                        break;

                case 'p': /* port number */
                        if (optarg) {
                                if (!isdigit(optarg[0])) {
                                        fprintf(stderr, _("Porta non valida. Per vedere le opzioni,\n%s --help\n"), argv[0]);
                                        exit(0);
                                } else if ((porta = strtol(optarg, NULL, 10))
					   <= 1024) {
                                        fprintf(stderr, _("Numero di porta non lecito.\n"));
                                        exit(0);
                                }
				sprintf(buf, "PORT %d", porta);
				cfg_add_opt(buf);
                        }
                        break;

                case 'u': /* username */
			sprintf(buf, "USER \"%s\"", optarg);
			cfg_add_opt(buf);
			sprintf(buf, "PASSWORD \"\"");
			cfg_add_opt(buf);
                        break;

                case 'v': /* cittaclient version */
                        printf("cittaclient version %s - Client for "
                               "Cittadella/UX BBS\n", CLIENT_RELEASE);
                        exit(0);
                        break;

#ifndef LOCAL
# ifdef LOGINPORT
                case 'r': /* Remote connection */
                        if (optarg)
                                strcpy(remote_key, optarg);
                        break;

		 case 'R': /* Remote host */
                        if (optarg)
                                strcpy(remote_host, optarg);
                        break;
# endif
#endif
                default:
                        exit(EXIT_FAILURE);
                }
	}
}

void crea_connessione(char *host, unsigned int port)
{
        serv_sock = conn_server(host, port);
}

int conn_server(char *host, int porta)
{
        struct hostent *hp;
        struct sockaddr_in sa;
        int s;

        printf(_("Connessione al server.... \n"));
        bzero(&sa, sizeof(struct sockaddr_in));

	/* printf("gethostbyname\"%s\".\n", host); */
        hp = gethostbyname(host);
        if (hp == NULL) {
                perror("gethostbyaddr");
                exit(1);
        }
        sa.sin_family = AF_INET;
        sa.sin_port   = htons(porta);
        sa.sin_addr   = *(struct in_addr *)hp->h_addr;

        if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
                perror("conn_server");
                exit(1);
        }

        signal(SIGALRM, timeout);
        alarm(30);

        if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
                printf(_("Impossibile aprire la connessione a %s %d\n"),
                       host, porta);
                exit(1);
        }

        alarm(0);
        signal(SIGALRM, SIG_IGN);

        return(s);
}

/* Write connection timeout message and exit the client */
/* Note: the parameter signum is not used               */
void timeout(int signum)
{
        IGNORE_UNUSED_PARAMETER(signum);
        printf("\rConnection timed out.\n");
        exit(1);
}

/*****************************************************************************/
/*****************************************************************************/
/*
 * Legge una stringa dal server.
 * Continua a leggere tramite elabora_input() finche'
 * non legge qualcosa di diverso da un comando
 */
void serv_gets(char *strbuf)
{
	char *buf;

        if (!prendi_da_coda(&server_in, &buf)) {
	        /* for ( ; elabora_input() & (NO_CMD|CMD_BINARY); ); */
                for ( ; elabora_input() != NO_CMD; );
                prendi_da_coda(&server_in, &buf);
        }
        strncpy(strbuf, buf, LBUF);
        Free(buf);
        /*
	if (!prendi_da_coda(&server_in, &buf)) {
		while ( (elabora_input() & (CMD_ESEGUITO | CMD_IN_CODA)))
                        prendi_da_coda(&server_in, &buf);
	}
	strncpy(strbuf, buf, LBUF);
	Free(buf);
        */
}

/*
 * Legge in binario dal server
 */
void serv_read(char *buf, int bytes)
{
        int len, rlen;

        len = 0;
        while (len < bytes) {
                rlen = read(serv_sock, &buf[len], bytes-len);
                if (rlen < 1) {
                        printf("%s",str_conn_closed);
                        exit(1);
                }
                len += rlen;
        }
}

/*
 * Manda una stringa al server
 */
void serv_puts(const char *string)
{
        while (controlla_server())
                elabora_input();

	serv_write(string, strlen(string), false);
	serv_write("\n", 1, false);
	if (DEBUG_MODE)
                printf("&C [%d] %s\n", (int)strlen(string), string);

	send_keepalive = false;
}

/*
 * Manda una stringa formattata al server
 */
void serv_putf(char *format, ...)
{
	va_list ap;
	char *str;
	int len = TMPBUF_LEN;
	int ret, ok = 0;

	/* Uses vsnprintf(), conforms to BSD, ISOC9X, UNIX98 standards */
	va_start(ap, format);
	CREATE(str, char, len, 0);
	while (!ok) {
		ret = vsnprintf(str, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(str, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	serv_puts(str);
	va_end(ap);
	Free(str);
}

/*
 * Trasmette in binario al server
 * Se progressbar==TRUE mostra una progress bar.
 */
void serv_write(const char *buf, int bytes, bool progressbar)
{
        int ret, prog_step, j;
        int progress, total;

        if (progressbar) {
                progress = 1;
                total = bytes;
                prog_step = total / 10;
                while (bytes > 0) {
                        if (prog_step > bytes)
                                prog_step = bytes;
                        if ( (ret = write(serv_sock, buf, prog_step)) > 0) {
                                buf += ret;
                                bytes -= ret;
				/* if (progressbar) *//* TODO eliminare */
				/* printf("Wrote %d/%d bytes\n", total-bytes, total); */
                        } else {
                                printf("%s",str_conn_closed);
                                exit(1);
                        }
                        if ((progressbar) && ((total-bytes) > prog_step*progress)) {
                                delnchar(11);
                                for (j = 0; j < progress-1; j++)
                                        putchar('-');
                                putchar('>');
                                for (j++; j < 10; j++)
                                        putchar(' ');
                                putchar(']');
                                progress++;
                                fflush(stdout);
                        }
                        fflush(NULL);
                }
        } else while (bytes > 0) {
                        if ( (ret = write(serv_sock, buf, bytes)) > 0) {
                                buf += ret;
                                bytes -= ret;
                        } else {
                                printf("%s",str_conn_closed);
                                exit(1);
                        }
                }
}

/*
 * Legge un carattere dal buffer di input del server e lo restituisce.
 * Se il buffer di input e' vuoto, lo riempie leggendo da socket.
 */
int serv_getc_r(struct serv_buffer * buf)
{
        int got;

        if (buf->start < buf->end)
                return buf->data[buf->start++];

        buf->start = buf->end = 0;
#ifdef EINTR
        while (( (got = read(serv_sock, buf->data, buf->size)) < 0) &&
               (errno == EINTR))
                ;
#else
        got = read(serv_sock, buf->data, buf->size);
#endif
        if (got > 0) {
                buf->end += got;
                return buf->data[buf->start++];
        }
        printf("%s",str_conn_closed);
        exit(1);
}

/*
 * Legge un carattere dal buffer di input del server e lo restituisce.
 * Se il buffer di input e' vuoto, lo riempie leggendo da socket.
 */
int serv_getc(void)
{
        return ( client_cfg.compressing
                 ? decompress_serv_getc(&static_buf)
                 : serv_getc_r(&static_buf) );
}

static int serv_buffer_has_input(void)
{
        return (static_buf.start < static_buf.end);
}

/*
 * Prende una riga di input dal socket, e se e' un comando dal server da
 * eseguire subito lo processa. Prende al piu' max caratteri.
 * Restituisce CMD_ESEGUITO se ha eseguito un comando,
 *             CMD_IN_CODA  se e' un comando da eseguire dopo,
 *             NO_CMD       se non e' un comando dal server.
 */
int elabora_input(void)
{
        char *buf;
	int len, bufsize = LBUF;
	char ch;

        CREATE(buf, char, bufsize, 0);
	len = 0;

        do {
                buf[len++] = ch = (char) serv_getc();
		if (len == bufsize) {
			bufsize *= 2;
			RECREATE(buf, char, bufsize);
		}
        } while((ch != 10) && (ch != 13) && (ch != 0));
        buf[len - 1] = 0;

	send_keepalive = false;

	if (DEBUG_MODE)
        	printf("&S %s\n", buf);

        /* Se c'e' altro input in coda, segnala in una variabile globale */
        server_input_waiting = serv_buffer_has_input();

        if (*buf == '8')                      /* Comandi da eseguire subito */
		return esegue_urgenti(buf);
        else if (*buf == '9') {               /* Comandi da eseguire dopo   */
                metti_in_coda(&comandi, buf);
		return CMD_IN_CODA;
        }
        /*
        else if (*buf == '3') {
                metti_in_coda(&server_in, buf);
                return CMD_BINARY;
        }
        */

        /* Se non e` un comando la stringa resta tale e quale */
	metti_in_coda(&server_in, buf);
        return NO_CMD;
}

#if 0
int elabora_input(char *str, const int max)
{
        int ch, len, rlen;
        char buf[2];

        len = 0;
        strcpy(str, "");
        do {
                rlen = read(serv_sock, &buf[0], 1);
                ch = (int) buf[0];
                if (rlen < 1) {
                        ch = 0;
                        printf(str_conn_closed);
                        exit(1);
                }
                str[len++] = ch;
        } while((ch != 10) && (ch != 13) && (ch != 0) && (len < 255));
        str[len - 1] = 0;

	if (DEBUG_MODE)
	        printf("&S %s\n", str);

	/* Comandi da eseguire subito */
        if (str[0] == '8') {
		return esegue_urgenti(str);
	/* Comandi da eseguire dopo   */
        } else if (str[0] == '9') {
                metti_in_coda(&comandi, str);
		return CMD_IN_CODA;
        }

        /* Se non e` un comando la stringa resta tale e quale */
        return NO_CMD;
}
#endif

/*****************************************************************************/
/*
 * Prende dalla coda dei comandi da sever non urgenti il primo comando
 * arrivato e lo elimina dalla coda. La stringa restituita in dest va
 * liberata con free().
 */
bool prendi_da_coda(struct coda_testo *coda, char **dest)
{
        struct blocco_testo *tmp;

        /* La coda e' vuota? */
        if (!coda->partenza) {
		*dest = NULL;
                return false;
	}

        tmp = coda->partenza;
        *dest = coda->partenza->testo;
        coda->partenza = coda->partenza->prossimo;
        Free(tmp);

        return true;
}

/*
 * Prende da coda i comandi di un tipo particolare.
 * mode = 0: tutto; 1: Chat;
 */
bool prendi_da_coda_t(struct coda_testo *coda, char **dest, int mode)
{
        struct blocco_testo *tmp, *prev;

	*dest = NULL;
        /* La coda e' vuota? */
        if (!coda->partenza)
                return false;

	if (mode == 0) {
		tmp = coda->partenza;
		*dest = coda->partenza->testo;
		coda->partenza = coda->partenza->prossimo;
		Free(tmp);
		return true;
	}

	prev = NULL;
	for (tmp = coda->partenza; tmp; prev = tmp, tmp = tmp->prossimo) {
		switch(mode) {
		 case 1:
			if ((tmp->testo[1] == 0)
			    && ((tmp->testo[2] == 4) || (tmp->testo[2] == 5)
				|| (tmp->testo[2] == 6))) {
				if (prev)
					prev->prossimo = tmp->prossimo;
				else
					coda->partenza = tmp->prossimo;
				*dest = tmp->testo;
				Free(tmp);
				return true;
			}
		}
        }
        return false;
}

/*
 * Aggiunge la stringa txt in fondo alla coda dei cmd da server non urgenti.
 * La stringa fornita in txt deve essere stata allocata dinamicamente,
 * in quanto verra` successivamente free()ata.
 */
void metti_in_coda(struct coda_testo *coda, char *txt)
{
        struct blocco_testo *nuovo;

        CREATE(nuovo, struct blocco_testo, 1, 0);
	nuovo->testo = txt;

        /* La coda e' vuota? */
        if (!coda->partenza) {
                nuovo->prossimo = NULL;
                coda->partenza = coda->termine = nuovo;
        } else {
                coda->termine->prossimo = nuovo;
                coda->termine = nuovo;
                nuovo->prossimo = NULL;
        }
}
