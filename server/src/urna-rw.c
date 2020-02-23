/*  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File: urna-rw.c                                                           *
*       Gestione salvataggi/lettura urna                                    *
****************************************************************************/

/* Wed Nov 12 23:20:27 CET 2003 -- piu o meno */

#include "config.h"
#include "urna-file.h"
#include "urna-strutture.h"
#include "utility.h"
#include "urna-cost.h"
#include "urna_comune.h"
#include "urna-servizi.h"
#include "memstat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int load_urna(struct urna *un, unsigned long progressivo);
int load_stat();
int load_urne();
struct urna_conf *load_conf(unsigned long progressivo);
struct urna_dati *load_dati(unsigned long progressivo, int modo);
struct urna_prop *load_prop(unsigned long progressivo, int modo);
struct urna_voti *load_voti(unsigned long progressivo, int num_voci,
                            int modo);
int load_voci(struct urna_conf *ucf, FILE * fp);
int save_conf(struct urna_conf *ucf, int level, int progressivo);
int save_dati(struct urna_dati *udt, long int progressivo);
int save_prop(struct urna_prop *upr, int progressivo);
int save_stat(int level);
int save_urna(struct urna *ur, int level);
int save_urne(int level);
int save_voci(struct urna_conf *ucf, FILE * fp);
int save_voti(struct urna_voti *uvt, int progressivo, int num_voci);
int compara(char *a, char *b);
void azzera_stat();
void rs_load_data();
int read_prop(unsigned long progressivo, int nvoti, char ***ptesto);
static int compstr(const void *a, const void *b);
extern void qsort();

/* 
 * compatibilita` col vecchio
 * da mettera a posto
 */

void rs_load_data()
{
   load_urne();
   return;
}

void rs_save_data()
{
   save_urne(1);
   return;
}

/* 
 * salva stat, ciascuna urna, i dati e i voti 
 *
 * per ogni urna ci sono 3 file,
 * URNA_CONF-nquesito --salvato al momento della creazione.
 * URNA_DATI-nquesito --salvato al momento della creazione.
 * VOTI/PROP
    ** URNA_VOTI-nquesito --salvato ogni volta.
    ** URNA_PROP-nquesito --aggiunto ogni volta.
 * se level=0 salva  i dati e i voti/proposte, altrimenti salva tutto.
 */

int save_urne(int level)
{
   struct urna *punto;
   unsigned int totali = 0, salvate = 0, errori = 0;
   unsigned long i;

   citta_log("SYSTEM:salvataggio sondaggi");

   /* 
    * * questo dovrebbe essere un evento davvero raro
    */

   if(save_stat(level)) {
      return -1;
   }

   for(i = 0; i < LEN_SLOTS * (ustat.urne_slots); i++) {
      punto = *(ustat.urna_testa + i);
      /* salta le urne concluse */
      if(punto == NULL) {
         continue;
      }
      totali++;
      switch (save_urna(punto, level)) {
      case -1:
         citta_logf("SYSTEM:problemi a salvare %ld", punto->progressivo);
         errori++;
         break;
      case 0:
         citta_logf("SYSTEM:urna %ld salvata", punto->progressivo);
         salvate++;
         break;
      }
   }
   citta_logf("SYSTEM:urne salvate: %d su %d totali non concluse (%d errori)",
        salvate, totali, errori);
   return (0);
}

/* 
 * Salva urna stat
 * e l'elenco delle urne attive
 * se e` stata modificata
 * o se level!=1
 * riporta a zero il flag 'modificata'
 */

