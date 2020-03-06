/*
 *  Copyright (C) 2003-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                            G. Garberoglio                   *
*                                                                             *
* File: imap4.c                                                               *
*       RFC 2060 implementation of IMAP4rev1 server for Cittadella/UX         *
******************************************************************************/

#include "config.h"

#ifdef USE_IMAP4

#define IMAP4_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "imap4.h"

#include "cittaserver.h"
#include "cml2html.h"
#include "extract.h"
#include "macro.h"
#include "mail.h"
#include "march.h"
#include "memstat.h"
#include "rooms.h"
#include "socket_citta.h"
#include "sysconfig.h"
#include "ut_rooms.h"
#include "utente.h"
#include "utility.h"
#include "versione.h"

#ifdef USE_STRING_TEXT
# include "argz.h"
#endif

#ifdef USE_CACHE_POST
#include "cache_post.h"
#endif

struct sessione *sessioni_imap4;
int imap4_maxdesc;
int imap4_max_num_desc = 5;/* Massimo numero di connessioni IMAP4 simultanee */

char imap4_version[] = "IMAP4rev1";

/* Stati della sessione */
#define NON_AUTH         1
#define AUTHENTICATED    2
#define SELECT           4
#define LOGOUT           8
#define SELECTED     ( AUTHENTICATED | SELECT )
#define ANY          ( NON_AUTH | AUTHENTICATED | SELECT | LOGOUT )

/* Prototipi funzioni in questo file: */
const char * get_atom(char *atom, const char *str);
const char * get_quoted(char *atom, const char *str);
const char * get_astring(char *atom, const char *str);
int parse_command(char *cmd, char *tag, char **arg);
int is_CHAR(char c);
int is_ATOM_CHAR(char c);
int is_TEXT_CHAR(char c);
int is_CTL(char c);
int is_wildcard(char c);
int is_quoted_special(char c);
int is_atom_special(char c);

int imap4_nuovo_descr(int s);
void imap4_chiudi_socket(struct sessione *h);
int imap4_elabora_input(struct sessione *t);
void imap4_interprete_comandi(struct sessione *p, char *command);
void imap4_kill_user(char *ut);

static void imap4_printf(struct sessione *p, int tag, int type, int stat,
			 const char *format, ...);/* CHK_FORMAT_5; */
static struct sessione * imap4_collegato(char *ut);

static void imap4_capability(struct sessione *p);
static void imap4_noop(struct sessione *p);
static void imap4_logout(struct sessione *p);
static void imap4_authenticate(struct sessione *p);
static void imap4_login(struct sessione *p, char *nome);
static void imap4_examine(struct sessione *p, char *name);
static void imap4_select(struct sessione *p, char *name);

static void imap4_maildrop(struct sessione *p);
static void imap4_free_maildrop(struct sessione *p);
/* static void imap4_printf(struct sessione *p, const char *format, ...) CHK_FORMAT_2; */

/* Argomenti dei comandi */
enum {
	NO_ARG,
	OPT_ARG,
	PASS_ARG
};

/* Lista dei comandi e strutture necessarie per la sua memorizzazione */
typedef union {
	void (*no_arg) (struct sessione *t);
	void (*pass_arg) (struct sessione *t, char *buf);
} cmd_ptr;

struct comando {
	char token[13];/* Nome del comando                                  */
	size_t len;    /* Lunghezza del comando                             */
	cmd_ptr cmd;   /* Puntatore alla funzione che implementa il comando */
	char argnum;   /* Numero argomenti della funzione                   */
	int  stato;    /* Stato attuale necessario per eseguire il comando  */
};

#define LTOKEN(I)   (imap4_cmd_list[(I)].len)
#define ARGSTART(I) (imap4_cmd_list[(I)].len + 1)

