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
* File : urna-vota.c                                                        * 
*     funzioni per votare                                                   *
* Sat Sep 20 23:25:04 CEST 2003                                             *
****************************************************************************/
#define USE_REFERENDUM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ansicol.h"
#include "cittaclient.h"
#include "cml.h"
#include "conn.h"
#include "extract.h"
#include "inkey.h"
#include "parm.h"
#include "text.h"
#include "urna-gestione.h"
#include "urna-servizio.h"
#include "urna-strutture.h"
#include "urna-vota.h"
#include "urna_comune.h"
#include "urna_errori.h"
#include "utility.h"
#include "memkeep.h"

void urna_vota(void);
void send_proposte(char * testa_proposte[]);
void stampa_proposte(struct urna_client *dati);
char vota_si_no(struct urna_client *dati, char *risposta);
char vota_singola(struct urna_client *dati, char *risposta);
char vota_multipla(struct urna_client *dati, char *risposta);
char vota_votazione(struct urna_client *dati, char *risposta);
char escidavvero();
/* */

void urna_vota(void)
{
   struct elenco_ref *elenco[MAX_URNE];
   char scelte_finali[] = "*************";
   int n_rs, scelto,i,num;
   char  res;
   char str_serv[2 * (MAX_VOCI + 1) + 10]; /* ogni voce|+inizio (SREN numero) */
   char risposte[MAX_VOCI + 7];
   char basta;
   char buf[LBUF];
   char votato[2];
   struct urna_client dati;

   azzera_dati(&dati);

   if((n_rs = get_elenco_ref(elenco,6)) == -1) {
      return;
   }

   if((n_rs = get_elenco_ref(elenco,6)) == 0) {
      printf(_("Non ci sono referendum o sondaggi a cui puoi votare.\n"));
      return;
   }

   scelto = scegli_ref(n_rs, elenco,"votare","#lTrvV");

   if(scelto  == -1) {
      cml_printf(_("Ciao, alla prossima!\n"));
      free_elenco_ref(elenco);
     return;
   };

   num=elenco[scelto]->num;
   free_elenco_ref(elenco);


   if(get_ref(&dati, num, "",1) == -1) {
      cml_printf(_("il sondaggio non esiste\n"));
      return;
   }

   serv_putf("SRIS %d",num);

  serv_gets(buf);

   if(buf[0]!='2'){
		   free_dati(&dati);
		   return;
   };

   par2strd(votato,&dati,"havotato",1);

   if(strcmp(votato,"1")==0){
     free_dati(&dati);
     cml_printf(_("hai gi&agrave; votato"));
     return;
   }

   par2strd(dati.titolo, &dati, "titolo", LEN_TITOLO);

   dati.testo = txt_create();
   urna_prendi_testo(&dati);

   if(urna_prendi_voci(&dati)) {
     free_dati(&dati);
      return;
   };

   printf(_("\n%sQuesito:%s\n"), hbold, hstop);
   stampa_quesito(dati.testo);

   if(dati.modo != MODO_PROPOSTA) {
      stampa_voci(&dati);
   } else {
      stampa_proposte(&dati);
   };

   /* 
    * res = s esci subito
    * res = n chiedi cosa fare ( rifare / abbandonare)
    * res= \0  numero di scelte fatte
    *
    * in risposte[0]='=' -> bianca
    */

   do {                         /*finito di rispondere */
      printf("\n");
      /*stampa_voci(&dati);*/
      switch (dati.modo) {
      case MODO_SI_NO:
         res = vota_si_no(&dati, risposte);
         break;
      case MODO_SCELTA_SINGOLA:
         res = vota_singola(&dati, risposte);
         break;
      case MODO_SCELTA_MULTIPLA:
         res = vota_multipla(&dati, risposte);
         break;
      case MODO_VOTAZIONE:
         res = vota_votazione(&dati, risposte);
         break;
      case MODO_PROPOSTA:
         res = chiedi_scelte(dati.testa_proposte, 0,
                    dati.max_len_prop,dati.max_prop, "proposte");
         break;
      }

      if(res == 's') {
     free_dati(&dati);
     serv_putf("SREN -1");
  		serv_gets(buf);
     if(buf[0]!='2')
		   cml_printf(_("Qualche errore!\n"));
		   else
		   cml_printf(_("Ripensaci!\n"));

         return;
      }

      if(res == 'n')
         strncpy(scelte_finali, "aArR", 4);

      cml_printf(_("\nPremi '<b>a</b>' per abbandonare,")
                 _(" '<b>r</b>' per rifare"));

      if(res != 'n') {
         cml_printf(_(" o '<b>s</b>' per spedire"));
         strncpy(scelte_finali, "aArRsS", 6);
      }

      printf(": ");
      basta = inkey_elenco(scelte_finali);

   } while((basta != 'a') && (basta != 's'));

   if(basta == 'a') {
     free_dati(&dati);
     serv_putf("SREN -1");
  		serv_gets(buf);
     if(buf[0]!='2')
		   cml_printf(_("Qualche errore!"));
		   else
		   cml_printf(_("Ripensaci!"));
      return;
   }

   sprintf(str_serv, "SREN %d", num);

   for(i = 0; i < dati.num_voci; i++) {
      strncat(str_serv, "|", 1);
      strncat(str_serv, risposte + i, 1);
   }
#if 0
	serv_puts(str_serv);

   serv_gets(buf);
   if(strncmp(buf, "200",3)) {
      putchar('\n');
      stampa_errore(buf);
      putchar('\n');
     free_dati(&dati);
      return;
   }

   /*
    * check su invia paarm?
    */
#endif
   send_proposte(dati.testa_proposte);
   serv_puts(str_serv);
   serv_gets(buf);

   if(buf[0] == '2')
      cml_print(_("\nOk, il tuo voto &egrave arrivato, grazie per il voto!\n"));
   else {
      putchar('\n');
      stampa_errore(buf);
      putchar('\n');

   }
   free_dati(&dati);
   return;
};

