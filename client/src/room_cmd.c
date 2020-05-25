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
* File : room_cmd.c                                                         *
*        Comandi per la gestione delle room                                 *
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ansicol.h"
#include "cittacfg.h"
#include "cittaclient.h"
#include "cml.h"
#include "comandi.h"
#include "conn.h"
#include "cterminfo.h"
#include "edit.h"
#include "editor.h"
#include "errore.h"
#include "extract.h"
#include "room_cmd.h"
#include "room_flags.h"
#include "tabc.h"
#include "msg_flags.h"
#include "user_flags.h"
#include "utility.h"
#include "macro.h"

const char blog_display_pre[] = BLOG_DISPLAY_PRE;

static int room_print_known(char mode, char floors, int riga);
static void debug_print_room_flags(void);

/***************************************************************************/
/***************************************************************************/
/*
 * Displays the room name with the correct colors and if it's a blog room
 * (ie the name starts with ':')prepends the blog_display_pre string.
 */
void print_room(const char *roomname)
{
	int offset = 0;

	push_color();
	if (roomname[0] == ':') {
		/* Blog room */
		setcolor(COLOR_ROOMTYPE);
		printf("%s", blog_display_pre);
		offset = 1;
	}
	setcolor(COLOR_ROOM);
	printf("%s", roomname + offset);
	setcolor(COLOR_ROOMTYPE);
	putchar('>');
	pull_color();
}


/*
 * Lista di tutte le room (SYSOP only).
 */
void room_list (void)
{
        char aaa[LBUF], ra[MAXLEN_UTNAME], fstr[LBUF];
        char nm[ROOMNAMELEN];
	int rlvl, wlvl, riga;
        long num, maxmsg, ora, flags, msgnum;
        struct tm *tmst;

        serv_puts("RLST");
        serv_gets(aaa);
        if (aaa[0]=='2') {
		printf(_("\nNum      Room Name       Post/Max LastPost  Flags    r/w       Room Aide"
			 "\n--- -------------------- -------- -------- -------- ----- -----------------\n"));
                riga = 4;
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        riga++;
                        if (riga == NRIGHE) {
                                hit_any_key();
                                riga = 1;
                        }
                        num = extract_long(aaa+4, 0);
                        extractn(nm, aaa+4, 1, ROOMNAMELEN);
                        /* fmnum = extract_long(aaa+4, 3); */
                        maxmsg = extract_long(aaa+4, 4);
                        ora = extract_long(aaa+4, 6);
                        tmst = localtime(&ora);
                        flags = extract_long(aaa+4, 7);
                        msgnum = extract_long(aaa+4, 8);
                        rlvl = extract_long(aaa+4, 9);
                        wlvl = extract_long(aaa+4, 10);
                        extractn(ra, aaa+4, 11, MAXLEN_UTNAME);
			sprintf(fstr, "%8s", "");
			if (flags & RF_BASE)
				fstr[0] = 'B';
			if (flags & RF_MAIL)
				fstr[0] = 'M';
			if (flags & RF_ENABLED)
				fstr[1] = '.';
			else
				fstr[1] = '!';
			if (flags & RF_SUBJECT)
				fstr[2] = 'S';
			if (flags & RF_REPLY)
				fstr[3] = 'R';
			if (flags & RF_ANONYMOUS)
				fstr[4] = 'A';
			else if (flags & RF_ASKANONYM)
				fstr[4] = 'a';
			if (flags & RF_INVITO) {
				if (flags & RF_AUTOINV)
					fstr[5] = 'I';
				else
					fstr[5] = 'i';
			} else if (flags & RF_GUESS)
				fstr[5] = '?';
			if (flags & RF_FIND)
				fstr[6] = 'F';
			if (flags & RF_CITTAWEB)
				fstr[7] = 'W';
			if (tmst->tm_year > 99)
				tmst->tm_year -= 100;
                        printf("%3ld %-20s %4ld/%3ld %02d/%02d/%02d %8s %2d/%2d %s\n",
                               num, nm, msgnum, maxmsg, tmst->tm_mday,
			       tmst->tm_mon+1, tmst->tm_year, fstr, rlvl,
			       wlvl, ra);
                }
        }
}

/*
 * Elenca le room conosciute, a seconda del modo:
 * mode : 0 - tutte le room              1 - Room con messaggi non letti
 *        2 - Room senza messaggi nuovi  3 - Tutte le room zappate
 *        4 - Room del piano corrente    5 - <k>nown rooms 'classico'
 */
