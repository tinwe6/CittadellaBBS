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
* File : aide.c                                                             *
*        Comandi generici per aide                                          *
****************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "cittaclient.h"
#include "cittacfg.h"
#include "aide.h"
#include "cml.h"
#include "conn.h"
#include "colors.h"
#include "edit.h"
#include "editor.h"
#include "errore.h"
#include "extract.h"
#include "histo.h"
#include "utility.h"
#include "msg_flags.h"
#include "sys_flags.h"

/*
 * prototipi delle funzioni in questo file 
 */
void bbs_shutdown(int mode, int reboot);
void force_scripts(void);
void Read_Syslog(void);
void statistiche_server(void);
void edit_news(void);
void enter_helping_hands(void);
void server_upgrade_flag(void);

/**************************************************************************/
/**************************************************************************/
/*
 * Chiede al server di eseguire uno shutdown (sysop only)
 * mode : -1 stop shutdown, 0 now, 1 ask delay.
 * Se reboot e' 0 chiede di fare un touch .killscript
 */
void bbs_shutdown(int mode, int reboot)
{
	int pre = 15;
        char buf[LBUF];

	if (mode == 1) {
		do {
			pre = new_sint_def(_(
			        "Minuti di preavviso prima dello shutdown"),
				15);
			if (pre > (60*24))
				printf(_(
				    "\n *** Massimo preavviso 24 ore!!\n\n"));
		} while (pre > (60*24));
	} else
		pre = mode;
	if (mode >= 0) {
		printf(sesso
		? _("Sei sicura di voler procedere allo shutdown del server? ")
		: _("Sei sicuro di voler procedere allo shutdown del server? ")
		);
		if (si_no() == 'n') {
			printf("Uff...\n");
			return;
		}
	}
	serv_putf("SDWN %d|%d", pre, reboot);
	serv_gets(buf);
	if (buf[0] == '2') {
		if (pre>=0)
			printf(_("Ok, autodistruzione attivata!\n"));
		else
			printf(_("Uff...\n"));
	} else
		printf(_("Non puoi richiedere lo shutdown del server.\n"));
}

/* Forza l'esecuzione degli script al shutdown/reboot */
void force_scripts(void)
{
        char buf[LBUF];

	serv_puts("FSCT");
	serv_gets(buf);
	if (buf[0] == '2')
		printf(_("Ok, al prossimo reboot/shutdown verranno eseguiti gli script.\n"));
	else
		printf(_("Non hai un livello sufficiente!\n"));
}

/*
 * Legge il file di log del sistema. (sysop only)
 */
void Read_Syslog(void)
{
        char aaa[LBUF];

        serv_puts("RSLG");
        serv_gets(aaa);
        if (aaa[0]=='2') {
                putchar('\n');
                while (serv_gets(aaa), strcmp(aaa, "000"))
                        printf("%s\n", &aaa[4]);
        } else
                cml_print(_("\nQuesto comando &egrave; riservato ai sysop.\n"));
}

/*
 * Statistiche del server
 */
