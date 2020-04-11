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
* File : messaggi.c                                                         *
*        Comandi di gestione dei messaggi nelle room.                       *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

#include "ansicol.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "cml.h"
#include "conn.h"
#include "comandi.h"
#include "comunicaz.h"
#include "edit.h"
#include "editor.h"
#include "errore.h"
#include "extract.h"
#include "friends.h"
#include "generic_cmd.h"
#include "inkey.h"
#include "macro.h"
#include "messaggi.h"
#include "metadata.h"
#include "metadata_ops.h"
#include "msg_flags.h"
#include "pager.h"
#include "prompt.h"
#include "room_flags.h"
#include "room_cmd.h"
#include "tabc.h"
#include "text.h"
#include "user_flags.h"
#include "utente_client.h"
#include "utility.h"

/* Nick per gli utenti anonimi */
#define UTENTE_ANONIMO _("un utente anonimo")

struct message {
        char autore[LBUF];
        char anonick[LBUF];
	char room[LBUF];
	char subject[LBUF];
	char reply_to[LBUF];
	char recipient[LBUF];
	char h1[LBUF];
	char h2[LBUF];
        long flags, ora;
	struct text *txt;
};

long locnum, remain;
static struct message *last_msg = NULL;
struct text *quoted_text;
struct text *hold_text;

bool post_timeout; /* TRUE se il tempo per un post e` scaduto. */
Metadata_List postmd;

/* Questa e' in local.c o in remote.c */
extern void msg_dump(struct text *txt, int autolog);

/* Prototipi delle funzioni in questo file */
void enter_message(int mode, bool reply, int aide);
bool read_msg(int mode, long local_number);
void set_all_read(void);
static struct message * receive_msg(Metadata_List *mdlist);
static void ciclo_msg(void);
static void delete_message(void);
static void undelete_message(void);
static void move_message(void);
static void copy_message(void);
static int entry_cmd(void);

/***************************************************************************/
/*
 * Invia un post nella room.
 * mode = 0 : editor interno, 1 : editor esterno, -1 : editor di default
 * reply != 0 : e' un reply
 * aide = 0 : Normale, 1 : RA o RH, 2 : Aide, 3 : Sysop
 */
