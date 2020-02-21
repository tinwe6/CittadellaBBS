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
* File: sysconf_client.c                                                    *
*       Comandi per configurazione del server da client.                    *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>

#include "cittaclient.h"
#include "cml.h"
#include "conn.h"
#include "extract.h"
#include "inkey.h"
#include "room_flags.h"
#include "sysconf_client.h"
#include "utility.h"

/* Prototipi funzioni in questo file */
void leggi_sysconfig(void);
void edit_sysconfig(void);

/****************************************************************************/
/****************************************************************************/
/*
 * Spedisce al client la configurazione del server
 */
void leggi_sysconfig(void)
{
        char buf[LBUF], aaa[LBUF];

        serv_puts("RSYS 0");
        serv_gets(buf);
        if (buf[0]=='2'){
                /* 1 - Configurazione del sistema */
                printf(_("\n *** Configurazione del sistema\n\n"));
                printf(_("TIC_TAC   = %ld\n"), extract_long(buf,2));
                printf(_("FREQUENZA = %d\n"), extract_int(buf,3));
                printf(_("Livello alla creazione: %d\n"), extract_int(buf,4));
                printf(_("Express per tutti: %s\n"), 
                       print_si_no(extract_int(buf,5)));
                printf(_("Numero minimo di posts per usare gli eXpress: %d\n"), 
                       extract_int(buf,6));
                printf(_("Salvataggio automatico: %s\n"), 
                       print_si_no(extract_int(buf,7)));
                printf(_("Tempo tra due salvataggi in minuti: %d\n"),
                       extract_int(buf,8));
                printf(_("Nameserver lento: %s\n"), 
                       print_si_no(extract_int(buf,9)));
                
                /* 2 - Info sul server */
                serv_gets(buf);
                printf(_("\n *** Informazioni sul server:\n\n"));
                extract(aaa,buf,2);
                printf(_("Software: %s\n"),aaa);
                extract(aaa,buf,3);
                printf(_("Versione: %s\n"),aaa);
                extract(aaa,buf,4);
                printf(_("Versione dell'ultimo client: %s\n"),aaa);
                extract(aaa,buf,5);
                printf(_("Nodo:     %s\n"),aaa);
                extract(aaa,buf,6);
                printf(_("Dove:     %s\n"),aaa);

                /* 3 - Path dei files */
                serv_gets(buf);
                printf(_("\n *** Path dei files:\n\n"));
                extract(aaa,buf,2);
                printf(_("File dei dati degli utenti: %s\n"),aaa);

                /* 4 - Idle */
                serv_gets(buf);
                printf(_("\n *** Gestione dell'idle:\n\n"));
                cml_printf(_("Butta fuori se &egrave; idle: %s\n"), 
                       print_si_no(extract_int(buf,2)));
                printf(_("Primo avvertimento idle dopo %d minuti\n"),
                       extract_int(buf,3));
                printf(_("Butta fuori dopo %d minuti idle\n"),
                       extract_int(buf,4));
                printf(_("Max min idle: %d\n"), extract_int(buf,5));
                printf(_("Timeout al login in minuti: %d\n"), extract_int(buf,6));

                /* 5 - Nome rooms principali */
                serv_gets(buf);
                printf(_("\n *** Nomi delle room principali:\n\n"));
                extract(aaa,buf,2);
                printf(_("Lobby:              %s>\n"),aaa);
                extract(aaa,buf,3);
                printf(_("Room dei Sysop:     %s>\n"),aaa);
                extract(aaa,buf,4);
                printf(_("Room degli aides:   %s>\n"),aaa);
                extract(aaa,buf,5);
                printf(_("Room dei Roomaides: %s>\n"),aaa);
                extract(aaa,buf,6);
                printf(_("Posta:              %s>\n"),aaa);
                extract(aaa,buf,7);
                printf(_("Room rompiballe:    %s>\n"),aaa);
                extract(aaa,buf,8);
                printf(_("Cestino:            %s>\n"),aaa);
                extract(aaa,buf,9);
                printf(_("Risultati Ricerca:  %s>\n"),aaa);
                extract(aaa,buf,10);
                printf(_("Blog:               %s>\n"),aaa);
        } else
                /* L'utente non possiede il livello di accesso necessario */
                cml_print(_("\nQuesto comando &egrave; riservato ai sysop. "
			    "L'incidente &egrave; stato segnalato.\n"));
}

/*
 * Edita la configurazione del sistema
 */
