/*
 *  Copyright (C) 2002-2005 by Marco Caldarelli and Riccardo Vianello
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
* File: cittaweb.c                                                            *
*       gestione delle richieste HTTP al cittaserver                          *
******************************************************************************/

#include "config.h"

#ifdef USE_CITTAWEB

/* For debug information */
#define DEBUG_CITTAWEB

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "cittaweb.h"
#include "webstr.h"

#include "cittaserver.h"
#include "blog.h"
#include "cml2html.h"
#include "extract.h"
#include "fileserver.h"
#include "macro.h"
#include "memstat.h"
#include "messaggi.h"
#include "metadata.h"
#include "rooms.h"
#include "socket_citta.h"
#include "sysconfig.h"
#include "utente.h"
#include "utility.h"
#include "versione.h"

#ifdef USE_STRING_TEXT
# include "argz.h"
#endif

#ifdef USE_CACHE_POST
#include "cache_post.h"
#endif

struct http_sess *sessioni_http;
int http_maxdesc;
int http_max_num_desc = 5;  /* Massimo numero di connessioni Web simultanee */

/* le due code per il wprintf */
#define CODA_HEADER 0
#define CODA_BODY   1

#define hbold "<STRONG>"
#define hstop "</STRONG>"

/* TODO internazionializzare, e: */
/* Questa roba va sistemata in /share e unificata con client utility.c */
const char *mese_it[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago",
                      "Set", "Ott", "Nov", "Dic"};
const char *mese_fr[] = {"Jan","Fev","Mar","Avr","Mai","Jui","Juil","Aou",
                      "Sep", "Oct", "Nov", "Dec"};
const char *mese_en[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
                      "Sep", "Oct", "Nov", "Dec"};
const char *mese_de[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago",
                      "Set", "Ott", "Nov", "Dic"};


/* Prototipi funzioni in questo file: */
int http_nuovo_descr(int s);
void http_chiudi_socket(struct http_sess *h);
void http_chiusura_cittaweb(int s);
int http_elabora_input(struct http_sess *t);
void http_interprete_comandi(struct http_sess *p, char *command);

static void http_get(struct http_sess *p, char *code, char *command);
static void http_send_lang(struct http_sess *p, const char *page, int lang);
static void http_send_room_list(struct http_sess *p, int lang);
static void http_send_index(struct http_sess *p, char *code, int lang);
static void http_send_userlist(struct http_sess *p, char *code, int lang);
static void http_send_bloglist(struct http_sess *p, char *code, int lang);
static void http_send_profile(struct http_sess *p, char *req, char *code,
                              int lang);
static void http_send_room(struct http_sess *p, char *req, char *code,
                           int lang);
static void http_send_blog(struct http_sess *p, char *req, char *code,
                           int lang);
static void http_send_info(struct http_sess *t, struct room *room);
static void http_send_post(struct http_sess *t, struct room *room);
static void http_send_image(struct http_sess *p, char *req, char *code);
static void http_send_file(struct http_sess *p, char *req, char *code);
static void http_preamble(struct http_sess *q, char *code, int length);
static void http_add_head(struct http_sess *q, const char *title,
			  const char *style, bool send_col);
static void html_close_tags(struct http_sess *t, int *color);
static void html_send_color_def(struct http_sess *p);
static void wprint_datal(struct http_sess *t, long ora);

static void wprintf(struct http_sess *t, int queue, const char *format, ...);
static void http_merge(struct http_sess *p);

/****************************************************************************/

int http_nuovo_descr(int s)
{
	char buf2[LBUF];
	int desc;
	struct http_sess *nuova_ses, *punto;
	socklen_t size;
	int socket_connessi, socket_attivi, i;
	struct sockaddr_in sock;
	struct hostent *da_dove;

	if ( (desc = nuova_conn(s)) < 0)
		return (-1);

	socket_connessi = socket_attivi = 0;

	for (punto = sessioni_http; punto; punto = punto->next)
		socket_connessi++;

	/* Controllo sul numero massimo di connessioni supportabili */

	if (socket_connessi >= http_max_num_desc) {
		sprintf(buf2, "%d Massimo numero di connessioni HTTP raggiunto.\r\n", ERROR);
		scrivi_a_desc(desc, buf2);
		close(desc);
		return(0);
	} else if (desc > http_maxdesc)
			http_maxdesc = desc;

	/* Poi va creata una nuova struttura sessione per il nuovo arrivato */

	CREATE(nuova_ses, struct http_sess, 1, TYPE_HTTP_SESSIONE);

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

	citta_logf("HTTP: Nuova connessione da [%s]", nuova_ses->host);

	/* Inizializziamo la struttura coi dati della nuova sessione */

	nuova_ses->finished = 0;

	nuova_ses->desc = desc;
	nuova_ses->length = 0;
	nuova_ses->conntime = time(0);
        init_coda(&nuova_ses->input);
        init_coda(&nuova_ses->output);
        init_coda(&nuova_ses->out_header);
        iobuf_init(&nuova_ses->iobuf);
	nuova_ses->idle.cicli = 0;
	nuova_ses->idle.warning = 0;
	nuova_ses->idle.min = 0;

	/* Lo appiccichiamo in cima alla lista     */
	nuova_ses->next = sessioni_http;


	sessioni_http = nuova_ses;

	return(0);
}

