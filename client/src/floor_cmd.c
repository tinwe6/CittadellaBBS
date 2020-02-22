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
* File : floor_cmd.c                                                        *
*        Comandi per la gestione dei floor                                  *
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "cittacfg.h"
#include "cittaclient.h"
#include "ansicol.h"
#include "cml.h"
#include "comandi.h"
#include "conn.h"
#include "edit.h"
#include "extract.h"
#include "floor_cmd.h"
#include "floor_flags.h"
#include "room_cmd.h"
#include "tabc.h"
#include "user_flags.h"
#include "utility.h"
#include "macro.h"

/* prototipi delle funzioni in questo file */
void floor_create(void);
void floor_delete(void);
void floor_list(void);
void floor_move_room(void);
void floor_new_fa(void);
void floor_info(int det);
void floor_edit_info(void);
int floor_known_rooms(void);

void floor_edit(int mode);
void floor_goto(int mode, int gotonext);
void floor_list_known(char mode);
void floor_swap(void);
void floor_new_fh(void);
void floor_zap(void);
static int floor_print_known(char mode, int riga);

/***************************************************************************/
/***************************************************************************/
/* 
 * Crea un nuovo floor.
 */
void floor_create(void)
{
	char name[FLOORNAMELEN], rname[ROOMNAMELEN], buf[LBUF];
	long fmnum, maxmsg, num, nnew, nnum, rnum;

	new_str_M(_("Nome del floor: "), name, FLOORNAMELEN-1);
	if (strlen(name) == 0)
		return;
	fmnum = new_long_def(_("Numero File Messaggi (0: Basic): "), 0);
	maxmsg = new_long_def(_("Numero massimo di messaggi: "), 
			      DFLT_MSG_PER_ROOM);
	serv_putf("FNEW %s|%ld|%ld", name, fmnum, maxmsg);
	serv_gets(buf);
	if (buf[0] == '2') {
		num = extract_long(buf+4, 0);
		rnum = extract_long(buf+4, 1);
		extractn(rname, buf+4, 2, ROOMNAMELEN-1);
		cml_printf(_("\nOk, il floor #%ld [%s] &egrave; stato creato.\n"),
			   num, name);
		cml_printf(_("\nLa corrispondente room di base &egrave; la room #%ld [%s].\n\n"), rnum, rname);
		serv_putf("GOTO %d|%s|0", 0, rname);
		serv_gets(buf);
		if (buf[0] == '2') {
			extractn(current_room, buf+4, 0, ROOMNAMELEN);
                        extract(room_type, buf+4, 1);
			nnum = extract_long(buf+4, 2);
			nnew = extract_long(buf+4, 3);
			utrflags = extract_long(buf+4, 4);
			rflags = extract_long(buf+4, 5);
			extractn(current_floor, buf+4, 6, FLOORNAMELEN);
			barflag = 0;
			cml_printf(_("<b>%s</b> - <b>%ld</b> nuovi su <b>%ld</b> messaggi.\n"), current_room, nnew, nnum);
			room_info(true, true);
		} else
			cml_print(_("...per&oacute; non riesco ad entrarci... mah!\n"));
	} else {
		switch(buf[1]) {
		case '2':
			printf(sesso ? _("Non sei autorizzata a creare nuovi floor.\n") : _("Non sei autorizzato a creare nuovi floor.\n"));
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
 * Elimina il floor corrente.
 */
void floor_delete(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler eliminare questo floor (s/n)? ") : _("\nSei sicuro di voler eliminare questo floor (s/n)? "));
	if (si_no() == 'n')
		return;	
	serv_puts("FDEL");
	serv_gets(buf);
	if (buf[0] == '2') {
		cml_print(_("\nOk, il floor &egrave; stato eliminato.\n"));
		room_goto(4, true, NULL);
	} else
	       cml_print(_("Non sei autorizzato a cancellare i floor.\n"));
}

/*
 * Lista di tutti i floor (SYSOP only).
 */
void floor_list (void)
{
        char aaa[LBUF], fa[MAXLEN_UTNAME], fstr[LBUF];
        char nm[MAXLEN_UTNAME];
	int rlvl, wlvl, riga;
        long num, numroom, numpost, ora, flags;
        struct tm *tmst;

        serv_puts("FLST");
        serv_gets(aaa);
        if (aaa[0]=='2') {
		printf("\nNum      Floor Name      Rooms  Post  LastPost"
		       "  Flags    r/w      Floor Aide");
		printf("\n--- -------------------- ----- ------ --------"
		       " -------- ----- -----------------\n");
		riga = 4;
                while (serv_gets(aaa), strcmp(aaa, "000")) {
                        riga++;
                        if (riga == NRIGHE) {
                                hit_any_key();
                                riga = 1;
                        }
                        num = extract_long(aaa+4, 0);
                        extractn(nm, aaa+4, 1, MAXLEN_UTNAME);
                        numroom = extract_long(aaa+4, 2);
                        numpost = extract_long(aaa+4, 3);
                        ora = extract_long(aaa+4, 4);
                        tmst = localtime(&ora);
                        flags = extract_long(aaa+4, 5);
                        rlvl = extract_long(aaa+4, 6);
                        wlvl = extract_long(aaa+4, 7);
                        extractn(fa, aaa+4, 8, MAXLEN_UTNAME);

			sprintf(fstr, "[%6s]", "");
			if (flags & FF_BASE)
				fstr[1] = 'B';
			if (flags & FF_MAIL)
				fstr[1] = 'M';
			if (flags & FF_ENABLED)
				fstr[2] = '.';
			else
				fstr[2] = '!';
			if (flags & FF_INVITO) {
				if (flags & FF_AUTOINV)
					fstr[3] = 'I';
				else
					fstr[3] = 'i';
			} else if (flags & FF_GUESS)
				fstr[3] = '?';
			if (flags & FF_SUBJECT)
				fstr[4] = 'S';
			if (flags & FF_REPLY)
				fstr[5] = 'R';
			if (flags & FF_ANONYMOUS)
				fstr[6] = 'A';
			else if (flags & FF_ASKANONYM)
				fstr[6] = 'a';
			if (tmst->tm_year > 99)
				tmst->tm_year -= 100;
                        printf("%3ld %-20s  %3ld  %5ld %02d/%02d/%02d %8s %2d/%2d %s\n", 
                               num, nm, numroom, numpost, tmst->tm_mday,
			       tmst->tm_mon+1, tmst->tm_year, fstr, rlvl,
			       wlvl, fa);
                }
        }
}

/*
 * Cambia floor alla room corrente.
 */
void floor_move_room(void)
{
	char buf[LBUF], floor[LBUF];

	get_floorname(_("Sposta la room nel floor: "), floor);
	if (floor[0] == 0)
		return;
	serv_putf("FMVR %s", floor);
	serv_gets(buf);
	if (buf[0] == '2') {
		cml_printf(_("Ok, la room [%s] &egrave; stata spostata nel floor [%s].\n"), current_room, floor);
		strcpy(current_floor, floor);
	} else if (buf[1] == '2')
		printf(_("Non sei autorizzato a spostare le room.\n"));
	else
		printf(_("Il floor [%s] non esiste.\n"), floor);
}

/*
 * Designa un nuovo Floor Aide per il floor corrente.
 */
void floor_new_fa(void)
{
	char nick[MAXLEN_UTNAME], buf[LBUF];
	
	get_username(_("Nome nuovo Floor Aide per questo floor: "), nick);
	if (nick[0] == '\0')
		return;
	serv_putf("FAID %s", nick);
	serv_gets(buf);
	if (buf[0] == '2')
		cml_printf(_("\nOk, %s &egrave; il nuovo Floor Aide.\n"),nick);
	else
		switch (buf[1]) {
		case '2':
			printf(_("Non sei autorizzato.\n"));
			break;
		case '4':
			printf(_("\nL'utente [%s] non ha un livello sufficiente.\n"), nick);
			break;
		case '5':
			printf(_("\nL'utente [%s] non esiste.\n"), nick);
			break;
		default:
			printf(_("\nErrore.\n"));
			break;
		}	
}

/*
 * Legge le info del floor corrente.
 * Se det!=0, da informazioni piu' dettagliate.
 */
void floor_info(int det)
{
	char buf[LBUF], faide[LBUF], fname[LBUF];
	int  rlvl, wlvl, riga = 4;
	long numpost, numroom, ct, mt, pt;
	
	serv_puts("FINF");
	serv_gets(buf);
	if (buf[0] != '2') {
		printf(_(" *** Problemi con il server.\n"));
		return;
	}
	rlvl = extract_int(buf+4, 0);
	wlvl = extract_int(buf+4, 1);
	numpost = extract_long(buf+4, 2);
	ct = extract_long(buf+4, 3);
	mt = extract_long(buf+4, 4);
	pt = extract_long(buf+4, 5);
	extract(faide, buf+4, 6);
	extract(fname, buf+4, 7);
      	/*flags = extract_long(buf+4, 8);*/
	numroom = extract_long(buf+4, 9);
	
	cml_printf(_("\n Floor '<b>%s</b>', creato il "), fname);
	stampa_datal(ct);
	cml_printf(_(".\n Floor Aide: <b>%s</b> - "), faide);
	if (numroom != 1)
		cml_printf(_("Sono presenti <b>%ld</b> room.\n"), numroom);
	else
		cml_printf(_("&Egrave; presente <b>1</b> room.\n"));
	if (det) {
		cml_printf(_(".\n Livello minimo di accesso <b>%d</b>, per scrivere <b>%d</b>.\n"), rlvl, wlvl);
		if (mt == 0)
			printf(_(" Questo floor non ha subito modifiche dalla sua creazione.\n"));
		else {
			printf(_(" Ultima modifica del floor il "));
			stampa_datal(mt);
			putchar('\n');
		}
		riga += 3;
		if (pt) {
			printf(_(" Ultimo post lasciato in una room del floor il "));
			stampa_datal(pt);
			putchar('\n');
			riga++;
		}
	}
	if (numpost) {
                switch(numpost) {
                case 0:
			cml_print(_(" Nessun messaggio &egrave; stato lasciato in questo floor.\n\n"));
			break;
                case 1:
			cml_printf(_(" &Egrave; stato lasciato un messaggio dalla sua creazione.\n\n"));
			break;
                default:
			cml_printf(_(" Sono stati lasciati <b>%ld</b> messaggi nelle sue roon dalla sua creazione.\n\n"), numpost);
                }
		riga++;
	}
	if (buf[1] == '1') {
		riga += 2;
		cml_printf(_(" <b>Descrizione del floor:</b>\n\n"));
		while (serv_gets(buf), strcmp(buf, "000")) {
			riga++;
                        if (riga == (NRIGHE - 1)) {
                                if (uflags[1] & UT_PAGINATOR)
                                        hit_any_key();
                                riga = 1;
                        }
			printf("%s\n", buf+4);
		}
	} else
		cml_print(_(" Non &egrave; attualmente presente una descrizione di questo floor.\n\n"));
	IFNEHAK;
}

/*
 * Edita descrizione floor corrente.
 * Attenzione: Questa funzione e' praticamente identica alla funzione
 * room_edit_info(), si possono percio' unificare!
 */
void floor_edit_info(void)
{
	char buf[LBUF], buf1[LBUF];
	char *filename;
	int riga;
	int fd;
	FILE *fp;

	filename = astrcat(TMPDIR, TEMP_EDIT_TEMPLATE);
	fd = mkstemp(filename);
	close(fd);
	chmod(filename, 0660);
	fp = fopen(filename, "w");

	serv_puts("FINF");
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(_("\n*** Problemi con il server.\n"));
		fclose(fp);
		return;
	}
	if (buf[1] == '1')
		while (serv_gets(buf), strcmp(buf, "000"))
			fprintf(fp, "%s\n", &buf[4]);
	fclose(fp);

	serv_puts("FIEB");
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(sesso ? _("\nNon sei autorizzata.\n") : _("\nNon sei autorizzato.\n"));
		return;
	}
	printf("\n");
	edit_file(filename);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf(_("\nProblemi di I/O\n"));
		serv_puts("ATXT");
		serv_gets(buf);
		return;
	}
	riga = 0;
	while ((fgets(buf1, 80, fp) != NULL) && (riga < MAXLINEEFLOORINFO)) {
		serv_putf("TEXT %s", buf1);
		riga++;
	}
	fclose(fp);
	unlink(filename);
	free(filename);

	serv_puts("FIEE");
	serv_gets(buf);
	printf("%s\n", &buf[4]);	
}