int save_stat(int level)
{
   FILE *fp;
   unsigned long int i, j, alloc;
   unsigned long int *numeri_urne;
   struct urna *punto;

   if(level == 0 && ustat.modificata == 0 &&
      esiste(URNA_DIR, FILE_USTAT, -1) == 0)
      return 0;

   if(save_file(URNA_DIR, FILE_USTAT, -1, &fp))
      return (-1);

   if(write_magic_number(fp, MAGIC_STAT, "stat")) {
      fclose(fp);
      return (-1);
   }

   if(fwrite(&ustat, sizeof(struct urna_stat), 1, fp) != 1) {
      citta_log("SYSLOG: ustat non salvato, non posso scrivere");
      fclose(fp);
      return (-1);
   }

   alloc = ustat.urne_slots * LEN_SLOTS;

   /* scrive i numeri delle urne attive */

   CREATE(numeri_urne, unsigned long int, alloc, TYPE_LONG);

   j = 0;

   /*
    * * ci si potrebbe fermare a j<ustat.attive
    * * ma chissene
    */

   for(i = 0; i < alloc; i++) {
      punto = (*(ustat.urna_testa + i));

      /* salta le urne concluse */

      if(punto == NULL) {
         continue;
      }

      *(numeri_urne + j) = punto->progressivo;
      j++;
   }

   if(fwrite(numeri_urne, sizeof(long int), alloc, fp) != alloc) {
      citta_log("SYSLOG: numeri delle urne non salvate");
      fclose(fp);
      Free(numeri_urne);
      return (-1);
   }

   fclose(fp);
   citta_log("SYSLOG: ustat salvato");
   Free(numeri_urne);

   ustat.modificata = 0;
   return (0);
}

/* salva un urna */

int save_urna(struct urna *u, int level)
{
   int errori = 0;

   /* 
    * level!=0 non servirebbe, perche' save_conf/save_dati gia`
    * lo controllano, ma mi salvo una chiamata
    */

   if(level != 0) {
      errori += save_conf(u->conf, level, u->progressivo);
   }
   errori += save_dati(u->dati, u->progressivo);
   errori += save_prop(u->prop, u->progressivo);
   /* bisognerebbe controllare che prop siano salvate davvero! */
   u->prop = NULL;
   return (errori == 0 ? 0 : -1);
}

/* 
 * salva 
 * configurazione:
 */

int save_conf(struct urna_conf *ucf, int level, int progressivo)
{
   FILE *fp;

   if(level == 0) {
      return 0;
   }

   if(save_file(URNA_DIR, FILE_UCONF, progressivo, &fp)) {
      citta_logf("SYSLOG: non posso aprire file conf");
      return (-1);
   }

   if(write_magic_number(fp, MAGIC_CONF, "conf")) {
      citta_logf("SYSLOG: non scrivo il mnumb del conf %d", progressivo);
      fclose(fp);
      return (-1);
   }

   /* salva la struct */
   if(fwrite(ucf, sizeof(struct urna_conf), 1, fp) != 1) {
      citta_logf("SYSLOG: errore scrivendo la conf di %d", progressivo);
      fclose(fp);
      return (-1);
   }

   /* le voci (lunghezza, voce) */
   if(save_voci(ucf, fp)) {
      citta_logf("SYSLOG: errore in salva voci");
      fclose(fp);
      return (-1);
   }

   fclose(fp);
   return 0;
}

/*
 * voci
 */

int save_voci(struct urna_conf *ucf, FILE * fp)
{
   size_t len;
   char *voce;
   int i;

   for(i = 0; i < ucf->num_voci; i++) {
      voce = *(ucf->voci + i);
      len = strlen(voce) + 1;
      if(fwrite(&len, sizeof(size_t), 1, fp) != 1)
         return -1;
      if(fwrite(voce, sizeof(char), len, fp) != len)
         return -1;
   }
   return 0;
}

/* 
 * salva le proposte
 * ogni proposta e` salvata come
 * matricola:numproposte\n
 * 01:lenprop:proposta\n 
 * 02:lenprop:proposta\n
 * -- deve cancellare le proposte da urna->prop...
 *
 */

