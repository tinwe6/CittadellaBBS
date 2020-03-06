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
* File: pop3.c                                                                *
*       RFC 1939 implementation of POP3 server for Cittadella/UX              *
******************************************************************************/

#include "config.h"

#ifdef USE_POP3

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

#include "pop3.h"

#include "cittaserver.h"
#include "cml2html.h"
#include "extract.h"
#include "macro.h"
#include "mail.h"
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

struct pop3_sess *sessioni_pop3;
int pop3_maxdesc;
int pop3_max_num_desc = 5;  /* Massimo numero di connessioni POP3 simultanee */

/* Stati della sessione */
#define AUTHORIZATION  1
#define TRANSACTION    2
#define UPDATE         4
#define QUIT           8
#define ANY          (AUTHORIZATION | TRANSACTION | UPDATE)

/* Prototipi funzioni in questo file: */
int pop3_nuovo_descr(int s);
void pop3_chiudi_socket(struct pop3_sess *h);
int pop3_elabora_input(struct pop3_sess *t);
void pop3_interprete_comandi(struct pop3_sess *p, char *command);
void pop3_kill_user(char *ut);

static struct pop3_sess * pop3_collegato(char *ut);
static void pop3_noop(struct pop3_sess *p);
static void pop3_quit(struct pop3_sess *p);
static void pop3_user(struct pop3_sess *p, char *nome);
static void pop3_pass(struct pop3_sess *p, char *pass);
static void pop3_stat(struct pop3_sess *p);
static void pop3_list(struct pop3_sess *p, char *msg);
static void pop3_retr(struct pop3_sess *p, char *msg);
static void pop3_dele(struct pop3_sess *p, char *msg);
static void pop3_rset(struct pop3_sess *p);
static void pop3_top(struct pop3_sess *p, char *msg);
static void pop3_uidl(struct pop3_sess *p, char *msg);
static void pop3_apop(struct pop3_sess *p, char *msg);
static void pop3_maildrop(struct pop3_sess *p);
static void pop3_free_maildrop(struct pop3_sess *p);
static void pop3_printf(struct pop3_sess *p, const char *format, ...) CHK_FORMAT_2;

/* Argomenti dei comandi */
enum {
	NO_ARG,
	OPT_ARG,
	PASS_ARG
};

/* Lista dei comandi e strutture necessarie per la sua memorizzazione */
typedef union {
	void (*no_arg) (struct pop3_sess *t);
	void (*pass_arg) (struct pop3_sess *t, char *buf);
} cmd_ptr;

struct comando {
	char token[5]; /* Nome del comando                                  */
	size_t len;    /* Lunghezza del comando                             */
	cmd_ptr cmd;   /* Puntatore alla funzione che implementa il comando */
	char argnum;   /* Numero argomenti della funzione                   */
	int  stato;    /* Stato attuale necessario per eseguire il comando  */
};

#define LTOKEN(I)   (pop3_cmd_list[(I)].len)
#define ARGSTART(I) (pop3_cmd_list[(I)].len + 1)

static const struct comando pop3_cmd_list[] = {
/* Minimal POP3 Commands  */
{ "STAT", 4, { (void *)&pop3_stat }, NO_ARG,   TRANSACTION   },
{ "LIST", 4, { (void *)&pop3_list }, OPT_ARG,  TRANSACTION   },
{ "RETR", 4, { (void *)&pop3_retr }, PASS_ARG, TRANSACTION   },
{ "DELE", 4, { (void *)&pop3_dele }, PASS_ARG, TRANSACTION   },
{ "NOOP", 4, { (void *)&pop3_noop }, NO_ARG,   TRANSACTION   },
{ "RSET", 4, { (void *)&pop3_rset }, NO_ARG,   TRANSACTION   },
{ "QUIT", 4, { (void *)&pop3_quit }, NO_ARG,   ANY           },
/* Optional POP3 Commands */
{ "TOP" , 3, { (void *)&pop3_top  }, PASS_ARG, TRANSACTION   },
{ "UIDL", 4, { (void *)&pop3_uidl }, PASS_ARG, TRANSACTION   },
{ "USER", 4, { (void *)&pop3_user }, PASS_ARG, AUTHORIZATION },
{ "PASS", 4, { (void *)&pop3_pass }, PASS_ARG, AUTHORIZATION },
{ "APOP", 4, { (void *)&pop3_apop }, PASS_ARG, AUTHORIZATION },
{     "", 0, { (void *)   NULL    }, 0       , 0             }};