/*
  Chiude i socket e aggiorna la lista delle sessioni http,
  cancellando la sessione h in input.
*/
void http_chiudi_socket(struct http_sess *h)
{
	struct http_sess *tmp;

	close(h->desc);
	if (h->desc == http_maxdesc)
		--http_maxdesc;

	if (h == sessioni_http)
		sessioni_http = sessioni_http->next;
	else {
		for(tmp = sessioni_http; (tmp->next != h) && tmp ;
		    tmp = tmp->next);

		tmp->next = h->next;
	}
        /* Pulizia */
        flush_coda(&h->input);
        flush_coda(&h->output);
        flush_coda(&h->out_header);
	Free(h);
}

void http_chiusura_cittaweb(int s)
{
	while(sessioni_http)
		http_chiudi_socket(sessioni_http);
        close(s);
}

/*
  Crea in t->coda testo
  una struttura con un campo per ogni linea.
*/
/* TODO Posso unificare con l'elabora_input di socket_citta.c ? */
int http_elabora_input(struct http_sess *t)
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
	if ((questo_giro = read(t->desc, buf+inizio+fino_a,
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
	*(buf + buflen) = '\0';

	/* Se l'input non contiene nessun newline esce senza fare nulla */
	for (i = inizio; !ISNEWL(*(buf + i)); i++)
		if (!*(buf + i))
			return(0);

	/* L'input contiene 1 o piu' newline. Elabora e mette in coda */
	tmp[0] = '\0';
	for (i = 0, k = 0; *(buf + i); ) {
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
                                t->idle.cicli = 0;
                                t->idle.min = 0;
                                t->idle.ore = 0;
			}

			if (flag) /* linea troppo lunga. ignora il resto */
				for ( ; !ISNEWL(*(buf + i)); i++);

			/* trova la fine della linea */
			for ( ; ISNEWL(*(buf + i)); i++)
			        ;

			/* elimina la linea dal'input buffer */
                        memmove(buf, buf+i, buflen-i+1);
                        buflen -= i;

			k = 0;
			i = 0;
		}
	}

        t->iobuf.ilen = buflen;
	return 1;
}

void http_interprete_comandi(struct http_sess *p, char *command)
{
	char req[LBUF], code[LBUF];

        p->length = 0;
	/* solo metodo GET implementato */
	if (!strncmp(command, "GET /", 5))
		http_get(p, code, command+5);
	else {
		/* e' stato richiesto qualcosa che non inizia col metodo GET */
		strncpy(req, command, LBUF < 128 ? LBUF : 128);
		citta_logf("HTTP: metodo non implementato '%s'", req);
		return;
	}

	/*
	  abbiamo - forse :) - costruito il corpo della risposta HTML
	  Calcoliamone la lunghezza e settiamo il Content-Length nel
	  preambolo della risposta
	  Gia' che ci siamo mettiamo pure il codice della risposta HTTP
	*/
        if (code[0]) {
                http_preamble(p, code, p->length);

                /* merging della coda HEADER e BODY */
                http_merge(p);
        }

	/* A questo punto spediamo e chiudiamo la connessione. */
#ifdef DEBUG_CITTAWEB
	citta_log("HTTP: impacchettato e spedito");
#endif
	p->finished = 1;
	dati_server.http_conn_ok++;
}

/****************************************************************************/

/* Implementa il metodo GET per il cittaweb */
static void http_get(struct http_sess *p, char *code, char *command)
{
	int i = 0, j;
	char req[LBUF];
	int lang = 0;

	/* Trova la lingua richiesta */
	for (j = 0; j < HTTP_LANG_NUM; j++)
		if (!strncmp(command, http_lang[j], 2) && command[2] == '/') {
			lang = j;
			i = 3;
			break;
		}

        *index(command,' ') = 0;
        strncpy(req, command+i, LBUF);

        citta_logf("command [%s] - req [%s]", command, req);

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta [%s]", req);
#endif
	if (!strncmp(req, "index", 5) || strlen(req) == 0)
		/* Index: invia la lista delle room.                     */
		http_send_index(p, code, lang);
	else if (!strncmp(req, "profile/", 8))
		/* Profile di un utente */
		http_send_profile(p, under2space(req+8), code, lang);
	else if (!strncmp(req, "userlist", 8))
		/* Lista degli utenti */
		http_send_userlist(p, code, lang);
	else if (!strncmp(req, "blog/", 5))
		/* Invia blog di un utente */
		http_send_blog(p, under2space(req+5), code, lang);
	else if (!strncmp(req, "favicon.ico", 11))
		http_send_image(p, req, code);
	else if (!strncmp(req, "images/", 7))
		http_send_image(p, req+7, code);
	else if (!strncmp(req, "file/", 5))
		http_send_file(p, req+5, code);
        else if (!strncmp(req, "blogs", 5)) {
		/* Lista dei blog attivi */
		http_send_bloglist(p, code, lang);
        } else
		/* altrimenti la richiesta e' di visualizzare una stanza */
		http_send_room(p, under2space(req), code, lang);
}

/* TODO Questo e` uno sporco hack...
   Allo startup il server dovrebbe contare il numero di room pubblicate. */
static int nrighe_index = 3;