/* TODO implement constants for mode and aide values */
void enter_message(int mode, bool reply, int aide)
{
	struct text *txt;
	char buf[LBUF], caio[LBUF], reply_to[LBUF], reply_subj[LBUF];
	char subj[LBUF] = {0};
	char anonick[LBUF] = {0};
	char fine;
        bool anon = false, spoiler = false;
	int sysopmail = 0; /* TODO define constants for sysopmail values */
        long flags, reply_num;
        Metadata_List mdlist;

	if (reply && !(rflags & RF_REPLY)) /* Niente reply qui.. */
		return;

	post_timeout = false;
	barflag = 0;

	if (mode == -1)
		mode = (uflags[0] & UT_EDITOR ? 1 : 0);

	if (rflags & RF_MAIL) {
		caio[0] = 0;
		if ((!reply) && (get_recipient(caio, 1) == false))
			return;
		if (!strcmp(caio, "Sysop"))
			sysopmail = 1;
		else if (!strcmp(caio, "Aide"))
			sysopmail = 2;
		else if (!strcmp(caio, "Room Aide"))
			sysopmail = 3;
		serv_putf("PSTB %s|%d|%d|%d", caio, reply, aide, sysopmail);
	} else
		serv_putf("PSTB |%d|%d|", reply, aide);

	serv_gets(buf);
	if (buf[0] != '2') {
		switch (buf[1]) {
		case '2':
			switch (buf[2]) {
			case '0':
                                printf(sesso ? _("\nNon sei autorizzata a lasciare messaggi in questa room.\n") :_("\nNon sei autorizzato a lasciare messaggi in questa room.\n"));
                                break;
			case '1':
				printf(_("\nL'utente %s non ha una mailbox.\n"), caio);
				break;
			case '2':
				printf(sesso ? _("\nNon sei autorizzata a lasciare messaggi di amministrazione in questa room.\n") :_("\nNon sei autorizzato a lasciare messaggi di amministrazione in questa room.\n"));
				break;
                        case '3':
                                printf("\nHai messo troppi post, attendi...\n");
                                break;
                        case '4':
                                printf(sesso ? _("\nNon sei autorizzata a lasciare messaggi nella BBS!\n") :_("\nNon sei autorizzato a lasciare messaggi nella BBS!\n"));
                                break;
                        case '5':
                                printf(sesso ? _("\nNon sei autorizzata a inviare mail!\n") :_("\nNon sei autorizzato a inviare mail!\n"));
                                break;
                        case '6':
                                cml_printf(sesso ? _("*** Mi dispiace, ma non sei autorizzata a lasciare messaggi anonimi.\n") : _("*** Mi dispiace, ma non sei autorizzato a lasciare messaggi anonimi.\n"));
                                break;
                        }
                        break;
                case '4':
			printf(sesso ? _("\nNon puoi mandare mail a te stessa!\n") : _("\nNon puoi mandare mail a te stesso!\n"));
			break;
		case '9':
			printf(_("\nUn altro utente sta scrivendo un messaggio. Devi aspettare che abbia finito.\n"));
			break;
		}
		return;
	}
	flags = extract_long(buf+4, 0);
	if (flags & RF_ANONYMOUS)
		anon = true;
        else if (flags & RF_ASKANONYM)
                if ((!(sflags[1] & SUT_NOANONYM))
                    && (new_si_no_def(_("Messaggio anonimo"), true))) {
                        anon = true;
                        new_str_M(_("Inserisci, se vuoi, un nick anonimo: "),
                                  anonick, MAXLEN_UTNAME);
                }
        if (flags & RF_SPOILER)
                if (new_si_no_def(_("Vuoi un avviso di spoiler"), false))
                        spoiler = true;
	strcpy(subj, "");
	if (num_parms(buf+4) > 1) { /* E' un reply */
		reply_num = extract_long(buf+4, 1);
		extract(reply_to, buf+4, 2);
		if (strlen(reply_to) == 0)
			strcpy(reply_to, UTENTE_ANONIMO);
		extract(reply_subj, buf+4, 3);
		if (strlen(reply_subj))
			cml_printf(_("\n Reply: <b>%s</b>, msg #<b>%ld</b> da <b>%s</b>\n"), reply_subj, reply_num, reply_to);
		else
			cml_printf(_("\n Reply al Messaggio #<b>%ld</b> da <b>%s</b>\n"), reply_num, reply_to);
	} else {
		reply = false;
		if (flags & RF_SUBJECT)
			new_str_m(_("\nSubject: "), subj, MAXLEN_SUBJECT - 1);
	}
	putchar('\n');

	txt = txt_create();
        md_init(&mdlist);

	bool edit = true;
	int ret = EDIT_DONE;
	do {
		if (edit) {
			ret = enter_text(txt, serverinfo.maxlineepost, mode,
					 &mdlist);
		}
		if (ret == EDIT_ABORT) {
			fine = 1;
		} else {
			if (ret == EDIT_TIMEOUT || post_timeout) {
				if (txt)
					txt_free(&txt);
				post_timeout = false;
				printf("Post timeout.\n");
				return;
			}
			switch (entry_cmd()) {
			case 'a':
				printf(_("Abort.\n"));
				printf(sesso ? _("Sei sicura (s/n)? ") :
				       _("Sei sicuro (s/n)? "));
				if (si_no() != 's') {
					edit = true;
					fine = 0;
				} else
					fine = 1;
				break;
			case 'c':
				printf(_("Continue.\n\n"));
				edit = true;
				fine = 0;
				break;
			case 'h':
				printf(_("Hold.\n"));
				if (hold_text)
					txt_free(&hold_text);
				hold_text = txt;
				txt = NULL;
				fine = 1;
				break;
			case 'p':
				printf(_("Print.\n\n"));
				txt_rewind(txt);
				txt_pager(txt);
				putchar('\n');
				edit = false;
				fine = 0;
				break;
			case 's':
				printf(_("Save.\n"));
				send_text(txt);
				fine = 2;
				break;
			}
		}
	} while (!fine);

	if (txt)
		txt_free(&txt);

	if (post_timeout) {
		post_timeout = false;
		return;
	}

        /* Abort */
	if (fine == 1) {
		serv_puts("ATXT");
		serv_gets(buf);
                md_free(&mdlist);
		return;
	}

        /* Save post */
	serv_putf("PSTE %s|%d|%d|%d|%d|%d|%s", subj, anon, reply, aide,
                  sysopmail, spoiler, anonick);
	serv_gets(buf);
	if (buf[0] == '2') {
                /* upload files */
                mdop_upload_files(&mdlist);

		printf(_("Post inviato.\n"));
                md_free(&mdlist);
		return;
	}
	switch (buf[1]) {
	case '4':
		printf(_("Nessun testo. Post annullato.\n"));
		break;
	case '2': /* Accesso proibito */
		switch (buf[2]) {
		case '2':
			printf(sesso ? _("\nNon sei autorizzata a lasciare messaggi di amministrazione in questa room.\n") :_("\nNon sei autorizzato a lasciare messaggi di amministrazione in questa room.\n"));
			break;
		}
		break;
	case '5':
		printf(_("\nL'utente '%s' non esiste.\n"), caio);
		break;
	case '8':
		printf(_("Si sono verificati problemi con il file dei messaggi. Rivolgiti al Sysop.\n"));
		break;
	case '9':
		printf(_("Errore: Post non inizializzato.\n"));
		break;
	}
        md_free(&mdlist);
}