static const struct comando imap4_cmd_list[] = {
/* IMAP4 Commands - Any state */
{"CAPABILITY",   10, { (void *)&imap4_capability   }, NO_ARG,   ANY   },
{"NOOP",          4, { (void *)&imap4_noop         }, NO_ARG,   ANY  },
{"LOGOUT",        6, { (void *)&imap4_logout       }, NO_ARG,   ANY  },
/* IMAP4 Commands - Non-Authenticated state */
{"AUTHENTICATE", 12, { (void *)&imap4_authenticate }, PASS_ARG, NON_AUTH   },
{"LOGIN",         5, { (void *)&imap4_login        }, PASS_ARG, NON_AUTH   },
/* IMAP4 Commands - Authenticated state */
{"SELECT",        6, { (void *)&imap4_select       }, PASS_ARG, AUTHENTICATED},
{"EXAMINE",       7, { (void *)&imap4_examine      }, PASS_ARG, AUTHENTICATED},
#if 0
{"CREATE",        6, { (void *)&imap4_create       }, PASS_ARG, AUTHENTICATED},
{"DELETE",        6, { (void *)&imap4_delete       }, PASS_ARG, AUTHENTICATED},
{"RENAME",        6, { (void *)&imap4_rename       }, PASS_ARG, AUTHENTICATED},
{"SUBSCRIBE",     9, { (void *)&imap4_subscribe    }, PASS_ARG, AUTHENTICATED},
{"UNSUBSCRIBE",  11, {(void *)&imap4_unsubscribe   }, PASS_ARG, AUTHENTICATED},
{"LIST",          4, {(void *)&imap4_list          }, PASS_ARG, AUTHENTICATED},
{"LSUB",          4, {(void *)&imap4_lsub          }, PASS_ARG, AUTHENTICATED},
{"STATUS",        6, {(void *)&imap4_status        }, PASS_ARG, AUTHENTICATED},
{"APPEND",        6, {(void *)&imap4_append        }, PASS_ARG, AUTHENTICATED},
/* IMAP4 Commands - Selected state */
{"CHECK",         5, {(void *)&imap4_check         }, NO_ARG,   SELECTED  },
{"CLOSE",         5, {(void *)&imap4_close         }, NO_ARG,   SELECTED  },
{"EXPUNGE",       7, {(void *)&imap4_expunge       }, NO_ARG,   SELECTED  },
{"SEARCH",        6, {(void *)&imap4_search        }, PASS_ARG, SELECTED  },
{"FETCH",         5, {(void *)&imap4_fetch         }, PASS_ARG, SELECTED  },
{"COPY",          4, {(void *)&imap4_copy          }, PASS_ARG, SELECTED  },
{"UID",           3, {(void *)&imap4_uid           }, PASS_ARG, SELECTED  },
#endif
/* IMAP4 Commands - Logout state */
{     "", 0, { (void *)   NULL    }, 0       , 0             }};

/* IMAP4 replies */
const char imap4_ok[] = "OK";
const char imap4_no[] = "NO";
const char imap4_bad[] = "BAD";

/* le due code per il wprintf */
#define CODA_HEADER 0
#define CODA_BODY   1

/****************************************************************************/

#define TAG      TRUE
#define NO_TAG   FALSE

#define RESP_OK         0
#define RESP_NO         1
#define RESP_BAD        2
#define RESP_PREAUTH    3  /* Always untagged */
#define RESP_BYE        4  /* Always untagged */
#define RESP_CAPABILITY 5  /* Always untagged */

char *response[] = { "OK",
		     "NO",
		     "BAD",
		     "PREAUTH",
		     "BYE",
		     "CAPABILITY"
};

#define NO_STATUS             0
#define STATUS_ALERT          1
#define STATUS_NEWNAME        2
#define STATUS_PARSE          3
#define STATUS_PERMANENTFLAGS 4
#define STATUS_READ_ONLY      5
#define STATUS_READ_WRITE     6
#define STATUS_TRYCREATE      7
#define STATUS_UIDVALIDITY    8
#define STATUS_UNSEEN         9

char *status[] = { "",
		   "ALERT",
		   "NEWNAME",
		   "PARSE",
		   "PERMANENTFLAGS",
		   "READ_ONLY",
		   "READ_WRITE",
		   "TRYCREATE",
		   "UIDVALIDITY",
		   "UNSEEN"
};

static void imap4_printf(struct sessione *p, int tag, int type, int stat,
			 const char *format, ...)
{
	struct coda_testo *coda;
	struct blocco_testo *nuovo;
	va_list ap;
	char fmt[3*LBUF];
	size_t pos;
#ifdef USE_MEM_STAT
	char *tmp;
	int size;
#endif

	/* Costruisce la stringa di formattazione del messaggio */
	if (!tag) {
		strcpy(fmt, "* ");
		pos = 2;
	} else
		pos = sprintf(fmt, "%s ", p->tag);

	pos += sprintf(fmt+pos, "%s ", response[type]);

	if (stat)
		pos += sprintf(fmt+pos, "[%s] ", status[stat]);

	pos += sprintf(fmt+pos, "%s", format);

	/* Costruisce la risposta */
	CREATE(nuovo, struct blocco_testo, 1, TYPE_BLOCCO_TESTO);

	va_start(ap, format);
#ifdef USE_MEM_STAT
	/* Non e' molto bello... si puo' fare di meglio... */
	size = vasprintf(&tmp, fmt, ap);
	CREATE(nuovo->testo, char, size+1, TYPE_CHAR);
	memcpy(nuovo->testo, tmp, size+1);
#else
	size = vasprintf(&nuovo->testo, fmt, ap);
#endif
	va_end(ap);

	/* Invia la risposta */
	coda = &(p->output);
	/* p->length += size; */

	if (coda->partenza) {
		coda->termine->prossimo = nuovo;
		coda->termine = nuovo;
	} else
		coda->partenza = coda->termine = nuovo;
	nuovo->prossimo = NULL;
#ifdef IMAP4_DEBUG
	logf("IMAP4 S: %s", nuovo->testo);
#endif
}

/****************************************************************************/
/* Prende un 'atom' da str, e lo copia in atom.
 * Restituisce un puntatore all'atomo successivo. */
