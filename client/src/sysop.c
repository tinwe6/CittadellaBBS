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
* File : sysop.c                                                            *
*        Comandi riservati ai sysop.                                        *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

#include "cittacfg.h"
#include "cittaclient.h"
#include "cml.h"
#include "conn.h"
#include "edit.h"
#include "extract.h"
#include "sysop.h"
#include "utility.h"

/***************************************************************************/
/*
 * Crea un nuovo file messaggi
 */
void fm_create(void)
{
	long fmlen;
	char buf[LBUF], desc[LBUF];

	fmlen = new_long(_("\nDimensioni nuovo file messaggi: "));
	new_str_m(_("Descrizione: "), desc, 64);               /* SISTEMARE */
	printf(sesso ? _("\nSei sicura di voler creare questo FM (s/n)? ") : _("\nSei sicuro di voler creare questo FM (s/n)? "));
	if (si_no() == 'n')
		return;
	serv_putf("FMCR %ld|%s", fmlen, desc);
	serv_gets(buf);
	printf("&S --> %s\n", buf);
	if (buf[0]=='2') {
		fmlen = extract_long(&buf[4], 0);
		printf(_("Nuovo file messaggi #%ld creato.\n"), fmlen);
	} else
		printf(sesso ? _("Non sei autorizzata ad usare questo comando.\n") : _("Non sei autorizzato ad usare questo comando.\n"));
}

/*
 * Cancella un file messaggi.
 */
void fm_remove(void)
{
	long fmnum;
	char buf[LBUF];

	fmnum = new_long(_("\nNumero file messaggi da eliminare: "));
	printf(sesso ? _("\nSei sicura di voler eliminare il FM #%ld (s/n)? ") : _("\nSei sicuro di voler eliminare il FM #%ld (s/n)? "), fmnum);
	if (si_no() == 'n')
		return;
	serv_putf("FMRM %ld", fmnum);
	serv_gets(buf);
	if (buf[0]=='2')
		printf(_("Ok, file messaggi eliminato.\n"));
	else
		printf(sesso ? _("Non sei autorizzata ad usare questo comando.\n") : _("Non sei autorizzato ad usare questo comando.\n"));
}

/*
 * Legge un messaggio dal file messaggi
 */
void fm_read(void)
{
	long fmnum, msgnum, msgpos;
	char buf[LBUF];

	fmnum = new_long(_("Numero file messaggi   : "));
	msgnum = new_long(_("Numero del messaggio   : "));
	msgpos = new_long(_("Posizione del messaggio: "));
	serv_putf("FMRP %ld|%ld|%ld", fmnum, msgnum, msgpos);
        serv_gets(buf);
	putchar('\n');
        if (buf[0]=='2') {
                while (serv_gets(buf), strcmp(buf, "000"))
			printf("%s\n", &buf[4]);
        } else
		printf(_("Errore [%s].\n"), &buf[4]);
}

/*
 * Cancella un messaggio dal file messaggi.
 */
void fm_msgdel(void)
{
	long fmnum, msgnum, msgpos;
	char buf[LBUF];

	fmnum = new_long(_("Numero file messaggi   : "));
	msgnum = new_long(_("Numero del messaggio   : "));
	msgpos = new_long(_("Posizione del messaggio: "));
	serv_putf("FMDP %ld|%ld|%ld", fmnum, msgnum, msgpos);
        serv_gets(buf);
        if (buf[0]=='2')
		printf(_("\nOk, messaggio cancellato.\n"));
        else
		printf(_("\nErrore [%s].\n"), &buf[4]);
}

/*
 * Visualizza gli headers dei post presenti in un file messaggi.
 */
void fm_headers(void)
{
	char buf[LBUF];
	long fmnum;

	fmnum = new_long(_("Numero file messaggi: "));
	serv_putf("FMHD %ld", fmnum);
        serv_gets(buf);
        if (buf[0]=='2') {
                while (serv_gets(buf), strcmp(buf, "000"))
			printf("%s\n", &buf[4]);
        } else
		printf(_("Errore.\n"));
}

/*
 *
 */
