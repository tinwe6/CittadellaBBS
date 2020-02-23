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
* Cittadella/UX client                  (C) M. Caldarelli and R. Vianello   *
*                                                                           *
* File : urna-servizio.c                                                    *
* Sat Sep 20 23:19:16 CEST 2003                                             *
****************************************************************************/

#define USE_REFERENDUM 1
#include "cittaclient.h"
#include "memkeep.h"
#include <time.h>
#include "urna-strutture.h"
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
#include "ansicol.h"
#include "parm.h"
#include "urna-gestione.h"
#include "urna-servizio.h"
#include "cittacfg.h"                                                         

/****************************************************************************/

int chiedi_scelte(char *risposte[], int min_scelte, int maxlen,
                  int max_scelte, char *prompt);
int get_elenco_ref(struct elenco_ref *elenco[], char tipo);
int new_str_m_abo(char *prompt, char *str, int max);
int scegli_ref(int n_rs, struct elenco_ref *elenco[], char *richiesta, const char *format);
int trim(char *str);
int urna_prendi_voci(struct urna_client *dati);
long get_ref(struct urna_client *dati, long num, char *ask, char vota);
void azzera_dati(struct urna_client *dati);
void crea_elenco_base(char str[], int len, int max);
void debugga_dati(struct urna_client *u, int n);
void free_dati(struct urna_client *dati);
void free_elenco_ref(struct elenco_ref *elenco[]);
void print_ref(int n_rs, struct elenco_ref *elenco[], const char *format);
void spaces(int n);
void stampa_motivo(int motivo);
void stampa_quesito(struct text *quesito);
void stampa_voci(const struct urna_client *dati);
void urna_prendi_testo(struct urna_client *dati);
void ordina_ref(int n_rs,struct elenco_ref *elenco[], char *format);
static int cmp_ref(const void *a, const void *b);
int print_sondaggio(struct elenco_ref *ref, const char *format, int maxtit);

/*
 * crea l'elenco base delle scelte
 * ovvero da 
 * a -- lettera
 * A -- LETTERA
 */

void crea_elenco_base(char str[], int len, int max)
{
   int i;

   for(i = 0; i <= len && i < max - 1; i++) {
      str[2 * i] = 'a' + i;
      str[2 * i + 1] = 'A' + i;
   }
   for(i = 2 * len + 2; i < max - 1; i++) {
      str[i] = 0;
   }
}

/*
 * legge il testo
 * da parm
 */

void urna_prendi_testo(struct urna_client *dati)
{
   int i, err;
   char *temp;

   temp = (char *)myCalloc(LEN_TESTO + 1, sizeof(char));

   txt_rewind(dati->testo);

   for(i = 0; i < MAXLEN_QUESITO; i++) {
      if((err = par2strd(temp, dati, "testo", LEN_TESTO)))
         printf(_("errori prendendo il testo %d %s"), err, temp);

      txt_put(dati->testo, temp);
   }

   myFree(temp);
   return;
}

/*
 * legge le voci da parm
 * da parm
 */

int urna_prendi_voci(struct urna_client *dati)
{
   int i, max_len, len;
   char id[9];

   for(i = 0; i < dati->num_voci; i++) {
      sprintf(id, "voce-%d", i);
      dati->testa_voci[i] =
        (char *)myCalloc(MAXLEN_VOCE, sizeof(char));
      if(par2strd(dati->testa_voci[i], dati, id, MAXLEN_VOCE - 1))
         printf(_("errori scaricando voce %i"), i);
   }

   max_len = 0;
   for(i = 0; i < dati->num_voci; i++) {
      len = strlen(dati->testa_voci[i]);
      if(len > max_len)
         max_len = len;
      dati->len[i] = len;
   }
   dati->max_len = max_len;
   return (0);
}

/*
 * Legge il sondaggio #n
 * Se n=0 chiede il numero all'utente.
 * Restituisce il numero di risposte possibili, -1 se non lo trova.
 * chiede anche i parm (che quindi vanno liberati dal chiamante)
 * se vota=1 manda al server l'indicazione che si sta votando
 * il referendum (da fare)
 */

