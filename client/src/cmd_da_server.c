/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File : cmd_da_server.c                                                    *
*        interroga il server ed esegue le operazioni richieste              *
****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "ansicol.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "cmd_da_server.h"
#include "cml.h"
#include "comandi.h"
#include "comunicaz.h"
#include "configurazione.h"
#include "conn.h"
#include "extract.h"
#include "friends.h"
#include "macro.h"
#include "messaggi.h"
#include "name_list.h"
#include "prompt.h"
#include "room_cmd.h"
#include "signals.h"
#include "text.h"
#include "user_flags.h"
#include "utility.h"
#include "generic_cmd.h"

#define MSG_BROADCAST 0
#define MSG_X         1
#define MSG_CHAT      2
#define MSG_CHAT_ME   4
#define MSG_CHAT_CMD  8

/* Da unificare con xmsg del diario */
struct msg {
	int type;
	int riga;
        char author[MAXLEN_UTNAME];
        struct name_list * dest;
	struct text * txt;
};

static struct msg msg; /* Qui vengono sistemati i messaggi in arrivo */
//struct name_list X_followup_rcpt1;
static char X_followup_rcpt[MAXLEN_UTNAME];

/* Prototipi delle funzioni in questo file */
int controlla_server(void);
int esegue_urgenti(char *str);
int esegue_comandi(int mode);
void esegue_cmd_old(void);
static int bx(char *str);
static int idle_cmd(char *str);
static int notifica(char *str);
static int notifica_post(char *str);

/******************************************************************************
******************************************************************************/
/*
 * Controlla se e' in arrivo dell'input dal server.
 */
int controlla_server()
{
        fd_set input_set;
        struct timeval t_nullo;

        int flag = 0;

        t_nullo.tv_sec = 0;
        t_nullo.tv_usec = 0;

        /* Controlliamo se c'e' qualcosa da leggere */
        FD_ZERO(&input_set);
        FD_SET(serv_sock, &input_set);

        if (select(serv_sock + 1, &input_set, NULL, NULL, &t_nullo) < 0) {
		if (errno == EINTR) {
			esegui_segnali(NULL, NULL);
		} else {
			perror("Select");
			exit(1);
		}
        }

        if (FD_ISSET(serv_sock, &input_set))
                flag = 1;

        return(flag);
}

/* Esegue il comando urgente 'str' proveniente dal server. */
int esegue_urgente(char *str)
{
	int result = CMD_ESEGUITO;

	setcolor(C_NOTIF_URG);
	switch(str[1]) {
	case '0':      /* Login TimeOut */
		printf("\n\nLogin Timeout.\n");
		exit(1);
		break;
	case '1':     /* Idle Timeout */
		printf(_("\n\n*** Ciao! Torna quando avrai meno sonno...\n\n"));
		break;
	case '2':     /* Kick Out: utente cacciato */
		printf(sesso ? _("\n\n*** Sei stata cacciata fuori da %s.\n") :
		       _("\n\n*** Sei stato cacciato fuori da %s.\n"), str+4);
		break;
	case '3':
		switch (str[2]) {
		case '0': /* Shutdown */
			del_prompt();
			if (extract_int(str+4, 1) == 1)
				cml_printf(_("<b>\a*** Attenzione: shutdown del server tra "));
			else
				cml_printf(_("<b>\a*** Attenzione: reboot quotidiano del server tra "));
			int minuti = extract_int(str+4, 0);
			if (minuti == 1) {
				cml_printf(_("un minuto.</b>\n"));
			} else {
				cml_printf(_("%d minuti.</b>\n"), minuti);
			}
			break;
		case '1': /* Shutdown Annullato */
			del_prompt();
			Beep();
			cml_print(_("<b>*** Shutdown annullato!!! :)</b>\n"));
			break;
		}
		break;
	case '4': /* Il server richiede l'host di connessione */

		break;
	case '5':
		Beep();
		del_prompt();
		cml_print(_("*** La room dove stavi &egrave; stata appena eliminata... :(\n"));
		room_goto(4, true, NULL);
		putchar('\n');
		break;
	case '6':
		Beep();
		switch (str[2]) {
		case '0':
			del_prompt();
			cml_print(_("\n*** Il tempo che avevi a disposizione per lasciare un messaggio qui &egrave; scaduto!\n"));
			post_timeout = true;
			result |= CMD_POST_TIMEOUT;
			break;
		case '1':
			cml_print(_(
"\n*** il sondaggio a cui stai votando sta per scadere \n"
"(continua a votare)--->"
				    ));
			break;
		break;
		}
	}
	Free(str);
	setcolor(C_NORMAL);

	return result;
}

