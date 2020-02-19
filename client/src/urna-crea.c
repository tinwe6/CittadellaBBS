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
* Cittadella/UX client         (C) M. Caldarelli and R. Vianello            *
*                                                                           *
* File : urna-crea.c                                                        *
*    funzioni per la gestione dei referendum e sondaggi.                    *
*Fri Nov 21 23:08:46 CET 2003 
* i18n
****************************************************************************/

#define USE_REFERENDUM 1
#include "cittaclient.h"
#include "urna-servizio.h"
#include "urna-gestione.h"
#include "memkeep.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "utility.h"
#include "conn.h"
#include "cml.h"
#include "text.h"
#include "edit.h"
#include "inkey.h"
#include "extract.h"
#include "urna_errori.h"
#include "urna-strutture.h"
#include "ansicol.h"
#include "memkeep.h"

#define FREE(a) FreE(a,__LINE__);

/****************************************************************************/

void urna_new(void);
int crea_stringa_per_server(char *str, struct urna_client *u);
void interrompi();
void chiedi_proposte(struct urna_client *dati, int min_scelte);
int scegli_diritto(struct urna_client *dati);

/****************************************************************************/

void urna_new(void)
{
   struct text *txt;
   char  buf[LBUF];
   char str_cl[(MAX_POS + 1) * ((MAX_POSLEN) + 1) + 6];
   char rs_tipo[12];
   int i, num_sond;
   struct tm tmst;
   struct urna_client *dati;
   char c;
   int risp;
   char *str;
   char str_si_no[3] = _("sn-");
   int min_scelte = 2;

   str_si_no[2] = Ctrl('X');

   dati= (struct urna_client*)myCalloc(1,sizeof(struct urna_client));
   azzera_dati(dati);
   strncpy(rs_tipo,"12345678901",12);

   strncpy(rs_tipo, _("sondaggio"),12);

   /*
    * risposte di default
    */

   txt = txt_create();

   printf(_("\n"));
   if(livello >= LVL_AIDE)
      dati->tipo = new_si_no_def(_("&Egrave un referendum?"), false);

   if(dati->tipo == 1)
      strncpy(rs_tipo, _("referendum"),12);

   serv_putf("SRNB %d", dati->tipo);

   serv_gets(buf);
   if(buf[0] != '2') {
      printf(_("\nNon puoi creare referendum o sondaggi in questa room.\n"));
     free_dati(dati);
     myFree(dati);
      return;
   }

   cml_printf(_("Con ^X interrompi immediatamente la creazione dell'urna\n"));

   cml_printf(_("&Egrave possibile votare scheda bianca [n]? "));

   risp = inkey_elenco_def(str_si_no, _('n'));
   if(risp == Ctrl('X')) {
      interrompi();
     free_dati(dati);
     myFree(dati);
      return;
   };

   dati->bianca = risp == _('n') ? 0 : 1;

  
 strcpy(dati->titolo,"");

  while(!min_lungh(dati->titolo,3)){
   cml_printf(_("\nTitolo (max %d char): "), LEN_TITOLO - 1);
   if(new_str_m_abo("", dati->titolo, LEN_TITOLO - 1)==-1){
      interrompi();
     free_dati(dati);
     myFree(dati);
      return;
   };
   if(!min_lungh(dati->titolo,3))
   cml_printf(_("Titolo troppo corto:%s\n"),dati->titolo);

}

   printf(_("\nInserisci il quesito (max 4 righe):\n"));
   if(get_text_col(txt, 4, 79, 0)==-1){
      interrompi();
     free_dati(dati);
     myFree(dati);
      return;
   };


   printf(_(_("\nTipo di risposte:\n ")));
   cml_printf(_("\t%d: S&iacute/No\n"), MODO_SI_NO);
   printf(_("\t%d: Scelta di uno\n "), MODO_SCELTA_SINGOLA);
   cml_printf(_("\t%d: Scelta tra pi&uacute d'uno\n"), MODO_SCELTA_MULTIPLA);
   printf(_("\t%d: Votazione\n"), MODO_VOTAZIONE);
   printf(_("\t%d: Proposta\n"), MODO_PROPOSTA);
   printf(_("\nScelta: "));

   c = -1;

   while(((c < '0') || (c > '4')) && c!=Ctrl('X'))
      c = inkey_sc(0);

   if(c == Ctrl('X')) {
         interrompi();
          free_dati(dati);
          myFree(dati);
          return;
   };

	 printf(_("%c"),c);

   dati->modo = c - '0';

   if(dati->modo == MODO_SCELTA_MULTIPLA)
      min_scelte = 3;

   if(dati->modo == MODO_VOTAZIONE)
      min_scelte = 1;


   switch (dati->modo) {
   case MODO_SI_NO:
      dati->num_voci = 2;
      dati->max_voci = 1;
     *(dati->testa_voci+0)=(char*)myCalloc(3,sizeof(char));
     *(dati->testa_voci+1)=(char*)myCalloc(3,sizeof(char));
      strcpy(*(dati->testa_voci), _("no"));
      strcpy(*(dati->testa_voci+1), _("si"));
      break;
   case MODO_PROPOSTA:
      printf(_("\nQuante proposte si possono dare (%d-%d)? "), 2,
             MAX_VOCI);
      risp = new_int_def(_(""),4);
      while((risp < 2 || (risp > MAX_VOCI)))
         risp = new_int_def(_("  Risposta non valida. Numero voci: "),4);
      dati->max_prop = risp;
      printf(_("\nQuanto lunghe possono essere? (%d-%d)? "), 15,
             MAXLEN_PROPOSTA);
      risp = new_int_def(_(""),31);
      while((risp < 15 || (risp > MAXLEN_PROPOSTA)))
         risp = new_int_def(_("  Risposta non valida. lunghezza: "),31);

      dati->max_len_prop = risp;
      break;
   default:
      dati->num_voci=chiedi_scelte(dati->testa_voci, min_scelte, 
                 MAXLEN_VOCE, MAX_VOCI,_("voci"));
   }

   if(dati->modo == MODO_SCELTA_MULTIPLA) {
      printf(_("\nQuante voci si possono scegliere (%d-%d)? "), 2,
             dati->num_voci);
      risp = new_int(_(""));
      while((risp < 2 || (risp > dati->num_voci)))
         risp = new_int(_("  Risposta non valida. Numero voci: "));
      dati->max_voci = risp;
   }

   if(dati->modo == MODO_VOTAZIONE) {
      dati->max_voci = dati->num_voci;
      dati->ast_voto = new_si_no_def(
			 _("\nSi pu&ograve votare solo per alcuni?"), false);
   }

   if(scegli_diritto(dati)){
         interrompi();
          free_dati(dati);
          myFree(dati);
          return;
   };

   printf(_("\nTermine ultimo per inviare il voto:\n"));
    new_date(&tmst, 1);

   dati->stop = mktime(&tmst);

   strcpy(dati->proponente, _(""));
   strcpy(dati->room, _(""));
   dati->start = dati->stop;

   cml_print(_("La consultazione terminer&agrave il "));
   stampa_data(dati->stop);
   printf(_("\n"));
   printf(_("\nVuoi inviare il %s (s/n)? "), rs_tipo);

   if(si_no() == _('n')) {
     free_dati(dati);
     myFree(dati);
      interrompi();
      return;
   };

   strcpy(str_cl,"SRNE ");
   crea_stringa_per_server(str_cl, dati);
   txt_rewind(txt);


   /* 
	* controllare che non sia
	* troppo lungo
	* !!?
	*/

   while((str=txt_get(txt)))
      serv_putf("PARM testo|%s", str);

   for(i = 0; i < dati->num_voci; i++) {
      serv_putf("PARM voce-%d|%s", i, *(dati->testa_voci+i));
   }

   serv_putf("PARM titolo|%s", dati->titolo);
   serv_putf("%s", str_cl);
   serv_gets(buf);

   if(buf[0] == '2') {
      num_sond = extract_int(buf + 4, 0);
      cml_printf(_("\nOk, il %s %d &egrave iniziato.\n"), rs_tipo, num_sond);
   } else {
      stampa_errore(buf);
   }
   free_dati(dati);
   myFree(dati);
   return;
}