void send_proposte(char * testa_proposte[])
{

   int i;
   
   for(i=0; i<MAX_VOCI;i++){
         if(testa_proposte[i]==NULL)
                     continue;
      serv_putf("PARM prop|%s", testa_proposte[i]);
   }
};


void stampa_proposte(struct urna_client *dati)
{
   printf(_("\n%sDevi proporre %d soluzioni.%s \n"), hbold, dati->max_prop,
          hstop);
   printf(_("\n%sLunghe al massimo %d caratteri%s\n"), hbold, dati->max_len_prop,
          hstop);
}

/*
 * torna 0 se si ha votato
 *
 */

char vota_si_no(struct urna_client *dati, char *risposta)
{

   char elenco_scelte[8];
   char ctrlx[] = { Ctrl('X'), 0 };

   char c;

   strcpy(elenco_scelte, "sSnN");
   strcat(elenco_scelte, ctrlx);

   cml_printf(_("\nRispondi s&iacute; o no (<b>s</b>/<b>n</b>"));

   if(dati->bianca) {
      cml_printf(_(" - Per votare scheda bianca premi <b>=</b>"));
      strcat(elenco_scelte, "=");
   }

   printf(_(")? "));
   c = inkey_elenco(elenco_scelte);

   if(c == Ctrl('X'))
      return escidavvero();

   if(c == 's') {
      risposta[CODIFICA_NO] = '0';
      risposta[CODIFICA_SI] = '1';
      cml_printf(_("S&iacute;.\n"));
   }

   if(c == 'n') {
      risposta[CODIFICA_NO] = '1';
      risposta[CODIFICA_SI] = '0';
      printf(_("No.\n"));
   }

   if(c == '=') {
      strcpy(risposta, "A");
      printf(_("Scheda Bianca\n"));
      return 0;
   }

   return 1;
}

/*
 * torna 0 se si ha votato
 * s se finito
 * n se rifai
 */

char vota_singola(struct urna_client *dati, char *risposta)
{
   int i;
   char elenco_scelte[2 * MAX_VOCI + 6];
   char ctrlx[] = { Ctrl('X'), 0 };
   char c;

   for(i = 0; i < dati->num_voci; i++)
      risposta[i] = '0';

   cml_printf(_("\nQuale &egrave la tua preferenza (a-%c)? "),
              'a' + dati->num_voci - 1);

   crea_elenco_base(elenco_scelte, dati->num_voci - 1, 2 * MAX_VOCI + 6);

   strcat(elenco_scelte, ctrlx);

   if(dati->bianca) {
      cml_printf(_("( Per votare scheda bianca premi <b>=</b> )"));
      strcat(elenco_scelte, "=");
   }

   c = inkey_elenco(elenco_scelte);

   if(c == Ctrl('X')) {
      return escidavvero();
   };

   printf("%s%c%s\n", hbold, c, hstop);
   if(c != '=') {
      printf(_("Risposta %c\n"), c);
   } else {
      printf(_("\nScheda bianca"));
      risposta[0] = '=';
   }
   if(c != '=')
      risposta[c - 'a'] = '1';

   return 0;
}

/*
 * torna 0 sempre
 */