/*****************************************************************************/
/*
 * Read messages in current room.
 * mode: 0 - Forward;  1 - New;            2 - Reverse;
 *       3 - Brief;    5 - Ultimi cinque;  6 - Ultimi N
 *       7 - Msg # N;  8 -                 9 -
 *      10 - Next;    11 - Prev;          12 - Back;
 */
/* TODO define constants for the modes */
bool read_msg(int mode, long local_number)
{
        char aaa[LBUF];
	struct message *msg;

	barflag = 0;
	switch (mode) {
	case 6:
		serv_putf("READ %d|%d", mode, new_int("Quanti? "));
		break;
	case 7:
                if (local_number > 0)
                        serv_putf("READ %d|%ld", mode, local_number);
                else
                        serv_putf("READ %d|%ld", mode,
                                  new_long(_("Leggi messaggio #: ")));
		break;
	default:
		serv_putf("READ %d", mode);
		break;
	}
	serv_gets(aaa);
	if (aaa[0] != '2')
		return true;

	locnum = extract_long(aaa+4, 0);
	remain = extract_long(aaa+4, 1);
        msg = receive_msg(&postmd);

        /* Aggiorna l'ultimo messaggio visto, per l'again */
        if (last_msg) {
                if (last_msg->txt)
                        txt_free(&last_msg->txt);
                Free(last_msg);
                last_msg = NULL;
        }
        last_msg = msg;

        /* Passa in modo comandi per messaggi */
	prompt_curr = P_MSG;
        if (mode < 8)
                ciclo_msg();

        return false;
}

void show_msg(char *room, long num)
{
        Metadata_List mdlist;
        struct message *msg;
        bool spoiler = false, metadata = false;
        char buf[LBUF];

        serv_putf("RMSG %s|%ld", room, num);
        serv_gets(buf);
        if (buf[0] != '2') {
                print_err_test_msg(buf, room);
                return;
        }

        msg = receive_msg(&mdlist);

        if (msg->flags & MF_SPOILER) {
                spoiler = true;
	}
        if (mdlist.num > 0) {
                metadata = true;
	}

        if (metadata || spoiler) {
		bool fine = false;

                do {
			int ch = 0;

			setcolor(C_MSGPROMPT);
                        cml_printf(_(
"    \\<<b>a</b>>gain   \\<<b>A</b>>llegati   (altro tasto per continuare) --- <b>Scegli:</b> "
				     ));
			while (ch == 0) {
				ch = getchar();
			}
			if (ch == 'a' || ch == 'A') {
				putchar(ch);
			}
                        putchar('\n');
                        switch (ch) {
                        case 'a':
                                putchar('\n');
                                setcolor(C_NORMAL);
                                txt_rewind(msg->txt);
                                pager(msg->txt, false, 0, 0, true,
                                      IS_SET(msg->flags, MF_SPOILER), &mdlist);
                                setcolor(C_DEFAULT);
                                putchar('\n');
                                break;
                        case 'A':
                                if (mdlist.num)
                                        mdop(&mdlist);
                                else
                                        cml_printf(_("Non ci sono allegati associati a questo post.\n"));
                                break;
			default:
				fine = true;
                        }
                } while (!fine);
        }
        md_free(&mdlist);
        Free(msg);
}