/* Cerca le room pubblicate via web e le inserisce nell'indice. */
static void http_send_room_list(struct http_sess *p, int lang)
{
	struct room *room;
	char tmp[ROOMNAMELEN];
	int riga = 0;
	int col = 0;

	for (room = lista_room.first; room; room = room->next) {
		if (room->data->flags & RF_CITTAWEB) {
			strncpy(tmp, room->data->name, ROOMNAMELEN);
			wprintf(p, CODA_BODY,
				"<TD><A href=\"%s/%s\">%s</A></TD>\r\n",
				http_lang[lang], space2under(tmp),
				room->data->name);
			col++;
			if (col == 3) {
				if (riga == 0)
					wprintf(p, CODA_BODY,
"<TD rowspan=\"%d\"><DIV class=\"center\"><H3>%s</H3></DIV></TD></TR><TR>",
						nrighe_index,
						HTML_INDEX_ROOM_LIST[lang]);
				else
					wprintf(p, CODA_BODY, "</TR><TR>\r\n");
				riga++;
				col = 0;
			}
		}
	}
	nrighe_index = riga+1;
	while (col < 3) {
		wprintf(p, CODA_BODY, "<TD>&nbsp;</TD>\r\n");
		col++;
	}
	wprintf(p, CODA_BODY, "</TR>\r\n");
}

static void http_send_lang(struct http_sess *p, const char *page, int lang)
{
	int i;

	wprintf(p, CODA_BODY, "<TABLE class=\"center\" border=\"0\" cellspacing=\"0\"><TR><TD><H5>");
	for (i = 0; i < HTTP_LANG_NUM; i++) {
		if (i == lang)
			wprintf(p, CODA_BODY, "%s ", http_lang_name[i]);
		else
			wprintf(p, CODA_BODY, "<A href=\"%s/%s\">%s</A> ",
				http_lang[i], page, http_lang_name[i]);
	}
	wprintf(p, CODA_BODY, "</H5></TD></TR></TABLE>\r\n");
}

/* Invia index.html, con la lista della room             */
static void http_send_index(struct http_sess *p, char *code, int lang)
{
        struct dati_ut *autore;
        struct tm *tmst;
        char roomlink[LBUF];

#ifdef DEBUG_CITTAWEB
	citta_log("HTTP invio index");
#endif

	/* build the header of the reply */
	strcpy(code, "200 OK");
	wprintf(p, CODA_BODY, HTML_HEADER);
	http_add_head(p, "Cittadella BBS - Indice Room.",HTML_INDEX_CSS,false);
	wprintf(p, CODA_BODY, HTML_INDEX_BODY, HTML_INDEX_TITLE[lang],
		HTML_INDEX_PREAMBLE[lang]);
	http_send_lang(p, "index", lang);

	wprintf(p, CODA_BODY, "<P><BR></P><TABLE class=\"rooms\" border=\"0\" cellspacing=\"1\"><COLGROUP><COL width=\"31%%\"><COL width=\"31%%\"><COL width=\"31%%\"><COL width=\"7%%\"></COLGROUP><TR>");

	http_send_room_list(p, lang);

	//wprintf(p, CODA_BODY, "</DIV><DIV id=\"side\">");
	wprintf(p, CODA_BODY,
                "<TR><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD>"
                "<TD><DIV class=\"center\"><H3><A href=\"%s/userlist\">%s</A></H3></DIV></TD>"
                "<TR><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD>"
                "<TD><DIV class=\"center\"><H3><A href=\"%s/blogs\">%s</A></H3></DIV></TD>"
                "</TR>",
		http_lang[lang], HTML_INDEX_USERLIST[lang], http_lang[lang], HTML_INDEX_BLOGLIST[lang]);

        wprintf(p, CODA_BODY, "<TR><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD></TR></TABLE>");

        /* Invia l'informazione sull'ultimo post inserito */
	wprintf(p, CODA_BODY, "<DIV id=\"lastpost\">");

        if (lastpost_room[0] == 0)
                wprintf(p, CODA_BODY, "<SPAN class=\"lastmsg\">%s</SPAN>",
                        HTML_INDEX_NONEW[lang]);
        else if (!strcmp(lastpost_room, mail_room)) {
                tmst = localtime(&lastpost_time);
                wprintf(p, CODA_BODY, "<SPAN class=\"lastmsg\">%s</SPAN><BR><SPAN class=\"msgh\">(%2.2d/%2.2d/%4d @%2.2d:%2.2d)</SPAN>", HTML_INDEX_LASTMAIL[lang],
                        tmst->tm_mday, tmst->tm_mon,
                        1900 + tmst->tm_year, tmst->tm_hour,
                        tmst->tm_min);
        } else {
                autore = trova_utente_n(lastpost_author);
                tmst = localtime(&lastpost_time);
                if (autore)
                        wprintf(p, CODA_BODY, "<SPAN class=\"lastmsg\">%s </SPAN><BR><SPAN class=\"msgh\"><B>%s</B>, %2.2d/%2.2d/%4d @%2.2d:%2.2d - da <B>%s</B></SPAN><BR>", HTML_INDEX_LASTMSG[lang],
                                lastpost_room, tmst->tm_mday, tmst->tm_mon,
                                1900 + tmst->tm_year, tmst->tm_hour,
                                tmst->tm_min,  autore->nome);
                else
                        wprintf(p, CODA_BODY, "<SPAN class=\"lastmsg\">%s</SPAN><BR><SPAN class=\"msgh\"><B>%s</B>, %2.2d/%2.2d/%4d @%2.2d:%2.2d - da <B>un utente anonimo</B>&nbsp;</SPAN><BR>", HTML_INDEX_LASTMSG[lang],
                                lastpost_room, tmst->tm_mday, tmst->tm_mon,
                                1900 + tmst->tm_year, tmst->tm_hour,
                                tmst->tm_min);
                if (lastpost_webroom) {
                        strcpy(roomlink, lastpost_room);
                        wprintf(p, CODA_BODY, "<SPAN text-align=\"center\"><A href=\"/%s#%ld\"><SPAN class=\"postlink\">%s</SPAN></A></SPAN>",
                                space2under(roomlink), lastpost_locnum,
                                HTML_INDEX_READIT[lang]);
                } else
                        wprintf(p, CODA_BODY, "<SPAN class=\"notavail\">%s</SPAN>",
                                HTML_INDEX_NOTAVAIL[lang]);
        }
        wprintf(p, CODA_BODY, "</DIV><DIV>&nbsp;<BR></DIV>\r\n");
	wprintf(p, CODA_BODY, HTML_TAILINDEX, BBS_HOST, REMOTE_PORT);
}