void statistiche_server(void)
{
        /* TODO queste stringhe sono ridondanti: sono gia' in utility.c
                -->   unificare.  */

        char * strg[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven",
		"Sab"};
        char * strm[] = {"Gen", "Feb", "Mar", "Apr", "Mag", "Giu",
		"Lug", "Ago", "Set", "Ott", "Nov", "Dic"};
        char buf[LBUF];
	int i;
	long ore[24], giorni[7], mesi[12], tmp1, tmp2;
	
        serv_puts("RSST");
        serv_gets(buf);

        if (buf[0] != '2') {
                printf(sesso ? _("\nNon sei autorizzata a leggere le statistiche del server.\n") : _("\nNon sei autorizzato a leggere le statistiche del server.\n"));
		return;
	}
	while (serv_gets(buf), strncmp(buf, "000", 3)) {
		switch(extract_int(buf+4, 0)) {
		 case SYSSTAT_GENERAL:
			printf("\n");
			printf(_("Run del server numero : %5ld\n"),
			       extract_long(buf+4, 1));
			printf(_("Ultima matricola      : %5ld\n"),
			       extract_long(buf+4, 2));
			printf(_("Numero login totali   : %5ld  (%ld ospiti, %ld nuovi utenti)\n"),
			       extract_long(buf+4, 3),
			       extract_long(buf+4, 4),
			       extract_long(buf+4, 5));
			printf(_("Numero validazioni    : %5ld\n"),
			       extract_long(buf+4, 6));
			printf(_("Connessioni avvenute  : %5ld  (%ld client locale, %ld client remoto)\n"),
			       extract_long(buf+4, 7),
			       extract_long(buf+4, 8),
			       extract_long(buf+4, 9));
			printf(_("Max. utenti contemp.  : %5ld, il "),
			       extract_long(buf+4, 10));
			stampa_data(extract_long(buf+4, 11));
			putchar('\n');
			break;
		 case SYSSTAT_CITTAWEB:
			printf(_("\nConnessioni HTTP al cittaweb:\n"
				 "  Numero di connessioni http: %5ld, "
				 "di cui %5ld corrette.\n"
				 "  Lettura room: %5ld  "
				 "  Lista utenti     : %5ld\n"
				 "  File di Help: %5ld  "
				 "  Lettura profile  : %5ld\n"),
			       extract_long(buf+4, 1), extract_long(buf+4, 2),
			       extract_long(buf+4, 3), extract_long(buf+4, 4),
			       extract_long(buf+4, 5), extract_long(buf+6, 6));
			if ( (tmp1 = extract_long(buf+4, 7)))
				printf(_("  Lettura blog: %5ld\n"), tmp1);
			break;
		 case SYSSTAT_MSG:
			printf(_("\nMessaggi Scambiati:\n"));
			printf(_("  post      : %5ld     mail    : %5ld"),
			       extract_long(buf+4, 4),
			       extract_long(buf+4, 3));
			if ( (tmp1 = extract_long(buf+4, 6)))
				printf(_("     blogs    : %5ld"), tmp1);
			printf(_("\n  broadcast : %5ld     X-msg   : %5ld"
				 "     chat msg : %5ld\n"),
			       extract_long(buf+4, 2), extract_long(buf+4, 1),
			       extract_long(buf+4, 5));
			break;
		 case SYSSTAT_REFERENDUM:
			printf(_("\nStatistiche referendum e sondaggi:\n"));
			tmp1 = extract_long(buf+4, 1);
			tmp2 = extract_long(buf+4, 4);
			printf(_("  Numero consultazioni: %5ld (di cui %ld "
				 "attive e %ld complete)\n"), tmp1,
			       extract_long(buf+4, 2), extract_long(buf+4, 3));
			if (tmp1 == 0)
				tmp1 = 1;
			printf(_("  Numero voti ricevuti: %5ld (in media %.1f/"
				 "consultazione)\n"
				 "  Slot per sondaggi   : %5ld\n"),
			       tmp2, (float)tmp2/tmp1, extract_long(buf+4, 5));
			putchar('\n');
			hit_any_key();
			break;
		 case SYSSTAT_ROOMS:
			printf(_("\nRooms, Floors & File Messages:\n"));
			printf(_(" Ultimo num. room ass. : %ld\n"),
			       extract_long(buf+4, 1));
			printf(_(" Numero room presenti  : %ld\n"),
			       extract_long(buf+4, 2));
			printf(_(" Numero room allocate nella struttura utente  : %ld\n"),
			       extract_long(buf+4, 3));
			printf(_(" Numero di mail nella casella di ogni utente  : %ld\n"),
			       extract_long(buf+4, 4));
			break;
		case SYSSTAT_FLOORS:
			printf(_(" Ultimo num. floor ass.: %ld\n"),
			       extract_long(buf+4, 1));
			printf(_(" Numero floors presenti: %ld\n"),
			       extract_long(buf+4, 2));
			break;
		case SYSSTAT_FILEMSG:
			printf(_(" Ultimo numero file msg: %ld\n"),
			       extract_long(buf+4, 1));
			printf(_(" Num. File msg presenti: %ld\n"),
			       extract_long(buf+4, 2));
			printf(_(" Num. file msg di base : %ld\n"),
			       extract_long(buf+4, 3));
			break;
		case SYSSTAT_CACHE:
			printf(_("\nCache per posts:\n"));
			printf(_("  Numero elementi     : %ld\n"),
			       extract_long(buf+4, 1));
			tmp1 = extract_long(buf+4, 2);
			tmp2 = extract_long(buf+4, 3);
			printf(_("  Richieste alla cache: %ld\n"), tmp1);
			printf("  Elementi trovati    : %ld", tmp2);
			if (tmp1)
				printf("   (%2d%%)", (int)(100.0*tmp2/tmp1));
			putchar('\n');
			putchar('\n');
			hit_any_key();
			break;
		case SYSSTAT_MEMORY:
			printf(_("\nMemoria Dinamica: attuale %ld bytes, max %ld bytes.\n"),
			       extract_long(buf+4, 1),extract_long(buf+4, 2));
			while (serv_gets(buf), strncmp(buf, "000", 3))
				printf(" %s\n", buf+4);
			break;
		case SYSSTAT_CONN_DAY:
			for(i=0; i<24; i++)
				ore[i] = extract_long(buf+4, i+1);
			putchar('\n');
			hit_any_key();
			printf(_("          *** Distribuzione dei collegamenti nella giornata ***\n"));
			print_histo_num(ore, 24, 18, 6, 0, "Ora");
			break;
		case SYSSTAT_CONN_WM:
			for(i=0; i<7; i++)
				giorni[i] = extract_long(buf+4, i+1);
			for(i=0; i<12; i++)
				mesi[i] = extract_long(buf+4, i+8);
			putchar('\n');
			hit_any_key();
			printf(_("\n\n\n          *** Distribuzione dei collegamenti nella settimana ***\n\n"));
			print_histo(giorni, 7, 16, 4, 0, _("Giorno"), strg);
			putchar('\n');
			hit_any_key();
			printf(_("\n\n\n          *** Distribuzione dei collegamenti nell'anno ***\n"));
			print_histo(mesi, 12, 18, 6, 0, _("Mese"), strm);
			break;
		}
	}
}