void fm_info(void)
{
	char buf[LBUF];
	long fmnum, size, lowest, highest, pos, nmsg, ndel, msglen;
	unsigned long flags;
	time_t ctime;
	/* int fmck; */
	char desc[LBUF];
	struct tm *tmst;

	fmnum = new_long(_("Numero file messaggi (o 0 per vederli tutti): "));
	serv_putf("FMRI %ld", fmnum);
        serv_gets(buf);
        if (buf[0]=='2') {
                while (serv_gets(buf), strcmp(buf, "000")) {
			fmnum = extract_long(&buf[4], 0);
			size = extract_long(&buf[4], 1);
			lowest = extract_long(&buf[4], 2);
			highest = extract_long(&buf[4], 3);
			pos = extract_long(&buf[4], 4);
			nmsg = extract_long(&buf[4], 5);
			ndel = extract_long(&buf[4], 6);
			flags = extract_long(&buf[4], 7);
			ctime = extract_long(&buf[4], 8);
			tmst = localtime(&ctime);
			/* fmck = extract_int(&buf[4], 9); */
			extract(desc, &buf[4], 10);
			msglen = size/(highest-lowest+1);
			printf(_("\n*** File Messaggi #%ld, '%s' creato il %d/%d/%d\n"),
			       fmnum, desc, 1900+tmst->tm_year,
			       tmst->tm_mon+1, tmst->tm_mday);
			printf(_("Size: %ld bytes - %ld messages, %ld deleted, %ld bytes/msg\n"),
			       size, nmsg, ndel, msglen);
			printf(_("Highest msg %ld, Lowest msg %ld, pos %ld, flags %ld\n"),
			       highest, lowest, pos, flags);
		}
        } else
		printf(_("Errore.\n"));
}

void fm_expand(void)
{
	char buf[LBUF];
	long fmnum, size, lowest, highest, pos, nmsg, ndel, msglen, newsize;
	unsigned long flags;
	time_t ctime;
	/* int fmck; */
	char desc[LBUF];
	struct tm *tmst;

	fmnum = new_long(_("Numero file messaggi da espandere: "));
	serv_putf("FMRI %ld", fmnum);
        serv_gets(buf);
        if (buf[0]=='2') {
                serv_gets(buf);
		fmnum = extract_long(&buf[4], 0);
		size = extract_long(&buf[4], 1);
		lowest = extract_long(&buf[4], 2);
		highest = extract_long(&buf[4], 3);
		pos = extract_long(&buf[4], 4);
		nmsg = extract_long(&buf[4], 5);
		ndel = extract_long(&buf[4], 6);
		flags = extract_long(&buf[4], 7);
		ctime = extract_long(&buf[4], 8);
		tmst = localtime(&ctime);
		/* fmck = extract_int(&buf[4], 9); */
		extract(desc, &buf[4], 10);
		if (highest == lowest)
			msglen = size;
		else
			msglen = size/(highest-lowest);
		printf(_("\n*** File Message #%ld, '%s' created %d/%d/%d\n"),
		       fmnum, desc, 1900+tmst->tm_year,
		       tmst->tm_mon+1, tmst->tm_mday);
		printf(_("Size: %ld bytes - %ld messages, %ld deleted, %ld bytes/msg\n"),
		       size, nmsg, ndel, msglen);
		printf(_("Highest msg %ld, Lowest msg %ld, pos %ld, flags %ld\n"),
		       highest, lowest, pos, flags);
		serv_gets(buf);
	} else {
		printf(_("Errore.\n"));
		return;
	}
	newsize = new_long(_("Nuova dimensione file messaggi: "));
	if (newsize<size+1) {
		printf(_("Con questo comando non puoi ridurre la dimensione del file messaggi.\n"));
		return;
	}
	printf(sesso ? _("\nSei sicura di voler portare il FM #%ld a %ld bytes (s/n)? ") : _("\nSei sicuro di voler portare il FM #%ld a %ld bytes (s/n)? "), fmnum, newsize);
	if (si_no() == 'n')
		return;
	serv_putf("FMXP %ld|%ld", fmnum, newsize);
        serv_gets(buf);
        if (buf[0]=='2')
		cml_printf(_("Ok, il file messaggi #%ld &egrave; stato portato a %ld bytes.\n"), fmnum, newsize);
	else
		printf(_("Non posso modificare la taglia del file messaggi.\n"));

}

void banners_modify(void)
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

	serv_puts("LBGT");
	serv_gets(buf);

	if (buf[0] != '2') {
		if (buf[1] == '2')
			printf(sesso ? _("\nNon sei autorizzata.\n") : _("\nNon sei autorizzato.\n"));
		else
			printf(_("\nProblemi con il file.\n"));
		fclose(fp);
		return;
	}

	while (serv_gets(buf), strcmp(buf, "000"))
		fprintf(fp, "%s\n", &buf[4]);
	fclose(fp);

	serv_puts("LBEB");
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
	while (fgets(buf1, 80, fp) != NULL) {
		serv_putf("TEXT %s", buf1);
		riga++;
	}
	fclose(fp);
	unlink(filename);
	free(filename);

	serv_puts("LBEE");
	serv_gets(buf);
	printf("%s\n", &buf[4]);
}