/* Invia la lista degli utenti della BBS */
static void http_send_userlist(struct http_sess *p, char *code, int lang)
{
	struct tm *tmst;
        struct lista_ut *punto;

	/* TODO: implement lang */
	IGNORE_UNUSED_PARAMETER(lang);

	strcpy(code, "200 OK");
	wprintf(p, CODA_BODY, HTML_HEADER);

	http_add_head(p, "Cittadella BBS - Lista Utenti.", HTML_USERLIST_CSS,
                      false);

	wprintf(p, CODA_BODY, "<BODY>\r\n<P class=\"titolo\">Welcome to CittaWeb</p>\r\n");
	wprintf(p, CODA_BODY, "<P class=\"userlist\"><PRE>\r\n");
	wprintf(p, CODA_BODY, "\r\nNome                      Lvl Last Call   Calls   Post   Mail   Xmsg   Chat\r\n");
	wprintf(p, CODA_BODY, "------------------------- --- ---------- ------- ------ ------ ------ ------\r\n");

        for (punto = lista_utenti; punto; punto = punto->prossimo) {
			tmst = localtime(&punto->dati->lastcall);
			wprintf(p, CODA_BODY, "<SPAN class=\"username\">%-25s </SPAN><SPAN class=\"level\">%3d </SPAN>",
                                punto->dati->nome, punto->dati->livello);
			wprintf(p, CODA_BODY, "<SPAN class=\"data\"><STRONG>%02d</STRONG>/<STRONG>%02d</STRONG>/<STRONG>%4d</STRONG> </SPAN>",
                                tmst->tm_mday, tmst->tm_mon+1,
                                1900+tmst->tm_year);
			wprintf(p, CODA_BODY, "<SPAN class=\"chiamate\">%6d </SPAN>", punto->dati->chiamate);
			wprintf(p, CODA_BODY, "<SPAN class=\"post\">");
			wprintf(p, CODA_BODY, "%6d ", punto->dati->post);
			wprintf(p, CODA_BODY, "</SPAN><SPAN class=\"mail\">");
			wprintf(p, CODA_BODY, "%6d ", punto->dati->mail);
			wprintf(p, CODA_BODY, "</SPAN><SPAN class=\"xmsg\">");
			wprintf(p, CODA_BODY, "%6d ", punto->dati->x_msg);
			wprintf(p, CODA_BODY, "</SPAN><SPAN class=\"chat\">");
			wprintf(p, CODA_BODY, "%6d</SPAN>\r\n", punto->dati->chat);
			/*
                        wprintf(p, CODA_BODY, "  <A href=\"http://%s:%d/profile?%s\">profile</A>\r\n", HTTP_HOST, HTTP_PORT, punto->dati->nome);
                        */
		}

	wprintf(p, CODA_BODY, "</PRE>\r\n");

	wprintf(p, CODA_BODY, HTML_TAIL, BBS_HOST, REMOTE_PORT);

	/* Aggiorna statistiche */
	dati_server.http_userlist++;
}

static void http_send_bloglist(struct http_sess *p, char *code, int lang)
{
	struct tm *tmst;
        blog_t *blog;
        const char *last_poster;
        struct dati_ut *ut;

	strcpy(code, "200 OK");
	wprintf(p, CODA_BODY, HTML_HEADER);

	http_add_head(p, "Cittadella BBS - Blogs.", HTML_BLOGLIST_CSS,
                      false);

	wprintf(p, CODA_BODY, "<BODY>\r\n<P class=\"titolo\">Welcome to CittaWeb</p>\r\n");
	wprintf(p, CODA_BODY, "<P class=\"blogs\"><PRE>\r\n");
	wprintf(p, CODA_BODY, "\r\nNome                      # post Ultimo post\r\n");
	wprintf(p, CODA_BODY, "------------------------- ------ -----------\r\n");

        for (blog = blog_first; blog; blog = blog->next) {
                ut = trova_utente_n(blog->msg->last_poster);
                if (ut)
                        last_poster = ut->nome;
                else
                        last_poster = empty_string;

                tmst = localtime(&blog->room_data->ptime);

                if (blog->user->flags[7] & UT_BLOG_WEB)
                        wprintf(p, CODA_BODY,
                                "<SPAN class=\"user\"><A href=\"%s/blog/%s\">%-25s </A></SPAN>",
                                http_lang[lang], space2under(blog->user->nome),
                                blog->user->nome);
                else
                        wprintf(p, CODA_BODY, "<SPAN class=\"user\">%-25s </SPAN>", blog->user->nome);
                wprintf(p, CODA_BODY, "<SPAN class=\"numpost\">%6d </SPAN>",
                        blog->msg->highloc);
                wprintf(p, CODA_BODY, "<SPAN class=\"LastPost\">@%02d:%02d %02d/%02d/%4d  by %s</SPAN>\r\n",
                        tmst->tm_hour, tmst->tm_min, tmst->tm_mday,
                        tmst->tm_mon+1, 1900+tmst->tm_year, last_poster);
        }
	wprintf(p, CODA_BODY, "</PRE>\r\n");
	wprintf(p, CODA_BODY, HTML_TAIL, BBS_HOST, REMOTE_PORT);

	/* Aggiorna statistiche TODO: usare bloglist */
	dati_server.http_userlist++;
}

