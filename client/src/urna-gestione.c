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
* Cittadella/UX client            (C) M. Caldarelli and R. Vianello         *
*                                                                           *
* File : urna-gestione.c                                                    *
*     funzioni per la gestione dei referendum e sondaggi.                   *
*    Sat Sep 20 23:18:24 CEST 2003                                          *
****************************************************************************/

#define USE_REFERENDUM 1

#include "urna_comune.h"
#include "cittaclient.h"
#include "utility.h"
#include "conn.h"
#include "inkey.h"
#include "cml.h"
#include "extract.h"
#include "urna_errori.h"
#include "memkeep.h"
#include "urna-vota.h"
#include "urna-strutture.h"
#include "urna-servizio.h"
#include "urna-gestione.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

void urna_completa(void);
void urna_delete(void);
void urna_postpone(void);
int trova_ref(int n_rs, struct elenco_ref *elenco[], int num);

/*****************************************************************************/

void urna_completa(void)
{
   int num,scelto;
   int n_rs;
   char buf[LBUF];
   struct elenco_ref *elenco[MAX_URNE];

   n_rs=get_elenco_ref(elenco,4);

   if(n_rs == -1) {
     return;
   };

   if(n_rs == 0) {
     printf(_("\nNon ci sono sondaggi che puoi concludere\n"));
     return;
   };

         scelto=scegli_ref(n_rs, elenco,"concludere","#lTr");

   if(scelto==-1){
         cml_printf(_("Ciao, alla prossima!\n"));
		 free_elenco_ref(elenco);
		 return;
   };

   num=elenco[scelto]->num;

   serv_putf("SSTP %ld", num);
   serv_gets(buf);
   if(buf[0] == '2')
      cml_print(_("Ok, il sondaggio &egrave stato terminato.\n"));
   else
      printf(_("Non puoi terminare questo sondaggio.\n"));

	 free_elenco_ref(elenco);
}

void urna_delete(void)
{
   int num,scelto;
   int n_rs;
   char buf[LBUF];
   struct elenco_ref *elenco[MAX_URNE];

   n_rs=get_elenco_ref(elenco,4);

   if(n_rs == -1) {
     return;
   };

   if(n_rs == 0) {
     printf(_("\nNon ci sono sondaggi che puoi cancellare\n"));
     return;
   };

  scelto=scegli_ref(n_rs, elenco,"cancellare","#lTr");

   if(scelto==-1){
         cml_printf(_("Ciao, alla prossima!\n"));
		 free_elenco_ref(elenco);
		 return;
   }

   num=elenco[scelto]->num;

   serv_putf("SDEL %ld", num);
   serv_gets(buf);
   if(buf[0] == '2')
      cml_print(_("Ok, il sondaggio &egrave stato eliminato.\n"));
   else
      printf(_("Non puoi cancellare questo sondaggio.\n"));

		 free_elenco_ref(elenco);
}

void urna_postpone(void)
{
   int num, scelto;
   int n_rs;
   char c;
   char buf[LBUF];
   time_t old_stop;
   struct tm *new_stop;
   struct elenco_ref *elenco[MAX_URNE];

   n_rs = get_elenco_ref(elenco, 4);

   if (n_rs == -1) {
     return;
   }

   if(n_rs == 0) {
     printf(_("\nNon ci sono sondaggi che puoi postporre\n"));
     return;
   }

   scelto = scegli_ref(n_rs, elenco,"posporre","#lTr");

   if (scelto == -1){
         cml_printf(_("Ciao, alla prossima!\n"));
		 free_elenco_ref(elenco);
		 return;
   }

   num=elenco[scelto]->num;
   old_stop = elenco[scelto]->stop;

   printf(_("Il sondaggio termina il "));
   stampa_datall(elenco[scelto]->stop);
   free_elenco_ref(elenco);

   new_stop = myCalloc(1, sizeof(struct tm));

   c = 'n';
   while(c != 's' && (mktime(new_stop) < old_stop)) {
      printf(_("\nNuovo termine per inviare il voto:\n"));
      new_date(new_stop, 1);
      if(mktime(new_stop) < old_stop) {
         cml_printf (_("\nNon puoi anticipare la scadenza;")
                  _("se vuoi terminare premi <b>s</b>.\n"));
         c = inkey_sc(0);
      }
   }

   if(c == 's') {
      myFree(new_stop);
      return;
   }

   serv_putf("SPST %ld|%ld", num, mktime(new_stop));
   serv_gets(buf);
   if(buf[0] == '2') {
      num = extract_int(buf + 4, 0);
      cml_printf(_("\nOk, il sondaggio %d &egrave stato modificato."), num);
   } else {
      printf(_("\nNon puoi modificare il sondaggio %d.\n"), num);
      stampa_errore(buf);
   }

   myFree(new_stop);
   return;
}

/* 
 * trova la posizione 
 * del referendum "num" 
 * o lettera 
 * da mettere a posto...
 */
int trova_ref(int n_rs, struct elenco_ref *elenco[], int num){
         int i=0;
         char str[4];

         snprintf(str,4,"%d",num);
          while(i < n_rs){
               if (num == elenco[i]->num||
                              strcmp(str, elenco[i]->lettera) == 0)
                     return i;
			   i++;
         }
         return -1;
}

/******************
 * ERRORI
 */

char *messaggi_errori_urna[] = {
		"il sondaggio non esiste",
		"non puoi votare questo sondaggio",
		"le risorse sono esaurite, prova dopo",
		"non stavi votando",
		"il tuo livello &egrave insufficiente",
		"non puoi votare",
		"hai gi&agrave votato",
		"i voti sono in formato sbagliato",
		"non puoi votare scheda bianca",
		"i voti sono in formato sbagliato",
		"non sei autorizzato a cancellare",
		"non sei autorizzato a posticipare",
		"errore nel protocollo",
		"hai dato pochi dati",
		"hai dato troppi voti",
		"hai dato pochi voti",
		"non &egrave; possibile astenersi",
		"non &egrave; possibile astenersi",
		"hai dato pochi dati",
		"hai gi&agrave; posticipato il massimo",
		"la data &egrave; nel passato",
		"stai anticipando, non si pu&ograve",
		"la data di fine &egrave; troppo vicina",
		"i dati per il sondaggio sono sbagliati",
		"hai dato troppe voci",
		"hai dato poche voci",
		"non sei Aide",
		"ci sono gi&agrave; il massimo dei sondaggi",
		"l'urna &egrave; terminata",
		"titolo troppo corto"
};


void stampa_errore(char* buf)
{
	long num;
	
	if(strlen(buf) < 4)
		num = -1;
	else {
		num = extract_int(buf+4,0);
	}


	if ((num <0 ) || (num >= END_URNA))
		cml_printf(_("\nErrore %ld sconosciuto, chiedi ad un Aide.\n"),
		       num);
	else
		cml_printf(messaggi_errori_urna[num]);
}