void banners_rehash(void)
{

}

void banners_add(void)
{

}

/* Setta flag SUT_UPDATE di tutti gli utenti, per passare dalla configurazione
 * del nuovo client al primo collegamento.
 */
void sysop_reset_consent(void)
{
        char buf[LBUF];

        printf(sesso
               ?
_("\nSei sicura di voler richiedere il consenso al trattamento dei dati? ")
               :
_("\nSei sicuro di voler richiedere il consenso al trattamento dei dati? ")
               );
        if (si_no() == 'n') {
                return;
        }
        serv_putf("RCST");
        serv_gets(buf);
        if (buf[0] != '2') {
                printf(sesso
                       ? _("\n*** Non sei autorizzata.\n")
                       : _("\n*** Non sei autorizzato.\n")
                       );
        } else {
                cml_print(_(
"\nOk, gli utenti dovranno dare il consenso per il trattamento dei dati\n"
"alla loro prossima connessione.\n"
                            ));
        }
}

/*
 * Displays a list of users that have not completed their registration,
 * that need to give/renew their consent to the terms, and/or that
 * requested their data to be deleted.
 */
void sysop_unregistered_users(void)
{
        char resp[LBUF],  name[40];
        char *args;
        int level, calls, posts;
        long last_call;
        struct tm *tmst;
        int choice, row;
        bool unregistered = false;
        bool need_consent = false;
        bool to_be_deleted = false;
        bool is_reg, has_cst, need_cst;

        const char *bool_str[] = {
                "<b;fg=1>No </b>",
                "</b;fg=2>Yes"
        };

        const char *bool_inv_str[] = {
                "</b;fg=2>No ",
                "<b;fg=1>Yes</b>"
        };

        cml_printf("\n"
                   " 1. Utenti che non hanno completato la registrazione.\n"
                   " 2. Utenti che devono accettare le condizioni.\n"
                   " 3. Utenti che vogliono essere cancellati.\n"
                   " 4. Tutte le categorie qui sopra\n\n.");
        choice = new_int("Scelta: ");

        switch (choice) {
        case 1:
                unregistered = true;
                break;
        case 2:
                need_consent = true;
                break;
        case 3:
                to_be_deleted = false;
                break;
        case 4:
                unregistered = true;
                need_consent = true;
                to_be_deleted = true;
                break;
        default:
                return;
        }

        serv_putf("UUSR %d|%d|%d", unregistered, need_consent, to_be_deleted);
        serv_gets(resp);
        if (resp[0] != '2') {
                printf(sesso
                       ? _("\nNon sei autorizzata.\n")
                       : _("\nNon sei autorizzato.\n"));
                return;
        }

        cml_printf(_(
"\n<fg=7>"
"Reg : <b;fg=1>No</b;fg=7>  se deve ancora completare la registrazione.\n"
"Cons: <b;fg=1>No</b;fg=7>  se non ha mai consentito al trattamento dati.\n"
"Rnew: <b;fg=1>Yes</b;fg=7> "
"se deve rinnovare il consenso al trattamento dati.\n"
"Del : <b;fg=1>Yes</b;fg=7> "
"se ha richiesto la cancellazione dei dati personali.\n"
"\n<b>"
"Nome                      Lvl Last Call   Calls   Post  Reg  Cons Rnew Del"
"</b>\n"
"------------------------- --- ---------- ------- ------ ---- ---- ---- ----"
"\n"
                     ));

        row = 8;
        while (serv_gets(resp), strcmp(resp, "000")) {
                row++;
                if (row == NRIGHE) {
                        hit_any_key();
                        row = 1;
                }
                args = resp + 4;
                extractn(name, args, 0, sizeof name);
                calls = extract_int(args, 1);
                posts = extract_int(args, 2);
                last_call = extract_long(args, 3);
                tmst = localtime(&last_call);
                level = extract_int(args, 4);
                is_reg = extract_int(args, 5);
                has_cst = extract_int(args, 6);
                need_cst = extract_int(args, 7);

                cml_printf(
"</b;fg=7>%-25s %3d %02d/%02d/%4d %6d %6d  %-3s  %-3s  %-3s  %-3s\n",
                           name, level, tmst->tm_mday, tmst->tm_mon + 1,
                           1900+tmst->tm_year, calls, posts, bool_str[is_reg],
                           bool_str[has_cst], bool_inv_str[need_cst],
                           bool_inv_str[true]
                           );
        }
        IFNEHAK
}