static void http_send_profile(struct http_sess *p, char *req, char *code,
                              int lang)
{
        struct dati_ut *utente;
	char buf[LBUF];

	/* TODO: implement lang */
	IGNORE_UNUSED_PARAMETER(lang);

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta profile %s", req);
#endif
        utente = trova_utente(req);
        if (utente == NULL) {
		/* Invalid request */
		strcpy(code, "404 Not Found");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_404);
	} else {
		strcpy(code,"200 OK");
		wprintf(p, CODA_BODY, HTML_HEADER);

		sprintf(buf, "Cittadella BBS - Profile %s", req);
		http_add_head(p, buf, NULL, false);

		wprintf(p, CODA_BODY, "<BODY>\r\n<TT>Welcome to CittaWeb<p>\r\n");
		wprintf(p, CODA_BODY, "<PRE>\r\n");
		wprintf(p, CODA_BODY, "</PRE>\r\n");
		wprintf(p, CODA_BODY, "<P></TT>\r\n");
                wprintf(p, CODA_BODY, HTML_TAIL, BBS_HOST, REMOTE_PORT);

		/* Aggiorna statistiche */
		dati_server.http_profile++;
	}
}

static void http_send_room(struct http_sess *p, char *req, char *code,
                           int lang)
{
	char buf[LBUF];
	struct room *room;

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta room %s", req);
#endif
	/* CHECK THE ROOM AND SEND */
	room = room_find(req);
	if (room == NULL) {
		/* Invalid request */
		strcpy(code, "404 Not Found");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_404);
	} else if ((room->data->flags & RF_CITTAWEB) == 0) {
		/* Room not available from web */
		strcpy(code, "403 Forbidden");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_NOT_AVAIL);
	} else {
		strcpy(code,"200 OK");
		wprintf(p, CODA_BODY, HTML_HEADER);

		sprintf(buf, "Cittadella BBS - Room %s>", req);
                http_add_head(p, buf, HTML_ROOM_CSS, true);

                wprintf(p, CODA_BODY, HTML_INDEX_BODY, buf,
                        HTML_ROOM_PREAMBLE[lang]);
                http_send_lang(p, req, lang);

		wprintf(p, CODA_BODY, "<PRE>\r\n");
		/* add to the queue the room info */
		http_send_info(p, room);
		/* add to the queue the contents of the room */
		http_send_post(p, room);
		wprintf(p, CODA_BODY, "</PRE>\r\n");

                wprintf(p, CODA_BODY, HTML_TAIL, BBS_HOST, REMOTE_PORT);

		/* Aggiorna statistiche */
		dati_server.http_room++;
	}
}

/* Invia il blog di un utente */
static void http_send_blog(struct http_sess *p, char *req, char *code, int lang)
{
	char buf[LBUF];
	struct room room;
        blog_t *blog;

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta blog %s", req);
#endif

        blog = blog_find(trova_utente(req));

	if (blog == NULL) {
		/* Invalid request */
		strcpy(code, "404 Not Found");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_404);
                return;
	}

        if (!(blog->user->flags[7] & UT_BLOG_WEB)) {
		/* Blog not available from web */
		strcpy(code, "403 Forbidden");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_NOT_AVAIL);
                return;
	}

        room.data = blog->room_data;
        room.msg = blog->msg;

        strcpy(code,"200 OK");
        wprintf(p, CODA_BODY, HTML_HEADER);

        sprintf(buf, "Cittadella BBS - %s's Blog", req);
        http_add_head(p, buf, HTML_ROOM_CSS, true);

        wprintf(p, CODA_BODY, HTML_INDEX_BODY, buf, HTML_ROOM_PREAMBLE[lang]);
        http_send_lang(p, req, lang);

        wprintf(p, CODA_BODY, "<PRE>\r\n");
        /* add to the queue the room info */
        http_send_info(p, &room);
        /* add to the queue the contents of the room */
        http_send_post(p, &room);
        wprintf(p, CODA_BODY, "</PRE>\r\n");

        wprintf(p, CODA_BODY, HTML_TAIL, BBS_HOST, REMOTE_PORT);

        /* Aggiorna statistiche */
        dati_server.http_blog++;
}

/* Invia il blog di un utente */
/* TODO aggiungere controllo sul path (ie file permessi) */
static void http_send_image(struct http_sess *p, char *req, char *code)
{
        char filename[LBUF];
        FILE *fp;
        struct stat filestat;
        char *image;
        size_t len;

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta immagine %s", req);
#endif

        snprintf(filename, 64, "%s/%s", IMAGES_DIR, req);
        fp = fopen(filename, "r");
        if (fp == NULL) {
		strcpy(code, "403 Forbidden");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_NOT_AVAIL);
                return;
	}

        fstat(fileno(fp), &filestat);

        len = filestat.st_size;
        citta_logf("Image Size = %d", (int)len);

        CREATE(image, char, len, TYPE_CHAR);

        fread(image, filestat.st_size, 1, fp);
        fclose(fp);

        p->length += len;
        metti_in_coda_bin(&p->output, image, len);
        code[0] = 0; /* no preamble */

        Free(image);
}