long get_ref(struct urna_client *dati, long num, char *ask, char vota)
{
   char buffer[LBUF];

   if(num == 0) {
      num = new_long(ask);
      if(num == 0)
         return (-1);
   }

   serv_putf("SINF %ld|%c", num, vota);
   serv_gets(buffer);
   if(buffer[0] != '2')
      return (-1);

   dati->num = extract_int(buffer + 4, POS_NUM);
   dati->tipo = extract_int(buffer + 4, POS_TIPO);
   extractn(dati->proponente, buffer + 4, POS_PROPONENTE, MAXLEN_UTNAME);
   extractn(dati->room, buffer + 4, POS_NOME_ROOM, ROOMNAMELEN);
   dati->modo = extract_int(buffer + 4, POS_MODO);
   dati->bianca = extract_int(buffer + 4, POS_BIANCA);
   dati->ast_voto = extract_int(buffer + 4, POS_ASTENSIONE_VOTO);
   dati->num_voci = extract_int(buffer + 4, POS_NUM_VOCI);
   dati->max_voci = extract_int(buffer + 4, POS_MAX_VOCI);
   dati->criterio = extract_int(buffer + 4, POS_CRITERIO);
   dati->val_crit = extract_int(buffer + 4, POS_VAL_CRITERIO);
   dati->anzianita = extract_int(buffer + 4, POS_ANZIANITA);
   dati->start = extract_long(buffer + 4, POS_START);
   dati->stop = extract_long(buffer + 4, POS_STOP);
   dati->max_prop = extract_int(buffer + 4, POS_NUM_PROP);
   dati->max_len_prop = extract_int(buffer + 4, POS_MAXLEN_PROP);
   extractn(dati->lettera, buffer + 4, POS_LETTERA, 3);

   get_parm(dati);
   return (num);
}

void stampa_quesito(struct text *quesito)
{
   char *buf;

   txt_rewind(quesito);
   while((buf = txt_get(quesito)))
		   if (buf && strcmp(buf,""))
			      printf(_("%s\n"), buf);
}

void stampa_voci(const struct urna_client *dati)
{
   int i;

   switch (dati->modo) {
   case MODO_SI_NO:
      printf(_("[S]i\n[N]o\n"));
      break;
   case MODO_SCELTA_SINGOLA:
   case MODO_SCELTA_MULTIPLA:
   case MODO_VOTAZIONE:
      for(i = 0; i < dati->num_voci; i++)
         printf(_(" (%c) %s\n"), 'a' + i, *(dati->testa_voci + i));
      break;
   case MODO_PROPOSTA:
      break;
   }
}

void spaces(int n)
{
   int i;

   if(n <= 0 || n > 40)
      return;
   for(i = 0; i < n; i++)
      printf(_(" "));
}

void stampa_motivo(int motivo)
{
   switch (motivo) {
   case POCHEVOCI:
      printf(_("hai definito poche voci\n"));
      break;
   case TROPPEVOCI:
      printf(_("hai definito troppe voci\n"));
      break;
   default:
      printf(_("Non so, chiedi al Sysop\n"));
   }
}

/*
 * fa vedere i dati
 * n=0 tutto
 * n=1 dati +testo
 * n=2 dati+testo+voci (se non c'e` testo non ci sono voci...)
 */

void debugga_dati(struct urna_client *u, int n)
{
   int i;
   char *tq;

   printf(_("\n"));
   printf(_("NUM%d"), u->num);
   printf(_("PROPONENTE  %s\n"), u->proponente);
   printf(_("NOME_ROOM  %s\n"), u->room);
   printf(_("TIPO  %d\n"), u->tipo);
   printf(_("MODO  %d\n"), u->modo);
   printf(_("TITOLO  %s\n"), u->titolo);
   printf(_("CRITERIO  %d\n"), u->criterio);
   printf(_("VAL_CRITERIO  %ld\n"), u->val_crit);
   printf(_("START  %ld\n"), u->start);
   printf(_("STOP  %ld\n"), u->stop);
   printf(_("NUM_VOCI  %d\n"), u->num_voci);
   printf(_("MAX_VOCI  %d\n"), u->max_voci);
   printf(_("TESTA VOCI  %p\n"), (void *)u->testa_voci);
   printf(_("ANZIANITA  %ld\n"), u->anzianita);
   printf(_("BIANCA  %d\n"), u->bianca);
   printf(_("ASTENSIONE_VOTO  %d\n"), u->ast_voto);
   if(n == 1) {
      printf(_("QUESITO:\n"));
      i = 0;
      txt_rewind(u->testo);
      tq = txt_get(u->testo);
      while(tq && i < MAXLEN_QUESITO) {
         printf(_("\t%s"), tq);
         tq = txt_get(u->testo);
         i++;
      }
      printf(_("\n"));
   }
   if(n >= 1) {
      printf(_("VOCI:\n"));
      while(tq) {
         printf(_("\t%s"), tq);
         tq = txt_get(u->testo);
      }
   }
}