int save_prop(struct urna_prop *upr, int progressivo)
{
   FILE *fp;
   struct urna_prop *testa;
   int i;
   char **prop;
   char *testo;
   int nuovo;

   if(upr == NULL) {
      return 0;
   }

   nuovo = esiste(URNA_DIR, FILE_UPROP, progressivo);

   switch (nuovo) {
   case 0:
      /* file esiste e non e` vuoto */

      if(load_file(URNA_DIR, FILE_UPROP, progressivo, &fp)) {
         citta_logf("SYSLOG: problemi con %s", FILE_UPROP);
         return (-1);
      }

      if(!check_magic_number(fp, MAGIC_PROP, "prop")) {
         citta_logf("SYSLOG: urna-prop %s %d non corretto",
              FILE_UPROP, progressivo);
         fclose(fp);
         return (-1);
      }

      fclose(fp);
      if(append_file(URNA_DIR, FILE_UPROP, progressivo, &fp)) {
         citta_logf("SYSLOG: non apro file voti di %d", progressivo);
         return (-1);
      }

      fprintf(fp, "\n");
      break;
   case -2:
      citta_logf("%s non e` un file regolare", FILE_UPROP);
      return (-1);
      break;
   default:
      /* file non esiste o  e` vuoto */
      if(save_file(URNA_DIR, FILE_UPROP, progressivo, &fp)) {
         citta_logf("SYSLOG: non apro file voti di %d", progressivo);
         return (-1);
      }
      if(write_magic_number(fp, MAGIC_PROP, "prop")) {
         citta_logf("SYSLOG: non scrivo il mnumb di %d", progressivo);
         fclose(fp);
         return (-1);
      }
      break;
   }

   /* 
    * matricola:numproposte\n1:l1p1\n2:l2p2:\n\n
    */

   for(testa = upr; testa; testa = testa->next) {
      citta_logf("::%ld:%d\n", testa->matricola, testa->num);
      if(fprintf(fp, "::%ld:%d\n", testa->matricola, testa->num) == 0) {
         citta_logf("SYSLOG: errori nelle proposte %d", progressivo);
         fclose(fp);
         return (-1);
      }

      i = 1;
      prop = testa->proposte;

      for(i = 0; i < testa->num; i++) {
         testo = *(prop + i);
         if(testo == NULL) {
            continue;
         }
         if(fprintf(fp, "%d:%s\n", i, testo) == 0) {
            citta_logf("SYSLOG: errori nelle proposte %d, %d", progressivo, i);
            fclose(fp);
            return (-1);
         }
      }

      if(fprintf(fp, "\n") == 0) {
         citta_logf("SYSLOG: errori nelle proposte %d", progressivo);
         fclose(fp);
         return (-1);
      }
   }
   deall_prop(upr);
   fclose(fp);
   return (0);
}

int save_dati(struct urna_dati *udt, long int progressivo)
{
   FILE *fp;

   if(save_file(URNA_DIR, FILE_UDATA, progressivo, &fp)) {
      citta_logf("SYSLOG: non apro file data di %ld", progressivo);
      return (-1);
   }

   if(write_magic_number(fp, MAGIC_DATA, "dati")) {
      citta_logf("SYSLOG: non scrivo il mnumb di %ld", progressivo);
      fclose(fp);
      return (-1);
   }

   if(fwrite(udt, sizeof(struct urna_dati), 1, fp) != 1) {
      citta_logf("SYSLOG: non scrivo i dati di %ld", progressivo);
      fclose(fp);
      return (-1);
   }

   if(fwrite(udt->uvot, sizeof(long int), (udt->voti_nslots) * LEN_SLOTS, fp)
      != (unsigned long)(udt->voti_nslots) * LEN_SLOTS) {
      citta_logf("SYSLOG: non scrivo i votanti");
      fclose(fp);
      return (-1);
   }

   if(fwrite(udt->ucoll, sizeof(long int), (udt->coll_nslots) * LEN_SLOTS, fp)
      != (unsigned long)(udt->coll_nslots) * LEN_SLOTS) {
      citta_logf("SYSLOG: non scrivo i votanti ");
      fclose(fp);
      return (-1);
   }

   if(save_voti(udt->voti, progressivo, udt->num_voci)) {
      citta_logf("SYSLOG: non scrivo i voti");
      fclose(fp);
      return (-1);
   }

   fclose(fp);
   return (0);
}

