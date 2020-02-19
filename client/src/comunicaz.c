/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File : comunicaz.c                                                        *
*        broadcasts, express e cazzabubboli vari...                         *
****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>

#include "cittaclient.h"
#include "cittacfg.h"
#include "ansicol.h"
#include "cittacfg.h"
#include "cml.h"
#include "comunicaz.h"
#include "conn.h"
#include "edit.h"
#include "errore.h"
#include "extract.h"
#include "friends.h"
#include "inkey.h"
#include "macro.h"
#include "prompt.h"
#include "strutt.h"
#include "tabc.h"
#include "user_flags.h"
#include "utility.h"

struct xmsg_t {
	char autore[MAXLEN_UTNAME];
        struct name_list * dest;
	int ore, min;
	int flags;
	struct text *txt;
};

static struct xmsg_t *xlog;
static struct xmsg_t xmsg_tmp;          /* Dati temporanei sull'X in arrivo */
static int chat_help = true;            /* 1 se va lasciato help chat       */

/* Prototipi delle funzioni statiche in questo file */
static void xlog_scroll(void);
static void xlog_newt(struct text *txt, const char *name,
                      struct name_list *dest, int ore, int min, int type);
static void xlog_xmsg(struct xmsg_t *xmsg, struct text *txt, const char *name,
                      struct name_list *dest, int ore, int min, int type);
static void xlog_print(int num);
static void xlog_print_all(void);
static void xlog_header(struct xmsg_t x);
static void xlog_list(void);
#ifdef LOCAL
static void xlog_dump(struct xmsg_t *xmsg, int autolog);
static void xlog_header_dump(FILE *fp, struct xmsg_t x);
#endif

/****************************************************************************/
/*   EXPRESS-MESSAGES E BROADCASTS                                          */
/****************************************************************************/
/*
 * Invia un Broadcast
 */
void broadcast(void)
{
        char buf[LBUF];
	struct text *txt;

        serv_puts("BRDC");
        serv_gets(buf);
        if (buf[0] != '2') {
		print_err_x(buf);
                return;
        }
	txt = txt_create();
	if (get_text_col(txt, serverinfo.maxlineebx, 79, 1) > 0)
		send_text(txt);
        serv_puts("BEND");
        serv_gets(buf);
        if (buf[0] != '2')
		print_err_x(buf);
	else {
		printf(_("Broadcast inviato.\n"));
		xlog_put_out(txt, NULL, XLOG_BOUT);
	}
	txt_free(&txt);
}

/*
 * Invia un express-message.
 */