const char * get_atom(char *atom, const char *str)
{
	int i = 0;

	for (i = 0; i < LBUF; i++) {
		if (is_ATOM_CHAR(str[i]))
			atom[i] = str[i];
		else if (str[i] == ' ') {
			atom[i] = 0;
			return str+i+1;
		} else if (str[i] == 0) {
			atom[i] = 0;
			return str+i;
		} else
			return NULL;
	}
	return NULL;
}

enum { NORMAL, QUOTED, END };

const char * get_quoted(char *atom, const char *str)
{
	int stato = NORMAL;
	int i = 0, j = 0;

	if (str[0] != '"')
		return NULL;
	for (i = 1; i < LBUF; i++) {
		switch (stato) {
		case NORMAL:
			if (is_TEXT_CHAR(str[i]) && !is_quoted_special(str[i]))
				atom[j++] = str[i];
			else if (str[i] == '\\')
				stato = QUOTED;
			else if (str[i] == '"') {
				atom[j] = 0;
				stato = END;
				return str+i+1;
			} else
				return NULL;
			break;
		case QUOTED:
			if (is_quoted_special(str[i]))
				atom[j++] = str[i];
			else
				return NULL;
			break;
		}
	}
	return NULL;
}

const char * get_astring(char *atom, const char *str)
{
	if (str[0] != '"')
		return get_atom(atom, str);

	return get_quoted(atom, str);
}

/* Parse client command command.
 * Copia il tag in tag, fa puntare arg all'inizio degli argomenti nella
 * stringa cmd. Restituisce TRUE se il comando era ben formato. */
int parse_command(char *cmd, char *tag, char **arg)
{
	int i = 0;

	for (i = 0; i < LBUF; i++) {
		if (is_ATOM_CHAR(cmd[i]))
			tag[i] = cmd[i];
		else if (cmd[i] == ' ') {
			tag[i] = 0;
			*arg = cmd+i+1;
			return TRUE;
		} else if (cmd[i] == 0) {
			tag[i] = 0;
			*arg = NULL;
			return FALSE;
		} else
			return FALSE;
	}
	return FALSE;
}

/* any 7-bit US-ASCII character except NUL */
int is_CHAR(char c)
{
	return ((c >= 1) && (c <= 127)) ? TRUE : FALSE;
}

/* any 7-bit US-ASCII character except NUL */
int is_ATOM_CHAR(char c)
{
	return (is_CHAR(c) && !is_atom_special(c)) ? TRUE : FALSE;
}

/* any 7-bit US-ASCII character except NUL */
int is_TEXT_CHAR(char c)
{
	return (is_CHAR(c) && (c != '\r') && (c != '\n')) ? TRUE : FALSE;
}

/* any ASCII control character and DEL */
int is_CTL(char c)
{
	if (((c >= 0x00) && (c <= 0x1f)) || (c == 0x7f))
		return TRUE;
	return FALSE;
}

int is_wildcard(char c)
{
	return ((c == '%') || (c == '$')) ? TRUE : FALSE;
}

int is_quoted_special(char c)
{
	return ((c == '"') || (c == '\\')) ? TRUE : FALSE;
}

int is_atom_special(char c)
{
	if (index("(){ ", c) || is_CTL(c) || is_wildcard(c)
	    || is_quoted_special(c))
		return TRUE;
	return FALSE;
}

/**************************************************************************/
void imap4_interprete_comandi(struct sessione *p, char *command)
{
	int i = 0;
	char * arg;

	if (!parse_command(command, p->tag, &arg)) {
		imap4_printf(p, NO_TAG, RESP_BAD, NO_STATUS, "No tag\r\n");
		return;
	}

#ifdef IMAP4_DEBUG
	logf("IMAP4 C: <%s>", command);
#endif
	while ( *(imap4_cmd_list[i].token) != '\0' ) {
		if (!strncasecmp(arg, imap4_cmd_list[i].token,
				 imap4_cmd_list[i].len)) {
			/* Controlla che l'utente e` nello stato giusto */
			if ((imap4_cmd_list[i].stato & p->stato) == 0) {
				imap4_printf(p, TAG, RESP_BAD, NO_STATUS,
					     "Wrong state\r\n");
				return;
			}

			/* Esegue il comando */
			switch (imap4_cmd_list[i].argnum) {
			case NO_ARG:
				imap4_cmd_list[i].cmd.no_arg(p);
				break;
			case OPT_ARG:
			case PASS_ARG:
				if (command[LTOKEN(i)] == '\0')
					command[ARGSTART(i)] = '\0';
				imap4_cmd_list[i].cmd.pass_arg(p,
					arg + ARGSTART(i));
				break;
			}
			/* Post execution command */
			return;
		}
		i++;
	}
	imap4_printf(p, TAG, RESP_BAD, NO_STATUS, "Invalid command\r\n");
}


