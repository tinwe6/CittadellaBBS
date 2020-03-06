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
* File:  urna-file.c                                                        *
*       Gestione dei file. Salva, Carica, Cancella e Backuppa               *
****************************************************************************/

#include "config.h"
#include <stdio.h>

#include "urna-file.h"
#include "urna-cost.h"
#include "urna_comune.h"
#include "memstat.h"
#include "utility.h"
#include "debug.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * prototipi
 */

int esiste(const char *dir, const char *file,int progressivo);
int backup(const char *file);
int append_file(const char *dir, const char *file, const int est,
                FILE ** fpp);
int save_file(const char *dir, const char *file, const int est, FILE ** fpp);
int load_file(const char *dir, const char *file, const int est, FILE ** fpp);
int canc_file(const char *dir, const char *file, const int est);
int write_magic_number(FILE * fp, char code, char *nome_file);
int check_magic_number(FILE * fp, char code, char *nome_file);
char *crea_nome(const char *dir, const char *file, int est);

/*
 * controlla che esista un file, che sia un file
 * standard e che la dim sia !=0
 * restituisce
 *  0 se il file esiste e non e` vuoto
 *  1 se esiste ma e` vuoto
 * -1 se non esiste
 * -2 per altri errori
 */

int esiste(const char *dir, const char *file, int progressivo)
{
   char *nomefile;
   struct stat stato;

   if(!(nomefile = crea_nome(dir, file, progressivo))) {
      return -2;
   }

   if(lstat(nomefile, &stato)) {
      if(errno == ENOENT) {
         Free(nomefile);
         return -1;
      };
      citta_logf("SYSLOG: file %s, %s", nomefile, strerror(errno));
      Free(nomefile);
      return -2;
   };

   Free(nomefile);

   if(!S_ISREG(stato.st_mode)) {
      citta_logf("SYSLOG: file %s non e` un file regolare", nomefile);
      return -2;
   }

   if(stato.st_size != 0)
      return 0;
    else
      return 1;
}

/*
 * rinomina il file in file.bak
 * restituisce 0 se e` tutto a posto,
 * -1 se ci sono problemi
 */

int backup(const char *file)
{
   char *bk;
   struct stat stato;

   if(strcmp(file, ".") == 0 || strcmp(file, "..") == 0) {
      citta_logf("SYSERR: nome file non corretto %s", file);
      return (-1);
   }

   if(stat(file, &stato)) {
      if(errno == ENOENT) {
         citta_logf("SYSLOG: file %s non esiste (non faccio bk)", file);
         return 0;
      }
    else {
      citta_logf("SYSLOG: file ->%s<- ha problemi:%s", file, strerror(errno));
      return 0;
	}
   }

   if(!S_ISREG(stato.st_mode)) {
      citta_logf("SYSLOG: file %s non e` un file regolare", file);
      return -1;
   }

   CREATE(bk, char, strlen(file) + 5, TYPE_CHAR);
   strncpy(bk, file, strlen(file));
   strncat(bk, ".bak", 4);

   if(rename(file, bk)) {
      citta_logf("SYSERR: errore nella creazione di bk!,%s", strerror(errno));
      Free(bk);
      return (-1);
   };

   Free(bk);
   return (0);
}

/*
 * Apre un file (a) sotto dir con estensione est
 * (se est e` tra 0 e 99999)
 * lo assegna a fp, fa un backup .bak del vecchio
 * restituisce 0 se e` tutto ok
 *
 * 1 se non e` riuscito ad aprire il file
 * -1 se il file contiene caratteri non accettabili (per ora /, . ..)
 * fp dev'essere malloccato da chi chiama
*/

int append_file(const char *dir, const char *file, const int est, FILE ** fpp)
{
   char *nomefile;

   if(!(nomefile = crea_nome(dir, file, est))) {
      return -1;
   };

   *fpp = fopen(nomefile, "a");
   if(!*fpp) {
      citta_logf("SYSERR: Non posso aprire %s in scrittura", nomefile);
      Free(nomefile);
      return (1);
   }
   citta_logf("SYSTEM:creato file %s", nomefile);
   Free(nomefile);

   return 0;
}

/*
 * Apre un file (w) sotto dir con estensione est
 * (se est e` tra 0 e 999)
 * lo assegna a fp, fa un backup .bak del vecchio
 * restituisce 0 se e` tutto ok
 * 1 se non e` riuscito ad aprire il file
 * -1 se il file contiene caratteri non accettabili (per ora /, . ..)
 * fp dev'essere malloccato da chi chiama
*/