/* Invia un file in allegato a un post */
/* TODO sistemare i messaggi di errore */
static void http_send_file(struct http_sess *p, char *req, char *code)
{
        size_t len;
        char *ptr, *data;

#ifdef DEBUG_CITTAWEB
	citta_logf("HTTP: richiesta file %s", req);
#endif
        ptr = index(req, '/');
        if (ptr == NULL) {
		strcpy(code, "403 Forbidden");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_NOT_AVAIL);
                return;
	}
        *ptr = 0;
        ptr++;

        citta_logf("filenum %ld, filename %s", strtol(req, NULL, 10), ptr);

        len  = fs_download(strtol(req, NULL, 10), ptr, &data);
        if (len == 0) {
		strcpy(code, "403 Forbidden");
		wprintf(p, CODA_BODY, HTML_HEADER);
		wprintf(p, CODA_BODY, HTML_NOT_AVAIL);
                return;
	}

        p->length += len;
        metti_in_coda_bin(&p->output, data, len);
        code[0] = 0; /* no preamble */

        Free(data);
}

/* Inserisce le info della room nella body HTML */
static void http_send_info(struct http_sess *t, struct room *room)
{
	struct room_data *rd;
	struct dati_ut *ra;
	FILE *fp;
        char filename[LBUF];
        char buf[LBUF], raide[LBUF];
        int color;
        Metadata_List mdlist;

        /* <SAMP> */
	wprintf(t, CODA_BODY, "<SPAN class=\"info\"><BR>\r\n");
	rd = room->data;
	ra = trova_utente_n(rd->master);
	if (ra)
		strcpy(raide, ra->nome);
	else
		strcpy(raide, "nessuno");
	wprintf(t, CODA_BODY, " Room '%s%s%s', creata il ", hbold, rd->name,
		hstop);
	wprint_datal(t, rd->ctime);
	wprintf(t, CODA_BODY, ".\r\n Room Aide: %s%s%s - Livello minimo di accesso %s%d%s, per scrivere %s%d%s.\r\n",
		hbold, raide, hstop, hbold, rd->rlvl, hstop, hbold, rd->wlvl,
		hstop);
	/* #ifdef DETAILED_ROOM_INFO */
	if (rd->flags & RF_INVITO)
		wprintf(t, CODA_BODY, " &Egrave; una room ad invito.\r\n");
	if (rd->flags & RF_GUESS)
		wprintf(t, CODA_BODY, " &Egrave; una guess room.\r\n");
	if (rd->flags & RF_ANONYMOUS)
		wprintf(t, CODA_BODY, " I post di questa room sono anonimi.\r\n");
	else if (rd->flags & RF_ASKANONYM)
		wprintf(t, CODA_BODY, " &Egrave; possibile lasciare messaggi anonimi in questa room.\r\n");
	if (rd->flags & RF_SPOILER)
		wprintf(t, CODA_BODY, " &Egrave; possibile immettere messaggi con spoiler.\r\n");
	if (rd->flags & RF_SUBJECT)
		wprintf(t, CODA_BODY, " I subject sono abilitati");
	else
		wprintf(t, CODA_BODY, " I subject non sono abilitati");
	if (rd->flags & RF_REPLY)
		wprintf(t, CODA_BODY, " e i Reply sono abilitati.\r\n");
	else
		wprintf(t, CODA_BODY, " e i Reply sono disabilitati.\r\n");
	if (rd->flags & RF_POSTLOCK) {
		wprintf(t, CODA_BODY, " Un solo utente alla volta pu&oacute; scrivere un messaggio.");
		if (rd->flags & RF_L_TIMEOUT)
			wprintf(t, CODA_BODY, " C'&egrave; un tempo limite per scrivere i post.");
	}
	if (rd->flags & RF_SONDAGGI)
		wprintf(t, CODA_BODY, " Tutti gli utenti possono postare messaggi in questa room.\r\n");
	if (rd->flags & RF_CITTAWEB)
		wprintf(t, CODA_BODY, " Questa room &egrave; accessibile via web.\r\n");
	if (rd->mtime == 0)
		wprintf(t, CODA_BODY, " Questa room non ha subito modifiche dalla sua creazione.\r\n");
	else {
		wprintf(t, CODA_BODY, " Ultima modifica della room il ");
		wprint_datal(t, rd->mtime);
		wprintf(t, CODA_BODY, "\r\n");
	}
	if (rd->ptime) {
		wprintf(t, CODA_BODY, " Ultimo post lasciato nella room il ");
		wprint_datal(t, rd->ptime);
		wprintf(t, CODA_BODY, "\r\n");
	}
	/* #endif */
	if (rd->highloc) {
                switch(rd->highloc) {
                case 0:
			wprintf(t, CODA_BODY, " Nessun messaggio &egrave; stato lasciato in questa room.\r\n\r\n");
			break;
                case 1:
			wprintf(t, CODA_BODY, " &Egrave; stato lasciato un messaggio dalla sua creazione.\r\n\r\n");
			break;
                default:
			wprintf(t, CODA_BODY, " Sono stati lasciati %s%ld%s messaggi dalla sua creazione.\r\n\r\n", hbold, rd->highloc, hstop);
                }
	}
        wprintf(t, CODA_BODY, "</SPAN>\r\n");

        /* Descrizione della room */
	sprintf(filename, "%s/%ld", ROOMS_INFO, rd->num);
	if ( (fp = fopen(filename, "r")) == NULL) {
		wprintf(t, CODA_BODY, " Non &egrave; attualmente presente una descrizione di questa room.\r\n\r\n");
		return;
	}
	wprintf(t, CODA_BODY, "<SPAN class=\"roomdesctit\">");
	wprintf(t, CODA_BODY, " Descrizione della room:\r\n</SPAN>\r\n");
	wprintf(t, CODA_BODY, "<BR><SPAN class=\"roomdesc\">");
	color = 6; /* TODO usare una costante!! */
        md_init(&mdlist);
	while (fgets(buf, LBUF, fp) != NULL) {
		char *outstr;
		outstr = cml2html_max(buf, NULL, 0, &color, &mdlist);
		wprintf(t, CODA_BODY, "%s", outstr);
		Free(outstr);
	}
        md_free(&mdlist);
	wprintf(t, CODA_BODY, "</SPAN>\r\n");
	html_close_tags(t, &color);
	fclose(fp);
}