/*
 * Edita descrizione room corrente
 */
void edit_news(void)
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

	serv_puts("MSGI 0|8");
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

	serv_puts("NWSB");
	serv_gets(buf);

	if (buf[0] != '2') {
		printf(sesso ? _("\nNon sei autorizzata.\n") :
		       _("\nNon sei autorizzato.\n"));
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
			       && (riga < serverinfo.maxlineenews)) {
				txt_put(txt, buf);
				riga++;
			}
			fclose(fp);
			unlink(filename);
			free(filename);
	} else
		get_text_full(txt, serverinfo.maxlineenews, MSG_WIDTH, false,
                              C_NEWS, NULL); /* TODO sistemare MD */

	printf(sesso
	       ? _("\nSei sicura di voler mantenere le modifiche (s/n)? ")
	       : _("\nSei sicuro di voler mantenere le modifiche (s/n)? ")
	       );
	if (si_no() == 's') {
		txt_rewind(txt);
		while ( (str = txt_get(txt)))
			serv_putf("TEXT %s", str);
		serv_puts("NWSE");
		serv_gets(buf);
		print_err_edit_news(buf);
	} else {
		serv_puts("ATXT");
		serv_gets(buf);
	}
	txt_free(&txt);
}

/*
 * 
 */
void enter_helping_hands(void)
{
	
}

/* Setta flag SUT_UPDATE di tutti gli utenti, per passare dalla configurazione
 * del nuovo client al primo collegamento.                                  */
void server_upgrade_flag(void)
{
	char buf[LBUF];

	printf(sesso ? _("\nSei sicura di voler settare il flag SUT_UPDATE? ") : _("\nSei sicuro di voler settare il flag SUT_UPDATE? "));
	if (si_no() == 'n')
		return;

	serv_putf("UPGS");
	serv_gets(buf);
	if (buf[0] != '2') {
		printf(sesso ? _("\n*** Non sei autorizzata.\n") :
		       _("\n*** Non sei autorizzato.\n"));
	} else
		cml_print(_("\nOk, il flag &egrave; stato settato.\n"));
}