/*
void chiedi_proposte(struct urna_client *dati, int min_scelte){
      dati->max_prop=chiedi_scelte(dati->testa_voci, min_scelte, _(" proposte "));
      return;
}
*/


/*
 * certo dovrebbe eessere
 * piu` furba
 */

int crea_stringa_per_server(char *str, struct urna_client *dati)
{
   char *sep = _("|");
   char *(client[MAX_POS]);
   int i;

   for(i = 0; i < MAX_POS; i++) {
      client[i] = myCalloc(MAX_POSLEN + 1, sizeof(char));
      /* boh... quanto è inefficiente? */
   }

   sprintf(client[POS_NUM], _("%d"), -1);
   sprintf(client[POS_TIPO], _("%d"), dati->tipo);
   sprintf(client[POS_PROPONENTE], _("%s"), dati->proponente);
   sprintf(client[POS_NOME_ROOM], _("%s"), dati->room);
   sprintf(client[POS_NUM_VOCI], _("%d"), dati->num_voci);
   sprintf(client[POS_BIANCA], _("%d"), dati->bianca);
   sprintf(client[POS_ASTENSIONE_VOTO], _("%d"), dati->ast_voto);
   sprintf(client[POS_MAX_VOCI], _("%d"), dati->max_voci);
   sprintf(client[POS_MODO], _("%d"), dati->modo);
   sprintf(client[POS_START], _("%ld"), dati->start);
   sprintf(client[POS_STOP], _("%ld"), dati->stop);
   sprintf(client[POS_CRITERIO], _("%d"), dati->criterio);
   sprintf(client[POS_VAL_CRITERIO], _("%ld"), dati->val_crit);
   sprintf(client[POS_ANZIANITA], _("%ld"), dati->anzianita);
   sprintf(client[POS_NUM_PROP], _("%d"), dati->max_prop);
   sprintf(client[POS_MAXLEN_PROP], _("%d"), dati->max_len_prop);
   for(i = 1; i < MAX_POS; i++) {
      strcat(str, sep);
      strcat(str, client[i]);
   }

   for(i = 0; i < MAX_POS; i++) {
      myFree(client[i]);
   }
   return (0);
};