static void http_send_post(struct http_sess *t, struct room *room)
{
	long fmnum, msgnum, msgpos, lowest, locnum, reply_num, i;
	char header[LBUF], err, *riga;
	char autore[LBUF], room_name[LBUF], subj[LBUF],
		reply_to[LBUF], h[LBUF];
	char *subject;
	long flags, ora;
        int color;
#ifdef USE_STRING_TEXT
	char *txt;
	size_t len;
	/* int nrighe; */
#else
	struct text *txt;
#endif
#ifdef USE_CACHE_POST
	char *header1;
# ifndef USE_STRING_TEXT
	struct text *txt1;
# endif
#endif
        Metadata_List mdlist;

	fmnum = room->data->fmnum;
	lowest = fm_lowest(room->data->fmnum);

	/* Ultimi CITTAWEB_POSTS post */
	if ( (i = room->data->maxmsg - CITTAWEB_POSTS) < 0)
		i = 0;
	while((room->msg->num[i] < lowest) && (i < room->data->maxmsg))
		i++;

        wprintf(t, CODA_BODY, "\r\n");

	if ((i < 0) || (i >= room->data->maxmsg)) {
		wprintf(t, CODA_BODY, "Attualmente non ci sono post in questa room.\r\n");
		return;
	}

	while (i < room->data->maxmsg) {
                subject = NULL;
		msgnum = room->msg->num[i];
		msgpos = room->msg->pos[i];
                /* NON USO FM multipli
		if (room->msg->fmnum)
		        fmnum = room->msg->fmnum[i];
                */
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
		        /* nrighe = argz_count(txt, len); */
# endif
			strcpy(header, header1);
#endif

			locnum = room->msg->local[i];
			extract(room_name, header, 1);
			ora = extract_long(header, 3);
			extract(subj, header, 4);
			flags = extract_long(header, 5);
			subject = str2html(subj);

			/* costruisce l'header del messaggio */
			wprintf(t, CODA_BODY, "<A name=\"%ld\"></A>", locnum);
			wprintf(t, CODA_BODY, "<SPAN class=\"msghdr\">");
			if (flags & MF_ANONYMOUS)
				wprintf(t, CODA_BODY, "***");
			else {
				extract(autore, header, 2);
				strdate(h, ora);
				wprintf(t, CODA_BODY,
                                        " %s%s%s, %s - msg #%s%ld%s da %s%s%s",
					hbold, room_name, hstop, h, hbold,
                                        locnum, hstop, hbold, autore, hstop);
			}
			if (flags & MF_RH)
				wprintf(t, CODA_BODY, " (%s Message)",
					NOME_ROOMHELPER);
			else if (flags & MF_RA)
				wprintf(t, CODA_BODY, " (%s Message)",
					NOME_ROOMAIDE);
			else if (flags & MF_AIDE)
				wprintf(t, CODA_BODY, " (%s Message)",
					NOME_AIDE);
			else if (flags & MF_SYSOP)
				wprintf(t, CODA_BODY, " (%s Message)",
					NOME_SYSOP);
			wprintf(t, CODA_BODY, "</SPAN>\r\n");

			/* costruisce il tipo di messaggio */
			if ( (reply_num = extract_long(header, 8))) {
				/* E' un reply */
				wprintf(t,CODA_BODY,"<SPAN class=\"subj\">");
				extract(reply_to, header, 7);

				if (subject[0])
					wprintf(t, CODA_BODY, " Reply : <STRONG>%s</STRONG>, msg #<STRONG>%ld</STRONG> da <STRONG>%s</STRONG>\r\n",
						subject, reply_num, reply_to);
				else {
					wprintf(t, CODA_BODY,
						" Reply al Messaggio #<STRONG>%ld</STRONG> da <STRONG>%s</STRONG>\r\n",
						reply_num, reply_to);
				}
				wprintf(t, CODA_BODY, "</SPAN>");
			} else if (subject[0]) {
				wprintf(t,CODA_BODY,"<SPAN class=\"subj\">");
                                wprintf(t, CODA_BODY, " Subject : <STRONG>%s</STRONG>\r\n",
					subject);
				wprintf(t, CODA_BODY, "</SPAN>");
			}

			/* ora il body del messaggio */
			wprintf(t, CODA_BODY, "\r\n<SPAN class=\"msgbdy\">");
			color = 7; /* TODO usare una costante */
#ifdef USE_STRING_TEXT
                        md_init(&mdlist);
			for (riga=txt; riga; riga=argz_next(txt, len, riga)) {
			 	char *outstr;
				outstr = cml2html_max(riga, NULL, 0, &color,
                                                      &mdlist);
				wprintf(t, CODA_BODY, "%s\r\n", outstr);
				Free(outstr);
			}
                        md_free(&mdlist);
			html_close_tags(t, &color);
#else
			txt_rewind(txt);
			while (riga = txt_get(txt), riga)
				wprintf(t, CODA_BODY, "%s\r\n", OK, riga);
#endif
			wprintf(t, CODA_BODY, "</SPAN>");

#ifndef USE_CACHE_POST
			txt_free(&txt);
#endif
		}
		i++;
                if (subject)
                        Free(subject);
	}
}