void room_list_known(int mode)
{
	int riga;

	switch (mode) {
	case 0:
	case 1:
	case 2:
	case 3:
		printf("\n");
		room_print_known(mode, 0, 3);
		break;
	case 4:
		setcolor(C_KR_HDR);
		printf(_("\n   Room con messaggi da leggere in questo floor:\n"));
		riga = room_print_known(1, 1, 4);
		setcolor(C_KR_HDR);
		printf(_("\n   Non ci sono messaggi nuovi in:\n"));
		riga++;
		riga = room_print_known(2, 1, riga);
		setcolor(C_KR_HDR);
		cml_printf(_("\n<b>   Altri floor con messaggi da leggere:</b>\n"));
		riga++;
		riga = room_print_known(1, 2, 4);
		setcolor(C_KR_HDR);
		cml_printf(_("\n<b>   Floor senza messaggi nuovi in:</b>\n"));
		riga++;
		room_print_known(2, 2, riga);
		break;
	case 5:
		setcolor(C_KR_HDR);
		printf(_("\n   Room con messaggi da leggere:\n"));
		riga = room_print_known(1, 0, 4);
		setcolor(C_KR_HDR);
		printf(_("\n   Non ci sono messaggi nuovi in:\n"));
		riga++;
		room_print_known(2, 0, riga);
		break;
	}
	setcolor(C_NORMAL);
	IFNEHAK;
}

/*
 * Vai in un altra room.
 * mode: 0 - goto -> set all messages read.
 *       1 - skip -> leave fullroom unchanged.
 *       2 - abandon -> only set read the effectively read msgs.
 *       3 - Scan -> next known room, even if there are no new messages.
 *       4 - Home -> Torna alla Lobby) senza prendere le ventimila.
 *       5 - Mail -> Abbandona e vai nella mailroom.
 *       6 - Home -> Abbandona e vai in Lobby).
 *       7 - Blog -> Abbandona e vai a casa.
 *       (Nei modi 0, 1 e 2 va nelle room con nuovi messaggi)
 * gotonext : FALSE --> chiede la room di destinazione (jump)
 *            TRUE  --> va nella prossima room
 * room_goto(4, 1) --> vai in Lobby), room_goto(5,1) --> vai in Mail)
 * room_goto(7, 1) --> vai a casa (blog).
 */
void room_goto(int mode, bool gotonext, const char *destroom)
{
	char buf[LBUF], roomname[LBUF];
	long msg_num, msg_new, room_new, room_zap;
        int dest = GOTO_ROOM, offset = 0;

	if (gotonext)
		roomname[0] = 0;
        else {
                if (destroom)
                        strcpy(roomname, destroom);
                else
                        get_roomname("", roomname, true);
                if (roomname[0] == 0)
                        return;
                else if (roomname[0] == ':') { /* Blog room */
                        dest = GOTO_BLOG;
                        offset++;
                }
        }
        if (mode == 7) {
                dest = GOTO_BLOG;
	}
	serv_putf("GOTO %d|%s|%d", mode, roomname+offset, dest);
	serv_gets(buf);
	if (buf[0] == '2') {
                barflag = 1;
                if (dest == GOTO_BLOG) {
                        extractn(roomname, buf+4, 0, ROOMNAMELEN);
                        sprintf(current_room, "</b>%s<b>%s", blog_display_pre,
				roomname);
                } else {
                        extractn(current_room, buf+4, 0, ROOMNAMELEN);
		}

		extract(room_type, buf+4, 1);
		msg_num = extract_long(buf+4, 2);
		msg_new = extract_long(buf+4, 3);
		utrflags = extract_long(buf+4, 4);
		rflags = extract_long(buf+4, 5);
		room_new = extract_long(buf+4, 6);
		room_zap = extract_long(buf+4, 7);
		extractn(current_floor, buf+4, 8, FLOORNAMELEN);

                if (DEBUG_MODE) {
			debug_print_room_flags();
                }
		if (!msg_new) {
			barflag = 0;
		}
		setcolor(C_COMANDI);
		cml_printf((msg_new == 1)
			   ? _(
"<b>%s</b>%s - <b>%ld</b> nuovo su <b>%ld</b> messaggi.\n"
			       ) : _(
"<b>%s</b>%s - <b>%ld</b> nuovi su <b>%ld</b> messaggi.\n"
				     ),
			   current_room, room_type, msg_new, msg_num);
                if (room_new || room_zap) { /* Altre info in Lobby */
                        if (room_zap == 1) {
                                cml_printf(_(
"         Hai <b>%ld</b> room con messaggi non letti "
"e <b>1</b> room dimenticata.\n"
					     ), room_new);
			} else {
                                cml_printf(_(
"         Hai <b>%ld</b> room con messaggi non letti "
"e <b>%ld</b> room dimenticate.\n"
					     ), room_new, room_zap);
			}
                }
                if (utrflags & UTR_NEWGEN) {
			room_info(true, true);
		}
		/* TODO */
		/* Attenzione!! il nome della dump room dovrebbe venire */
                /* inviato dal server all'inizio del collegamento!!     */
		if (!strcmp(current_room, "Cestino")) {
			cestino = true;
		} else {
			cestino = false;
		}
        } else {
		if (buf[1] == '7') {
                        if (offset) {
                                printf(_("Il blog di '%s' non esiste.\n"),
                                       roomname + offset);
                        } else {
                                printf(_("La room '%s' non esiste.\n"),
                                       roomname);
			}
                } else {
			printf(sesso
			       ? _(
			    "Non sei autorizzata ad accedere alla room '%s'.\n"
				   ) : _(
		            "Non sei autorizzato ad accedere alla room '%s'.\n"
					 ),
			       roomname + offset);
		}
	}
}