/* 
 * salva voti 
 */

int save_voti(struct urna_voti *uvt, int progressivo, int num_voci)
{
   FILE *fp;
   struct urna_voti *testa;
   int i;

   if(uvt == NULL) {
      return 0;
   }

   if(save_file(URNA_DIR, FILE_UVOTI, progressivo, &fp)) {
      citta_logf("SYSLOG: non apro file voti di %d", progressivo);
      return (-1);
   }

   if(write_magic_number(fp, MAGIC_VOTI, "voti")) {
      citta_logf("SYSLOG: non scrivo il mnumb di voti di %d", progressivo);
      fclose(fp);
      return (-1);
   }

   for(i = 0; i < num_voci; i++) {
      testa = uvt + i;
      if(testa == NULL) {
         citta_logf("SYSLOG: errori nei voti di  %d", progressivo);
      }
      if(fwrite(testa, sizeof(struct urna_voti), 1, fp) != 1) {
         citta_logf("SYSLOG: problemi a scrivere i voti%d... %d", i, progressivo);
         fclose(fp);
         return (-1);
      }

      /*
       * quando leggo, mi posso fidare
       * del fatto che la testa
       * rimanga null?
       */

#if URNA_GIUDIZI
      if(testa->giudizi == NULL) {
         continue;
      }

      if(fwrite(testa->giudizi, sizeof(long), NUM_GIUDIZI, fp) != NUM_GIUDIZI) {
         citta_logf("SYSLOG: problemi a scrivere i giudizi%d... %d", i,
              progressivo);
         fclose(fp);
         return (-1);
      }
#endif
   }

   fclose(fp);
   return (0);
}

/*
 ************************ LOAD *********************************************
 */

/* 
 * carica stat / l'elenco delle urne -- crea le urne e le pone
 * a inattive
 */