/****************************************************************************/
int imap4_nuovo_descr(int s)
{
	char buf2[LBUF];
	int desc;
	struct sessione *nuova_ses, *punto;
	int size, socket_connessi, socket_attivi, i;
	struct sockaddr_in sock;
	struct hostent *da_dove;

	if ( (desc = nuova_conn(s)) < 0)
		return (-1);

	socket_connessi = socket_attivi = 0;

	for (punto = sessioni_imap4; punto; punto = punto->prossima)
		socket_connessi++;

	/* Controllo sul numero massimo di connessioni supportabili */

	if (socket_connessi >= imap4_max_num_desc) {
		sprintf(buf2, "* BYE Massimo numero di connessioni IMAP4 raggiunto.\r\n");
		scrivi_a_desc(desc, buf2);
		close(desc);
		return(0);
	} else if (desc > imap4_maxdesc)
			imap4_maxdesc = desc;

	/* Poi va creata una nuova struttura sessione per il nuovo arrivato */

	CREATE(nuova_ses, struct sessione, 1, TYPE_SESSIONE);

	/* Vanno prese le info relative al sito di provenienza */

	size = sizeof(sock);
	if (getpeername(desc, (struct sockaddr *) &sock, &size) < 0) {
		Perror("getpeername");
		*nuova_ses->host = 0;
	} else if (nameserver_lento ||
		   !(da_dove = gethostbyaddr((char *)&sock.sin_addr,
				       sizeof(sock.sin_addr), AF_INET))) {
		if (!nameserver_lento)
			Perror("gethostbyaddr");
		i = sock.sin_addr.s_addr;
		sprintf(nuova_ses->host, "%d.%d.%d.%d", (i & 0x000000FF),
			(i & 0x0000FF00) >> 8, (i & 0x00FF0000) >> 16,
			(i & 0xFF000000) >> 24 );
	} else {
		strncpy(nuova_ses->host, da_dove->h_name, 49);
		*(nuova_ses->host + 49) = 0;
	}

	/* Si controlla se il sito e' bandito totalmente */

	/* Se e' previsto uno shutdown avverte e chiude la connessione */

	logf("IMAP4: Nuova connessione da [%s]", nuova_ses->host);

	/* Inizializziamo la struttura coi dati della nuova sessione */
	nuova_ses->socket_descr = desc;
	/* nuova_ses->length = 0; */
	*nuova_ses->in_buf  = '\0';
	*nuova_ses->out_buf = '\0';
	nuova_ses->ora_login = time(0);
        init_coda(&nuova_ses->input);
        init_coda(&nuova_ses->output);

	nuova_ses->idle.cicli = 0;
	nuova_ses->idle.warning = 0;
	nuova_ses->idle.min = 0;

	/* Attenzione... va fatto un free, realloc, etc.. */
	utr_alloc1(&nuova_ses->fullroom, &nuova_ses->room_flags,
		   &nuova_ses->room_gen);
	mail_alloc1(&nuova_ses->mail);

	/* Lo appiccichiamo in cima alla lista     */
	nuova_ses->prossima = sessioni_imap4;
	sessioni_imap4 = nuova_ses;

	/* Saluta e entra in stato authorization */
	imap4_printf(nuova_ses, NO_TAG, RESP_OK, NO_STATUS,
		     "IMAP4 Service for Cittadella/UX Ready.\r\n");
	nuova_ses->stato = NON_AUTH;

	return(0);
}

/*
  Chiude i socket e aggiorna la lista delle sessioni imap4,
  cancellando la sessione h in input.
*/
void imap4_chiudi_socket(struct sessione *h)
{
	struct sessione *tmp;

	close(h->socket_descr);
	if (h->socket_descr == imap4_maxdesc)
		--imap4_maxdesc;

	if (h == sessioni_imap4)
		sessioni_imap4 = sessioni_imap4->prossima;
	else {
		for(tmp = sessioni_imap4; (tmp->prossima != h) && tmp ;
		    tmp = tmp->prossima);

		tmp->prossima = h->prossima;
	}
        /* Pulizia */
        mail_free(h->mail);
        imap4_free_maildrop(h);
	Free(h->fullroom);
	Free(h->room_flags);
	Free(h->room_gen);
        flush_coda(&h->input);
        flush_coda(&h->output);
	Free(h);
}

void imap4_chiusura_server(int s)
{
	while(sessioni_imap4)
		imap4_chiudi_socket(sessioni_imap4);
        close(s);
}