/*
 * Esegue tutti i comandi presenti nella coda dei comandi da server non
 * urgenti e vuota la coda.
 */
int esegue_comandi(int mode)
{
        char *buf;
	char ret = 0;

        while (prendi_da_coda_t(&comandi, &buf, mode)) {
                switch(buf[1]) {
                case '0':
                        ret = bx(buf);
			break;
                case '1':
                        ret = notifica(buf);
			setcolor(C_NORMAL);
			break;
                case '2':
                        ret = idle_cmd(buf);
			break;
		case '3':
			ret = notifica_post(buf);
			setcolor(C_NORMAL);
			break;
		case '4':
                        carica_userconfig(0);
			break;
		}
		Free(buf);
        }
	return ret;
}

/*
 * Visualizza i messaggi tenuti da parte
 */
void esegue_cmd_old(void)
{
	if (comandi.partenza != NULL) {
		printf(sesso ? _("\nQuesti messaggi sono stati tenuti da parte mentre eri impegnata.\n\n") : _("\nQuesti messaggi sono stati tenuti da parte mentre eri impegnato.\n\n"));
		esegue_comandi(0);
			CLRLINE;
	}
}

/****************************************************************************/
/*
 * Tratta broadcast, xmsg e messaggi in chat in arrivo dal server.
 */