char vota_multipla(struct urna_client *dati, char *risposta)
{

   char ctrlx[] = { Ctrl('X'), 0 };
   char elenco_scelte[2 * MAX_VOCI + 6];
   char c;
   int scelte_date = 0;
   int i,bianca=0;


   cml_printf
     (_("\nScegli le  tue preferenza (a-%c), max %d "),
      'a' + dati->num_voci - 1, dati->max_voci);
   cml_printf(_("\nRipetendo annulli la scelta"));
   crea_elenco_base(elenco_scelte, dati->num_voci - 1, 2 * MAX_VOCI + 6);

   strcat(elenco_scelte, "!");
   strcat(elenco_scelte, ctrlx);

   cml_printf(_("\nCon <b>Ctrl-X</b> o <b>!</b> finisci"));

   if(dati->bianca) {
      cml_printf(_(", per votare scheda bianca premi <b>=</b> "));
      strcat(elenco_scelte, "=");
   }

   printf(_("\n"));

   do {                         /*fine scelte */
      c = inkey_elenco(elenco_scelte);
      if (c==Ctrl('X')){
           return 0;
     };

      printf("%s%c%s:", hbold, c, hstop);
      fflush(stdout);
      if(c == '=') {
         if(bianca) {
            printf(_("Scheda bianca tolta\n"));
            bianca = 0;
         } else
            bianca = -1;
      }
      if(c != '=' && c != '0' && c != '!') {
         if(risposta[c - 'a'] == '1') {
            scelte_date--;
            risposta[c - 'a'] = '0';
         } else {
            scelte_date++;
            risposta[c - 'a'] = '1';
         }
      }
      if(bianca) {
         cml_printf(_("Stai votando scheda bianca:\n")
                    _("   premi <b>=</b> per toglierla")
				   );
      }
      for(i = 0; i < dati->num_voci; i++) {
         printf(" (%c) %s  ", 'a' + i, *(dati->testa_voci + i));
         spaces(dati->max_len - dati->len[i] + 1);
         if(risposta[i] == '1')
            printf(_("scelto"));
         printf("\n");
      }
      cml_printf(_("\nCon <b>!</b> finisci.\n"));
      if((scelte_date > dati->max_voci) && !bianca) {
         printf(_("Stai scegliendo troppe voci!\n"));
         c = 0;
      };
      printf("\n");
   } while(c != '!' && c != Ctrl('X'));
   return 0;
}

char vota_votazione(struct urna_client *dati, char *risposta)
{

   char ctrlx[] = { Ctrl('X'), 0 };
   int i;
   /* int scelte_date; */
   char elenco_scelte[2 * MAX_VOCI + 6];
   char elenco_voti[]="0123456789999"; /* i 9 in piu` servono per ! = */
   char c;
   char v;
   int bianca=0;

   /* scelte_date = dati->num_voci; */
   cml_printf(_("\nVota per i candidati (a-%c):\n "), 'a' + dati->num_voci - 1);
   cml_printf(_(" scegli la lettera da votare "));
   crea_elenco_base(elenco_scelte, dati->num_voci - 1, 2 * MAX_VOCI + 6);
   strcat(elenco_scelte, "!");
   strcat(elenco_scelte, ctrlx);

   for(i = 0; i < dati->num_voci; i++)
      risposta[i] = '0';

   if(dati->bianca) {
      cml_printf(_(" (<b>=</b> per votare scheda bianca)"));
      strcat(elenco_scelte, "=");
   }
   cml_printf(_(",\n e poi vota da <b>0</b>-<b>9</b>"));
   if(dati->ast_voto) {
      cml_printf(_(" (<b>=</b> per astenerti),"));
      strcat(elenco_voti, "=");
      for(i = 0; i < dati->num_voci; i++)
         risposta[i] = '=';
   }
   cml_printf(_("\ncon <b>^X</b> o <b>!</b> finisci\n"));
   do {                         /*fine scelte */

      c = inkey_elenco(elenco_scelte);

      if(c == Ctrl('X') || c == '!')
         return 0;

      if(c != 13 && c!='=') {
         printf("%s%c%s:%c", hbold, c, hstop, risposta[c - 'a']);
         fflush(stdout);
         v = inkey_elenco(elenco_voti);
         printf("%s%c%s\n", hbold, v, hstop);
         fflush(stdout);
         risposta[c - 'a'] = v;
      }

      if(c == '=') {
         if(bianca) {
            printf(_("scheda bianca tolta\n"));
            bianca = 0;
         } else
            bianca = -1;
      }
      if(bianca)
         cml_printf(_("\n<fg=3;b>Stai votando scheda bianca:")
                    _(" premi = per toglierla.<fg=7;/b>"));
      cml_printf(_("\nCon <b>!</b> finisci"));
      printf("\n");
      for(i = 0; i < dati->num_voci; i++) {
         cml_printf(" <fg=1>(%c)<fg=7><b> %s </b> ", 'a' + i, *(dati->testa_voci + i));
         spaces(dati->max_len - dati->len[i] + 1);
         if(risposta[i] == '=')
            printf("=");
         else
            printf("%c", risposta[i]);
         printf("\n");
      }
      printf("\n");
   } while(1);
}


char escidavvero(){
   char c;
      cml_printf(_("Vuoi interrompere il voto? (s/n)"));
      c = inkey_elenco("sSnN");
     return c;
}