int load_stat()
{
   FILE *fp;
   int letto;
   unsigned int i, j;
   unsigned long progressivo;
   unsigned long int alloc;
   unsigned long int *numeri_urne;

   switch (esiste(URNA_DIR, FILE_USTAT, -1)) {
   case -1:
      citta_logf("SYSLOG: %s non esiste", FILE_USTAT);
      azzera_stat();
      return (0);
      break;
   case 1:
      citta_logf("SYSLOG: %s e` vuoto", FILE_USTAT);
      azzera_stat();
      return (0);
      break;
   }

   letto = load_file(URNA_DIR, FILE_USTAT, -1, &fp);

   if(letto == -1) {
      citta_logf("SYSLOG: non posso aprire %s", FILE_USTAT);
      azzera_stat();
      return (-1);
   }

   ustat.urna_testa = NULL;

   if(!(check_magic_number(fp, MAGIC_STAT, "stat"))) {
      citta_logf("SYSLOG: versione %s non corretta", FILE_USTAT);
      azzera_stat();
      fclose(fp);
      return (-1);
   }

   if(fread(&ustat, sizeof(struct urna_stat), 1, fp) != 1) {
      citta_logf("SYSLOG: dati su %s corrotti", FILE_USTAT);
      azzera_stat();
      fclose(fp);
      return (-1);
   }
#ifdef USA_DEFINENDE
   ut_definende = 0;
   ut_def = NULL;
#endif

   if(ustat.attive > LEN_SLOTS * MAX_SLOT) {
      citta_logf("SYSLOG: dati su %s corrotti (troppi slot)", FILE_USTAT);
      ustat.attive = LEN_SLOTS * MAX_SLOT;
   }

   if(ustat.attive < 0) {
      citta_logf("SYSLOG: dati su %s corrotti (slot negativi!)", FILE_USTAT);
      ustat.attive = 0;
   }

   /* 
    * teoricamente dovrebbe aver scritto solo
    * quelli necessari, ma siccome ne scrive
    * ustat.urne_slots*LEN_SLOTS con urne_slots
    * vecchio, provo a leggere quelli...
    */

   alloc = ustat.urne_slots * LEN_SLOTS;

   if(ustat.attive>alloc)
	   citta_logf("SYSLOG: non erano state salvate tutte le urne!");

/*   ustat.urne_slots = ustat.attive / LEN_SLOTS + 1; */


   CREATE(numeri_urne, unsigned long int, alloc, TYPE_LONG);

   CREATE(ustat.urna_testa, struct urna *, alloc, TYPE_POINTER);

   /* legge i numeri delle urne */
   if(ustat.attive > 0) {
      if(fread(numeri_urne, sizeof(long int), alloc, fp) != alloc) {
         citta_logf("SYSLOG: dati su %s corrotti (numeri_urne)", FILE_USTAT);
         azzera_stat();
         return (0);
      }
   }

/* manca controllino sul perche' fallisca */
   /* 
    * qui NON vengono caricate le urne, ma solo
    * i numeri progressivi
    */

   j = 0;
   for(i = 0; i < ustat.attive; i++) {
      progressivo = *(numeri_urne + i);
      if(progressivo ==0) {
         continue;
      }
      CREATE(*(ustat.urna_testa + j), struct urna, 1, TYPE_URNA);

      (*(ustat.urna_testa + j))->progressivo = progressivo;
      (*(ustat.urna_testa + j))->semaf &= !SEM_U_ATTIVA;
      j++;
   }

/*   ustat.urne_slots = ustat.attive / LEN_SLOTS + 1;*/

   ustat.modificata = 0;
   CREATE(ustat.votanti, struct votanti, 1, TYPE_VOTANTI);

   ustat.votanti->num = -1;
   ustat.votanti->utente = -1;
   ustat.votanti->inizio = -1;
   ustat.votanti->next = NULL;
   fclose(fp);
   Free(numeri_urne);
#if SANE_STAT
   if(esiste(URNA_DIR, SANE_FILE_STAT, -1) >= 0) {
      citta_log("controllo la coerenza di ustat");
      if(ustat.complete == 0) {
         ustat.complete = ustat.ultima_urna - 1 - ustat.attive;
         if(ustat.complete < 0)
            ustat.complete = 0;
         citta_logf("urne complete =0 messo a %ld", ustat.complete);
      }
      if(ustat.totali < ustat.attive + ustat.complete) {
         citta_logf
           ("urne totali (%ld) < attive (%ld) + complete (%ld), messo a %ld",
            ustat.totali, ustat.complete, ustat.attive,
            ustat.attive + ustat.complete);
         ustat.totali = ustat.attive + ustat.complete;
      }
   }
#endif
   return (0);
}

/* 
 * carica tutte le urne 
 */

int load_urne()
{
   unsigned long i;
   unsigned long j = 0;
   struct urna *punto;
   unsigned long num_quesito;

   citta_log("SYSTEM: Lettura urna stat");

   if(load_stat()) {
      citta_log("SYSERROR: non si possono usare i referendum!");
      return -1;
   }

   if(ustat.attive == 0)
      return 0;

   /* 
    * * le urne sono gia` state allocate in load_stat 
    * * qui vengono solo caricati ad allocati gli altri dati 
    */

   for(i = 0; i < ustat.attive && i < LEN_SLOTS * (ustat.urne_slots); i++) {
      punto = *(ustat.urna_testa + i);
      if(punto == NULL) {
         continue;
      }
      num_quesito = punto->progressivo;

      /*CREATE(nuova, struct urna, 1, TYPE_URNA); */
      /*citta_logf("SYSTEM: urna %ld allocata", num_quesito); */

      if(load_urna(punto, num_quesito)) {
         citta_logf("SYSERR: lettura urna andata male %ld", num_quesito);
         Free(punto);
         *(ustat.urna_testa + i) = NULL;
         continue;
      }
      citta_logf("SYSTEM: urna %ld letta", num_quesito);
      punto->semaf |= SEM_U_ATTIVA;
      /*
       * * punto->votanti=0;
       */
      j++;
   }
   ustat.attive = j;
   citta_logf("SYSLOG: urne attive:%ld", ustat.attive);
   return 0;
}