/*
 * inserisce in **risposte
 * le voci/proposte
 * calloca *risposte (ma non risposte!)
 * return 
 * -1 abort
 *  num risp altrimenti
 */

int chiedi_scelte(char *risposte[], int min_scelte, int maxlen,
                  int max_scelte, char *prompt)
{

   int risp, i, j, pos, risposta;
   char pr[maxlen];
   char default_resp[maxlen+2];
   char c;

   printf(_("\nInserisci le %s (%d-%d),")
          _(" e finisci con invio.\n"), prompt,
          min_scelte, max_scelte);

#if 0
   risp = new_int("");

   while((risp < min_scelte || risp > max_scelte) && risp != 0)
      risp = new_int("  Risposta non valida. Numero di scelte? ");

   printf("\nInserire le %s: (invio per finire)\n", prompt);
#endif
	i=0;
    while(i < max_scelte){
      risposte[i] = (char *)myCalloc(maxlen+2, sizeof(char));
      strcpy(risposte[i], "");
      sprintf(pr, " (%c) ", 'a' + i);
      /* new_str_m(pr, risposte[i], maxlen - 1); */
      if(new_str_m_abo(pr, risposte[i], maxlen) == -1) {
         break;
      }
      if(strlen(risposte[i]) == 0){
			  if (i>=min_scelte)
					  break;
			  printf(_("\nnon bastano\n"));
	  } else
			i++; 
   }

   risp = i;

   /* just to be sure.. */
   c = 'a';

   while(((c != '0') && (c != 10)) || risp < min_scelte) {
      if(risp >= min_scelte) {
         cml_printf(_(" (<b>invio</b> per finire"));
      } else {
      		cml_printf(_("\nnon bastano..."));
	}
      if(risp != 0) {
         cml_printf(_(", lettera <b>a</b>-<b>%c</b> per cambiare il testo"),
		    'a' + risp - 1);
      }

      if(risp < max_scelte) {
         cml_printf(_(",\n  lettera <b>%c</b> per aggiungerne un'altra"),
		    'a' + risp);
      }
      printf("): ");


      c = 'a' + max_scelte + 1;

      while((c != '0') && (c != 10)
            && ((c < 'a') || (c > ('a' + risp)) || c > ('a' + max_scelte)))
         c = inkey_sc(0);

      printf(_("%c\n"), c);

      if(c != '0' && c != 10) {
         pos = c - 'a';
         if(pos == risp && risp < max_scelte) {
            risp++;

            if(risposte[pos] == NULL) {
               risposte[pos] =
                 (char *)myCalloc(maxlen+2, sizeof(char));
            }
         }
	if(pos+1==risp)
         printf(_("\nAggiungi la %s: (^X per cancellarla)\n"), prompt);
	 else
         printf(_("\nCorreggi la %s: (^X per cancellarla)\n"), prompt);
		
         strcpy(default_resp, risposte[pos]);

         /* 
          * non uso new_str_def_m perche'
          * non ritorna l'Abort 
          */

         cml_printf(_(" (%c) [%s]: "), c, default_resp);
         risposta = c_getline(default_resp, maxlen, false, false);

         if (default_resp[0] != '\0') {
            strncpy(risposte[pos], default_resp, maxlen);
         }
         /*
          * * definizione in .h???
          * *  if(risposta== GETLINE_ABORT)
          */

         if(risposta == -1)
            strcpy(risposte[pos], "");

         c = 'a' + risp + 2;
      }

      /*
       * rinormalizza e stampa
       */

      printf(_("\nRisposte:\n"));
      j = 0;
      for(i = 0; i < risp; i++) {
         if(strlen(risposte[i]) == 0)
            continue;
		 if(i!=j)
                strncpy(risposte[j], risposte[i], maxlen);
         printf(" (%c) %s\n", 'a' + j, risposte[j]);
         j++;
      }
      risp = j;
   }
   return risp;
}

