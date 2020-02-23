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
* File : urna-leggi.c                                                       *
*     funzioni per la lettura delle urne esistenti                          *
* Sat Sep 20 23:27:33 CEST 2003
****************************************************************************/
#define USE_REFERENDUM 1

#include "urna-strutture.h"
#include "ansicol.h"
#include "urna-servizio.h"
#include "cml.h"
#include "utility.h"
#include "text.h"
#include "cittaclient.h"
#include "conn.h"
#include "extract.h"
#include <stdio.h>
#include <stdlib.h>
#include "parm.h"
#include "memkeep.h"
#include "urna-gestione.h"
 /**/ /**/ 

void urna_read(long n);
void urna_list(int level);
void urna_check(void);

 /**/

/*
 * Legge il sondaggio #n
 * Se n=0 chiede il numero all'utente.
 * Restituisce il numero di risposte possibili, -1 se non lo trova.
 * urna dati contiene un puntatore a urna_scelte, che contiene
 */
void urna_read(long n)
/* TODO: parametro n non implementato */
{
   struct urna_client dati;
   long num,err;
   struct elenco_ref *elenco[MAX_URNE];
   int n_rs = 0;
   int scelto;
   const char this_format[50]="t%t20l # z%t40Tipo: t%n%s  finisce il% %t20s%n";
   
   IGNORE_UNUSED_PARAMETER(n);

   azzera_dati(&dati);

   /* per leggere il sondaggio chiede solo le liste non zappate
    * (In ogni caso lo fa il server...)                         */

   setcolor(C_QUESTION);

   n_rs=get_elenco_ref(elenco,1);
   if(n_rs == -1) {
	 return;
   }
   if(n_rs == 0) {
	 printf(_("\nNon ci sono sondaggi che puoi leggere.\n"));
	 return;
   }


   scelto=scegli_ref(n_rs, elenco,_("scegliere"),this_format);

   if(scelto==-1){
      cml_printf(_("Ciao, alla prossima!\n"));
      free_elenco_ref(elenco);
      return;
   }

   num=elenco[scelto]->num;
   free_elenco_ref(elenco);

   if(get_ref(&dati, num, "",1) == -1) {
      cml_printf(_("il sondaggio non esiste.\n"));
      return;
   }
   
   if( (err=par2strd(dati.titolo,&dati,"titolo",LEN_TITOLO)))
      printf(_("errore %ld\n"),err);


   dati.testo = txt_create();

   urna_prendi_testo(&dati);

   setcolor(C_URNA);

   if(dati.tipo == TIPO_REFERENDUM)
	  printf(_("\nReferendum"));
   else
	  printf(_("\nSondaggio "));
   cml_printf(_(" #<b>%-2ld</b>: "), num);
   setcolor(C_URNA_TITOLO);
   cml_printf(_("%s\n"), dati.titolo);
   setcolor(C_URNA);
   cml_printf(_("\tCreato da <b>%s</b> in <b>%s</b>.\n\n"),
			  dati.proponente, dati.room);

#if 0
   cml_printf(_("                creato da "));
   setcolor(C_USER);
   printf(_("%s"), dati->proponente);
   setcolor(C_URNA);
   printf(_(" in "));
   setcolor(C_ROOM);
   cml_printf(_("%s</b>>\n\n"), dati->room);
#endif


   setcolor(C_URNA);
   setcolor(C_URNA_TITOLO);
   printf(_("%sQuesito:%s\n"), hbold, hstop);
   setcolor(C_URNA);
   stampa_quesito(dati.testo);

   if(urna_prendi_voci(&dati)){
	 free_dati(&dati);
	  return;
   }

   if(dati.modo!=MODO_PROPOSTA){
		   printf(_("\n%sRisposte possibili:\n%s"), hbold, hstop);
   			stampa_voci(&dati);
   }
   printf("\n");
 
   /* diventa una funzione? */
   switch (dati.modo) {
   case MODO_SCELTA_SINGOLA:
	  cml_printf(_("Si pu&ograve dare una sola risposta.\n"));
	  break;
   case MODO_SCELTA_MULTIPLA:
	  printf(_("Si possono dare %d risposte.\n"), dati.max_voci);
	  break;
   }

   if(dati.bianca)
	  cml_printf(_("Si pu&ograve votare scheda bianca.\n"));

   if((dati.modo == MODO_VOTAZIONE) && (dati.ast_voto))
	  cml_printf(_("Ci si pu&ograve astenere sulla singola voce."));

   cml_print(_("\nLa consultazione &egrave iniziata "));
   stampa_datall(dati.start);
   cml_print(_("\n\te si concluder&agrave <b>"));
   stampa_datall(dati.stop);
   cml_print(_("</b>.\n\nPossono votare tutti gli utenti<b>"));

   /* diventa una funzione? */
   switch (dati.criterio) {
   case 0:
	  break;
   case 1:
	  printf(_(" di livello %s"), titolo[dati.val_crit]);
	  break;
   case 2:
	  printf(_(" registrati prima del "));
	  stampa_data(dati.anzianita);
	  break;
   case 3:
	  printf(_(" con un minimo di %ld chiamate all'attivo"), dati.val_crit);
	  break;
   case 4:
	  printf(_(" che hanno lasciato almeno %ld post"), dati.val_crit);
	  break;
   case 5:
	  printf(_(" che hanno inviato almeno %ld xmsg"), dati.val_crit);
	  break;
   case 6:
	  if(titolo[dati.val_crit])
		 printf(_(" maschi"));
	  else
		 printf(_(" femmine"));
   }
   cml_print(_("</b>.\n"));
   free_dati(&dati);
   return;
}