/* 
 * carica i dati
 * per non fare casino con le allocazione, ogni
 * struttura ha in partenza un NULL su tutti
 * i suoi puntatori.
 * Se qualcosa fallisce ci pensa dealloca_urna a
 * liberare tutto (tranne urna, che viene deallocata
 * dalla chiamante)
 */

int load_urna(struct urna *u, unsigned long progressivo)
{
   u->conf = NULL;
   u->dati = NULL;
   u->prop = NULL;

   if(!(u->conf = load_conf(progressivo))) {
      dealloca_urna(u);
      return -1;
   }

   if(!(u->dati = load_dati(progressivo, u->conf->modo))) {
      dealloca_urna(u);
      return -1;
   }

#if 0
   if(!(u->prop = load_prop(progressivo, u->conf->modo))) {
      dealloca_urna(u);
      return -1;
   }
#endif

/* le proposte NON vengnono caricate */
   u->prop = NULL;

   return 0;
}

struct urna_conf *load_conf(unsigned long progressivo)
{
   FILE *fp;
   struct urna_conf *ucf;

   if(load_file(URNA_DIR, FILE_UCONF, progressivo, &fp)) {
      citta_logf("SYSLOG: non posso aprire file conf");
      return (NULL);
   }

   if(!check_magic_number(fp, MAGIC_CONF, "conf")) {
      citta_logf("SYSLOG: versione %s non corretta", FILE_UCONF);
      fclose(fp);
      return (NULL);
   }

   /* leggi la struct */

   CREATE(ucf, struct urna_conf, 1, TYPE_URNA_CONF);

   if(fread(ucf, sizeof(struct urna_conf), 1, fp) != 1) {
      citta_logf("SYSLOG: errore leggendo la conf di %ld", progressivo);
      fclose(fp);
      return (NULL);
   }

   ucf->voci = NULL;

   /* le voci (lunghezza, voce) */
   if(load_voci(ucf, fp)) {
      fclose(fp);
      return (NULL);
   }

   fclose(fp);
   return ucf;
}

int load_voci(struct urna_conf *ucf, FILE * fp)
{
   size_t len;
   int i;

   CREATE(ucf->voci, char *, ucf->num_voci, TYPE_POINTER);

   for(i = 0; i < ucf->num_voci; i++) {
      if(fread(&len, sizeof(size_t), 1, fp) != 1)
         return -1;
      if(len > MAXLEN_VOCE)
         return -1;

      CREATE(*(ucf->voci + i), char, len + 1, TYPE_CHAR);

      if(fread(*(ucf->voci + i), sizeof(char), len, fp) != len)
         return (-1);
   }
   return (0);
}

/*
 * per ora non fa nulla, se non 
 * mettere a NULL proposte
 * (in effetti ora e` ridicolo, ma e` gia` pronto per 
 * modifiche
 */

struct urna_prop *load_prop(unsigned long progressivo, int modo)
{
   struct urna_prop *upr = NULL;

   IGNORE_UNUSED_PARAMETER(progressivo);
   IGNORE_UNUSED_PARAMETER(modo);
#if 0
   /*
    * nel caso si volesse fare qualcosa
    */

   if(modo != MODO_PROP)
      upr = NULL;
#endif

   return (upr);
}

/* 
 * legge le proposte
 * va raffinato: ora appena trova un errore 
 * chiude 
 * bisogna modificare il protocollo
 */