int save_file(const char *dir, const char *file, const int est, FILE ** fpp)
{
   char *nomefile;

   if(!(nomefile = crea_nome(dir, file, est))) {
           return -1;
   };

   if(backup(nomefile)){
           citta_logf("non posso fare il backup di %s",file);
           Free(nomefile);
           return(1);
   }

   *fpp = fopen(nomefile, "w");
   if(!(*fpp)) {
           citta_logf("SYSERR: Non posso aprire %s in scrittura", nomefile);
           Free(nomefile);
           return (1);
   }

   Free(nomefile);
   return 0;
}

/*
 * apre un file (in lettura) sotto dir con estensione est (se est>=0), e
 * lo assegna a fp esce con
 *   0 se tutto va bene,
 *  -1 se ci sono  problemi (compresi caratteri non accettabili),
 *   1 se il file è lungo 0 (in questo caso lo chiude).
 * Se il file non esiste lo crea, e lo tratta come nel caso di file
 * di lunghezza 0
 */

int load_file(const char *dir, const char *file, const int est, FILE ** fpp)
{
   char *nomefile;
   struct stat status;

   if(!(nomefile = crea_nome(dir, file, est))) {
      return -1;
   };

   *fpp = fopen(nomefile, "r");

   if(!*fpp) {
      citta_logf("SYSLOG: errore in apertura:%d,%s", errno, strerror(errno));
      if(errno == ENOENT) {
         citta_logf("SYSERR: file %s non esiste, lo creo", nomefile);
         *fpp = fopen(nomefile, "w");
         if(*fpp == NULL) {
            citta_logf("SYSERR: non posso creare %s", nomefile);
            Free(nomefile);
            return -1;
         }
         fclose(*fpp);
         Free(nomefile);
         return (1);
      } else {
         citta_logf("SYSERR: Non posso aprire %s in lettura", nomefile);
         Free(nomefile);
         return (-1);
      }
   }

   if(stat(nomefile, &status)) {
      fclose(*fpp);
      Free(nomefile);
      return (-1);
   }

   if(status.st_size == 0) {
      citta_logf("SYSERR: file %s vuoto: lo chiudo", nomefile);
      fclose(*fpp);
      Free(nomefile);
      return (1);
   }

   Free(nomefile);
   return 0;
}

/*
 * cancella il file con estensione -est (se est>=0) (est<999999)
 * -1 se ci sono problemi, 0 se e` tutto ok
 */

int canc_file(const char *dir, const char *file, const int est)
{
   char *nomefile;

   if(!(nomefile = crea_nome(dir, file, est))) {
      return -1;
   };

   citta_logf("SYSLOG:cancello il file %s del sondaggio/ref %d", nomefile, est);

   if(unlink(nomefile)) {
      citta_logf("SYSERR: errore cancellando %s", nomefile);
      Free(nomefile);
      return -1;
   }

   Free(nomefile);
   return 0;
}

/* scrive il magic_number  - ritorna -1 se ci sono problemi  */

int write_magic_number(FILE * fp, char code, char *nome_file)
{
   char mnumb[5];

   sprintf(mnumb, "%2s%1c%1c", MAGIC_CODE, code, VERSIONE);
   mnumb[4] = 0;

   if(fwrite(mnumb, sizeof(char), 5, fp) != 5) {
      citta_logf("errore scrivendo il mnumb su %s", nome_file);
      return (-1);
   }
   /*   */
   return (0);
}

/* controlla il magic_number - 0 se tutto e` OK */
int check_magic_number(FILE * fp, char code, char *nome_file)
{
   char mnumb[5];
   char mcheck[5];

   sprintf(mcheck, "%2s%1c%1c", MAGIC_CODE, code, VERSIONE);
   mcheck[4] = 0;
   if(fread(mnumb, sizeof(char), 5, fp) != 5) {
      citta_logf("errore leggendo il mnumb in %s errato!", nome_file);
      return (-1);
   }
   /*   */

   return (strcmp(mnumb, mcheck) == 0);
}

/*
 * crea e alloca
 * nomefile,
 * ="dir/file-est"
 * nel caso est>0 <99999,
 * altrimenti
 * dir/file
 * controlla che file non contenga "/" e non sia "." o ".."
 */

char *crea_nome(const char *dir, const char *file, int est)
{

   int lnomefile;
   char *nomefile = NULL;

   lnomefile = strlen(dir) + 1;
   lnomefile += strlen(file);
   lnomefile += 8;

   if(strcmp(file, ".") == 0 || strcmp(file, "..") == 0) {
      citta_logf("%s ha caratteri non validi!", file);
      return NULL;
   }
   if(index(file, '/') != NULL) {
      citta_logf("%s ha caratteri non validi!", file);
      return NULL;
   };

   CREATE(nomefile, char, lnomefile, TYPE_CHAR);

   if(est > 0 && est < 99999)
      sprintf(nomefile, "%s/%s-%d", dir, file, est);
   else
      sprintf(nomefile, "%s/%s", dir, file);

   return nomefile;
}