void interrompi()
{
   char buf[LBUF * 10];

   serv_putf(_("SRNE -1"));
   serv_gets(buf);
   if(buf[0] != '2') {
      printf(_("\nConsultazione mai iniziata\n"));
   } else {
      printf(_("\nConsultazione interrotta\n"));
   }
   return;
}


int scegli_diritto(struct urna_client *dati)
{

   char c = 0;
   struct tm tmst;

   cml_print(_("\nAventi diritto al voto :\n")
             _(" 1. Tutti\n")
             _(" 2. Livello minimo\n")
             _(" 3. Anzianit&agrave\n")
             _(" 4. Numero chiamate\n")
             _(" 5. Numero di post\n")
             _(" 6. Numero di eXpress message\n") " 7. Sesso\n" "Scelta: [1] ");

   while((c < '1' || c > '7') && c!=10 && c!=Ctrl('X')){
      c = inkey_sc(0);
   }

   if (c==Ctrl('X')){
         return -1;
   };

   if(c==10){
         c='1';
   }

   dati->criterio = c - '0' - 1;

   switch (dati->criterio) {
   case 1:
      dati->val_crit = new_int(_("Livello minimo per votare: "));
      break;
   case 2:
      printf(_("Possono votare gli utenti che si sono registrati prima del:\n"));
      new_date(&tmst, -1);
      dati->anzianita = mktime(&tmst);
      printf(_(" (Votano gli utenti registrati prima del "));
      stampa_data(dati->anzianita);
      printf(_(")\n"));
      break;
   case 3:
      dati->val_crit = new_int(_("Numero minimo di chiamate: "));
      break;
   case 4:
      dati->val_crit = new_int(_("Numero minimo di post: "));
      break;
   case 5:
      dati->val_crit = new_int(_("Numero minimo di eXpress messages: "));
      break;
   case 6:
      printf(_("Sesso (m/f): "));
      c = 0;
      while(c != _('m') && c != _('f'))
         c = tolower(inkey_sc(0));
      if(c == _('m')) {
         printf(_("Maschi.\n"));
         dati->val_crit = 1;
      } else {
         printf(_("Femmine.\n"));
         dati->val_crit = 0;
      }
      break;
   }
   return 0;
}