int read_prop(unsigned long progressivo, int nvoti, char ***ptesto)
{
   FILE *fp;
   char c;
   char nextc;
   char **testo;
   char proposta[MAXLEN_PROPOSTA];
   int i, j, n, prog;
   unsigned long int matricola;
   size_t lent;
   int slots, num_proposte;

   if(load_file(URNA_DIR, FILE_UPROP, progressivo, &fp)) {
      citta_logf("SYSLOG: non posso aprire file prop");
      return (0);
   }

   if(!check_magic_number(fp, MAGIC_PROP, "prop")) {
      citta_logf("SYSLOG: versione %s non corretta", FILE_UPROP);
      fclose(fp);
      return (0);
   }

   /* leggi la struct */
   n = 0;
   slots = 1;
   /*CREATE(testo, char **, slots*LEN_SLOTS, TYPE_POINTER);
    * * */

   testo = (char **)calloc(sizeof(char **), slots * LEN_SLOTS);

   for(i = 0; i < nvoti; i++) {
      while((c = fgetc(fp)) == '\n') ;

      if(c != ':')
         citta_logf("mancano i duepunti:%d", i);

      if(fscanf(fp, ":%ld:%d\n", &matricola, &num_proposte) != 2) {
         citta_logf("qualcosa non va nella lettura delle urne... pos %d", i);

         return -1;
      }

/* 
      txt_putf(txt, "utente %d:", i);
 * matricola...
 */
      for(j = 0; j < num_proposte; j++) {
         fscanf(fp, "%d:", &prog);
         nextc = ' ';
         /* salta tutti i bianchi */

         while(nextc == ' ')
            nextc = fgetc(fp);
         if(nextc == '\n') {
            citta_logf("??, proposta vuota %d, posto %d->%d", i, j, prog);
            continue;
         }
         ungetc(nextc, fp);
         /*fscanf(fp,"%s\n",proposta); */
         if(fgets(proposta, MAXLEN_PROPOSTA, fp) == NULL) {
            citta_logf("??, proposta vuota %d, posto %d->%d", i, j, prog);
            continue;
         }
         if(prog != j) {
            citta_logf("??, proposta %d, posto %d->%d", i, j, prog);
            continue;
         }
         lent = strlen(proposta);
         /* cancella il \n alla fine */
         if(lent > 0 &&proposta[lent-1] == '\n')
            			proposta[lent-1] = '\0';

         if(lent > MAXLEN_PROPOSTA)
            lent = MAXLEN_PROPOSTA;

/*   	    		CREATE(*(testo+n), char, lent+1, TYPE_CHAR);
 */
         *(testo + n) = (char *)malloc((lent + 2) * sizeof(char));
         strncpy(*(testo + n), proposta, lent + 1);
         n++;
         if(n >= slots * LEN_SLOTS) {
            slots++;
            testo =
              (char **)realloc(testo, slots * LEN_SLOTS * sizeof(char **));
         }
      }
   }

   fclose(fp);
   qsort(testo, n, sizeof(char *), compstr);
   *ptesto = testo;
   return (n);
}

static int compstr(const void *a, const void *b)
{
  return strcasecmp((const char *)*(char **)a, (const char *)*(char **)b);
}

/* 
 * carica dati
 */

struct urna_dati *load_dati(unsigned long progressivo, int modo)
{
   FILE *fp;
   struct urna_dati *udt;
   long int letti, alloc;

   if(load_file(URNA_DIR, FILE_UDATA, progressivo, &fp)) {
      citta_logf("SYSLOG: problemi con %s", FILE_UDATA);
      return (NULL);
   }
   /*   */

   if(!check_magic_number(fp, MAGIC_DATA, "dati")) {
      citta_logf("SYSLOG: urna-data %s %ld non corretto", FILE_UDATA, progressivo);
      fclose(fp);
      return (NULL);
   }

   CREATE(udt, struct urna_dati, 1, TYPE_URNA_DATI);

   if(fread(udt, sizeof(struct urna_dati), 1, fp) != 1) {
      citta_logf("SYSLOG: non riesco a leggere i dati");
      fclose(fp);
      return (NULL);
   }

   if(udt->nvoti > MAX_UTENTI) {
      citta_logf("SYSLOG: non riesco a leggere i dati");
      fclose(fp);
      return (NULL);
   }

   udt->uvot = NULL;

/* normalizza il numero di slots
 * dava un problema banale: 10/10+1=2...
 *
 * udt->voti_nslots=((udt->nvoti)/LEN_SLOTS)+1;
 *
 * ma andrebbero comunque fattid ei controlli
 */

   alloc = udt->voti_nslots * LEN_SLOTS;
   CREATE(udt->uvot, long, alloc, TYPE_LONG);