/*
 * Visualizza le room organizzate per floor.
 */
int floor_known_rooms(void)
{
	char buf[LBUF], nome[LBUF];
	long num;
	int riga = 1;
	
	serv_puts("FKRA");
        serv_gets(buf);
        if (buf[0] != '2')
		return riga;

	while (serv_gets(buf), strcmp(buf, "000")) {
		riga++;
		if (riga == (NRIGHE - 1)) {
			hit_any_key();
			riga = 1;
		}
		extract(nome, buf+4, 1);
		num = extract_int(buf+4, 2);
		if (extract_int(buf+4, 0) == 0) {
			/* riga++; */
			if (riga == (NRIGHE - 1)) {
				hit_any_key();
				riga = 1;
			}
			printf("\n%3ld. %-20s\n", num, nome);
		} else {
			printf("%26s%3ld. %-20s   ", "", num, nome);
			if (extract_int(buf+4, 4))
				printf("N");
			else
				printf(" ");
			if (extract_int(buf+4, 3))
				printf("Z");
			putchar('\n');
		}
	}
	putchar('\n');
	riga++;
	return riga;
}

/**********************************************************************/
/*
 * Visualizza le room organizzate per floor.
 */
static int floor_print_known(char mode, int riga)
{
	char buf[LBUF], rn[LBUF];
	long n;
	size_t lung = 0;
	
	serv_putf("RKRL %d", mode);
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
			printf("%3ld. %-20s", n, rn);
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
			printf("%s  ", rn);
		}
	putchar('\n');
	riga++;
	return riga;
}