/* POP3 replies */
const char pop3_ok[] = "+OK";
const char pop3_err[] = "-ERR";

/****************************************************************************/

int pop3_nuovo_descr(int s)
{
	char buf2[LBUF];
	int desc;
	struct pop3_sess *nuova_ses, *punto;
	int size, socket_connessi, socket_attivi, i;
	struct sockaddr_in sock;
	struct hostent *da_dove;

	if ( (desc = nuova_conn(s)) < 0)
		return (-1);

	socket_connessi = socket_attivi = 0;

	for (punto = sessioni_pop3; punto; punto = punto->next)
		socket_connessi++;

	/* Controllo sul numero massimo di connessioni supportabili */

	if (socket_connessi >= pop3_max_num_desc) {
		sprintf(buf2, "%d Massimo numero di connessioni POP3 raggiunto.\r\n", ERROR);
		scrivi_a_desc(desc, buf2);
		close(desc);
		return(0);
	} else if (desc > pop3_maxdesc)
			pop3_maxdesc = desc;

	/* Poi va creata una nuova struttura sessione per il nuovo arrivato */

	CREATE(nuova_ses, struct pop3_sess, 1, TYPE_POP3_SESSIONE);

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

	citta_logf("POP3: Nuova connessione da [%s]", nuova_ses->host);

	/* Inizializziamo la struttura coi dati della nuova sessione */
	nuova_ses->desc = desc;
	nuova_ses->length = 0;
	*nuova_ses->in_buf  = '\0';
	*nuova_ses->out_buf = '\0';
	nuova_ses->conntime = time(0);
        init_coda(&nuova_ses->input);
        init_coda(&nuova_ses->output);

	nuova_ses->idle.cicli = 0;
	nuova_ses->idle.warning = 0;
	nuova_ses->idle.min = 0;

	/* Attenzione... va fatto un free, realloc, etc.. */
	utr_alloc1(&nuova_ses->fullroom, &nuova_ses->room_flags,
		   &nuova_ses->room_gen);
	nuova_ses->mail_list = mail_alloc();

	/* Lo appiccichiamo in cima alla lista     */
	nuova_ses->next = sessioni_pop3;
	sessioni_pop3 = nuova_ses;

	/* Saluta e entra in stato authorization */
	pop3_printf(nuova_ses, "+OK Cittadella/UX POP3 server ready\r\n");
	nuova_ses->stato = AUTHORIZATION;
	nuova_ses->finished = FALSE;

	return(0);
}

/*
  Chiude i socket e aggiorna la lista delle sessioni pop3,
  cancellando la sessione h in input.
*/
void pop3_chiudi_socket(struct pop3_sess *h)
{
	struct pop3_sess *tmp;

	close(h->desc);
	if (h->desc == pop3_maxdesc)
		--pop3_maxdesc;

	if (h == sessioni_pop3)
		sessioni_pop3 = sessioni_pop3->next;
	else {
		for(tmp = sessioni_pop3; (tmp->next != h) && tmp ;
		    tmp = tmp->next);

		tmp->next = h->next;
	}
        /* Pulizia */
        mail_free(h->mail_list);
        pop3_free_maildrop(h);
	Free(h->fullroom);
	Free(h->room_flags);
	Free(h->room_gen);
        flush_coda(&h->input);
        flush_coda(&h->output);
	Free(h);
}

void pop3_chiusura_server(int s)
{
	while(sessioni_pop3)
		pop3_chiudi_socket(sessioni_pop3);
        close(s);
}