/* Vai nella room 'room' in prompt lettura messaggio locale #num.
 * Restituisce True se ha successo, False altrimenti. */
bool goto_msg(char *room, long num)
{
        char *destroom;
        long destnum = num;
        char buf[LBUF];

        serv_putf("TMSG %s|%ld", room, num);
        serv_gets(buf);
        if (buf[0] != '2') {
                print_err_test_msg(buf, room);
                return false;
        }
        putchar('\n');
        destroom = Strdup(room);
        clear_last_msg(true);
        room_goto(2, false, destroom);
        read_msg(7, destnum);
        Free(destroom);
        return true;
}

static struct message * receive_msg(Metadata_List *mdlist)
{
        char aaa[LBUF], cmlstr[LBUF], col[LBUF];
        int hlines = 0;
        long reply_lnum, nlines;
        struct message *msg;

        serv_gets(aaa);
        if (aaa[0] != '2') {
                print_err_fm(aaa+4);
                return NULL;
        }

	CREATE(msg, struct message, 1, 0);

        extract(msg->room, aaa+4, 1);
        extract(msg->autore, aaa+4, 2);
        msg->ora = extract_long(aaa+4, 3);
        extract(msg->subject, aaa+4, 4);
        msg->flags = extract_long(aaa+4, 5);
        extract(msg->recipient, aaa+4, 6);
        extract(msg->reply_to, aaa+4, 7);
        reply_lnum = extract_long(aaa+4, 8);
        extract(msg->anonick, aaa+4, 9);
        nlines = extract_long(aaa+4, 10);

        /* This is to take care of a buf in citta_post() pre-0.4.5     */
        /* TODO eliminate after the file message has made a round trip */
        if (num_parms(aaa) < 11) {
                reply_lnum = 0;
                nlines = extract_long(aaa+4, 8);
        }

        putchar('\n');
        /* Costruisce prima riga di Header del messaggio */
        color2cml(cmlstr, C_MSG_HDR);
        if ((msg->flags & MF_ANONYMOUS) && !(rflags & RF_ASKANONYM)) {
                strcpy(msg->autore, _("Anonimo"));
                sprintf(msg->h1, "%s <b>****</b>", cmlstr);
        } else {
                if (!(msg->flags & MF_ANONYMOUS)) {
                        strcpy(last_profile, msg->autore);
                        strcpy(last_mail, msg->autore);
                }
                strdate(msg->h2, msg->ora);
                if (msg->flags & MF_ANONYMOUS) {
                        color2cml(col, C_MANONIMO);
                } else if (msg->flags & MF_CITTA) {
                        color2cml(col, C_MBBS);
                } else if (is_friend(msg->autore)) {
                        color2cml(col, C_MFRIEND);
                } else if (is_enemy(msg->autore)) {
                        color2cml(col, C_MENEMY);
                } else {
                        color2cml(col, C_MUSER);
                }
                if (msg->flags & MF_ANONYMOUS) {
                        if (msg->anonick[0] == 0) {
                                strcpy(msg->anonick, UTENTE_ANONIMO);
                        }
                        sprintf(msg->h1,
                                _("%s <b>%s</b>, %s - da [%s%s%s]"),
                                cmlstr, msg->room, msg->h2, col, msg->anonick,
                                cmlstr);
                } else {
                        sprintf(msg->h1,
                                _("%s <b>%s</b>, %s - da %s%s%s"),
                                cmlstr, msg->room, msg->h2, col, msg->autore,
                                cmlstr);
                }
                strcpy(msg->h2, "");
                if (msg->flags & MF_RH) {
                        sprintf(msg->h2, " (%s Message)", NOME_ROOMHELPER);
                } else if (msg->flags & MF_RA) {
                        sprintf(msg->h2, " (%s Message)", NOME_ROOMAIDE);
                } else if (msg->flags & MF_AIDE) {
                        sprintf(msg->h2, " (%s Message)", NOME_AIDE);
                } else if (msg->flags & MF_SYSOP) {
                        sprintf(msg->h2, " (%s Message)", NOME_SYSOP);
                }
                strcat(msg->h1, msg->h2);
                if (msg->flags & MF_MAIL) {
                        sprintf(msg->h1+strlen(msg->h1),
                                _(" per <b>%s</b>"), msg->recipient);
                }
        }