/*
  Crea in t->coda testo
  una struttura con un campo per ogni linea.
*/
int imap4_elabora_input(struct sessione *t)
{
	int fino_a, questo_giro, inizio, i, k, flag;
        size_t buflen;
	char tmp[MAX_L_INPUT+2];

	fino_a = 0;
	flag = 0;
	inizio = strlen(t->in_buf); /* Al posto di questo strlen() mi */
	/* conviene tenere un puntatore nella struct sessione */

	/* Legge un po' di roba */
	if ((questo_giro = read(t->socket_descr, t->in_buf+inizio+fino_a,
				MAX_STRINGA-(inizio+fino_a)-1)) > 0)
		fino_a += questo_giro;
	else if (questo_giro < 0) {
		if (errno != EWOULDBLOCK) {
			Perror("Read1 - ERRORE");
			return(-1);
		}
	} else {
		log("SYSERR: Incontrato EOF in lettura su socket.");
		return(-1);
	}

        buflen = inizio + fino_a;
	*(t->in_buf + buflen) = '\0';

	/* Se l'input non contiene nessun newline esce senza fare nulla */
	for (i = inizio; !ISNEWL(*(t->in_buf + i)); i++)
		if (!*(t->in_buf + i))
			return(0);


	/* L'input contiene 1 o piu' newline. Elabora e mette in coda */
	tmp[0] = '\0';
	for (i = 0, k = 0; *(t->in_buf + i); ) {
		if (!ISNEWL(*(t->in_buf + i)) &&
		    !(flag = (k >= (MAX_L_INPUT - 2)))) {
			if (*(t->in_buf + i) == '\b') {  /* backspace */
				if (k) /* c'e' qualcosa di cancellabile */
					k--;
			} else if (isascii(*(t->in_buf + i)) &&
				   isprint(*(t->in_buf + i))) {
				*(tmp + k) = *(t->in_buf + i);
				k++;
			} /* altrimenti carattere di cui non ci frega nulla */
			i++;
		} else {
			*(tmp + k) = '\0';
			if (tmp[0] != '\0') {      /* SEMPRE VERA ?? */
				metti_in_coda(&t->input, tmp, k);
				tmp[0] = '\0';
                                t->idle.cicli = 0;
                                t->idle.min = 0;
                                t->idle.ore = 0;
			}

			if (flag) /* linea troppo lunga. ignora il resto */
				for ( ; !ISNEWL(*(t->in_buf + i)); i++);

                        /* trova la fine della linea */
                        for ( ; ISNEWL(*(t->in_buf + i)); i++);

                        /* elimina la linea dal in_buffer */
                        memmove(t->in_buf, t->in_buf+i, buflen-i+1);
                        buflen -= i;

			k = 0;
			i = 0;
		}
	}

	return 1;
}

/*
 * pop_3collegato(nick) restitutisce il puntatore alla struttura sessione
 *                      dell'utente con nome nick se esiste, altrimenti NULL.
 */
static struct sessione * imap4_collegato(char *ut)
{
        struct sessione *punto, *coll;

        coll = NULL;
        for (punto = sessioni_imap4; !coll && punto; punto = punto->prossima)
                if ((punto->utente) && (!strcasecmp(ut, punto->utente->nome)))
                                coll = punto;
        return coll;
}

/* Chiude la sessione appartenente all'utente *ut           *
 * ATTENZIONE: e` un po' brutale, magari e` migliorabile... */
void imap4_kill_user(char *ut)
{
        struct sessione *sess;

        sess = imap4_collegato(ut);
}

/*****************************************************************************/
/*
 * Implementazione comandi IMAP4
 */

static void imap4_capability(struct sessione *p)
{
	imap4_printf(p, NO_TAG, RESP_CAPABILITY, NO_STATUS, "%s\r\n",
		     imap4_version);
}

static void imap4_noop(struct sessione *p)
{
	imap4_printf(p, TAG, RESP_OK, NO_STATUS, "NOOP Completed\r\n");
}

static void imap4_logout(struct sessione *p)
{
        /*
       	struct room *room;
        long i, del;
        */

	if (p->stato != NON_AUTH) {
		/* Transaction state */
		p->stato = LOGOUT;

#if 0
		/* Cancella i messaggi */
		room = room_find(mail_room);
		room->msg = p->mail;
		del = 0;
		for (i = 0; i < p->mail_num; i++) {
			if (p->msg[i].mark_deleted) {
				msg_del(room, p->msg[i].id);
				del++;
			}
		}
		room->msg = NULL;
		mail_save1(p->utente, p->mail, &p->mail_highmsg,
			   &p->mail_highloc);
#endif
	}
	imap4_printf(p, NO_TAG, RESP_BYE, NO_STATUS,
		     "Cittadella/UX Server terminating connection\r\n");
	imap4_printf(p, TAG, RESP_OK, NO_STATUS, "LOGOUT Completed\r\n");

	/* Release exclusive-access lock */
	p->stato = LOGOUT;
}

/*****************************************************************************/

static void imap4_authenticate(struct sessione *p)
{
	imap4_printf(p, TAG, RESP_NO, NO_STATUS, "Unsupported authentication mechanism\r\n");
}