/*
 * Elenca le room conosciute, a seconda del modo:
 * mode : 0 - tutte le room              1 - Room con messaggi non letti
 *        2 - Room senza messaggi nuovi  3 - Tutte le room zappate
 *        4 - Room del piano corrente    5 - <k>nown rooms 'classico'
 */
void floor_list_known(char mode)
{
	int riga;
	
	switch (mode) {
	case 0:
	case 1:
	case 2:
	case 3:
		putchar('\n');
		floor_print_known(mode, 3);
		break;
	case 5:
		cml_printf(_("\n   <b>Floors con messaggi da leggere:</b>\n"));
		riga = floor_print_known(1, 4);
		cml_printf(_("\n   <b>Non ci sono messaggi nuovi in:</b>\n"));
		riga++;
		floor_print_known(2, riga);
		break;
	}
	IFNEHAK;
}

/*
 * Vai in un altro floor. Dopo il goto ci si ritrova nella baseroom del floor.
 * mode: 0 - goto -> set all messages read.
 *       1 - skip -> leave fullroom unchanged.
 *       2 - abandon -> only set read the effectively read msgs.
 *       3 - Scan -> next known room, even if there are no new messages.
 *       4 - Home -> Torna alla Lobby) senza pescare una carta.
 *       5 - Mail -> Vai nella mailroom.
 *       (Nei modi 0, 1 e 2 va nelle room con nuovi messaggi)
 * gotonext : ==0 --> chiede la room di destinazione (jump)
 *            !=0 --> va nella prossima room
 * floor_goto(4, 1) --> vai in Lobby), room_goto(5,1) --> vai in Mail)
 */