/*
 * libera la struttura dati
 * ma non il puntatore
 */

void free_dati(struct urna_client *dati)
{

   int i;

   for(i = 0; i < MAX_VOCI; i++) {
      if(dati->testa_voci[i] != NULL)
         myFree(dati->testa_voci[i]);
   }

   for(i = 0; i < MAX_VOCI; i++) {
      if(dati->testa_proposte[i] != NULL)
         myFree(dati->testa_proposte[i]);
   }

   if(dati->parm != NULL)
      parm_free(dati->parm);

   if(dati->testo != NULL) {
      txt_rewind(dati->testo);
      txt_free(&dati->testo);
   }
}

void azzera_dati(struct urna_client *dati)
{

   int i;

   for(i = 0; i < MAX_VOCI; i++) {
      dati->testa_voci[i] = NULL;
      dati->testa_proposte[i] = NULL;
   }

   dati->parm = NULL;

   dati->testo = NULL;
   strcpy(dati->titolo, "");
}

int new_str_m_abo(char *prompt, char *str, int max)
{
   strcpy(str, "");
   cml_printf("%s", prompt);
   return c_getline(str, max, false, false);
}

void free_elenco_ref(struct elenco_ref *elenco[])
{
   int i;

   for(i = 0; i < MAX_URNE; i++) {
      if(elenco[i] != NULL) {
         myFree(elenco[i]);
      }
   }
   return;
}

/* 
 * prende l'elenco dei sondaggi
 * di tipo "tipo"lo mette nelle strutture elenco
 * ritorno: il numero di referendum trovati
 *
 *invia la lista
 *dei referendum
 *ora fa anche le funzioni di SCVL
 * 
 * SLST n|t 
 *
 * n=0 manda i ref delle stanze non zappate (--vedi livello)
 * n=1 manda tutti i ref/urne (--vedi livello)
 * n=2 manda i ref votabili delle stanza non zappate, indica se
 *     sono da votare
 * n=4 manda tutti i ref/sond nelle room _NON_ zappate creati
 *     dall'utente. 
 *
 * n=12 manda tutti i ref/sondaggi nelle room _NON_ zappate
 *     modificabili dall'utente
 *
 * ----il codice si basa su 2%2=0%2=0   
 *
 * Restituisce 
 * numero, tipo, titolo, modo, termine, lettera, stanza, zappata,
 * votabile, votato, posticipabile, proponente
 *
 * se n=0,1,4: votabile, votato sono non prevedibili
 * se n=2 o n=4 manda solo i ref nelle  stanze non zappate
 * (ovvero non c'e` modo di sapere se ho votato se non 
 * unzappo la room!)
 *
 * Da aggiungere -- struttura per le lunghezze massime varie
 */

int get_elenco_ref(struct elenco_ref *elenco[], char tipo)
{

   int i, n_rs = 0;
   char buf[LBUF];

   serv_putf("SLST %d", tipo);
   serv_gets(buf);
   if(buf[0]!='2'){
	stampa_errore(buf);
	return -1;
   }


   for(i = 0; i < MAX_URNE; i++)
      elenco[i] = NULL;

   while(serv_gets(buf), strcmp(buf, "000")) {
      elenco[n_rs] =
        (struct elenco_ref *)myCalloc(sizeof(struct elenco_ref), 1);
      elenco[n_rs]->num = extract_int(buf + 4, 0);
      elenco[n_rs]->tipo = extract_int(buf + 4, 1);
      extractn(elenco[n_rs]->titolo, buf + 4, 2, LEN_TITOLO);
      elenco[n_rs]->modo = extract_int(buf + 4, 3);
      elenco[n_rs]->stop = extract_long(buf + 4, 4);
      extractn(elenco[n_rs]->lettera, buf + 4, 5, LEN_TITOLO);
      extractn(elenco[n_rs]->room, buf + 4, 6, LEN_TITOLO);
      elenco[n_rs]->zap = extract_int(buf + 4, 7);
      elenco[n_rs]->votabile = extract_int(buf + 4, 8);
      elenco[n_rs]->votato = extract_int(buf + 4, 9);
      elenco[n_rs]->posticipabile = extract_int(buf + 4, 10);
      n_rs++;
      if(n_rs > MAX_URNE) {
         printf(_("raggiunto il massimo?"));
         break;
      }
   }
   return n_rs;
}