static int bx(char *str)
{
        int a, ore, min;
        char nick[MAXLEN_UTNAME];
	char ret = 0;

        switch(str[2]) {
        case '0': /* Broadcast in arrivo */
                extractn(nick, &str[4], 0, MAXLEN_UTNAME);
                ore = extract_int(&str[4],1);
                min = extract_int(&str[4],2);
                Beep();
                /*
                 * msg.dest punta ancora alla struttura dest
                 * precedente, quindi se c'e` un X e poi un broadcast
                 * l'X perde i suoi dest
                 */
		/* nl_free(&msg.dest); */
		if (msg.txt)
			txt_free(&msg.txt);
		msg.txt = txt_create();
		msg.type = MSG_BROADCAST;
                txt_putf(msg.txt, _("*** <b>Broadcast</b> da <b>%s</b> alle %02d:%02d\n"), nick, ore, min);
                msg.riga = 1;
                msg.dest = NULL;
		xlog_new(nick, ore, min, XLOG_BIN);
                break;
        case '1': /* X-msg in arrivo */
                extractn(nick, &str[4], 0, MAXLEN_UTNAME);
		strcpy(last_profile, nick);
		strcpy(last_xfu, nick);
                ore = extract_int(&str[4],1);
                min = extract_int(&str[4],2);
                Beep();
		if (msg.txt) {
                        nl_free(&msg.dest);
			txt_free(&msg.txt);
                }
		msg.dest = nl_create("");
		msg.txt = txt_create();
		msg.type = MSG_X;
                txt_putf(msg.txt, _("*** <b>Express Message</b> da <b>%s</b> alle %02d:%02d\n"), nick, ore, min);
                msg.riga = 1;
		if (uflags[3] & UT_FOLLOWUP)
			strcpy(X_followup_rcpt, nick);
		xlog_new(nick, ore, min, XLOG_XIN);
                break;
        case '2': /* Linea di testo broadcast/x-msg */
		txt_put(msg.txt, str+4);
                msg.riga++;
                break;
        case '3': /* Fine ricezione messaggio b/x */
		del_prompt();
		if (msg.type == MSG_BROADCAST)
			setcolor(C_BROADCAST);
		else
			setcolor(C_X);
		txt_rewind(msg.txt);
		cml_print(txt_get(msg.txt));
		/* TODO show only if more than one or it's just the user */
                if (msg.dest)
                        nl_print(_("*** Destinatari: "), msg.dest);
		push_color();
                for (a = 1; a < msg.riga ; a++) {
			setcolor(C_EDIT_PROMPT);
			putchar('>');
			pull_color();
                        cml_print(txt_get(msg.txt));
			push_color();
			setcolor(C_DEFAULT);
			putchar('\n');
		}
		pull_color();
		xlog_put_in(msg.txt, msg.dest);
		txt_free(&msg.txt);
		setcolor(C_NORMAL);

                msg.riga = 0;
		if (X_followup_rcpt[0] && (uflags[3] & UT_FOLLOWUP)
		    && (prompt_curr != P_EDT) && (prompt_curr != P_ENT)) {
			express(X_followup_rcpt);
			strcpy(X_followup_rcpt, "");
		} else
			IFNEHAK;

		if (prompt_curr == P_MSG)
			putchar('\n');
		ret = 1;
		break;
        case '4': /* Messaggio in arrivo dal canale chat */
		if (msg.txt)
			txt_free(&msg.txt);
		msg.txt = txt_create();
		extractn(msg.author, str+4, 0, 29);
                msg.riga = 0;
		msg.type = MSG_CHAT;
		break;
        case '5': /* Riga testo in canale chat */
		if ((msg.riga == 0) && (str[4] == '/')) {
			if (!strncmp(str+5, "me ", 3)) {
				msg.type = MSG_CHAT_ME;
				txt_putf(msg.txt, " <b>%s</b> %s\n",
					 msg.author, str+8);
				txt_putf(msg.txt, "%s", str+8);
				msg.riga++;
				break;
                /*
                  } else if (!strncmp(str+5, "nick ", 5)) {
                  break;
                */
			}
		}
		switch (msg.type) {
		case MSG_CHAT_ME:
			txt_putf(msg.txt, "    %s %s\n", msg.author, str+4);
			break;
		case MSG_CHAT:
			txt_putf(msg.txt, "<b>%s</b>> %s\n", msg.author,
				 str+4);
			break;
		}
		msg.riga++;
                break;
        case '6': /* Messaggio chat terminato */
		del_prompt();
                Beep();
		txt_rewind(msg.txt);
		setcolor(C_CHAT_HDR);
		for (a = 0; a < msg.riga ; a++)
			cml_print(txt_get(msg.txt));
		setcolor(C_NORMAL);
#ifdef LOCAL
		if (AUTO_LOG_CHAT)
			chat_dump(msg.author, msg.txt, true);
#endif
		txt_free(&msg.txt);

		if (prompt_curr == P_MSG)
			putchar('\n');
                msg.riga = 0;
		strcpy(msg.author, "");
		ret = 1;
		break;
        case '7': /* Aggiunge destinatario alla lista */
	          /* TODO: why is the last arg (userid = 1) set to 1? */
                nl_insert(msg.dest, str+4, 1);
                break;
       }
	return ret;
}

/*
 * Notifiche entrata/uscita utenti dalla BBS o dal canale chat corrente.
 */