static void imap4_login(struct sessione *p, char *str)
{
        struct dati_ut *utente;
	char nome[LBUF], pass[LBUF];
	const char *tmp;

	if (p->utente) {
		p->utente = NULL;
		imap4_printf(p, TAG, RESP_NO, NO_STATUS,
			     "Already logged in\r\n");
		return;
	}
	tmp = get_astring(nome, str);
	if (tmp == NULL) {
		imap4_printf(p, TAG, RESP_BAD, NO_STATUS,
			     "Expecting 'user password'\r\n");
		return;
	}
	tmp = get_astring(pass, tmp);
	if (tmp == NULL) {
		imap4_printf(p, TAG, RESP_NO, NO_STATUS,
			     "Bad password'\r\n");
		return;
	}

	utente = trova_utente(nome);
	if ((utente == NULL) || (utente->livello < MINLVL_IMAP4)) {
		imap4_printf(p, TAG, RESP_NO, NO_STATUS,
			     "not a valid BBS user\r\n");
		return;
	}
	if (collegato(nome) || imap4_collegato(nome)) {
		imap4_printf(p, TAG, RESP_NO, NO_STATUS,
			     "%s is already logged in\r\n", nome);
		return;
	}
	if (!check_password(pass, utente->password)) {
		imap4_printf(p, TAG, RESP_NO, NO_STATUS,
			     "invalid password\r\n");
                logf("IMAP4: Pwd errata per [%s] da [%s]", nome, p->host);
		p->utente = NULL;
		return;
        }

	p->utente = utente;

	utr_load1(p->utente, p->fullroom, p->room_flags, p->room_gen);
	mail_load1(p->utente, p->mail, &p->mail_highmsg,&p->mail_highloc);

	/* p->utente->chiamate++;  SOSTITUIRE, TOGLIERE ? */
	logf("IMAP4 login [%s] da [%s].", nome, p->host);

	p->stato = AUTHENTICATED;
	imap4_printf(p, TAG, RESP_OK, NO_STATUS, "LOGIN completed\r\n");
	/* imap4_maildrop(p); */
}

/*****************************************************************************/
static void imap4_select(struct sessione *p, char *name)
{
	struct room *nr, *punto;

	nr = next_room_mode(p, name, 0);

	if (nr == NULL) {
		p->stato = AUTHENTICATED;
		imap4_printf(p, TAG, RESP_NO, NO_STATUS, "Select failure, now in authenticated state: no such room\r\n");

		/*
                imap4_printf(p, TAG, RESP_NO, NO_STATUS, "Select failure, now in authenticated state: can't access room\r\n");
                */
	}
	p->stato = SELECT;
	imap4_printf(p, TAG, RESP_OK, NO_STATUS, "Select completed, now in selected state.\r\n");

}

static void imap4_examine(struct sessione *p, char *name)
{
	imap4_printf(p, TAG, RESP_NO, NO_STATUS, "Unsupported authentication mechanism\r\n");
}

/*****************************************************************************/


#if 0
static void imap4_pass(struct sessione *p, char *pass)
{
	if (p->utente == NULL) {
		imap4_printf(p, "%s first send USER command\r\n", imap4_err);
		return;
	}

	if (!check_password(pass, p->utente->password)) {
		imap4_printf(p, "%s invalid password\r\n", imap4_err);
                logf("IMAP4: Pwd errata per [%s] da [%s]", p->utente->nome,
		     p->host);
		p->utente = NULL;
		return;
        }

	utr_load1(p->utente, p->fullroom, p->room_flags, p->room_gen);
	mail_load1(p->utente, p->mail, &p->mail_highmsg,&p->mail_highloc);

	/* p->utente->chiamate++;  SOSTITUIRE, TOGLIERE ? */
	logf("IMAP4 login [%s] da [%s].", p->utente->nome, p->host);

	p->stato = TRANSACTION;
	imap4_maildrop(p);
	imap4_printf(p, "%s %s's maildrop has %ld messages (%ld octets)\r\n",
		imap4_ok, p->utente->nome, p->mail_num, p->mail_size);
}

static void imap4_stat(struct sessione *p)
{
	long totnum = 0;
	size_t totlen = 0;
	int i;

	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted) {
			totnum++;
			totlen += p->msg[i].size;
		}
	}
	imap4_printf(p, "%s %ld %d\r\n", imap4_ok, totnum, totlen);
}

static void imap4_list(struct sessione *p, char *msg)
{
	int i;
	size_t totlen;
	long num, totnum;

	if (*msg) {
		num = strtol(msg, NULL, 10);
		if ((num <= 0) || (num > p->mail_num)) {
			imap4_printf(p, "%s no such message, only %ld messages in maildrop\r\n", imap4_err, p->mail_num);
			return;
		}
		if (p->msg[num-1].mark_deleted) {
			imap4_printf(p,"%s message marked as deleted\r\n",imap4_err);
			return;
		}
		imap4_printf(p, "%s %ld %d\r\n", imap4_ok, num, p->msg[num-1].size);
		return;
	}
	totnum = 0;
	totlen = 0;
	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted) {
			totnum++;
			totlen += p->msg[i].size;
		}
	}
	imap4_printf(p, "%s %ld messages (%d octets)\r\n", imap4_ok,
	       totnum, totlen);
	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted)
			imap4_printf(p, "%d %d\r\n", i+1, p->msg[i].size);
	}
	imap4_printf(p, ".\r\n");
}