/*
  Crea in t->coda testo
  una struttura con un campo per ogni linea.
*/
int pop3_elabora_input(struct pop3_sess *t)
{
	int fino_a, questo_giro, inizio, i, k, flag;
        size_t buflen;
	char tmp[MAX_L_INPUT+2];

	fino_a = 0;
	flag = 0;
	inizio = strlen(t->in_buf); /* Al posto di questo strlen() mi */
	/* conviene tenere un puntatore nella struct sessione */

	/* Legge un po' di roba */
	if ((questo_giro = read(t->desc, t->in_buf+inizio+fino_a,
				MAX_STRINGA-(inizio+fino_a)-1)) > 0)
		fino_a += questo_giro;
	else if (questo_giro < 0) {
		if (errno != EWOULDBLOCK) {
			Perror("Read1 - ERRORE");
			return(-1);
		}
	} else {
		citta_log("SYSERR: Incontrato EOF in lettura su socket.");
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


void pop3_interprete_comandi(struct pop3_sess *p, char *command)
{
	int i = 0;

#ifdef POP3_DEBUG
	citta_logf("POP3 C: <%s>", command);
#endif
	while ( *(pop3_cmd_list[i].token) != '\0' ) {
		if (!strncasecmp(command, pop3_cmd_list[i].token,
				 pop3_cmd_list[i].len)) {
			/* Controlla che l'utente e` nello stato giusto */
			if ((pop3_cmd_list[i].stato & p->stato) == 0) {
				pop3_printf(p, "%s Wrong state\r\n", pop3_err);
				return;
			}

			/* Esegue il comando */
			switch (pop3_cmd_list[i].argnum) {
			case NO_ARG:
				pop3_cmd_list[i].cmd.no_arg(p);
				break;
			case OPT_ARG:
			case PASS_ARG:
				if (command[LTOKEN(i)] == '\0')
					command[ARGSTART(i)] = '\0';
				pop3_cmd_list[i].cmd.pass_arg(p,
					 command + ARGSTART(i));
				break;
			}
			/* Post execution command */
			return;
		}
		i++;
	}
	pop3_printf(p, "%s Invalid command\r\n", pop3_err);
}

/*
 * pop_3collegato(nick) restitutisce il puntatore alla struttura pop3_sess
 *                      dell'utente con nome nick se esiste, altrimenti NULL.
 */
static struct pop3_sess * pop3_collegato(char *ut)
{
        struct pop3_sess *punto, *coll;

        coll = NULL;
        for (punto = sessioni_pop3; !coll && punto; punto = punto->next)
                if ((punto->utente) && (!strcasecmp(ut, punto->utente->nome)))
                                coll = punto;
        return coll;
}

/* Chiude la sessione appartenente all'utente *ut           *
 * ATTENZIONE: e` un po' brutale, magari e` migliorabile... */
void pop3_kill_user(char *ut)
{
        struct pop3_sess *sess;

        sess = pop3_collegato(ut);
        if (sess)
                sess->finished = TRUE;
}

/*****************************************************************************/
/*
 * Implementazione comandi POP3
 */

static void pop3_noop(struct pop3_sess *p)
{
	pop3_printf(p, "%s\r\n", pop3_ok);
}

static void pop3_quit(struct pop3_sess *p)
{
	struct room *room;
	long i, del;

	if (p->stato == AUTHORIZATION) {
		pop3_printf(p, "%s bye\r\n", pop3_ok);
		p->stato = QUIT;
		p->finished = TRUE;
		return;
	}
	/* Transaction state */
	p->stato = UPDATE;

	/* Cancella i messaggi */
	room = room_find(mail_room);
	room->msg = p->mail_list;
	del = 0;
	for (i = 0; i < p->mail_num; i++) {
		if (p->msg[i].mark_deleted) {
			msg_del(room, p->msg[i].id);
			del++;
		}
	}
	room->msg = NULL;

	pop3_printf(p, "%s bye (%ld messages deleted)\r\n", pop3_ok, del);
	/*
        pop3_printf(p, "%s some deleted messages not removed\r\n", pop3_err);
        */

	mail_save1(p->utente, p->mail_list);

	/* Release exclusive-access lock */

	p->stato = QUIT;
	p->finished = TRUE;
}

static void pop3_user(struct pop3_sess *p, char *nome)
{
        struct dati_ut *utente;

	if (p->utente) {
		p->utente = NULL;
		pop3_printf(p, "%s\r\n", pop3_err);
		return;
	}
	utente = trova_utente(nome);
	if ((utente == NULL) || (utente->livello < MINLVL_POP3)) {
		pop3_printf(p, "%s not a valid BBS user\r\n", pop3_err);
		return;
	}
	if (collegato(nome) || pop3_collegato(nome)) {
		pop3_printf(p, "%s %s is already logged in\r\n",pop3_err,nome);
		return;
	}
	p->utente = utente;
	pop3_printf(p, "%s %s has a mailbox here\r\n", pop3_ok, nome);
}

static void pop3_pass(struct pop3_sess *p, char *pass)
{
	if (p->utente == NULL) {
		pop3_printf(p, "%s first send USER command\r\n", pop3_err);
		return;
	}

	if (!check_password(pass, p->utente->password)) {
		pop3_printf(p, "%s invalid password\r\n", pop3_err);
                citta_logf("POP3: Pwd errata per [%s] da [%s]", p->utente->nome,
		     p->host);
		p->utente = NULL;
		return;
        }

	utr_load1(p->utente, p->fullroom, p->room_flags, p->room_gen);
	mail_load1(p->utente, p->mail_list);

	/* p->utente->chiamate++;  SOSTITUIRE, TOGLIERE ? */
	citta_logf("POP3 login [%s] da [%s].", p->utente->nome, p->host);

	p->stato = TRANSACTION;
	pop3_maildrop(p);
	pop3_printf(p, "%s %s's maildrop has %ld messages (%ld octets)\r\n",
		pop3_ok, p->utente->nome, p->mail_num, p->mail_size);
}

static void pop3_stat(struct pop3_sess *p)
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
#ifdef MACOSX
	pop3_printf(p, "%s %ld %lu\r\n", pop3_ok, totnum, totlen);
#else
	pop3_printf(p, "%s %ld %d\r\n", pop3_ok, totnum, totlen);
#endif
}

static void pop3_list(struct pop3_sess *p, char *msg)
{
	int i;
	size_t totlen;
	long num, totnum;

	if (*msg) {
		num = strtol(msg, NULL, 10);
		if ((num <= 0) || (num > p->mail_num)) {
			pop3_printf(p, "%s no such message, only %ld messages in maildrop\r\n", pop3_err, p->mail_num);
			return;
		}
		if (p->msg[num-1].mark_deleted) {
			pop3_printf(p,"%s message marked as deleted\r\n",pop3_err);
			return;
		}
#ifdef MACOSX
		pop3_printf(p, "%s %ld %lu\r\n", pop3_ok, num, p->msg[num-1].size);
#else
		pop3_printf(p, "%s %ld %d\r\n", pop3_ok, num, p->msg[num-1].size);
#endif
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
#ifdef MACOSX
	pop3_printf(p, "%s %ld messages (%lu octets)\r\n", pop3_ok,
	       totnum, totlen);
#else
	pop3_printf(p, "%s %ld messages (%d octets)\r\n", pop3_ok,
	       totnum, totlen);
#endif
	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted)
#ifdef MACOSX
			pop3_printf(p, "%d %lu\r\n", i+1, p->msg[i].size);
#else
			pop3_printf(p, "%d %d\r\n", i+1, p->msg[i].size);
#endif
	}
	pop3_printf(p, ".\r\n");
}