/****************************************************************************/

/* The HTTP preamble */
static void http_preamble(struct http_sess *q, char *code, int length)
{
	/* HTTP 1.1, hopefully RFC2616 */

	long ct;
	char *tmstr;

	wprintf(q, CODA_HEADER, "HTTP/1.1 %s\r\n", code);

	ct = time(0);
	tmstr = asctime(localtime(&ct));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	wprintf(q, CODA_HEADER, "Date: %s\r\n", tmstr);

	wprintf(q, CODA_HEADER, "Server: CittaWeb/%s (Unix) Cittadella rulez\r\n",
		SERVER_RELEASE);

	wprintf(q, CODA_HEADER, "Connection: close\r\n"
		"Content-length: %d\r\n"
		"Content-type: text/html; charset=iso-8859-1\r\n"
                "Pragma: no-cache\r\n"
                "Cache-Control: no-cache\r\n"
                "Expires: -1\r\n"
		"\r\n", length);
}

/* Insert HTML header for the page, with title *title and style *style */
static void http_add_head(struct http_sess *q, const char *title,
			  const char *style, bool send_col)
{
	wprintf(q, CODA_BODY,
		"<HEAD><TITLE>Cittadella/UX %s - %s</TITLE>\r\n",
		SERVER_RELEASE, title);
	wprintf(q, CODA_BODY,
		"<BASE href=\"http://%s:%d/\">\r\n", HTTP_HOST, HTTP_PORT);

	if (style) {
                wprintf(q, CODA_BODY, HTML_START_CSS);
                wprintf(q, CODA_BODY, style);
                if (send_col)
                        html_send_color_def(q);
                wprintf(q, CODA_BODY, HTML_END_CSS);
        }

	wprintf(q, CODA_BODY,
"<META http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\r\n"
"<META name=\"Generator\" content=\"Cittadella/UX/%s\">\r\n"
"<META name=\"Author\" content=\"Cittadella BBS\">\r\n"
"<META name=\"Description\" content=\"Cittadella BBS Preview\">\r\n"
"</HEAD>\r\n", SERVER_RELEASE);
}

/* Chiude i tag aperti dall'interprete cml2html */
static void html_close_tags(struct http_sess *t, int *color)
{
        char buf[LBUF];

        buf[0] = 0;
        htmlstr_close_tags(buf, *color);
        if (*buf)
                wprintf(t, CODA_BODY, "%s", buf);
        *color = 7; /* TODO usare una costante */
        wprintf(t, CODA_BODY, "\r\n");
}


const char col_code[] = {'K', 'R', 'G', 'Y', 'B', 'M', 'C', 'W'};
const char *html_colors[] = {
	"Black\0", "Red\0",     "Lime\0", "Yellow\0",
	"Blue\0",  "Fuchsia\0", "Aqua\0", "Silver\0"};

static void html_send_color_def(struct http_sess *p)
{
        int i, j;

        for (i = 0; i < 8; i++)
                for (j = 0; j < 8; j++)
                        wprintf(p, CODA_BODY,
                                "SPAN.%c%c{color:%s;background-color:%s}\r\n",
                                col_code[i], col_code[j],
                                html_colors[i], html_colors[j]);
}

/* Mette in coda data in italiano con mese lungo */
static void wprint_datal(struct http_sess *t, long ora)
{
	extern char *mese_esteso[];
        struct tm *tmst;

	tmst = localtime(&ora);
	wprintf(t, CODA_BODY, "%d %s %d alle %2.2d:%2.2d", tmst->tm_mday,
		     mese_esteso[tmst->tm_mon], 1900+tmst->tm_year,
		     tmst->tm_hour, tmst->tm_min);
}

/****************************************************************************/
/*
  se coda = CODA_HEADER -> header della risposta HTTP
  se coda = CODA_BODY   -> corpo della risposta HTTP
                           e viene incrementato t->length (lungh body)
*/
static void wprintf(struct http_sess *t, int queue, const char *format, ...)
{
	va_list ap;
	int size;
#ifdef USE_MEM_STAT
	char *tmp;
#endif

	va_start(ap, format);
#ifdef USE_MEM_STAT
	/* TODO Non e' molto bello... si puo' fare di meglio... */
	size = vasprintf(&tmp, format, ap);
#else
	size = vasprintf(&nuovo->testo, format, ap);
#endif
	va_end(ap);

	switch (queue) {
	case CODA_BODY:
                metti_in_coda(&t->output, tmp, size);
		t->length += size;
		break;
	case CODA_HEADER:
                metti_in_coda(&t->out_header, tmp, size);
		break;
	}
#ifdef USE_MEM_STAT
        free(tmp);
#endif
}

/* merge della coda out_header con la coda output della sessione http. */
static void http_merge(struct http_sess *p)
{
	merge_code(&p->out_header, &p->output);
	p->output.partenza = p->out_header.partenza;
	p->output.termine = p->out_header.termine;
	p->out_header.partenza = NULL;
	p->out_header.termine = NULL;
}

#endif /* USE_CITTAWEB */

/*
Color names and sRGB values
Black = "#000000" Green = "#008000"
Silver = "#C0C0C0" Lime = "#00FF00"
Gray = "#808080" Olive = "#808000"
White = "#FFFFFF" Yellow = "#FFFF00"
Maroon = "#800000" Navy = "#000080"
Red = "#FF0000" Blue = "#0000FF"
Purple = "#800080" Teal = "#008080"
Fuchsia = "#FF00FF" Aqua = "#00FFFF"
*/