void express(char *rcpt)
{
	struct name_list *nl;
	struct text *txt;
        char buf[LBUF];
        const char *caio;
        int i, inviati, scollegati;

	setcolor(C_X);

        serv_puts("XMSG");
        serv_gets(buf);
        if (buf[0] != '2') {
		print_err_x(buf);
                return;
        }

	if (rcpt && (*rcpt != '\0')) {
		cml_printf(_("\n*** <b>Follow-up</b>: EXpress message per <b>%s</b>\n"), rcpt);
                serv_putf("DEST %s", rcpt);
                serv_gets(buf);
                if (buf[0] != '2') {
                        print_err_x(buf);
                        serv_puts("XEND");
                        serv_gets(buf);
                        return;
                }
                nl = nl_create("");
                nl_insert(nl, rcpt, 1);
	} else {
		if (rcpt)
			putchar('\n');
		nl = get_multi_recipient(TC_MODE_X);
                if (nl == NULL) {
                        cml_printf(_("Messaggio annullato.\n"));
                        serv_puts("XEND");
                        serv_gets(buf);
                        return;
                }
	}

	txt = txt_create();
	if (uflags[0] & UT_LINE_EDT_X) {
		if (get_textl_cursor(txt, 78, serverinfo.maxlineebx - 2,
				     false) > 0)
			send_text(txt);
	} else if (get_text_col(txt, serverinfo.maxlineebx, 79, 1) > 0)
		send_text(txt);

	serv_puts("XEND");
	serv_gets(buf);
	if (buf[0] != '2') {
		print_err_x(buf);
		txt_free(&txt);
		nl_free(&nl);
		return;
	}

	i = 0;
	serv_gets(buf);
	while (buf[0] != '0') {
		caio = nl_get(nl, i);
		if (buf[0] != '2') {
			print_err_x(buf);
                        nl_setid(nl, i, false);
                } else {
                        printf(_("eXpress message inviato a %s.\n"), caio);
                        if ((!(uflags[3] & UT_X) &&
                             (!(uflags[3] & UT_XFRIEND) || !is_friend(caio)))
                            || is_enemy(caio)) {
                                push_color();
                                setcolor(C_X);
                                cml_printf(_("<b>*** ATTENZIONE:</b> Non accetti Xmsg da %s, non potr&agrave; risponderti!<b>\n"), caio);
                                pull_color();
                        }
                        nl_setid(nl, i, true);
                }

		serv_gets(buf);
		i++;
        }
	inviati = extract_int(buf+4, 0);
	scollegati = extract_int(buf+4, 1);

	if (scollegati) {
		printf(_("Vuoi inviare il messaggio in mail (s/n)? "));
		serv_putf("XINM %d", (si_no() == 's') ? 1 : 0);
		serv_gets(buf);
		if ((buf[0] == '2') && (extract_int(buf+4, 0)))
			cml_printf(_("Ok, il tuo messaggio verr&agrave; letto al prossimo collegamento.\n"));
		strcpy(last_x, "");
	}

        if (inviati)
                xlog_put_out(txt, nl, XLOG_XOUT);
        else
                nl_free(&nl);

	txt_free(&txt);
}

/****************************************************************************/
/*      COMANDI CHAT                                                        */
/****************************************************************************/

/* Restituisce true se l'utente e' in una chat room */
bool user_in_chat()
{
        return canale_chat != 0;
}

/*
 * Entra in un canale Chat.
 */
void ascolta_chat (void)
{
        char canale;
        char buf[LBUF];

        putchar('\n');
        if (user_in_chat()) {
                cml_printf(sesso ? _("Sei gi&agrave; connessa al canale #%d della chat. Vuoi cambiare canale? ") : _("Sei gi&agrave; connesso al canale #%d della chat. Vuoi cambiare canale? "), canale_chat);
                if (si_no() == 'n')
                        return;
        }

        new_str_m(_("Ascolta canale # "), buf, 3);
        canale = strtol(buf, NULL, 10);
        if ((canale > 0) && (canale <= serverinfo.num_canali_chat)) {
                serv_putf("CASC %d", canale);
                serv_gets(buf);
                if (buf[0] != '2')
                        switch (buf[1]) {
			 case '0':
				 printf(sesso ? _("Non sei autorizzata ad entrare nella SYSOPCHAT.\n") : _("Non sei autorizzato ad entrare nella SYSOPCHAT.\n"));
                                break;
			 case '2':
                                printf(sesso ? _("Non sei autorizzata ad usare la chat.\n") :  _("Non sei autorizzato ad usare la chat.\n"));
                                break;
			 case '4':
                                printf(_("Canale di chat non valido.\n"));
                                break;
                        }
                else
                        canale_chat = canale;
        } else
                printf(_("\nSono disponibili i canali chat #1-#%d per gli utenti validati,\ne il canale #3 riservato ai sysop.\n"),
		       serverinfo.num_canali_chat - 1);
}

/*
 * Esce dalla Chat.
 */
void lascia_chat (void)
{
        char buf[LBUF];
  
        if (!user_in_chat()) {
                printf(sesso ? _("\nNon sei connessa a nessun canale della chat.\n") : _("\nNon sei connesso a nessun canale della chat.\n"));
		return;
	}	
	serv_puts("CLAS");
	serv_gets(buf);    
	if (buf[0]!='2')
		switch(buf[1]) {
		 case '0':
			 printf(sesso ? _("\nNon sei connessa a nessun canale della chat.\n") : _("\nNon sei connesso a nessun canale della chat.\n"));
			break;
		 case '2':
			 printf(sesso ? _("Non sei autorizzata ad usare la chat.\n") :  _("Non sei autorizzato ad usare la chat.\n"));
			break;
		}
	else {
		printf(_("\nOk, hai lasciato il canale #%d della chat.\n"),
		       canale_chat);      
		canale_chat = 0;
	}
}