/*
 * Crea una nuova room.
 */
void room_create(void)
{
	char name[ROOMNAMELEN], buf[LBUF];
	long fmnum, maxmsg, num, nnew, nnum;

	new_str_M(_("Nome della Room: "), name, ROOMNAMELEN-1);
	if (strlen(name) == 0)
		return;
        /* NON USO FM multipli
	fmnum = new_long_def(_("Numero File Messaggi (0: Basic): "), 0);
        */
        fmnum = 0;
	maxmsg = new_long_def(_("Numero massimo di messaggi: "),
			      DFLT_MSG_PER_ROOM);
	serv_putf("RNEW %s|%ld|%ld", name, fmnum, maxmsg);
	serv_gets(buf);
	if (buf[0] == '2') {
		num = extract_long(&buf[4], 0);
		cml_printf(_("\nOk, la room #%ld [%s] &egrave; stata creata.\n"
			     "\n"), num, name);
		serv_putf("GOTO %d|%s", 0, name);
		serv_gets(buf);
		if (buf[0] == '2') {
			extractn(current_room, &buf[4], 0, ROOMNAMELEN);
                        extract(room_type, &buf[4], 1);
			nnum = extract_long(buf+4, 2);
			nnew = extract_long(buf+4, 3);
			utrflags = extract_long(buf+4, 4);
			rflags = extract_long(buf+4, 5);
			barflag = 0;
			cml_printf((nnum == 1)
				   ? _(
"<b>%s</b> - <b>%ld</b> nuovo su <b>%ld</b> messaggi.\n"
				       ) : _(
"<b>%s</b> - <b>%ld</b> nuovi su <b>%ld</b> messaggi.\n"
					     ),
				   current_room, nnew, nnum);
			room_edit(0);
			room_info(false, false);
		} else
			cml_print(_("...per&oacute; non riesco ad entrarci... mah!\n"));
	} else {
		switch(buf[1]) {
		case '2':
			printf(sesso
			       ? _(
				   "Non sei autorizzata a creare nuove room.\n"
				   )
			       : _(
				   "Non sei autorizzato a creare nuove room.\n"
				   ));
			break;
		case '8':
			printf(_("Numero File Messaggi non valido.\n"));
			break;
		default:
			printf(_("Errore: %s\n"), buf+4);
			break;
		}
	}
}

/*
 * Edita la configurazione della room corrente.
 * mode = 0 : non modifica il nome della room, 1: chiede anche il nome.
 */
void room_edit(int mode)
{
	char name[LBUF], room_master[LBUF], buf[LBUF];
	int rlvl, wlvl;
	long flags, newflags = 0;

	serv_puts("REDT 0"); /* Get conf */
	serv_gets(buf);
	if (buf[0] == '2') {
		extract(name, buf+4, 0);
		/* tipo = extract_int(buf+4, 1); */
		rlvl = extract_int(buf+4, 2);
		wlvl = extract_int(buf+4, 3);
		/* fmnum = extract_long(&buf[4], 4); */
		/* maxmsg = extract_long(&buf[4], 5); */
		flags = extract_long(&buf[4], 6);
		extract(room_master, buf+4, 7);
		if (mode)
			new_str_def_M(_("Nome della Room"), name,
				      ROOMNAMELEN - 1);
		rlvl = new_int_def(_("Livello minimo per lettura"), rlvl);
		wlvl = new_int_def(_("Livello minimo per scrittura"), wlvl);
                /*
                fmnum = new_long_def(_("Numero File Messaggi"), fmnum);
                maxmsg = new_long_def(_("Numero massimo di messaggi: "), maxmsg);
                */
		if (new_si_no_def(_("Subject"), IS_SET(flags, RF_SUBJECT)))
			newflags |= RF_SUBJECT;
		if (new_si_no_def(_("Reply"), IS_SET(flags, RF_REPLY)))
			newflags |= RF_REPLY;
		if (new_si_no_def(_("Invito"), IS_SET(flags, RF_INVITO)))
			newflags |= RF_INVITO;
		if (newflags & RF_INVITO) {
			if(new_si_no_def(_("Invito automatico"),
					 IS_SET(flags, RF_AUTOINV)))
				newflags |= RF_AUTOINV;
		} else if (new_si_no_def(_("Guess Room"),
					 IS_SET(flags, RF_GUESS)))
			newflags |= RF_GUESS;
		if (new_si_no_def(_("Upload/Download files"),
                                  IS_SET(flags, RF_DIRECTORY)))
                        newflags |= RF_DIRECTORY;
		if (new_si_no_def(_("Messaggi anonimi"),
				  IS_SET(flags, RF_ANONYMOUS)))
			newflags |= RF_ANONYMOUS;
		if (!(newflags & RF_ANONYMOUS))
			if (new_si_no_def(_("Post anonimo facoltativo"),
                                          IS_SET(flags, RF_ASKANONYM)))
                                newflags |= RF_ASKANONYM;
		if (new_si_no_def(_("Messaggi con spoiler"),
				  IS_SET(flags, RF_SPOILER)))
			newflags |= RF_SPOILER;
		if (new_si_no_def(_("Un solo post alla volta"),
				  IS_SET(flags, RF_POSTLOCK)))
			newflags |= RF_POSTLOCK;
		if (newflags & RF_POSTLOCK)
			if (new_si_no_def(_("Tempo massimo per postare"),
                                          IS_SET(flags, RF_L_TIMEOUT)))
                                newflags |= RF_L_TIMEOUT;
		if (new_si_no_def(_("Sondaggi per tutti"),
				  IS_SET(flags, RF_SONDAGGI)))
			newflags |= RF_SONDAGGI;
		if (new_si_no_def(_("Ricerca nei messaggi"),
				  IS_SET(flags, RF_FIND)))
			newflags |= RF_FIND;
		if (new_si_no_def(_("Pubblica su Web"),
				  IS_SET(flags, RF_CITTAWEB)))
			newflags |= RF_CITTAWEB;
		if (new_si_no_def(_("Attivata"), IS_SET(flags, RF_ENABLED)))
			newflags |= RF_ENABLED;
		flags = newflags;
		if (mode == 1) {
			printf(_("\nVuoi mantenere le modifiche (s/n)? "));
			if (si_no() == 'n') {
				serv_putf("REDT 2");
				serv_gets(buf);
				return;
			}
		}
		serv_putf("REDT 1|%s|%d|%d|%ld", name, rlvl, wlvl, flags);
		serv_gets(buf);
		cml_printf(_("\nOk, la room [%s] &egrave; stata editata.\n"),
			   name);
	} else if (buf[1] == '2')
		printf(sesso ? _("Non sei autorizzata a editare le room.\n") :
		       _("Non sei autorizzato a editare le room.\n"));
	else
	        cml_printf(_("Le room virtuali non si possono editare.\n"));
}