static void pop3_retr(struct pop3_sess *p, char *msg)
{
	long num;
	char * str;

	if (msg == '\0') {
		pop3_printf(p,"%s message number required\r\n", pop3_err);
		return;
	}
	num = strtol(msg, NULL, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		pop3_printf(p, "%s no such message, only %ld messages in maildrop\r\n", pop3_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		pop3_printf(p, "%s message marked as deleted\r\n", pop3_err);
		return;
	}
#ifdef MACOSX
	pop3_printf(p, "%s %d\r\n", pop3_ok, p->msg[num-1].size);
#else
	pop3_printf(p, "%s %d\r\n", pop3_ok, p->msg[num-1].size);
#endif
	/* Send message */
	txt_rewind(p->msg[num-1].msg);
	while ( (str = txt_get(p->msg[num-1].msg))) {
                if (str[0] == '.') /* byte-stuff the line */
                        pop3_printf(p, ".%s\r\n", str);
                else
                        pop3_printf(p, "%s\r\n", str);
        }
	pop3_printf(p, ".\r\n");
}

static void pop3_dele(struct pop3_sess *p, char *msg)
{
	long num;

	if (msg == '\0') {
		pop3_printf(p,"%s message number required\r\n",pop3_err);
		return;
	}
	num = strtol(msg, NULL, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		pop3_printf(p, "%s no such message, only %ld messages in maildrop\r\n", pop3_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		pop3_printf(p, "%s message %ld already deleted\r\n", pop3_err, num);
		return;
	}
	p->msg[num-1].mark_deleted = TRUE;
	pop3_printf(p, "%s message %ld deleted\r\n", pop3_ok, num);
}

static void pop3_rset(struct pop3_sess *p)
{
	int i;

	for (i = 0; i < p->mail_num; i++)
		p->msg[i].mark_deleted = FALSE;
	pop3_printf(p, "%s maildrop has %ld messages (%ld octets)\r\n",
		pop3_ok, p->mail_num, p->mail_size);
}

static void pop3_top(struct pop3_sess *p, char *msg)
{
	long num, lines;
	char *endptr, *str;

	num = strtol(msg, &endptr, 10);
	if ((num <= 0) || (num > p->mail_num)) {
		pop3_printf(p, "%s no such message, only %ld messages in maildrop\r\n", pop3_err, p->mail_num);
		return;
	}
	if (p->msg[num-1].mark_deleted) {
		pop3_printf(p, "%s message marked as deleted\r\n", pop3_err);
		return;
	}
	lines = strtol(endptr, NULL, 10);
	if (lines < 0) {
		pop3_printf(p, "%s number of lines must be non-negative\r\n",
			    pop3_err);
		return;
	}
	pop3_printf(p, "%s top of message follows\r\n", pop3_ok);
	txt_rewind(p->msg[num-1].msg);
	/* Send header */
	while ( (str = txt_get(p->msg[num-1].msg)) && (*str)) {
                if (str[0] == '.') /* byte-stuff the line */
                        pop3_printf(p, ".%s\r\n", str);
                else
                        pop3_printf(p, "%s\r\n", str);
        }
	pop3_printf(p, "\r\n");

	/* Send first 'lines' lines of the body */
	while (lines && (str = txt_get(p->msg[num-1].msg))) {
                if (str[0] == '.') /* byte-stuff the line */
                        pop3_printf(p, ".%s\r\n", str);
                else
                        pop3_printf(p, "%s\r\n", str);
		lines--;
	}
	pop3_printf(p, ".\r\n");
}

static void pop3_uidl(struct pop3_sess *p, char *msg)
{
	int i;
	long num;

	if (*msg) {
		num = strtol(msg, NULL, 10);
		if ((num <= 0) || (num > p->mail_num)) {
			pop3_printf(p, "%s no such message, only %ld messages in maildrop\r\n", pop3_err, p->mail_num);
			return;
		}
		if (p->msg[num-1].mark_deleted) {
			pop3_printf(p, "%s message marked as deleted\r\n",
				    pop3_err);
			return;
		}
		pop3_printf(p, "%s %ld %ld\r\n", pop3_ok, num, p->msg[num-1].id);
		return;
	}
	pop3_printf(p, "%s\r\n", pop3_ok);
	for (i = 0; i < p->mail_num; i++) {
		if (!p->msg[i].mark_deleted)
			pop3_printf(p, "%d %ld\r\n", i+1, p->msg[i].id);
	}
	pop3_printf(p, ".\r\n");

}

static void pop3_apop(struct pop3_sess *p, char *msg)
{
	pop3_printf(p, "%s Not yet supported\r\n", pop3_err);
}

/****************************************************************************/
static void pop3_maildrop(struct pop3_sess *p)
{
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
		if (p->mail_list->num[i] >= lowest)
			p->mail_num++;
	}
	if (p->mail_num == 0)
		return; /* No mail */

	CREATE(p->msg, struct pop3_msg, p->mail_num, TYPE_POP3_MSG);

	i = 0;
	j = 0;

        /*
        while((p->mail_list->num[i] < lowest) && (i < room->data->maxmsg))
                i++;
        */

	for ( ; i < room->data->maxmsg; i++) {
		msgnum = p->mail_list->num[i];
		msgpos = p->mail_list->pos[i];
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

			locnum = p->mail_list->local[i];
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
			txt_put(msg, "");
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
}

static void pop3_free_maildrop(struct pop3_sess *p)
{
        int i;

	for (i = 0; i < p->mail_num; i++)
                txt_free(&p->msg[i].msg);
        Free(p->msg);
}

static void pop3_printf(struct pop3_sess *p, const char *format, ...)
{
	struct coda_testo *coda;
	struct blocco_testo *nuovo;
	va_list ap;
	int size;
#ifdef USE_MEM_STAT
	char *tmp;
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
#ifdef POP3_DEBUG
	citta_logf("POP3 S: %s", nuovo->testo);
#endif
}

#endif /* USE_POP3 */