/*
 * Invia un messaggio in Chat.
 */
void chat_msg(void)
{
	struct text *txt;
        char buf[LBUF];

	cml_print(_("Messaggio in chat. [Help: <b>Ctrl-I</b>]"));
        serv_puts("CMSG");
        serv_gets(buf);
        if (buf[0] != '2') {
                if (buf[1] == '2')
			printf(sesso ? _("Non sei autorizzata ad usare la chat.\n") :  _("Non sei autorizzato ad usare la chat.\n"));
		else
			printf(sesso ? _("\nNon sei connessa a nessun canale della chat.\n") : _("\nNon sei connesso a nessun canale della chat.\n"));
                return;
        }
	if (chat_help) {
		putchar('\n');
		chat_help = false;
	} else if (uflags[0] & UT_ESPERTO)
                cleanline();
	else
		putchar('\n');
	txt = txt_create();
	if (get_textl_cursor(txt, 78-strlen(nome), serverinfo.maxlineebx-2,
			     true) > 0)
		send_text(txt);
#ifdef LOCAL
	if (AUTO_LOG_CHAT)
 		chat_dump(nome, txt, false);
#endif
	txt_free(&txt);
        serv_puts("CEND");
        serv_gets(buf);
        return;
}

/*
 * Lista utenti collegati nei canali chat.
 */
void chat_who(void)
{
	struct text *txt;
	int i, lung;
        char buf[LBUF], nm[MAXLEN_UTNAME], *str, ok;

        serv_puts("CWHO");
        serv_gets(buf);
        if (buf[0] == '2') {
		txt = txt_create();
                while (serv_gets(buf), strcmp(buf, "000"))
			txt_put(txt, buf+4);
		if (!txt_len(txt))
			printf(_("Nessun utente in chat\n"));
		else {
			for (i = 1; i <= serverinfo.num_canali_chat; i++) {
				txt_rewind(txt);
				lung = 12;
				ok = 0;
				cml_printf(_("Canale #<b>%-2d</b>: "), i);
				while ((str = txt_get(txt)) != NULL) {
					if (extract_int(str, 0) == i) {
						ok = 1;
						extractn(nm, str, 1,
							 MAXLEN_UTNAME);
						lung += strlen(nm)+2;
						if (lung>79) {
							printf("\n%-12s","");
							lung = 14+strlen(nm);
						}
						printf("%s  ", nm);
					}
				}
				if (!ok)
					printf(_("nessuno."));
				putchar('\n');
			}
		}
		txt_free(&txt);
	}
}

#ifdef LOCAL
/* Trascrive il messaggio in chat nel file di dump.
 * mode = FALSE: messaggio in uscita, TRUE: messaggio in entrata.  */
void chat_dump(char *nome, struct text *txt, bool mode)
{
	FILE *fp;
	char *str;
	
	if ((txt == NULL) || (LOG_FILE_CHAT == NULL))
		return;

	if ((fp = fopen(LOG_FILE_CHAT, "a+")) == NULL) {
		printf(_("*** Errore apertura file [%s].\n"), LOG_FILE_CHAT);
		return;
	}

	txt_rewind(txt);

	if (mode) {
		while((str = txt_get(txt)))
			fprintf(fp, "%s\n", str);
	} else {
		while((str = txt_get(txt)))
			fprintf(fp, "%s> %s\n", nome, str);
	}
	fprintf(fp, "\n");
	fclose(fp);
}
#endif

/****************************************************************************/
/* DIARIO DEI MESSAGGI                                                      */
/****************************************************************************/
/*
 * Inizializza il diario degli Xmsg.
 */
void xlog_init(void)
{
	CREATE(xlog, struct xmsg_t, MAX_XLOG, 0);
	/*
	int i;

	for (i = 0; i < MAX_XLOG; i++) {
		strcpy(xlog[i].autore, "");
		xlog[i].txt = NULL;
	}
	*/
}