/*
 * Elimina la room corrente.
 */
void room_delete(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler eliminare questa room (s/n)? ")
	       : _("\nSei sicuro di voler eliminare questa room (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_puts("RDEL");
	serv_gets(buf);
	if (buf[0] == '2') {
		cml_print(_("\nOk, la room &egrave; stata eliminata.\n"));
		room_goto(4, true, NULL);
	} else if (buf[1] == '2')
		printf(sesso ? _("Non sei autorizzata a cancellare le room.\n")
		       : _("Non sei autorizzato a cancellare le room.\n"));
	else if (buf[2] == '1')
	        cml_printf(_("Le room di base non si possono cancellare.\n"));
	else
	        cml_printf(_("Le room virtuali non si possono cancellare.\n"));
}

static void X(char *str, long flag)
{
        if (flag) {
                setcolor(C_ROOM_INFO_X);
                putchar('X');
        } else
                putchar(' ');
        setcolor(C_ROOM_INFO_FLAG);
        cml_printf(" %s", str);
}

/*
 * Legge le info sulla room corrente.
 * Se detailed == true, da informazioni piu' dettagliate.
 * La descrizione della room viene stampata solo se prof == TRUE
 */
void room_info(bool detailed, bool prof)
{
	char buf[LBUF], raide[LBUF], rname[LBUF];
	int  rlvl, wlvl, riga = 0;
	long local, ct, mt, pt, flags;

	/* TODO implementare il flag prof */
	IGNORE_UNUSED_PARAMETER(prof);

        if ((*room_type == ':') && !(rflags & RF_BLOG)) {
                setcolor(C_ROOM_INFO);
                cml_printf(_("\nQuesta &egrave; una room virtuale che contiene i risultati della tua ricerca.\nPuoi leggere i post trovati allo stesso modo che nelle altre room.\n"));
                setcolor(C_DEFAULT);
                IFNEHAK;
                return;
        }

	serv_puts("RINF");
	serv_gets(buf);
	if (buf[0] != '2') {
		printf(_(" *** Problemi con il server.\n"));
		return;
	}
	rlvl = extract_int(buf+4, 0);
	wlvl = extract_int(buf+4, 1);
	local = extract_long(buf+4, 2);
	ct = extract_long(buf+4, 3);
	mt = extract_long(buf+4, 4);
	pt = extract_long(buf+4, 5);
	extract(raide, buf+4, 6);
	extract(rname, buf+4, 7);
      	flags = extract_long(buf+4, 8);

        printf("\n%63s", "");
        if (detailed)
                X("Attivata<u>!</u>", flags & RF_ENABLED);
	setcolor(C_ROOM_INFO);
        if (rflags & RF_BLOG) {
                cml_printf(_("\r Blog di "));
                setcolor(C_ROOM_INFO_USER);
        } else {
                cml_printf(_("\r Room '"));
                setcolor(C_ROOM_INFO_ROOM);
        }
        printf("%s", rname);
	setcolor(C_ROOM_INFO);
        if (rflags & RF_BLOG)
                cml_printf(_(" creato il "));
        else
                cml_printf(_("' creata il "));
	stampa_giorno(ct);
        riga++;

        printf("\n%63s", "");
        if (detailed)
                X("Su <u>i</u>nvito", flags & RF_INVITO);
        if (!(rflags & RF_BLOG)) {
                setcolor(C_ROOM_INFO);
                cml_printf(_("\r Room Aide: "));
                setcolor(C_ROOM_INFO_USER);
                cml_printf("%s", raide);
                setcolor(C_ROOM_INFO);
                cml_printf(_(" - r/w lvl: "));
                setcolor(C_ROOM_INFO_NUM);
                printf("%d", rlvl);
                setcolor(C_ROOM_INFO);
                putchar('/');
                setcolor(C_ROOM_INFO_NUM);
                printf("%d", wlvl);
                riga++;
        }

        printf("\n%63s", "");
        if (detailed)
                X("<u>G</u>uess room", flags & RF_GUESS);
        putchar('\r');
        switch(local) {
        case 0:
                setcolor(C_ROOM_INFO);
                cml_print(_(" Nessun messaggio dalla creazione."));
                break;
        case 1:
                setcolor(C_ROOM_INFO);
                cml_print(_(" &Egrave; stato lasciato un post dalla creazione.\n\n"));
                break;
        default:
                setcolor(C_ROOM_INFO_NUM);
                printf(" %ld ", local);
                setcolor(C_ROOM_INFO);
                cml_printf(_("post dalla sua creazione."));
        }
        riga++;
        putchar('\n');
        if (detailed) {
                printf("\n ");
                //putchar(' ');
                X("Subject", flags & RF_SUBJECT);
                printf("   ");
                X("Pubblicata su Citta<u>W</u>eb", flags & RF_CITTAWEB);
                printf("   ");
                X("<u>U</u>pload/download   ", flags & RF_DIRECTORY);
                printf("   ");
                X("<u>S</u>ondaggi", flags & RF_SONDAGGI);
                printf("\n ");

                X("<u>R</u>eply  ", flags & RF_REPLY);
                printf("   ");
                X("Messaggi <u>a</u>nonimi      ", flags & RF_ANONYMOUS);
                printf("   ");
                X("Un <u>p</u>ost alla volta", flags & RF_POSTLOCK);
                printf("   ");
                X("<u>R</u>icercabile", flags & RF_FIND);
                printf("\n ");

                X("Spoi<u>l</u>er", flags & RF_SPOILER);
                printf("   ");
                X("Anonimo <u>f</u>acoltativo   ", flags & RF_ASKANONYM);
                printf("   ");
                X("<u>T</u>empo limitato    ", flags & RF_L_TIMEOUT);
                putchar('\n');
                putchar('\n');
                riga += 5;
        }
        setcolor(C_ROOM_INFO);
        if (pt) {
                printf(_(" Ultimo post: "));
                stampa_data_breve(pt);
                printf(" -");
        }
        if (mt != ct) {
                printf(_(" Ultima modifica: "));
                stampa_data_breve(mt);
        } else
                cml_printf(_("Questa room non ha subito modifiche."));
        putchar('\n');
        riga++;

        setcolor(C_ROOM_INFO_DESC);
	if (buf[1] == '1') {
		riga += 2;
                if (!(rflags & RF_BLOG))
                        cml_print(_("\n <b>Descrizione della room:</b>\n\n"));
		while (serv_gets(buf), strcmp(buf, "000")) {
			riga++;
                        if (riga == (NRIGHE - 1)) {
                                if (uflags[1] & UT_PAGINATOR)
                                        hit_any_key();
                                riga = 1;
                        }
			cml_printf("%s\n", buf+4);
		}
	} else
		cml_print(_("\n Non &egrave; attualmente presente una descrizione di questa room.\n\n"));
        riga += 2;

        setcolor(C_DEFAULT);
	IFNEHAK;
}

/*
 * Edita descrizione room corrente
 */
void room_edit_info(void)
{
	struct text *txt = NULL;
	char buf[LBUF];
	char *filename, *str;
	int fd;
	int riga, ext_editor;
	FILE *fp;

	ext_editor = ((uflags[0] & UT_EDITOR) && (EDITOR != NULL)) ? 1 : 0;
	if (ext_editor) {
		filename = astrcat(TMPDIR, TEMP_EDIT_TEMPLATE);
		fd = mkstemp(filename);
		close(fd);
		chmod(filename, 0660);
		fp = fopen(filename, "w");
	}

	serv_puts("RINF");
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(_("\n*** Problemi con il server.\n"));
		fclose(fp);
		return;
	}
	txt = txt_create();
	if (buf[1] == '1') {
		if (ext_editor) {
			while (serv_gets(buf), strcmp(buf, "000"))
				fprintf(fp, "%s\n", &buf[4]);
			fclose(fp);
		} else {
			while (serv_gets(buf), strcmp(buf, "000"))
				txt_put(txt, &buf[4]);
		}
	}

	serv_puts("RIEB");
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(sesso ? _("\nNon sei autorizzata.\n")
		       : _("\nNon sei autorizzato.\n"));
		txt_free(&txt);
		return;
	}
	putchar('\n');

	if (ext_editor) {
			edit_file(filename);
			fp = fopen(filename, "r");
			if (fp == NULL) {
				printf(_("\nProblemi di I/O\n"));
				serv_puts("ATXT");
				serv_gets(buf);
				txt_free(&txt);
				return;
			}
			riga = 0;
			while ((fgets(buf, LBUF, fp) != NULL)
			       && (riga < serverinfo.maxlineeroominfo)) {
				txt_put(txt, buf);
				riga++;
			}
			fclose(fp);
			unlink(filename);
			free(filename);
	} else {
		get_text_full(txt, serverinfo.maxlineeroominfo, MSG_WIDTH,
                              false, C_ROOM_INFO, NULL);
	}

	printf(sesso
	       ? _("\nSei sicura di voler mantenere le modifiche (s/n)? ")
	       : _("\nSei sicuro di voler mantenere le modifiche (s/n)? ")
	       );
	if (si_no() == 's') {
		txt_rewind(txt);
		while ( (str = txt_get(txt))) {
			serv_putf("TEXT %s", str);
		}
		serv_puts("RIEE");
		serv_gets(buf);
		print_err_edit_room_info(buf);
	} else {
		serv_puts("ATXT");
		serv_gets(buf);
	}
	txt_free(&txt);
}