static void imap4_retr(struct sessione *p, char *msg)
{
	long num;
	char * str;

	if (msg == '\0') {
		imap4_printf(p,"%s message number required\r\n", imap4_err);
		return;
	}
	num = strtol(msg, NULL, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		imap4_printf(p, "%s no such message, only %ld messages in maildrop\r\n", imap4_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		imap4_printf(p, "%s message marked as deleted\r\n", imap4_err);
		return;
	}
	imap4_printf(p, "%s %d\r\n", imap4_ok, p->msg[num-1].size);
	/* Send message */
	txt_rewind(p->msg[num-1].msg);
	while ( (str = txt_get(p->msg[num-1].msg))) {
                if (str[0] == '.') /* byte-stuff the line */
                        imap4_printf(p, ".%s\r\n", str);
                else
                        imap4_printf(p, "%s\r\n", str);
        }
	imap4_printf(p, ".\r\n");
}

static void imap4_dele(struct sessione *p, char *msg)
{
	long num;

	if (msg == '\0') {
		imap4_printf(p,"%s message number required\r\n",imap4_err);
		return;
	}
	num = strtol(msg, NULL, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		imap4_printf(p, "%s no such message, only %ld messages in maildrop\r\n", imap4_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		imap4_printf(p, "%s message %ld already deleted\r\n", imap4_err, num);
		return;
	}
	p->msg[num-1].mark_deleted = TRUE;
	imap4_printf(p, "%s message %ld deleted\r\n", imap4_ok, num);
}

static void imap4_rset(struct sessione *p)
{
	int i;

	for (i = 0; i < p->mail_num; i++)
		p->msg[i].mark_deleted = FALSE;
	imap4_printf(p, "%s maildrop has %ld messages (%ld octets)\r\n",
		imap4_ok, p->mail_num, p->mail_size);
}

static void imap4_top(struct sessione *p, char *msg)
{
	long num, lines;
	char *endptr, *str;

	num = strtol(msg, &endptr, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		imap4_printf(p, "%s no such message, only %ld messages in maildrop\r\n", imap4_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		imap4_printf(p, "%s message marked as deleted\r\n", imap4_err);
		return;
	}
	lines = strtol(endptr, NULL, 10);
	if (lines < 0) {
		imap4_printf(p, "%s number of lines must be non-negative\r\n",
			    imap4_err);
		return;
	}
	imap4_printf(p, "%s top of message follows\r\n", imap4_ok);
	txt_rewind(p->msg[num-1].msg);
	/* Send header */
	while ( (str = txt_get(p->msg[num-1].msg)) && (*str)) {
                if (str[0] == '.') /* byte-stuff the line */
                        imap4_printf(p, ".%s\r\n", str);
                else
                        imap4_printf(p, "%s\r\n", str);
        }
	imap4_printf(p, "\r\n");

	/* Send first 'lines' lines of the body */
	while (lines && (str = txt_get(p->msg[num-1].msg))) {
                if (str[0] == '.') /* byte-stuff the line */
                        imap4_printf(p, ".%s\r\n", str);
                else
                        imap4_printf(p, "%s\r\n", str);
		lines--;
	}
	imap4_printf(p, ".\r\n");
}

static void imap4_uidl(struct sessione *p, char *msg)
{
	int i;
	long num;

	if (*msg) {
		num = strtol(msg, NULL, 10);
		if ((num <= 0) || (num > p->mail_num)) {
			imap4_printf(p, "%s no such message, only %ld messages in maildrop\r\n", imap4_err, p->mail_num);
			return;
		}
		if (p->msg[num-1].mark_deleted) {
			imap4_printf(p, "%s message marked as deleted\r\n",
				    imap4_err);
			return;
		}
		imap4_printf(p, "%s %ld %ld\r\n", imap4_ok, num, p->msg[num-1].id);
		return;
	}
	imap4_printf(p, "%s\r\n", imap4_ok);
	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted)
			imap4_printf(p, "%d %ld\r\n", i+1, p->msg[i].id);
	}
	imap4_printf(p, ".\r\n");

}

static void imap4_apop(struct sessione *p, char *msg)
{
	imap4_printf(p, "%s Not yet supported\r\n", imap4_err);
}
#endif

/****************************************************************************/
static void imap4_maildrop(struct sessione *p)
{
#if 0
	struct room *room;
	long fmnum, msgnum, msgpos, lowest, locnum, i, j;
	char autore[LBUF], room_name[LBUF], subject[LBUF], dest[LBUF];
        char date[LBUF], header[LBUF], err, *riga;
	long flags, ora;

#ifdef USE_STRING_TEXT
	char *txt;
	size_t len;
	int nrighe;
#else
	struct text *txt;
#endif
#ifdef USE_CACHE_POST
	char *header1;
# ifndef USE_STRING_TEXT
	struct text *txt1;
# endif
#endif
	struct text *msg;
	struct html_tag_count tc;

	room = room_find(mail_room);
	fmnum = room->data->fmnum;
	lowest = fm_lowest(room->data->fmnum);

	p->mail_num = 0;
	for (i = 0; i < room->data->maxmsg; i++) {
		if (p->mail->num[i] >= lowest)
			p->mail_num++;
	}
	if (p->mail_num == 0)
		return; /* No mail */

	CREATE(p->msg, struct imap4_msg, p->mail_num, TYPE_IMAP4_MSG);

	i = 0;
	j = 0;

        /*
        while((p->mail->num[i] < lowest) && (i < room->data->maxmsg))
	i++;
        */

	for ( ; i < room->data->maxmsg; i++) {
		msgnum = p->mail->num[i];
		msgpos = p->mail->pos[i];
#ifdef USE_CACHE_POST
#ifdef USE_STRING_TEXT
		err = cp_get(fmnum, msgnum, msgpos, &txt, &len, &header1);
# else
		err = cp_get(fmnum, msgnum, msgpos, &txt1, &header1);
		txt = txt1;
# endif
#else
		txt = txt_create();
		err = fm_get(fmnum, msgnum, msgpos, txt, header);
#endif
		if (err == 0) {

#ifdef USE_CACHE_POST
#ifdef USE_STRING_TEXT
			nrighe = argz_count(txt, len);
# endif
			strcpy(header, header1);
#endif
			msg = txt_create();
			p->msg[j].size = 0;
			p->msg[j].id = msgnum;
			p->msg[j].mark_deleted = 0;

			locnum = p->mail->local[i];
			extract(room_name, header, 1);
			extract(autore, header, 2);
			ora = extract_long(header, 3);
			extract(subject, header, 4);
			flags = extract_long(header, 5);
			extract(dest, header, 6);
                        strdate_rfc822(date, LBUF, ora);
			txt_putf(msg, "Date: %s", date);
			txt_putf(msg, "From: %s @Cittadella <no-reply@bbs.cittadellabbs.it>", autore);
			txt_putf(msg, "Subject: %s", subject);
			txt_putf(msg, "Sender: %s", BBS_EMAIL);
			txt_putf(msg, "To: %s", dest);
			txt_putf(msg, "Message-ID: <%ld@CittadellaBBS> ",
                                 msgnum);
			txt_putf(msg, "");
#ifdef USE_STRING_TEXT
			for (riga = txt; riga;
			     riga = argz_next(txt, len, riga)) {
			 	char *outstr;
				outstr = cml2html_max(riga, NULL, 0, &tc);
				txt_putf(msg, "%s", riga);
				p->msg[j].size += strlen(riga)+2;
				Free(outstr);
			}
#else
			txt_rewind(txt);
			while (riga = txt_get(txt), riga)
				txt_putf(msg, "%s", OK, riga);
#endif

#ifndef USE_CACHE_POST
			txt_free(&txt);
#endif
			p->msg[j].msg = msg;
			j++;
		}
	}
#endif
}

static void imap4_free_maildrop(struct sessione *p)
{
#if 0
        int i;

	for (i = 0; i < p->mail_num; i++)
                txt_free(&p->msg[i].msg);
        Free(p->msg);
#endif
}

#if 0
static void imap4_printf(struct sessione *p, const char *format, ...)
{
	struct coda_testo *coda;
	struct blocco_testo *nuovo;
	va_list ap;
#ifdef USE_MEM_STAT
	char *tmp;
	int size;
#endif

	CREATE(nuovo, struct blocco_testo, 1, TYPE_BLOCCO_TESTO);

	va_start(ap, format);
#ifdef USE_MEM_STAT
	/* Non e' molto bello... si puo' fare di meglio... */
	size = vasprintf(&tmp, format, ap);
	CREATE(nuovo->testo, char, size+1, TYPE_CHAR);
	memcpy(nuovo->testo, tmp, size+1);
#else
	size = vasprintf(&nuovo->testo, format, ap);
#endif
	va_end(ap);

	coda = &(p->output);
	p->length += size;

	if (coda->partenza) {
		coda->termine->prossimo = nuovo;
		coda->termine = nuovo;
	} else
		coda->partenza = coda->termine = nuovo;
	nuovo->prossimo = NULL;
#ifdef IMAP4_DEBUG
	logf("IMAP4 S: %s", nuovo->testo);
#endif
}

static void imap4_vprintf(struct sessione *p, const char *format,
			  va_list ap)
{
	struct coda_testo *coda;
	struct blocco_testo *nuovo;
#ifdef USE_MEM_STAT
	char *tmp;
	int size;
#endif

	CREATE(nuovo, struct blocco_testo, 1, TYPE_BLOCCO_TESTO);

	va_start(ap, format);
#ifdef USE_MEM_STAT
	/* Non e' molto bello... si puo' fare di meglio... */
	size = vasprintf(&tmp, format, ap);
	CREATE(nuovo->testo, char, size+1, TYPE_CHAR);
	memcpy(nuovo->testo, tmp, size+1);
#else
	size = vasprintf(&nuovo->testo, format, ap);
#endif
	va_end(ap);

	coda = &(p->output);
	p->length += size;

	if (coda->partenza) {
		coda->termine->prossimo = nuovo;
		coda->termine = nuovo;
	} else
		coda->partenza = coda->termine = nuovo;
	nuovo->prossimo = NULL;
#ifdef IMAP4_DEBUG
	logf("IMAP4 S: %s", nuovo->testo);
#endif
}

#endif

#endif /* USE_IMAP4 */
