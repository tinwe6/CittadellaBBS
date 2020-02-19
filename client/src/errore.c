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
* File : errore.c                                                           *
*        Funzioni per stampare i messaggi di errore dal server.             *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cittaclient.h"
#include "ansicol.h"
#include "cml.h"
#include "conn.h"
#include "errore.h"
#include "extract.h"
#include "msg_flags.h"
#include "utility.h"

/***************************************************************************/
/* Stampa messaggio di errore su invio di X/Broadcast.
 * Restituisce TRUE se il messaggio va comunque inserito nel diario. */
bool print_err_x(char *str)
{
	char caio[LBUF];
        bool ret;

	extract(caio, str+4, 0);
        ret = false;

	switch(extract_int(str+1, 0)) {
	case X_ACC_PROIBITO:
		cml_printf(sesso ? _("\nNon sei autorizzata ad inviare Xmessage.\n") : _("\nNon sei autorizzato ad inviare Xmessage.\n"));
		break;
	case X_NO_SUCH_USER:
		cml_printf(_("\nL'utente '%s' non esiste.\n"), caio);
		strcpy(last_x, "");
		break;
	case X_EMPTY:
		cml_printf(_("Nessun testo. Messaggio annullato.\n"));
		break;
	case X_TXT_NON_INIT:
		cml_printf(_("Errore: messaggio non inizializzato.\n"));
		break;
	case X_UTENTE_NON_COLLEGATO:
		cml_printf(extract_int(str+4, 1) ? _("%s non &egrave; collegata in questo momento.\n") : _("%s non &egrave; collegato in questo momento.\n"), caio);
		strcpy(last_x, "");
		break;
	case X_SELF:
		cml_printf(sesso ? _("Non puoi mandare messaggi a te stessa!")
                           : _("Non puoi mandare messaggi a te stesso!"));
		strcpy(last_x, "");
		break;
	case X_RIFIUTA:
		cml_printf(_("Mi dispiace, %s rifiuta gli Xmsg.\n"),
                           caio);
		strcpy(last_x, "");
		break;
	case X_SCOLLEGATO:
		cml_printf(extract_int(str+4, 1) ? _("Spiacente ma nel frattempo %s si &egrave; scollegata.\n") : _("Spiacente ma nel frattempo %s si &egrave; scollegato.\n"), caio);
		ret = true;
                break;
	 case X_OCCUPATO:
		 cml_printf(extract_int(str+4,1) ? _("%s &egrave; impegnata. Lo legger&agrave; il prima possibile.\n") : _("%s &egrave; impegnato. Lo legger&agrave; il prima possibile.\n"), caio);
		ret = true;
                break;
	 case X_UTENTE_CANCELLATO:
		 cml_printf(_("L'utente #%s &egrave stato nel frattempo cancellato. Mi dispiace!\n"), caio);
                 break;
	}
	return ret;
}

/* Stampa messaggio di errore su invio di X, nell'input dei destinatari. */
void print_err_destx(char *str)
{
	char caio[LBUF];

	extract(caio, str+4, 0);
	switch(extract_int(str+1, 0)) {
	case X_NO_SUCH_USER:
		cml_printf(_("L'utente '<b>%s</b>' non esiste."), caio);
		break;
	case X_TXT_NON_INIT:
		cml_printf(_("Errore: messaggio non inizializzato."));
		break;
	case X_UTENTE_NON_COLLEGATO:
		cml_printf(extract_int(str+4, 1) ? _("<b>%s</b> non &egrave; collegata in questo momento.") : _("<b>%s</b> non &egrave; collegato in questo momento."),caio);
		break;
	case X_SELF:
		cml_printf(sesso ? _("Non puoi mandare messaggi a te stessa!")
                       : _("Non puoi mandare messaggi a te stesso!"));
		break;
	case X_RIFIUTA:
		cml_printf(_("Mi dispiace, <b>%s</b> rifiuta gli Xmsg."), caio);
		break;
	}
}