/*
 * Zap current room.
 */
void room_zap(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler dimenticare questa room (s/n)? ") :_("\nSei sicuro di voler dimenticare questa room (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_puts("RZAP");
	serv_gets(buf);
	if (buf[0]!='2')
		printf(_("Non puoi zappare questa room.\n"));
	else
		room_goto(4, true, NULL);
}

/*
 * Invita un utente nella room corrente.
 */
void room_invite(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF];

	get_username(_("Invita l'utente: "), nick);
	if (nick[0] == '\0')
		return;
	serv_putf("RINV %s", nick);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("\nOk, %s &egrave; stato invitato.\n"), nick);
	else
		switch (buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata a invitare utenti in questa room.\n") : _("Non sei autorizzato a invitare utenti in questa room.\n"));
			break;
		case '4':
			printf(_("\nL'utente [%s] non ha un livello sufficiente.\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		case '7':
			printf(_("\nQuesta room non richiede l'invito.\n"));
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}
}

void room_invited_who(void)
{
	size_t lung, flag = 0;
	char aaa[LBUF];

	serv_puts("RINW");
	serv_gets(aaa);
	if (aaa[0] != '2') {
		if (aaa[1] == '7')
			cml_printf(_("\nQuesta room non &egrave; a invito.\n"));
		else
			printf(sesso ? _("Non sei autorizzata a vedere la lista degli utenti invitati in questa room.\n") :_("Non sei autorizzato a vedere la lista degli utenti invitati in questa room.\n"));
		return;
	}

	lung = 2;
	while (serv_gets(aaa), strcmp(aaa, "000")) {
		if (!flag) {
			flag = 1;
			printf(_("I seguenti utenti sono stati invitati in questa room:\n  "));
		}
		lung += strlen(aaa) - 2;
		if (lung > 79) {
			printf("\n  ");
			lung = strlen(aaa) - 2;
		}
		printf("%s  ", aaa+4);
	}
	if (!flag)
		cml_printf(_("\b\bNessun utente &egrave; stato invitato in questa room."));
	putchar('\n');
}

/*
 * Invia la richiesta di invito a una particolare room al server.
 */
void room_invite_req(void)
{
	char name[LBUF], buf[LBUF];

	new_str_M(_("Nome della Room: "), name, ROOMNAMELEN-1);
	serv_putf("RIRQ %s", name);
	serv_gets(buf);
	if (buf[0] == '2') {
		if (buf[2]=='0')
			printf(_("Ok, hai acquistato l'accesso alla room '%s'.\n"), name);
		else
			cml_print(_("La tua richiesta di invito &egrave; stata inviata e verr&agrave; presa in considerazione.\n"));
	} else
		switch (buf[1]) {
		case '2':
			printf(_("Non puoi richiedere l'accesso alla room '%s'.\n"), name);
			break;
		case '4':
			cml_printf(_("La room '%s' non &egrave; una room a invito.\n"), name);
			break;
		case '7':
			printf(_("La room '%s' non esiste.\n"), name);
			break;
		}
}

/*
 * Designa un nuovo Room Aide per la room corrente
 */
void room_new_ra(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF];

	get_username(_("Nome nuovo room aide per questa room: "), nick);
	if (nick[0] == '\0')
		return;
	serv_putf("RAID %s", nick);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("\nOk, %s &egrave; il nuovo Room Aide.\n"), nick);
	else
		switch (buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata.\n") : _("Non sei autorizzato.\n"));
			break;
		case '4':
			printf(_("\nL'utente [%s] non ha un livello sufficiente.\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		case '7':
		        cml_printf(_("\nNon &egrave; possibile assegnare un Room Aide a una room virtuale.\n"));
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}
}

/*
 * Designa un nuovo Room Helper per la room corrente
 */
void room_new_rh(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF];

	get_username(_("Nome room helper per questa room: "), nick);
	if (nick[0] == '\0')
		return;
	serv_putf("RNRH %s", nick);
	serv_gets(buf);
	if (buf[0] == '2') {
		if (extract_int(buf+4, 0))
			cml_printf(_("\nOk, %s &egrave; un Room Helper per questa room.\n"), nick);
		else
			cml_printf(_("\nOk, %s non &egrave; pi&uacute; Room Helper.\n"), nick);
	} else
		switch (buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata.\n") : _("Non sei autorizzato.\n"));
			break;
		case '4':
			printf(_("\nL'utente [%s] non ha un livello sufficiente.\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		case '7':
		        cml_printf(_("\nNon &egrave; possibile assegnare un Room Helper a una room virtuale.\n"));
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}
}

/*
 * Kick out di un utente dalla room.
 */
void room_kick(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF], motivo[LBUF];

	get_username(_("Butta fuori l'utente: "), nick);
	if (nick[0] == '\0')
		return;
	new_str_m(_("Motivo: "), motivo, 70);
	if (motivo[0] == '\0') {
		printf(_("Attenzione! devi motivare la tua scelta di buttare fuori l'utente!\n"));
		return;
	}
	serv_putf("RKOB %s|%s", nick, motivo);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("\nOk, %s &egrave; stato cacciato.\n"), nick);
	else
		switch (buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata a cacciare utenti da questa room.\n") : _("Non sei autorizzato a cacciare utenti da questa room.\n"));
			break;
		case '4':
			if (buf[2] == '0')
				printf(_("\nL'utente [%s] non ha accesso a questa room.\n"), nick);
			else
				cml_printf(_("\nL'utente <b>%s</b> &egrave; gi&agrave; stato cacciato dalla room!\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		case '7':
		        cml_printf(_("\nNon &egrave; possibile cacciare un utente da una room virtuale.\n"), nick);
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}
}

/*
 * End Kick out period per un utente.
 */
void room_kick_end(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF];

	get_username(_("Riammetti l'utente: "), nick);
	if (nick[0] == '\0')
		return;
	serv_putf("RKOE %s", nick);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("\nOk, %s &egrave; stato riammesso.\n"), nick);
	else
		switch (buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata a riammettere utenti in questa room.\n") : _("Non sei autorizzato a riammettere utenti in questa room.\n"));
			break;
		case '4':
			if (buf[2] == '0')
				printf(_("\nL'utente [%s] non ha accesso a questa room.\n"), nick);
			else
				cml_printf(_("\nL'utente <b>%s</b> non &egrave; stato cacciato da questa room!\n"), nick);
			break;
			printf(_("\nL'utente [%s] non ha accesso a questa room.\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		case '7':
		        cml_printf(_("\nNon puoi farlo in una room virtuale.\n"));
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}
}

void room_kicked_who(void)
{
	size_t lung, flag = 0;
	char aaa[LBUF];

	serv_puts("RKOW");
	serv_gets(aaa);
	if (aaa[0] != '2') {
	        if (aaa[1] == '7')
		        cml_printf(_("Non &grave; possibile cacciare utenti dalle room virtuali!\n"));
		else
		        printf(sesso ? _("Non sei autorizzata a vedere la lista degli utenti cacciati da questa room.\n") : _("Non sei autorizzato a vedere la lista degli utenti cacciati da questa room.\n"));
		return;
	}

	lung = 2;
	while (serv_gets(aaa), strcmp(aaa, "000")) {
		if (!flag) {
			flag = 1;
			printf(_("I seguenti utenti sono stati cacciati da questa room:\n  "));
		}
		lung += strlen(aaa) - 2;
		if (lung > 79) {
			printf("\n  ");
			lung = strlen(aaa) - 2;
		}
		printf("%s  ", aaa+4);
	}
	if (!flag)
		cml_printf(_("\b\bNessun utente &egrave; stato cacciato da questa room."));
	putchar('\n');
}

/*
 * Scambia la posizione di due room.
 */
void room_swap(void)
{
	char buf[LBUF], room1[LBUF], room2[LBUF];

	get_roomname(_("Room #1: "), room1, false);
	if (room1[0] == 0)
		return;
	get_roomname(_("Room #2: "), room2, false);
	if (room2[0] == 0)
		return;
	serv_putf("RSWP %s|%s", room1, room2);
	serv_gets(buf);
	if (buf[0] == '2')
		printf(_("Ok, le room [%s] e [%s] sono state scambiate.\n"),
		       room1, room2);
	else {
		if (buf[1] == '2')
			printf(sesso ? _("Non sei autorizzata a scambiarle.\n") : _("Non sei autorizzato a scambiarle.\n"));
		else {
			if (extract_int(buf+4, 0)==1)
				printf(_("La room [%s] non esiste.\n"), room1);
			else
				printf(_("La room [%s] non esiste.\n"), room2);
		}
	}
}

/*
 * Scambia la posizione di due room.
 */
void room_resize(void)
{
	char buf[LBUF];
	long n;

	n = new_long(_("Numero di slot per la room: "));
	if ((n<2) || (n>MAXROOMLEN)) {
		printf(_("Valore non valido.\n"));
		return;
	}
	serv_putf("RLEN %ld", n);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("Ok, il numero di slot di questa room &egrave; stato portato a %ld.\n"), n);
	else if (buf[1] == '7')
	        cml_printf(_("Non &egrave; possibile modificare il numero di slot di una room virtuale.\n"));
	else
		cml_printf(sesso ? _("Non sei autorizzata a modificare il numero di slot della room.\n") : _("Non sei autorizzato a modificare il numero di slot della room.\n"));
}

void room_zap_all(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler dimenticare tutte le room (s/n)? ") : _("\nSei sicuro di voler dimenticare tutte le room (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_puts("RZPA");
	serv_gets(buf);
	if (buf[0] != '2')
		printf(_("Non puoi zappare tutte le room.\n"));
	else
		room_goto(4, true, NULL);
}

void room_unzap_all(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di volerti ricordare di tutte le room (s/n)? ") : _("\nSei sicuro di volerti ricordare di tutte le room (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_puts("RUZP");
	serv_gets(buf);
	if (buf[0] != '2')
		printf(_("Non puoi farlo!\n"));
}
/***************************************************************************/
/*
 * Funzione che si occupa di ricevere i nomi delle room dal server
 * e di stamparle in modo decente. Se floors e' zero, usa il comando
 * RKRL (tutte le room) altrimenti usa i floors.
 */
static int room_print_known(char mode, char floors, int riga)
{
	char buf[LBUF], rn[LBUF];
	long n;
	size_t lung = 0;

	switch (floors) {
	case 0: serv_putf("RKRL %d", mode);
		break;
	case 1: serv_putf("FKRL %d", mode);
		break;
	case 2: serv_putf("FKRM %d", mode);
		break;
	default: return 0;
	}
        serv_gets(buf);
        if (buf[0] != '2')
		return riga;
	if (uflags[4] & UT_KRCOL)
		while (serv_gets(buf), strcmp(buf, "000")) {
			if (lung == 3) {
				putchar('\n');
				lung = 0;
				riga++;
				if (riga == (NRIGHE - 1)) {
					hit_any_key();
					riga = 1;
				}
			}
			extract(rn, buf+4, 0);
			n = extract_long(buf+4, 1);
			setcolor(C_KR_NUM);
			printf("%3ld. ", n);
			setcolor(C_KR_ROOM);
			printf("%-20s", rn);
                        /*
                        if (extract_int(buf+4, 2))
                                printf("Z");
                        */
			lung++;
		}
	else
		while (serv_gets(buf), strcmp(buf, "000")) {
			extract(rn, buf+4, 0);
			lung += strlen(rn) + 2;
			if (lung > 79) {
				putchar('\n');
				lung = strlen(rn) + 2;
				riga++;
				if (riga == (NRIGHE - 1)) {
					hit_any_key();
					riga = 1;
				}
			}
			setcolor(C_KR_ROOM);
			printf("%s  ", rn);
		}
	putchar('\n');
	riga++;
	return riga;
}


static void debug_print_room_flags(void)
{
	printf("Room Flags:\n");
	cml_printf("RF_BASE      : %s\n", print_si_no(rflags & RF_BASE));
	cml_printf("RF_MAIL      : %s\n", print_si_no(rflags & RF_MAIL));
	cml_printf("RF_ENABLED   : %s\n", print_si_no(rflags & RF_ENABLED));
	cml_printf("RF_INVITO    : %s\n", print_si_no(rflags & RF_INVITO));
	cml_printf("RF_SUBJECT   : %s\n", print_si_no(rflags & RF_SUBJECT));
	cml_printf("RF_GUESS     : %s\n", print_si_no(rflags & RF_GUESS));
	cml_printf("RF_DIRECTORY : %s\n", print_si_no(rflags & RF_DIRECTORY));
	cml_printf("RF_ANONYMOUS : %s\n", print_si_no(rflags & RF_ANONYMOUS));
	cml_printf("RF_ASKANONYM : %s\n", print_si_no(rflags & RF_ASKANONYM));
	cml_printf("RF_REPLY     : %s\n", print_si_no(rflags & RF_REPLY));
	cml_printf("RF_AUTOINV   : %s\n", print_si_no(rflags & RF_AUTOINV));
	cml_printf("RF_FLOOR     : %s\n", print_si_no(rflags & RF_FLOOR));
	cml_printf("RF_SONDAGGI  : %s\n", print_si_no(rflags & RF_SONDAGGI));
	cml_printf("RF_CITTAWEB  : %s\n", print_si_no(rflags & RF_CITTAWEB));
	cml_printf("RF_BLOG      : %s\n", print_si_no(rflags & RF_BLOG));
	cml_printf("RF_POSTLOCK  : %s\n", print_si_no(rflags & RF_POSTLOCK));
	cml_printf("RF_L_TIMEOUT : %s\n", print_si_no(rflags & RF_L_TIMEOUT));
	cml_printf("RF_FIND      : %s\n", print_si_no(rflags & RF_FIND));
	cml_printf("RF_DYNROOM   : %s\n", print_si_no(rflags & RF_DYNROOM));
	cml_printf("RF_DYNMSG    : %s\n", print_si_no(rflags & RF_DYNMSG));
}