        /* Costruisce seconda riga di header del messaggio */
        if (reply_lnum) { /* E' un reply */
                color2cml(cmlstr, C_MSG_SUBJ);
                if (strlen(msg->reply_to) == 0)
                        strcpy(msg->reply_to, UTENTE_ANONIMO);
                if (msg->subject[0]) {
                        sprintf(msg->h2,
_("%s Reply: <b>%s</b>, msg #<b>%ld</b> da <b>%s</b>"),
                                cmlstr, msg->subject, reply_lnum,
                                msg->reply_to);
                } else {
                        sprintf(msg->h2,
                          _("%s Reply al Messaggio #<b>%ld</b> da <b>%s</b>"),
                                cmlstr, reply_lnum, msg->reply_to);
                }
        } else if (msg->subject[0]) {
                color2cml(cmlstr, C_MSG_SUBJ);
                sprintf(msg->h2, _("%s Subject: <b>%s</b>"), cmlstr,
                        msg->subject);
        } else {
                msg->h2[0] = '\0';
        }

        msg->txt = txt_create();
        txt_put(msg->txt, msg->h1);
        cml_printf("%s",msg->h1);
        putchar('\n');
        hlines++;
        if (msg->h2[0] != '\0') {
                txt_put(msg->txt, msg->h2);
                cml_printf("%s",msg->h2);
                putchar('\n');
                hlines++;
        }
        color2cml(cmlstr, C_MSG_BODY);
        txt_putf(msg->txt, "%s", cmlstr);
        setcolor(C_MSG_BODY);

        md_init(mdlist);

        if (msg->flags & MF_SPOILER) {
                cml_printf(_(
"\n   *** <b>SPOILER!</b> ***   Premi <a> per leggere il messaggio.\n"
			     ));
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        txt_put(msg->txt, aaa+4);
                        cml_extract_md(aaa+4, mdlist);
                }
        } else {
	        /* we add 1 to nlines (arg 10 from server: num lines in body)
		   to account for the empty line between header and body.   */
                msg_pager(msg->txt, nlines + 1, hlines, true, false, mdlist);
	}
        setcolor(C_DEFAULT);
        putchar('\n');

        return msg;
}