/* 
 * Sceglie un referendum tra quelli di elenco_voti
 * torna la posizione in elenco_ref,
 * -1 se non sono stati scelti
 */

int scegli_ref(int n_rs, struct elenco_ref *elenco[], char *richiesta, const char *format)
{
  char prompt[81];
  char str[4]="";
  int num, i=0;
	
   
  if(n_rs == 0) {
    return -1;
  }

  if(n_rs == 1) {
    print_ref(n_rs, elenco,format);
    cml_printf(_("\nC'&egrave; solo il %s #%d da %s\nVa bene?"),
	       NOME_TIPO(elenco[0]->tipo), elenco[0]->num, richiesta);
    if (new_si_no_def("", true)) {
      return 0;
    } else {
      return -1;
    }
  }

  strncpy(prompt,("\nNumero o lettera della consultazione," 
		  " (invio per un elenco)"), 80);
  new_str_m(prompt, str, 4);
  if(strcmp(str, "") == 0) {
    print_ref(n_rs, elenco,format);
    printf("\n");
    strcpy(prompt, _("\nNumero o lettera della consultazione, invio per uscire: "));
    new_str_m(prompt, str, 3);
  }
  if(strcmp(str, "") == 0) {
    return -1;
  }

  num = trim(str);
  while((i < n_rs) && (num != elenco[i]->num) &&
	strcmp(str, elenco[i]->lettera) != 0) {
    i++;
  }
  
  if(i == n_rs) {
    printf(_("Non esiste (o non lo puoi votare), peccato!\n"));
    return -1;
  }
  return i;
}

/*
 * stampa i referendum
 * in elenco
 * da migliorare l'elenco delle cose da stampare
 *
 *
 * string
 * # numero--
 * l lettera--
 * t tipo--
 * m modo--
 * T titolo--
 * s stop
 * r room--
 * z zap
 * v puo` votare
 * V ha votato
 * p posticipabile 
 */


void ordina_ref(int n_rs, struct elenco_ref *elenco[], char *format)
{
        IGNORE_UNUSED_PARAMETER(format);

        qsort(elenco, n_rs, sizeof(struct elenco_ref *), cmp_ref);
}

static int cmp_ref(const void *a, const void *b){
        struct elenco_ref *ap;
	struct elenco_ref *bp;

	ap = *(struct elenco_ref **)a;
	bp = *(struct elenco_ref **)b;
	if (ap->tipo == TIPO_REFERENDUM && bp->tipo == TIPO_SONDAGGIO) {
	        return -1;
	}
	if(ap->tipo == TIPO_SONDAGGIO && bp->tipo == TIPO_REFERENDUM) {
	        return +1;
	}

	if (ap->num < bp->num) {
	        return -1;
	}
	if (ap->num > bp->num) {
	        return +1;
	}
	return 0;
}

void print_ref(int n_rs, struct elenco_ref *elenco[], const char *format)
{
        struct elenco_ref *ref;
	size_t maxtit;
	int i, linee;

	maxtit = 0;
	linee = 0;

	setcolor(C_URNA);
	if (index(format,'T')) {
	        for (i = 0; i < n_rs; i++) {
		        ref = elenco[i];
			if (strlen(ref->titolo) > maxtit) {
			        maxtit = strlen(ref->titolo);
			}
		}
	}

	for(i = 0; i < n_rs; i++) {
	        printf("\n");
		linee += print_sondaggio(elenco[i], format, maxtit);
		if (linee > NRIGHE-2) {
		        hit_any_key();
			linee = 0;
		}
	}
	return;
}

/*
 * secondo tentativo di formattare
 * %s stringa, chiusa con %
 * %n acapo
 * altrimenti %, e prosegue
 * \n in una stringa mette \n, e non a capo
 * %tnn tabba alla linea tab (sempre 2 cifre, per ora)
 */