void edit_sysconfig(void)
{
        char buf[LBUF], cmd[LBUF], aaa[LBUF], bbb[LBUF], ccc[LBUF], ddd[LBUF],
             eee[LBUF], fff[LBUF], ggg[LBUF], hhh[LBUF], iii[LBUF];
        long aa;
        int scelta, a, b, c, d, e, f, g;
        
        printf(_("\nModifica la configurazione del server:\n"
		 "\nCosa vuoi modificare?\n"
		 "1) Configurazione generale del sistema\n"
		 "2) Informazioni sul server\n"
		 "3) Path dei files\n"
		 "4) Gestione dell'idle\n"
		 "5) Nomi delle room principali\n"
		 "6) Niente\n"
		 "Scelta: "));
  
        scelta = 0;
        while ((scelta < '1') || (scelta > '6'))
                scelta = inkey_sc(0);

        printf("%c\n\n", scelta);
        
        if (scelta == '6')
		return;

	/* Legge dal server la configurazione attuale */
	serv_putf("RSYS %c", scelta);
	serv_gets(buf);
    
	if (buf[0]!='2'){
		cml_print(_("\nQuesto comando &egrave; riservato ai sysop.\n"));
		return;
	}

	cmd[0] = 0;
	switch(scelta) {
	case '1': /* 1 - Configurazione del sistema */
		aa = new_long_def(_("TIC_TAC"), extract_long(buf,2));
		a = new_int_def(_("FREQUENZA"), extract_int(buf,3));
		b = new_int_def(_("Livello alla creazione"),
				extract_int(buf,4));
		c = new_si_no_def(_("Express per tutti"),
				  extract_bool(buf,5));
		d = new_int_def(_("Numero minimo di posts per usare gli eXpress"), extract_int(buf,6));
		e = new_si_no_def(_("Salvataggio automatico"),
				  extract_bool(buf,7));
		f = new_int_def(_("Tempo tra due salvataggi in minuti"),
				extract_int(buf,8));
		g = new_si_no_def(_("Nameserver lento"),
				  extract_bool(buf,9));
		sprintf(cmd,"ESYS 1|%ld|%d|%d|%d|%d|%d|%d|%d", aa, a,
			b, c, d, e, f, g);
		break;

	case '2': /* 2 - Info sul server */
		extract(aaa,buf,2);
		new_str_def_m(_("Software"), aaa, 49);
		extract(bbb,buf,3);
		new_str_def_m(_("Versione"), bbb, 9);
		extract(ccc,buf,4);
		new_str_def_m(_("Versione dell'ultimo client"), ccc, 9);
		extract(ddd,buf,5);
		new_str_def_m(_("Nodo"), ddd, 49);
		extract(eee,buf,6);
		new_str_def_m(_("Dove"), eee, 49);
		sprintf(cmd,"ESYS 2|%s|%s|%s|%s|%s", aaa, bbb, ccc,
			ddd, eee);
		break;

	case '3': /* 3 - Path dei files */
		extract(aaa,buf,2);
		new_str_def_M(_("File dei dati degli utenti"), aaa, 49);
		sprintf(cmd,"ESYS 3|1|%s",aaa);
		break;
		
	case '4': /* 4 - Idle */
		a = new_si_no_def(_("Butta fuori se &egrave; idle"),
				  extract_bool(buf,2));
		b = new_int_def(_("Primo avvertimento idle (min)"),
				extract_int(buf,3));
		c = new_int_def(_("Butta fuori idle dopo min"),
				extract_int(buf,4));
		d = new_int_def(_("Max min idle"), extract_int(buf,5));
		e = new_int_def(_("Timeout al login in minuti"),
				extract_int(buf,6));
		sprintf(cmd,"ESYS 4|%d|%d|%d|%d|%d", a, b, c, d, e);
		break;
		
	case '5': /* 5 - Nome rooms principali */
		extract(aaa,buf,2);
		new_str_def_M(_("Lobby"), aaa, ROOMNAMELEN - 1);
		extract(bbb, buf, 3);
		new_str_def_M(_("Room dei Sysop"), bbb, ROOMNAMELEN - 1);
		extract(ccc, buf, 4);
		new_str_def_M(_("Room degli Aide"), ccc, ROOMNAMELEN - 1);
		extract(ddd, buf, 5);
		new_str_def_M(_("Room dei Room Aide"), ddd,
			      ROOMNAMELEN - 1);
		extract(eee, buf, 6);
		new_str_def_M(_("Posta"), eee, ROOMNAMELEN - 1);
		extract(fff, buf, 7);
		new_str_def_M(_("Room rompiballe"), fff, ROOMNAMELEN - 1);
		extract(ggg, buf, 8);
		new_str_def_M(_("Cestino"), ggg, ROOMNAMELEN - 1);
		extract(hhh, buf, 8);
		new_str_def_M(_("Risultati Ricerca"), hhh, ROOMNAMELEN - 1);
		extract(iii, buf, 8);
		new_str_def_M(_("Blog"), iii, ROOMNAMELEN - 1);
		sprintf(cmd, "ESYS 5|%s|%s|%s|%s|%s|%s|%s|%s|%s", aaa, bbb,
			ccc, ddd, eee, fff, ggg, hhh, iii);
		break;
	}
	printf(_("\nVuoi mantenere le modifiche? (s/n) "));
	if (si_no() == 's') {
		serv_puts(cmd);
		serv_gets(buf);
		if (buf[0] == '2')
			printf(_("Modifiche salvate.\n"));
		else
			printf(_("*** Errore! problemi con il server!\n"));
	} else
		printf(_("Nessuna modifica salvata.\n"));
}