/*
 * Tratta Xmsg in partenza: inserisce nel diario e salva nel log file
 */
void xlog_put_out(struct text *txt, struct name_list *dest, int type)
{
        char caio[MAXLEN_UTNAME];
        time_t ora;
	struct tm *tmst;
#ifdef LOCAL
        struct xmsg_t xmsg;
#endif

	if ((uflags[3] & UT_MYXCC)
#ifdef LOCAL
            || AUTO_LOG_X
#endif
            ) {
		time(&ora);
		tmst = localtime(&ora);
                if (type == XLOG_XOUT) {
                        nl_purgeid(dest, -1);
                        if (nl_num(dest) > 1)
                                strcpy(caio, _("X multiplo"));
                        else
                                strncpy(caio, nl_get(dest, 0), MAXLEN_UTNAME);
                } else
                        caio[0] = 0;
        }

        if (uflags[3] & UT_MYXCC) {
                xlog_newt(txt, caio, dest, tmst->tm_hour, tmst->tm_min, type);
#ifdef LOCAL
                if (AUTO_LOG_X)
                        xlog_dump(xlog+MAX_XLOG-1, 1);
#endif
        } else {
#ifdef LOCAL
                if (AUTO_LOG_X) {
                        xlog_xmsg(&xmsg, txt, caio, dest, tmst->tm_hour,
                                  tmst->tm_min, type);
                        xlog_dump(&xmsg, 1);
                }
#endif
                nl_free(&dest);
        }
}


/*
 * Tratta Xmsg in arrivo: inserisce nel diario e salva nel log file
 */
void xlog_put_in(struct text *txt, struct name_list *dest)
{
	char *str;

	xlog_scroll();
	strcpy(xlog[MAX_XLOG-1].autore, xmsg_tmp.autore);
	xlog[MAX_XLOG-1].ore = xmsg_tmp.ore;
	xlog[MAX_XLOG-1].min = xmsg_tmp.min;
	xlog[MAX_XLOG-1].flags = xmsg_tmp.flags;
	xlog[MAX_XLOG-1].dest = dest;
	xlog[MAX_XLOG-1].txt = txt_create();
	if (prompt_xlog_max < MAX_XLOG)
		prompt_xlog_max++;

	txt_rewind(txt);
	txt_get(txt);
	while((str = txt_get(txt)) != NULL)
		txt_put(xlog[MAX_XLOG-1].txt, str);

#ifdef LOCAL
	if (AUTO_LOG_X)
		xlog_dump(xlog+MAX_XLOG-1, 1);
#endif
}

/*
 * Scrolla il diario degli Xmsg, facendo spazio nell'ultimo posto a un
 * nuovo xmsg.
 */
static void xlog_scroll(void)
{
	int i;
	
	if (xlog[0].txt != NULL) {
		txt_free(&xlog[0].txt);
                nl_free(&xlog[0].dest);
		strcpy(xlog[0].autore, "");
	}
	for (i = 0; i < MAX_XLOG-1; i++) {
		strcpy(xlog[i].autore, xlog[i+1].autore);
		xlog[i].txt = xlog[i+1].txt;
		xlog[i].ore = xlog[i+1].ore;
		xlog[i].min = xlog[i+1].min;
		xlog[i].flags = xlog[i+1].flags;
		xlog[i].dest = xlog[i+1].dest;
	}
	strcpy(xlog[MAX_XLOG-1].autore, "");
	xlog[MAX_XLOG-1].txt = NULL;	
}

/*
 * Elimina il messaggio numero 'num' dal diario degli Xmsg, e scrolla i
 * messaggi successivi
 */
static void xlog_del(int num)
{
	int i;
	
	if ((num < 0) || (num > MAX_XLOG-1) || (xlog[num].txt == NULL))
		return;
	txt_free(&xlog[num].txt);
	strcpy(xlog[num].autore, "");
	for (i = num; i > 0; i--) {
		strcpy(xlog[i].autore, xlog[i-1].autore);
		xlog[i].txt = xlog[i-1].txt;
		xlog[i].ore = xlog[i-1].ore;
		xlog[i].min = xlog[i-1].min;
		xlog[i].flags = xlog[i-1].flags;
	}
	strcpy(xlog[0].autore, "");
	xlog[0].txt = NULL;	
	prompt_xlog_n--;
	prompt_xlog_max--;
}