int print_sondaggio(struct elenco_ref *ref, const char *format, int maxtit)
{
   int linee;
   const char *s;
   char nums[3] = "";
   int pos = 0;
   int cs = 0;

   s = format;
   linee = 1;
   while(*s) {
      switch (*s) {
      case 't':
	 setcolor(C_URNA);
         if(ref->tipo)
            cs += cml_printf(_("Referendum"));
         else
            cs += cml_printf(_("Sondaggio"));
         break;
      case 'l':
         setcolor(C_URNA_LETTERA);
         cs += cml_printf("%s ", ref->lettera);
         break;
      case '#':
         setcolor(C_URNA_NUMERO);
         cs += cml_printf("(#%ld)", ref->num);
         break;
      case 'm':
         setcolor(C_URNA);
         cs += cml_printf(_("  Metodo: <b>"));
         switch (ref->modo) {
         case MODO_SI_NO:
            cs += cml_printf(_("S&iacute/No "));
            break;
         case MODO_SCELTA_SINGOLA:
         case MODO_SCELTA_MULTIPLA:
            cs += cml_printf(_("Scelta "));
            break;
         case MODO_VOTAZIONE:
            cs += cml_printf(_("Votazione "));
            break;
         case MODO_PROPOSTA:
            cs += cml_printf(_("Proposta "));
            break;
         }
         cml_printf("</b>");
         break;
      case 'T':
         setcolor(C_URNA_TITOLO);
         /*cml_printf("%-51s\t", ref->titolo);
          */
         cs += cml_printf(" %*s", -maxtit, ref->titolo);
         break;
      case 'r':
         setcolor(C_URNA_ROOM);
         cs += cml_printf(_(" in %s"), ref->room);
         break;
      case 'z':
         if(ref->zap)
            setcolor(C_URNA_ZAPPATA);
         cs += cml_printf(_(" in %s"), ref->room);
         break;
      case 's':
         setcolor(C_URNA);
         cs+=stampa_data(ref->stop);
         break;
      case 'v':
         setcolor(C_URNA_VOTA);
         if(ref->votabile == 0)
            cs += cml_printf(_(" <b>non puoi votarlo</b>"));
         break;
      case 'V':
         setcolor(C_URNA_VOTATA);
         if(ref->votato == 1)
            cs += cml_printf(_(" <b>hai gi&agrave; votato</b>"));
         else
            cs += cml_printf(_(" <b>puoi ancora votarlo</b>"));
         break;
      case ' ':
         printf(" ");
         cs++;
         break;
      case '%':
         if(*(s + 1) == 'n') {
            s++;
            cml_printf("\n");
            linee++;
            cs = 0;
            break;
         }
         if(*(s + 1) == 't' && *(s + 2) && *(s + 3)) {
            strncpy(nums, s + 2, 2);
            pos = atoi(nums);
            if(pos < cs) {
               printf("\n");
               cs = 0;
               linee++;
            }
            cs += printf("%*s", pos - cs, " ");
            s += 3;
            break;
         }
         if(*(s + 1) == 's') {
            s += 2;
            while(*s && *s != '%') {
               cs += printf("%c", *s);
               s++;
            }
            break;
         }
         cs += cml_printf("%%");
         break;
      default:
         break;
      }
      if(*s)
         s++;
   }
   return linee;
}
/*
 * cancella gli spazi all'inizio e alla fine di str
 * e num=strtol str
 */

int trim(char *str)
{

   char *str1, *str2;

   /*
    * str2 punta al primo carattere non spazio
    */

   if (str[0]==0){
		   str[0]='0';
		   str[1]=0;
   }
   for(str2 = str; (*str2 == ' ' && *str2 != 0); str2++);

   /*
    * copia fino alla fine
    */
   str1 = str;
   while(*str2 != 0)
      *(str1++) = *(str2++);
   /*
    * mette un \0 in fondo
    */

   *str2 = 0;

   /*
    * e ora torna indietro...
    */

   str2--;

   while(str2 != str && *str2 == ' ')
      *(str2--) = 0;

   return strtol(str, NULL, 10);
}