static int notifica(char *str)
{
        char col1[LBUF], col2[LBUF];
        int sex;

        sex = (str[4] == 'M') ? 0 : 1;
        color2cml(col1, C_NOTIF_NORM);
        if (is_friend(str+5))
                color2cml(col2, C_NFRIEND);
        else if (is_enemy(str+5)) {
                if (index("237", str[2]))
                        color2cml(col2, C_NCENEMY);
                else
                        color2cml(col2, C_NENEMY);
        } else
                color2cml(col2, C_NUSER);

        setcolor(C_NOTIF_NORM);

        switch(str[2]) {
        case '0':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s si &egrave; appena collegata.\n") : _("*** %s%s%s si &egrave; appena collegato.\n"), col2, &str[5], col1);
		break;
        case '1':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s &egrave; uscita dalla bbs.\n") : _("*** %s%s%s &egrave; uscito dalla bbs.\n"), col2, &str[5], col1);
		break;
        case '2':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s &egrave; entrata nella chat.\n") : _("*** %s%s%s &egrave; entrato nella chat.\n"), col2, &str[5], col1);
		break;
        case '3':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s &egrave; uscita dalla chat.\n") : _("*** %s%s%s &egrave; uscito dalla chat.\n"), col2, &str[5], col1);
		break;
        case '4':
                Beep();
		del_prompt();
                cml_printf(_("*** La connessione di %s%s%s &egrave; caduta. Torner&agrave; presto?\n"), col2, &str[5], col1);
		break;
        case '5':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s &egrave; andata a schiacciarsi un pisolino.\n") : _("*** %s%s%s &egrave; andato a schiacciarsi un pisolino.\n"),
			   col2, &str[5], col1);
		break;
	case '6': /* Chat timeout */
                Beep();
		del_prompt();
		cml_print(sesso ? _("*** Oops.. sei scivolata fuori dalla chat!\n") : _("*** Oops.. sei scivolato fuori dalla chat!\n"));
                canale_chat = 0;
		break;
        case '7':
                Beep();
		del_prompt();
                cml_printf(sex ? _("*** %s%s%s &egrave; scivolata fuori dalla chat.\n") : _("*** %s%s%s &egrave; scivolato fuori dalla chat.\n"), col2, &str[5], col1);
		break;
	 default:
		return 0;
        }
	return 1;
}

/*
 * Notifiche sull'arrivo di nuovi post/mail/blog.
 */
static int notifica_post(char *str)
{
        char col1[LBUF], col2[LBUF];

        color2cml(col1, C_NOTIF_NORM);
        if (is_friend(str+4))
                color2cml(col2, C_NFRIEND);
        else if (is_enemy(str+4))
                color2cml(col2, C_NENEMY);
        else
                color2cml(col2, C_NUSER);

        setcolor(C_NOTIF_NORM);

        switch(str[2]) {
	case '0':
                Beep();
		del_prompt();
		if (strlen(&str[4]))
			cml_printf(_("*** %s%s%s ha lasciato un nuovo post in questa room.\n"), col2, &str[4], col1);
		else
			cml_printf(_("*** C'&egrave; un nuovo post in questa room.\n"));
		barflag = 1;
		break;
	case '1':
		Beep();
		del_prompt();
		cml_printf(_("*** &Egrave; arrivato un nuovo mail.\n"));
		break;
	case '2':
		Beep();
		del_prompt();
		if (strlen(&str[4]))
			cml_printf(_("*** %s%s%s ha lasciato nel frattempo un nuovo post.\n"), col2, &str[4], col1);
		else
			cml_print(_("*** Un altro post &egrave; stato lasciato nel frattempo.\n"));
		barflag = 1;
		break;
	case '3':
		Beep();
		del_prompt();
		if (strlen(&str[4]))
			cml_printf(_("*** %s%s%s ha lasciato un nuovo post nel tuo blog.\n"), col2, &str[4], col1);
		else
			cml_print(_("*** Un altro post &egrave; stato lasciato nel tuo blog.\n"));
		break;
	default:
		return 0;
        }
	return 1;
}

/*
 * Notifiche idle.
 */
static int idle_cmd(char *str)
{
	setcolor(C_IDLE);
        switch(str[2]) {
        case '0':
                Beep();
		del_prompt();
                cml_print(_("*** TOC-TOC! C'&egrave; nessuno l&agrave; fuori?!?\n"));
		break;
        case '1':
                Beep();
		del_prompt();
                printf(sesso ? _("*** Ultimo avvertimento! Stai per essere cacciata fuori...\n") : _("*** Ultimo avvertimento! Stai per essere cacciato fuori...\n"));
                serv_puts("QUIT");
                serv_gets(str);
                exit(1);
                break;
	default:
		setcolor(C_NORMAL);
		return 0;
        }
	setcolor(C_NORMAL);
	return 1;
}

/****************************************************************************/