/*
 * Inizializza la struttura per un nuovo messaggio.
 */
void xlog_new(char *name, int ore, int min, int type)
{
	strcpy(xmsg_tmp.autore, name);
	xmsg_tmp.ore = ore;
	xmsg_tmp.min = min;
	xmsg_tmp.flags = type;
	xmsg_tmp.txt = NULL;
}

/*
 * Inizializza e inserisce il nuovo messaggio. Se AUTO_LOG_X salva il
 * messaggio anche su file.
 */
static void xlog_newt(struct text *txt, const char *name,
                      struct name_list *dest, int ore, int min, int type)
{
	xlog_scroll();
        xlog_xmsg(xlog+(MAX_XLOG-1), txt, name, dest, ore, min, type);

	if (prompt_xlog_max < MAX_XLOG)
		prompt_xlog_max++;
}


/*
 * Inserisce l'express message nella struttura xmsg.
 */
static void xlog_xmsg(struct xmsg_t *xmsg, struct text *txt, const char *name,
                      struct name_list *dest, int ore, int min, int type)
{
	char *str;

	strcpy(xmsg->autore, name);
	xmsg->ore = ore;
	xmsg->min = min;
	xmsg->flags = type;
	xmsg->dest = dest;        
	xmsg->txt = txt_create();

	/* xlog_new(name, ore, min, type); */
	txt_rewind(txt);
	while ( (str = txt_get(txt)) != NULL)
		txt_put(xmsg->txt, str);
}

/*
 * Stampa il messaggio numero 'num' del diario.
 */
static void xlog_print(int num)
{
	char * str;

	if ((num < 0) || (num > MAX_XLOG-1) || (xlog[num].txt == NULL))
		return;
	setcolor(C_XLOG);
	txt_rewind(xlog[num].txt);
	xlog_header(xlog[num]);
	push_color();
	while( (str = txt_get(xlog[num].txt))) {
		setcolor(C_EDIT_PROMPT);
		putchar('>');
		pull_color();
		cml_print(str);
		push_color();
		setcolor(C_DEFAULT);
		putchar('\n');
	}
	pull_color();
	putchar('\n');
}

#ifdef LOCAL
/* Salva il messaggio corrente nel file di dump degli X.
 * Se autolog = 1, e' un salvataggio finale e non stampa "msg salvato." */
static void xlog_dump(struct xmsg_t *xmsg, int autolog)
{
	FILE *fp;
	char *str;

	if (xmsg->txt == NULL) {
		printf(_("X-msg vuoto.\n"));
		return;
	}

	if (LOG_FILE_X == NULL) {
		printf(_("*** Errore: devi specificare il file di dump per gli X.\n"));
		return;
	}

	if ((fp = fopen(LOG_FILE_X, "a+")) == NULL) {
		printf(_("*** Errore apertura file [%s].\n"), LOG_FILE_X);
		return;
	}

	txt_rewind(xmsg->txt);
	xlog_header_dump(fp, *xmsg);
	while((str = txt_get(xmsg->txt)))
		fprintf(fp, "%s\n", str);
	fprintf(fp, "\n");
	fclose(fp);

	if (!autolog)
		printf(_("*** Messaggio salvato.\n"));
}