void print_err_fm(char *str)
{
	int err;
	long fmnum, msgnum, msgpos;

	err    = extract_int(str, 0);
	fmnum  = extract_long(str, 1);
	msgnum = extract_long(str, 2);
	msgpos = extract_long(str, 3);
	switch(err) {
	case FMERR_ERASED_MSG:
		cml_printf(_("\nMi dispiace, il messaggio #%ld &egrave; stato sovrascritto.\n"), msgnum);
		return;
	case FMERR_DELETED_MSG:
		cml_printf(_("\nMi dispiace, il messaggio #%ld &egrave; stato cancellato.\n"), msgnum);
		return;
	case FMERR_NO_SUCH_MSG:
		cml_printf(_("\nMi dispiace, il messaggio #%ld &egrave; inesistente!\n"), msgnum);
		return;
	case FMERR_OPEN:
		printf(_("\nNon riesco ad aprire il file dei messaggi #%ld.\n"), fmnum);
		break;
	case FMERR_NOFM:
		printf(_("\nIl file dei messaggi #\%ld non esiste!!\n"), fmnum);
		break;
	case FMERR_CHKSUM:
	case FMERR_MSGNUM:
		cml_printf(_("Il messaggio (%ld, %ld, %ld) &egrave; corrotto (Err #%d)\n"), fmnum, msgnum, msgpos, err);
		break;
	}
	printf(_("Per favore segnala il problema ai Sysop. Grazie.\n"));
}

void print_err_edit_room_info(const char *buf)
{
	switch (buf[0]) {
	case '1':
		switch(buf[1]) {
		case '0':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_("*** <b>Problemi con il server!</b> Segnala il problema ai Sysop.\n"));
			pull_color();
			break;
		case '4':
			cml_printf(_("Nessun testo. Modifica delle Room Info annullata.\n"));
			break;
		case '9':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_("*** <b>Operazione non autorizzata.</b>\n"));
			pull_color();
			break;
		}
		break;
	case '2':
		cml_printf(_("Ok, le Room Info sono state aggiornate.\n"));
		break;
	}
}

void print_err_edit_profile(const char *buf)
{
	switch (buf[0]) {
	case '1':
		switch(buf[1]) {
		case '0':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_(" *** <b>Problemi con il server!</b> Segnala il problema ai Sysop.\n"));
			pull_color();
			break;
		case '4':
			cml_printf(_("Nessun testo. Modifica del Profile annullata.\n"));
			break;
		case '9':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_(" *** <b>Operazione non autorizzata.</b>\n"));
			pull_color();
			break;
		}
		break;
	case '2':
		cml_printf(_("Ok, il profile &egrave; stato aggiornato.\n"));
		break;
	}
}

void print_err_edit_news(const char *buf)
{
	switch (buf[0]) {
	case '1':
		switch(buf[1]) {
		case '0':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_(" *** <b>Problemi di scrittura!</b> Segnala il problema al Sysop.\n"));
			pull_color();
			break;
		case '4':
			cml_printf(_("Nessun testo. Modifica delle News annullata.\n"));
			break;
		case '9':
			push_color();
			setcolor(C_ERROR);
			cml_printf(_(" *** <b>Operazione non autorizzata.</b>\n"));
			pull_color();
			break;
		}
		break;
	case '2':
		cml_printf(_("Ok, le news sono state aggiornate.\n"));
		break;
	}
}

void print_err_test_msg(const char *buf, const char *room)
{
        putchar('\n');
        push_color();
        setcolor(C_ERROR);
        switch (buf[1]) {
        case '2':
                cml_printf(_("*** Mi dispiace, non hai accesso alla room <b>%s</b>.\n"), room);
                break;
        case '4':
                cml_printf(_("*** Mi dispiace, non si possono allegare messaggi in <b>%s</b>.\n"), room);
                break;
        case '7':
                cml_printf(_("*** Mi dispiace, la room <b>%s</b> &egrave; inesistente!\n"), room);
                break;
        default:
                cml_printf(_("*** Purtroppo il messaggio non &egrave; pi&uacute; presente nella BBS.\n"));
        }
        pull_color();
}