void floor_goto(int mode, int gotonext)
{
	char buf[LBUF], floorname[LBUF];
	long nnum, nnew;

	if (gotonext)
		floorname[0] = 0;
	else {
		get_floorname("", floorname);
		if (floorname[0] == 0)
			return;
	}
	barflag = 1;
	serv_putf("GOTO %d|%s|1", mode, floorname);
	serv_gets(buf);
	if (buf[0] == '2') {
		extractn(current_room, &buf[4], 0, ROOMNAMELEN);
		extract(room_type, &buf[4], 1);
		nnum = extract_long(buf+4, 2);
		nnew = extract_long(buf+4, 3);
		utrflags = extract_long(buf+4, 4);
		rflags = extract_long(buf+4, 5);
		if (!nnew)
			barflag = 0;
		cml_printf(_("<b>%s</b>%s - <b>%ld</b> nuovi su <b>%ld</b> messaggi.\n"), current_room, room_type, nnew, nnum);
		if (utrflags & UTR_NEWGEN)
			floor_info(0);
	} else {
		if (buf[1] == '7')
			printf(_("Il floor '%s' non esiste.\n"), floorname);
		else
			printf(sesso ? _("Non sei autorizzata ad accedere al floor '%s'.\n") : _("Non sei autorizzato ad accedere al floor '%s'.\n"), floorname);
	}
}