static void xlog_header_dump(FILE *fp, struct xmsg_t x)
{
	time_t ora;
	struct tm *tmst;
	
	time(&ora);
	tmst = localtime(&ora);
	
	switch (x.flags) {
	case XLOG_XIN:
		fprintf(fp, _("+++ Express Message da %s alle %02d:%02d del %02d/%02d/%04d\n"),
			x.autore, x.ore, x.min, tmst->tm_mday, tmst->tm_mon+1,
			1900 + tmst->tm_year);
		break;
	case XLOG_XOUT:
		fprintf(fp, _("--- Express Message per %s alle %02d:%02d del %02d/%02d/%04d\n"),
			x.autore, x.ore, x.min, tmst->tm_mday, tmst->tm_mon+1,
			1900 + tmst->tm_year);
		break;
	case XLOG_BIN:
		fprintf(fp, _("+++ Broadcast da %s alle %02d:%02d del %02d/%02d/%04d\n"),
			x.autore, x.ore, x.min,	tmst->tm_mday, tmst->tm_mon+1,
			1900 + tmst->tm_year);
		break;
	case XLOG_BOUT:
		fprintf(fp, _("--- Broadcast inviato alle %02d:%02d del %02d/%02d/%04d\n"),
			x.ore, x.min, tmst->tm_mday, tmst->tm_mon + 1,
			1900 + tmst->tm_year);
		break;
	}
	/* 
	 * il valore di ritorno di nl_num andrebbe cambiato,
	 * visto che su x.dest==NULL torna 5
	 */
        if (x.dest && nl_num(x.dest) > 1) {
                fprintf(fp, _("Destinatari: "));
                nl_fprint(fp, x.dest);
        }
}
#endif

/*
 * Visualizza l'header del messaggio x.
 */
static void xlog_header(struct xmsg_t x)
{
	switch (x.flags) {
	case XLOG_XIN:
		cml_printf(_("+++ <b>Express Message</b> da <b>%s</b> alle %02d:%02d\n"), x.autore, x.ore, x.min);
		break;
	case XLOG_XOUT:
		cml_printf(_("--- <b>Express Message</b> per <b>%s</b> alle %02d:%02d\n"), x.autore, x.ore, x.min);
		break;
	case XLOG_BIN:
		cml_printf(_("+++ <b>Broadcast</b> da <b>%s</b> alle %02d:%02d\n"), x.autore, x.ore, x.min);
		break;
	case XLOG_BOUT:
		cml_printf(_("--- <b>Broadcast</b> inviato alle %02d:%02d\n"),
			   x.ore, x.min);
		break;
	}
	/* 
	 * il valore di ritorno di nl_num andrebbe cambiato,
	 * visto che su x.dest==NULL torna 5
	 */
        if (x.dest && (nl_num(x.dest) > 1))
		nl_print(_("*** Destinatari: "), x.dest);
}

/*
 * Stampa tutti i messaggi nel diario.
 */
static void xlog_print_all(void)
{
	int i;

	for (i = 0; i < MAX_XLOG; i++)
		xlog_print(i);
}

/*
 * Stampa la lista dei messaggi nel diario degli X.
 */
static void xlog_list(void)
{
	int i, first;

	for (i = 0; (xlog[i].txt == NULL) && (i < MAX_XLOG); i++);
	first = i;
	for (; i < MAX_XLOG; i++) {
		printf("%2d. ", i - first +1);
		xlog_header(xlog[i]);
	}
}

/*
 * Funzione per leggere e modificare il diario dei messaggi.
 */