static void ciclo_msg(void)
{
        int cmd;
	bool fine;
	char cmdstr[LBUF], *qt;

	fine = false;
        while ( (cmd = get_msgcmd(cmdstr, postmd.num)) != 0) {
		del_prompt();
		switch (cmd) {
		case 1: /* next */
                        md_free(&postmd);
                        md_init(&postmd);
			fine = read_msg(10, -1);
			break;
		case 2: /* back */
                        md_free(&postmd);
                        md_init(&postmd);
			fine = read_msg(11, -1);
			break;
		case 3: /* again */
			if (last_msg) {
				putchar('\n');
				setcolor(C_NORMAL);
				txt_rewind(last_msg->txt);
				pager(last_msg->txt, false, 0, 0, true,
                                      IS_SET(last_msg->flags, MF_SPOILER),
				      &postmd);
				setcolor(C_DEFAULT);
                                putchar('\n');
			} else
				fine = read_msg(12, -1);
			break;
		case 4:
			if (!cestino)
				copy_message();
			break;
		case 5:
			if (cestino)
				undelete_message();
			else
				delete_message();
			break;
		case 6:
			if (!cestino)
				move_message();
			break;
		case 7: /* reply */
			if (!cestino) {
				enter_message(-1, true, 0);
				if (uflags[4] & UT_AGAINREP)
					fine = read_msg(12, -1);
			}
                        break;
		case 8: /* quote */
			if (last_msg->txt == NULL)
				break;
			if (quoted_text)
				txt_free(&quoted_text);
			quoted_text = txt_dup(last_msg->txt);
			cml_printf(_(" *** <b>Message Quoted.</b>\n"));
                        break;
		case 9: /* free quote */
			if (quoted_text) {
				txt_free(&quoted_text);
				cml_printf(_(" *** <b>'Quote' cancellato.</b>\n"));
			}
                        break;
		case 10: /* examine quote */
			if (!quoted_text) {
				printf(_(" *** Nessun 'quote' presente.\n"));
				break;
			}
			cml_printf(_(" *** <b>Quotation:</b>\n"));
			txt_rewind(quoted_text);
			while((qt = txt_get(quoted_text)))
				cml_printf("> %s\n", qt);
                        break;
		case 11:
			msg_dump(last_msg->txt, 0);
			break;
                case 12: /* Esamina il metadata */
                        if (mdop(&postmd))
                                return;
                        break;
		case 13: /* Registra riferimento a post */
                        strcpy(postref_room, current_room);
                        postref_locnum = locnum;
			cml_printf(_(" *** <b>Il riferimento a questo post registrato.</b>\n"));
                        break;
		case 14: /* Vedi sorgente CML */
			if (last_msg) {
				putchar('\n');
				setcolor(C_NORMAL);
				txt_rewind(last_msg->txt);
				pager(last_msg->txt, false, 0, 0, false,
				      false, NULL);
				putchar('\n');
                        }
			break;
		case 100:
			profile(NULL);
			putchar('\n');
			break;
		case 101:
			ora_data();
			putchar('\n');
			break;
		case 102:
			list_host();
			putchar('\n');
			break;
		case 103:
			lock_term();
			putchar('\n');
			break;
		case 110:
			express(NULL);
			putchar('\n');
			break;
		case 111:
			express(last_xfu);
			putchar('\n');
			break;
		case 199:
			if (!(uflags[1] & UT_DISAPPEAR))
				putchar('\n');
			push_color();
			setcolor(C_HELP);
			if (!cestino)
				cml_printf(_(
" <b>---X Comandi di Lettura X---</b>\n"
" <a>gain: Visualizza nuovamente il messaggio.\n"
" <A>llegati: esamina gli allegati associati al messaggio.\n"
" \\<b>ack: Legge il messaggio precedente.\n"
" <c>opy: Copia il messaggio in un'altra room.\n"
" <C>ML: visualizza il sorgente CML del post.\n"
" <D>elete: Cancella il messaggio.\n"
" <d>ump to file: salva il post in DUMP_FILE.\n"
" <e>xamine quote: vedi il testo citato.\n"
" \\<f>ree quote: cancella la citazione.\n"
" <m>ove: Sposta il messaggio in un'altra room.\n"
" <n>ext: Legge il prossimo messaggio.\n"
" <P>ost reference: memorizza il riferimento a questo post.\n"
" <q>uote: Memorizza il messaggio per citarlo nel prossimo post.\n"
" <r>eply: Rispondi al messaggio.\n"
" <s>top: Esce dal prompt di lettura dei messaggi.\n"
"\n <b>---X Altri comandi X---</b>\n"
" <p>rofile    <t>ime & date        <w>ho\n"
" <x>-msg      <v> Rispondi Xmsg    <%%> Lock term    <?> Questo aiuto\n\n"));
			else
				cml_printf(_(
" <b>---X Comandi di Lettura X---</b>\n"
" <a>gain: Visualizza nuovamente il messaggio.\n"
" <A>llegati: esamina gli allegati associati al messaggio.\n"
" \\<b>ack: Legge il messaggio precedente.\n"
" <C>ML: visualizza il sorgente CML del post.\n"
" un<D>elete: Reimmetti il messaggio.\n"
" <d>ump to file: salva il post in DUMP_FILE.\n"
" <e>xamine quote: vedi il testo citato.\n"
" \\<f>ree quote: cancella la citazione.\n"
" <n>ext: Legge il prossimo messaggio.\n"
" <P>ost reference: memorizza il riferimento a questo post.\n"
" <q>uote: Memorizza il messaggio per citarlo nel prossimo post.\n"
" <s>top: Esce dal prompt di lettura dei messaggi.\n"
"\n <b>---X Altri comandi X---</b>\n"
" <p>rofile    <t>ime & date        <w>ho\n"
" <x>-msg      <v> Rispondi Xmsg    <%%> Lock term    <?> Questo aiuto\n\n"));
			pull_color();
			break;
		}
		if (fine) {
                        clear_last_msg(false);
			return;
		}
	}
        clear_last_msg(true);
}