/*
 * Edita la configurazione del floor corrente.
 * mode = 0 : non modifica il nome del floor, 1: chiede anche il nome.
 */
void floor_edit(int mode)
{
	char name[LBUF], floor_master[LBUF], buf[LBUF];
	int rlvl, wlvl;
	long flags, newflags = 0;

	serv_puts("FEDT 0"); /* Get conf */
	serv_gets(buf);
	if (buf[0] == '2') {
		extract(name, buf+4, 0);
		/* tipo = extract_int(buf+4, 1); */
		rlvl = extract_int(buf+4, 2);
		wlvl = extract_int(buf+4, 3);
		/* fmnum = extract_long(buf+4, 4); */
		/* maxmsg = extract_long(buf+4, 5); */
		flags = extract_long(buf+4, 6);
		extractn(floor_master, buf+4, 7, MAXLEN_UTNAME-1);
		if (mode) {
			new_str_def_M(_("Nome del Floor"), name,
				      ROOMNAMELEN-1);
		}
		rlvl = new_int_def(_("Livello minimo per lettura"), rlvl);
		wlvl = new_int_def(_("Livello minimo per scrittura"), wlvl);
                /*
                fmnum = new_long_def(_("Numero File Messaggi"), fmnum);
                maxmsg = new_long_def(_("Numero massimo di messaggi: "),maxmsg);
                */
		if (new_si_no_def(_("Subject"), IS_SET(flags, FF_SUBJECT)))
			newflags |= FF_SUBJECT;
		if (new_si_no_def(_("Reply"), IS_SET(flags, FF_REPLY)))
			newflags |= FF_REPLY;
		if (new_si_no_def(_("Invito"), IS_SET(flags, FF_INVITO)))
			newflags |= FF_INVITO;
		if (newflags & FF_INVITO) {
			if(new_si_no_def(_("Invito automatico"),
					 IS_SET(flags, FF_AUTOINV)))
				newflags |= FF_AUTOINV;			
		} else {
			if (new_si_no_def(_("Guess Room"),
					  IS_SET(flags, FF_GUESS)))
				newflags |= FF_GUESS;
		}
		if (new_si_no_def(_("Messaggi anonimi"),
				  IS_SET(flags, FF_ANONYMOUS)))
			newflags |= FF_ANONYMOUS;
		if (!(newflags & FF_ANONYMOUS))
			if (new_si_no_def(_("Post anonimo facoltativo"),
                                          IS_SET(flags, FF_ASKANONYM)))
                                newflags |= FF_ASKANONYM;
		if (new_si_no_def(_("Attivata"), IS_SET(flags, FF_ENABLED)))
			newflags |= FF_ENABLED;
		flags = newflags;
		if (mode == 1) {
			printf(_("\nVuoi mantenere le modifiche (s/n)? "));
			if (si_no()=='n') 
				return;
		}
		serv_putf("REDT 1|%s|%d|%d|%ld", name, rlvl, wlvl, flags);
		serv_gets(buf);
		cml_printf(_("\nOk, la room [%s] &egrave; stata editata.\n"),
			   name);
	} else
		printf(sesso ? _("Non sei autorizzata a editare le room.\n") : _("Non sei autorizzato a editare le room.\n"));
}

/*
 * Zap current floor. (NOT IMPLEMENTED YET)
 */
void floor_zap(void)
{
	char buf[LBUF];
	
	printf(sesso ? _("\nSei sicura di voler dimenticare questa room (s/n)? ") : _("\nSei sicuro di voler dimenticare questa room (s/n)? "));
	if (si_no() == 'n')
		return;	
	serv_puts("RZAP");
	serv_gets(buf);
	if (buf[0] != '2')
	       printf(_("Non puoi zappare questa room.\n"));
	else
		room_goto(4, true, NULL);
}

/*
 * Designa un nuovo Floor Helper per la room corrente (DA FARE)
 */
void floor_new_fh(void)
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
		 default:
			 printf(_("\nErrore.\n"));
			break;
		}	
}

/*
 * Scambia la posizione di due floor. (DA FARE)
 */
void floor_swap(void)
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