void xlog_browse(void)
{
	int c, i, num, gnum, fine = 0;

	for (i = 0; (i < MAX_XLOG) && (xlog[i].txt == NULL); i++);
	
	if (i == MAX_XLOG) {
		printf(_("Non hai nessun messaggio nel diario.\n"));
		return;
	}

	putchar('\n');
	xlog_print(i);
	prompt_curr = P_XLOG;
	prompt_xlog_n = 1;
	prompt_xlog_max = MAX_XLOG - i;
	
	while (!fine) {
		print_prompt();
		c = inkey_sc(1);
		switch(c) {
		case 'b':
			printf(_("Back.\r"));
			del_prompt();
			if ((--i == -1) || (xlog[i].txt == NULL))
				fine = 1;
			else
				xlog_print(i);
			prompt_xlog_n--;
			break;
		case 'D':
			printf(_("Delete.\r"));
			del_prompt();
			printf(sesso ? _("Sei sicura di voler cancellare questo messaggio (s/n)? ") : _("Sei sicuro di voler cancellare questo messaggio (s/n)? "));
			if (si_no() == 's') {
				xlog_del(i);
				for (; (xlog[i].txt == NULL) && (i < MAX_XLOG);
				     i++);
				if (i == MAX_XLOG)
					return;
				xlog_print(i);
			}
			break;
#ifdef LOCAL
		case 'd': /* dump Xmsg */
			printf(_("Dump to file.\r"));
			xlog_dump(xlog+i, 0);
			break;
#endif
		case 'g':
			printf(_("Goto.\r"));
			del_prompt();
			num = new_int(_("\nVai al messaggio numero: "));
			gnum = MAX_XLOG - prompt_xlog_max + num - 1;
			if ((gnum < 0) || (gnum >= MAX_XLOG)
			    || (xlog[gnum].txt == NULL)) {
				printf(_("Non esiste il messaggio #%d.\n\n"),
				       num);
				break;
			}
			i = gnum;
			prompt_xlog_n = num;
			xlog_print(i);
			break;
		case 'l': /* List */
			printf(_("List.\r"));
			del_prompt();
			printf(_("\nLista dei messaggi nel diario.\n"));
			xlog_list();
			break;
		case 'n': /* Next */
		case ' ':
			printf(_("Next.\r"));
			del_prompt();
			if (++i == MAX_XLOG)
				fine = 1;
			else
				xlog_print(i);
			prompt_xlog_n++;
			break;
		case 'p': /* Print all */
			printf(_("Print all.\r"));
			printf(_("\nMessaggi nel diario.\n"));
			del_prompt();
			xlog_print_all();
			break;
		case 'r': /* Reply */
			printf(_("Reply.\r"));
			del_prompt();
			if (strlen(xlog[i].autore))
				express(xlog[i].autore);
			break;
		 case 's':
			printf(_("Stop.\r"));
			del_prompt();
			fine = 1;
			break;
		case '?':
		case Key_F(1):
			printf(_("Help.\n"));
			del_prompt();
#ifdef LOCAL
			printf(_("\nComandi per il diario dei messaggi:\n"
     "  <b>ack             <space>, <n>ext    <D>elete           <r>eply\n"
     "  <d>ump             <l>ist             <g>oto msg num     <p>rint all\n"
     "  <?> help           <s>top\n\n"));
#else
			printf(_("\nComandi per il diario dei messaggi:\n"
     "  <b>ack             <space>, <n>ext    <D>elete           <r>eply\n"
     "                     <l>ist             <g>oto msg num     <p>rint all\n"
     "  <?> help           <s>top\n\n"));
#endif
			break;
		default:
			del_prompt();
			break;
		}
	}
}

/****************************************************************************/
/*
 * Invia un emote a un altro utente.
 */
#if 0
void send_emote(void)
{
	struct text *txt;
        char buf[LBUF];
        char caio[30];

	setcolor(C_X);
	if (!get_recipient(caio, 0))
		return;

        serv_putf("XMSG %s", caio);
        serv_gets(buf);

        if (buf[0] != '2') {
		print_err_x(buf);
                return;
        }
	txt = txt_create();
	if (uflags[0] & UT_LINE_EDT_X) {
		if (get_textl_cursor(txt, 78, serverinfo.maxlineebx - 2,
				     false) > 0)
			send_text(txt);
	} else if (get_text_col(txt, serverinfo.maxlineebx, 79, 1) > 0)
		send_text(txt);
	serv_puts("XEND");
	serv_gets(buf);

	if (buf[0] != '2') {
		if (print_err_x(buf))
			xlog_put_out(txt, caio, XLOG_XOUT);
	} else {
		xlog_put_out(txt, caio, XLOG_XOUT);
		printf(_("eXpress message inviato a %s.\n"), caio);
		if (!(uflags[3] & UT_X) && (!(uflags[3] & UT_XFRIEND)
					    || !is_friend(caio)))
			cml_printf(_("\n<b>*** ATTENZIONE:</b> Non accetti Xmsg da %s, non potr&agrave; risponderti!\n"), caio);
	}
	txt_free(&txt);
}
#endif