   letti=fread(udt->uvot, sizeof(long int), alloc, fp);
   if(letti!= alloc) {
      citta_logf("SYSLOG: non riesco a leggere i votanti:provati %ld, letti %ld", alloc, letti);
      fclose(fp);
      return (NULL);
   }

   if(udt->ncoll > MAX_UTENTI) {
      citta_logf("SYSLOG: non riesco a leggere i dati");
      fclose(fp);
      return (NULL);
   }

   udt->ucoll = NULL;

/* normalizza il numero di slots
 * dava un problema banale: 10/10+1=2...
 *
 * udt->coll_nslots=(udt->ncoll/LEN_SLOTS)+1;
 * andrebbero comunque fatti dei controlli
 */
   alloc = udt->coll_nslots * LEN_SLOTS;
   CREATE(udt->ucoll, long, alloc, TYPE_LONG);

   letti=fread(udt->ucoll, sizeof(long int), alloc, fp);
   if(letti!= alloc) {
      citta_logf("SYSLOG: non riesco a leggere i collegati: provati %ld, letti %ld", alloc, letti);
      fclose(fp);
      return (NULL);
   }

   udt->voti = NULL;
   if(!(udt->voti = load_voti(progressivo, udt->num_voci, modo))) {
      fclose(fp);
      return (NULL);
   }

   fclose(fp);
   return udt;
}

struct urna_voti *load_voti(unsigned long progressivo, int num_voci, int modo)
{
   FILE *fp;
   int i;
   struct urna_voti *testa;
   struct urna_voti *uvt;

   if(num_voci == 0) {
      CREATE(uvt, struct urna_voti, 1, TYPE_URNA_VOTI);

      return uvt;
   }

   CREATE(uvt, struct urna_voti, num_voci, TYPE_URNA_VOTI);

   if(load_file(URNA_DIR, FILE_UVOTI, progressivo, &fp)) {
      citta_logf("SYSLOG: problemi con %s", FILE_UDATA);
      return (NULL);
   }
   /*   */

   if(!check_magic_number(fp, MAGIC_VOTI, "voti")) {
      citta_logf("SYSLOG: urna-data %s %ld non corretto", FILE_UVOTI, progressivo);
      fclose(fp);
      return (NULL);
   }

   for(i = 0; i < num_voci; i++) {
      testa = uvt + i;
      if(fread(testa, sizeof(struct urna_voti), 1, fp) != 1) {
         citta_logf("SYSLOG: problemi a leggere i voti %d... %ld", i, progressivo);
         fclose(fp);
         return (NULL);
      }
#if URNA_GIUDIZI
      if(testa->giudizi == NULL || modo != MODO_VOTAZIONE) {
         continue;
      }

      if(fread(testa->giudizi, sizeof(long), NUM_GIUDIZI, fp) != NUM_GIUDIZI) {
         citta_logf("SYSLOG: problemi a scrivere i giudizi%d... %d", i,
              progressivo);
         fclose(fp);
         return (NULL);
      }
#else
      IGNORE_UNUSED_PARAMETER(modo);
#endif
   }

   fclose(fp);
   return (uvt);
}
void azzera_stat()
{

#if U_DEBUG
   citta_log("SYSLOG: azzera stat");
#endif

   ustat.attive = 0;
   ustat.totali = 0;
#ifdef USA_DEFINENDE
   ut_def = NULL;
#endif
   ustat.modificata = 0;
   ustat.voti = 0;
   ustat.complete = 0;
   ustat.ultima_urna = 1;
   ustat.ultima_lettera = 0;
   ustat.urne_slots = 0;
   ustat.urna_testa = NULL;
   /* 
    * * ustat.votanti esiste sempre... semplifica parecchio!
    */
   CREATE(ustat.votanti, struct votanti, 1, TYPE_VOTANTI);

   ustat.votanti->num = -1;
   ustat.votanti->utente = -1;
   ustat.votanti->inizio = -1;
   ustat.votanti->next = NULL;
}