void clear_last_msg(bool clear_md)
{
	if (last_msg) {
		if (last_msg->txt)
			txt_free(&last_msg->txt);
		Free(last_msg);
		last_msg = NULL;
	}
        if (clear_md)
                md_free(&postmd);
}

static void delete_message(void)
{
	char buf[LBUF];

	printf(sesso ? _("Sei sicura di voler cancellare questo messaggio (s/n)? ") : _("Sei sicuro di voler cancellare questo messaggio (s/n)? "));
	if (si_no() != 's')
		return;
	serv_puts("MDEL");
	serv_gets(buf);
	if (buf[0]=='2')
		cml_print(_("Ok, il messaggio &egrave; stato cancellato.\n\n"));
}

static void undelete_message(void)
{
	char buf[LBUF];

	printf(sesso ? _("Sei sicura di voler riesumare questo messaggio (s/n)? ") : _("Sei sicuro di voler riesumare questo messaggio (s/n)? "));
	if (si_no() != 's')
		return;
	serv_puts("MDEL");
	serv_gets(buf);
	if (buf[0] == '2')
		cml_print(_("Ok, il messaggio &egrave; stato rimesso nella room d'origine.\n\n"));
}

static void move_message(void)
{
	char buf[LBUF], room[LBUF];

	printf(_("Room di destinazione : "));
	get_roomname("", room, false);
	if (strlen(room) == 0)
		return;
	if (!strcmp(room, "Mail")) {
		printf(_("Non puoi spostare i messaggi in mail.\n\n"));
		return;
	}
	printf(_("Vuoi procedere allo spostamento (s/n)? "));
	if (si_no() != 's')
		return;
	serv_putf("MMOV %s", room);
	serv_gets(buf);
	if (buf[0]=='2')
		cml_print(_("Ok, il messaggio &egrave; stato spostato.\n\n"));
	else
		printf(sesso ? _("Non sei autorizzata a spostare questo messaggio in %s.\n\n") : _("Non sei autorizzato a spostare questo messaggio in %s.\n\n"), room);
}

static void copy_message(void)
{
	char buf[LBUF], room[LBUF];

	get_roomname(_("Room di destinazione : "), room, false);
	if (strlen(room) == 0)
		return;
	if (!strcmp(room, "Mail")) {
		printf(_("Non puoi copiare i messaggi in mail.\n\n"));
		return;
	}
	printf(_("Vuoi procedere (s/n)? "));
	if (si_no() != 's')
		return;
	serv_putf("MCPY %s", room);
	serv_gets(buf);
	if (buf[0]=='2')
		cml_printf(_("Ok, il messaggio &egrave; stato copiato.\n\n"));
	else
		printf(sesso ? _("Non sei autorizzata a copiare questo messaggio in %s.\n\n") : _("Non sei autorizzato a copiare questo messaggio in %s.\n\n"), room);
}

static int entry_cmd(void)
{
	int c = 0;
	char prompt_tmp;

	prompt_tmp = prompt_curr;
	prompt_curr = P_ENT;
	printf(_("Entry cmd (? lista opzioni) -> "));
	while ((c!='a') && (c!='s') && (c!='c') && (c!='h') && (c!='p')) {
		c = inkey_sc(0);
		if ((c == '?') || (c == Key_F(1)))
			printf(_("Help\n  <a>bort\n  <c>ontinue\n  <h>old\n"
				 "  <p>rint\n  <s>ave\n\n"
				 "Entry cmd (? lista opzioni) -> "));
		c = tolower(c);
	}
	prompt_curr = prompt_tmp;
	return c;
}

/* Setta tutti i messaggi di tutte le room conosciute come letti. */
void set_all_read(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler lasciar perdere tutti i nuovi messaggi (s/n)? ") : _("\nSei sicuro di voler lasciar perdere tutti i nuovi messaggi (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_puts("RALL");
	serv_gets(buf);
	if (buf[0] != '2')
		printf(_("Tsk, tsk... te li devi leggere proprio tutti!\n"));
}
