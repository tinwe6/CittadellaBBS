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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: email.c                                                             *
*       Invia Email in rete.                                                *
****************************************************************************/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "email.h"
#include "macro.h"
#include "memstat.h"
#include "text.h"
#include "utente.h"
#include "utility.h"

/*****************************************************************************/
/*
 * Invia un email su internet.
 *
 * Se txt non e' NULL, il body del messaggio e' nella struttura txt,
 * altrimenti lo prende dal file 'filename'.
 *
 * Il subject e' in 'subj' e 'rcpt' e' l'indirizzo Email dei destinatario.
 * Se rcpt == NULL, il mail viene inviato a tutti gli utenti della BBS,
 * ai soli utenti validati o ai soli utenti col flag UT_NEWSLETTER settato,
 * a seconda che 'mode' sia EMAIL_ALL, EMAIL_VALID, o EMAIL_NEWSLETTER.
 *
 * Restituisce TRUE se l'email e` stato inviato, FALSE altrimenti.
 */
bool send_email(struct text *txt, char * filename, char *subj, char *rcpt,
		int mode)
{
        struct lista_ut *utente;
        char buf[LBUF];
        bool ret;
        FILE *fp;
        char *tmpf, *tmpr, *str;
#ifdef MKSTEMP
	int fdf, fdr;
#endif

	/* Crea la lista dei destinatari */
	if (rcpt == NULL) {
#ifdef MKSTEMP
		tmpr = Strdup(TEMP_RCPT_TEMPLATE);
		fdr = mkstemp(tmpr);
		fp = fdopen(fdr, "a");
#else
		tmpr = tempnam(TMPDIR, PFX_RCPT);
		fp = fopen(tmpr, "a");
#endif
		utente = lista_utenti;
		switch (mode) {
		case EMAIL_ALL:
			for (utente = lista_utenti; utente;
			     utente = utente->prossimo) {
				if (strlen(utente->dati->email))
					fprintf(fp,",%s", utente->dati->email);
			}
			break;
		case EMAIL_VALID:
			for (utente = lista_utenti; utente;
			     utente = utente->prossimo) {
				if (utente->dati->livello > LVL_NON_VALIDATO)
					fprintf(fp,",%s", utente->dati->email);
			}
			break;
		case EMAIL_NEWSLETTER:
			for (utente = lista_utenti; utente;
			     utente = utente->prossimo) {
				if ((utente->dati->livello > LVL_NON_VALIDATO)
				    && (utente->dati->flags[5]&UT_NEWSLETTER))
					fprintf(fp,",%s", utente->dati->email);
			}
			break;
		}
		fclose(fp);
	}

	if (txt) {
#ifdef MKSTEMP
                tmpf = Strdup(TEMP_EMAIL_TEMPLATE);
                fdf = mkstemp(tmpf);
                fp = fdopen(fdf, "a");
#else
                tmpf = tempnam(TMPDIR, PFX_EMAIL);
                fp = fopen(tmpf,"a");
#endif
		txt_rewind(txt);
		while ( (str = txt_get(txt)))
			fprintf(fp, "%s\n", str);
		fclose(fp);
	} else
		tmpf = filename;

	if (rcpt == NULL)
		sprintf(buf, "mail -n -s '[Cittadella BBS] %s' -b \"`cat %s`\" '%s' < %s\n",
			subj, tmpr, BBS_EMAIL, tmpf);
	else
		sprintf(buf, "mail -n -s '[Cittadella BBS] %s' '%s' < %s\n", subj,
			rcpt, tmpf);
	if (system(buf) == -1) {
		citta_logf("SYSERR: Email [%s] to [%s] not sent.", subj, rcpt);
		ret = false;
	} else
		ret = true;
	
	if (txt) {
		unlink(tmpf);
		Free(tmpf);
	}

	if (rcpt == NULL) {
		unlink(tmpr);
		Free(tmpr);
	}

	return ret;
}

#ifdef NO_DOUBLE_EMAIL
int check_double_email (char *email) {
        FILE *email_fh;
        char comp_email[MAXLEN_EMAIL];
        bool found = false;

        email_fh = fopen(FILE_DOUBLE_EMAIL, "r");
        while (email_fh && !feof(email_fh) && !found) {
                fscanf(email_fh,"%s\n",comp_email);
                found = !strcmp(comp_email, email);
        }
        if (email_fh) 
                fclose(email_fh);

        if (!found) {
                email_fh = fopen(FILE_DOUBLE_EMAIL, "a");
                fprintf(email_fh, "%s\n", email);
                fclose(email_fh);
        }
        return(!found);
}
#endif /* NO_DOUBLE_EMAIL */