/*
 * Lista dei Sondaggi attivi.
 * se level =1 tutti
 * altrimenti solo quelli delle room non zappate
 */
void urna_list(int level)
{
   int n_rs;
   struct elenco_ref *elenco[MAX_URNE];

   setcolor(C_URNA);

   n_rs=get_elenco_ref(elenco,level);
   if(n_rs==-1)
		return;

   print_ref(n_rs,elenco,_("t l#z%t35T%n  m%t35%s termina il% s%n"));
   free_elenco_ref(elenco);
}


/* Controlla se ci sono sondaggi/referendum da votare.
 * Viene eseguita al login.
 */
void urna_check(void)
{
   int i, n_rs = 0, n_s = 0, n_r = 0;
   struct elenco_ref * elenco[MAX_URNE];
   if((n_rs=get_elenco_ref(elenco,6))==-1){
		   return;
   }

   if((n_rs=get_elenco_ref(elenco,6))==0){
		   return;
   }

   for(i=0;i<n_rs;i++){
		   switch(elenco[i]->tipo){
				   case TIPO_REFERENDUM: n_r++;
										 break;
				   case TIPO_SONDAGGIO: n_s++;
										 break;
				   default: printf("something is wrong\n");
		   }
   }

   /*  dovrebbe diventare un get_ref */
   setcolor(C_URNA);

   /* i18n non adeguato...*/

   ordina_ref(n_rs,elenco,"");
   switch (n_r){
		   case 0:
				   break;
		   case 1:
		 cml_printf(_("\nC'&egrave <b>un referendum</b> che dovresti votare"));
		 break;
		   default:
		 cml_printf(_("\nCi sono <b>%d referendum</b> che dovresti votare"), n_r);
   }

   if(n_r>0 && n_s>0)
		   cml_printf(_(", e"));

   switch (n_s){
		   case 0:
				   break;
		   case 1:
         cml_printf(_("\nC'&egrave <b>un sondaggio</b> al quale puoi partecipare"));
		 break;
		   default:
      cml_printf(_("\nCi sono <b>%d sondaggi</b> ai quali puoi partecipare"), n_s);
   }

   printf(_(":\n"));

   print_ref(n_rs,elenco,"t %t13l#%t25T%n%t13zvV%n");
   putchar('\n');
   free_elenco_ref(elenco);

}


/*
void urna_check(void)
{
   int i, n_rs = 0, n_s = 0, n_r = 0;
   long elenco_rs[MAX_REFERENDUM];
   int tipo_rs[MAX_REFERENDUM];
   char *elenco_titoli[MAX_REFERENDUM + MAX_SONDAGGI];
   char buf[LBUF];

   serv_puts(_("SLST 2"));
   serv_gets(buf);


   if(buf[0] != '2')
	  return;

   while(serv_gets(buf), strcmp(buf, "000")) {
	  elenco_rs[n_rs] = extract_int(buf + 4, 0);
	  elenco_titoli[n_rs] = CalloC(sizeof(char), LEN_TITOLO,"legtitoli");
	  extractn(elenco_titoli[n_rs], buf + 4, 2, LEN_TITOLO);
	  tipo_rs[n_rs] = extract_int(buf + 4, 1);
	  if(tipo_rs[n_rs] == TIPO_REFERENDUM)
		 n_r++;
	  else
		 n_s++;
	  n_rs++;
   }

   if((n_s + n_r) < 1)
	  return;

   setcolor(C_URNA);


   switch (n_r){
		   case 0:
				   break;
		   case 1:
		 cml_printf(_("\nC'&egrave <b>un referendum</b> che dovresti votare"));
		 break;
		   default:
		 cml_printf(_("\nCi sono <b>%d referendum</b> che dovresti votare"), n_r);
   };

   if(n_r>0 && n_s>0)
		   cml_printf(_(", e"));

   switch (n_s){
		   case 0:
				   break;
		   case 1:
         cml_printf(_("\nC'&egrave <b>un sondaggio</b> al quale puoi partecipare"));
		 break;
		   default:
      cml_printf(_("\nCi sono <b>%d sondaggi</b> ai quali puoi partecipare"), n_s);
   };

   printf(_(":\n"));


   for(i = 0; i < n_rs; i++) {
	  if(tipo_rs[i] == TIPO_REFERENDUM) {
		 printf(_("Referendum "));
		 cml_printf(_("#<b>%2ld</b>: "), elenco_rs[i]);
		 setcolor(C_URNA_TITOLO);
		 cml_printf(_("%s\n"), elenco_titoli[i]);
		 setcolor(C_URNA);
	  }
   }
   for(i = 0; i < n_rs; i++) {
	  if(tipo_rs[i] == TIPO_SONDAGGIO) {
		 printf(_("Sondaggio "));
		 cml_printf(_("#<b>%2ld</b>: "), elenco_rs[i]);
		 setcolor(C_URNA_TITOLO);
		 cml_printf(_("%s\n"), elenco_titoli[i]);
		 setcolor(C_URNA);
	  }
   }
   putchar('\n');

   for(i = 0; i < n_rs; i++)
	  FreE(elenco_titoli[i],_("leggi311"));
}
*/
