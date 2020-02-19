
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
* File: urna-to-ascii.c                                                     *
* --- stampa i contenuti dei file urna                                ---   *
****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "urna-file.h"
#include "urna-strutture.h"
#include "utility.h"
#include "urna-cost.h"
#include "urna_comune.h"
#include "urna-servizi.h"
#include "memstat.h"

int load_stat_ascii();
int load_file_ascii(char *file);
int load_urna(unsigned long progressivo);
struct urna_conf *load_conf(unsigned long progressivo);
struct urna_dati *load_dati(unsigned long progressivo, int modo);
struct urna_prop *load_prop(unsigned long progressivo, int modo);
int read_prop(unsigned long progressivo, int nvoti, char ***ptesto);
int res_proposta(unsigned long progressivo, int nvoti);
int load_voci(struct urna_conf *ucf, FILE * fp);
int compstr(char ** a, char **b);

extern FILE * stderr;
FILE * logfile;

/*
 ************************ LOAD *********************************************
 */

/* 
 *
 *
 */


int main(int argv, char **argc)
{
   int num;
   char file[201];

   printf("%d\n", argv);
   if(argv == 1) {
      printf("%s -a: print all\n%s n: print urna n\n", argc[0], argc[0]);
      printf("%s -s: print stat\n", argc[0]);
      return 0;
   };

   if(strcmp(argc[1], "-a") == 0) {
      printf("we're working on it!\n");
      return 0;
   };

   if(strcmp(argc[1], "-s") == 0) {
	   strcpy(file,"");
	   if(argv==3){
		  strncpy(file,argc[2],200);
      		  load_file_ascii(file);
	   } else {;
      		load_stat_ascii();
	   }
      return 0;
   }
   num = atoi(argc[1]);
   if(num <= 0) {
      return -1;
   }
   load_urna(num);
   return 0;
};

int load_stat_ascii()
{
   FILE *fp;
   struct urna_stat ustat;
   unsigned int letto;
   unsigned int i;
   unsigned long progressivo;
   unsigned long int alloc;
   unsigned long int *numeri_urne;

   switch (esiste(URNA_DIR, FILE_USTAT,0)) {
   case -1:
      printf("%s/%s non esiste\n", URNA_DIR,FILE_USTAT);
      return (1);
      break;
   case 1:
      printf("%s e` vuoto\n", FILE_USTAT);
      return (1);
      break;
   case 0:
	  break;
   default:
	  printf("altri errori su %s",FILE_USTAT);
   }

   letto = load_file(URNA_DIR, FILE_USTAT, -1, &fp);

   if(letto == -1) {
      printf("non posso aprire %s", FILE_USTAT);
      return (-1);
   }

   if(!(check_magic_number(fp, MAGIC_STAT, "stat"))) {
      printf("versione %s non corretta", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   if(fread(&ustat, sizeof(struct urna_stat), 1, fp) != 1) {
      printf("dati su %s corrotti", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   printf("\n");
   printf("totali:%ld\n", ustat.totali);
   printf("complete:%ld\n", ustat.complete);
   printf("attive:%ld\n", ustat.attive);
   printf("voti:%ld\n", ustat.voti);
   printf("ultima urna:%ld\n", ustat.ultima_urna);
   printf("ultima lettera:%d\n", ustat.ultima_lettera);
   printf("urne_slots:%ld\n", ustat.urne_slots);

   if(ustat.attive > LEN_SLOTS * MAX_SLOT || ustat.attive < 0) {
      printf("SYSLOG: dati su %s corrotti" 
            "(troppi slot/negativi!)", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   ustat.urne_slots = ustat.attive / LEN_SLOTS + 1;

   alloc = ustat.urne_slots * LEN_SLOTS;

   numeri_urne = (long int *)calloc(sizeof(long int), alloc);

   /* legge i numeri delle urne */

   if(fread(numeri_urne, sizeof(long int), alloc, fp) != alloc) {
      printf("SYSLOG: dati su %s corrotti (numeri_urne)\n", FILE_USTAT);
   }

   printf("\n");

   for(i = 0; i < ustat.attive; i++) {
      progressivo = *(numeri_urne + i);
      printf(" %ld", progressivo);
   }
   printf("\n");

   free(numeri_urne);
   return (0);
}

int load_file_ascii(char * file)
{
   FILE *fp;
   struct urna_stat ustat;
   unsigned int letto;
   unsigned int i;
   unsigned long progressivo;
   unsigned long int alloc;
   unsigned long int *numeri_urne;

   fp=fopen(file,"r");

   if(fp==NULL) {
      printf("non posso aprire %s", FILE_USTAT);
      return (-1);
   }

   if(!(check_magic_number(fp, MAGIC_STAT, "stat"))) {
      printf("versione %s non corretta", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   if(fread(&ustat, sizeof(struct urna_stat), 1, fp) != 1) {
      printf("dati su %s corrotti", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   printf("\n");
   printf("attive:%ld\n", ustat.attive);
   printf("totali:%ld\n", ustat.totali);
   printf("voti:%ld\n", ustat.voti);
   printf("complete:%ld\n", ustat.complete);
   printf("ultima urna:%ld\n", ustat.ultima_urna);
   printf("ultima lettera:%d\n", ustat.ultima_lettera);
   printf("urne_slots:%ld\n", ustat.urne_slots);

   if(ustat.attive > LEN_SLOTS * MAX_SLOT || ustat.attive < 0) {
      printf("SYSLOG: dati su %s corrotti" 
            "(troppi slot/negativi!)", FILE_USTAT);
      fclose(fp);
      return (-1);
   }

   ustat.urne_slots = ustat.attive / LEN_SLOTS + 1;

   alloc = ustat.urne_slots * LEN_SLOTS;

   numeri_urne = (long int *)calloc(sizeof(long int), alloc);

   /* legge i numeri delle urne */

   if(fread(numeri_urne, sizeof(long int), alloc, fp) != alloc) {
      printf("SYSLOG: dati su %s corrotti (numeri_urne)\n", FILE_USTAT);
   }

   printf("\n");

   for(i = 0; i < ustat.attive; i++) {
      progressivo = *(numeri_urne + i);
      printf(" %ld", progressivo);
   }
   printf("\n");

   free(numeri_urne);
   return (0);
}
int load_urna(unsigned long progressivo)
{
		struct urna_conf* ucf;
		struct urna_dati* udt;

   if(!load_conf(progressivo)) {
      return -1;
   };

   if(!(udt=load_dati(progressivo, 0))) {
      return -1;
   };
   if(!res_proposta(progressivo,udt->nvoti)){
		   return -1;
   }
   printf("SYSLOG: errore leggendo la conf di %ld\n", progressivo);
   return (0);
}

struct urna_conf *load_conf(unsigned long progressivo)
{
   FILE *fp;
   struct urna_conf *ucf;
   int i;

   if(load_file(URNA_DIR, FILE_UCONF, progressivo, &fp)) {
      printf("SYSLOG: non posso aprire file conf\n");
      return (NULL);
   }

   if(!check_magic_number(fp, MAGIC_CONF, "conf")) {
      printf("SYSLOG: versione %s non corretta\n", FILE_UCONF);
      fclose(fp);
      return (NULL);
   }

   /* leggi la struct */

   ucf = calloc(1, sizeof(struct urna_conf));

   if(fread(ucf, sizeof(struct urna_conf), 1, fp) != 1) {
      printf("SYSLOG: errore leggendo la conf di %ld\n", progressivo);
      fclose(fp);
      return (NULL);
   }

   ucf->voci = NULL;
   printf("*******CONF******\n");

   printf("progressivo:%ld\n", ucf->progressivo);
   printf("lettera:%d\n", ucf->lettera);
   printf("proponente:%ld\n", ucf->proponente);
   printf("room:%ld\n", ucf->room_num);
   printf("start:%ld\n", ucf->start);
   printf("stop:%ld\n", ucf->stop);
   printf("titolo:%s\n", ucf->titolo);
   printf("testo:\n");
   for(i = 0; i < MAXLEN_QUESITO; i++) {
      printf("\t:%s\n", ucf->testo[i]);
   }
   printf("num_prop:%d\n", ucf->num_prop);
   printf("num_voci:%d\n", ucf->num_voci);
   printf("max_voci:%d\n", ucf->max_voci);
   printf("max_len_prop:%d\n", ucf->maxlen_prop);
   printf("tipo:%d\n", ucf->tipo);
   printf("modo:%d\n", ucf->modo);
   printf("bianca:%d\n", ucf->bianca);
   printf("astensione:%d\n", ucf->astensione);
   printf("tipo_crit:%d\n", ucf->crit);
   printf("val:%ld\n", ucf->val_crit.numerico);
   ucf->voci = NULL;

   if(load_voci(ucf, fp)) {
      fclose(fp);
      return (NULL);
   }

   return ucf;
}

int load_voci(struct urna_conf *ucf, FILE * fp)
{
   size_t len;
   char *testo;
   int i;

   printf("voci:\n");

   for(i = 0; i < ucf->num_voci; i++) {
      if(fread(&len, sizeof(size_t), 1, fp) != 1)
         return -1;
      if(len > MAXLEN_VOCE)
         return -1;

      testo = calloc(len + 1, sizeof(char));

      if(fread(testo, sizeof(char), len, fp) != len)
         return (-1);
      printf("\t%d:%s\n", i, testo);
      free(testo);
   }
   return (0);
}

struct urna_voti *load_voti(unsigned long progressivo, int num_voci, int modo)
{

   FILE *fp;
   int i;
   struct urna_voti testa;

   printf("***voti:\n");

   if(load_file(URNA_DIR, FILE_UVOTI, progressivo, &fp)) {
      printf("SYSLOG: problemi con %s\n", FILE_UDATA);
      return (NULL);
   }

   if(!check_magic_number(fp, MAGIC_VOTI, "voti")) {
      printf("SYSLOG: urna-data %s %ld non corretto\n", FILE_UVOTI,
             progressivo);
      fclose(fp);
      return (NULL);
   }

   for(i = 0; i < num_voci; i++) {
      if(fread(&testa, sizeof(struct urna_voti), 1, fp) != 1) {
         printf("SYSLOG: problemi a leggere i voti %d... %ld\n", i,
                progressivo);
         continue;
      }
	  printf("\tvoce %i:\n",i);
      printf("\t\tnumvoti:%ld\n", testa.num_voti);
      printf("\t\ttot_voti:%ld\n", testa.tot_voti);
      printf("\t\tastensioni:%ld\n", testa.astensioni);

   }

   fclose(fp);
   return 0;
};

/*
 * per ora non fa nulla, se non 
 * mettere a NULL proposte
 * (in effetti ora e` ridicolo, ma e` gia` pronto per 
 * modifiche
 */

struct urna_prop *load_prop(unsigned long progressivo, int modo)
{
   struct urna_prop *upr;

   return (upr);
};

int res_proposta(unsigned long progressivo, int nvoti)
{
   FILE *pr;
   int j;
   int num_prop;
   int linea;
   char **testo;

   if(load_file(URNA_DIR, FILE_UPROP, progressivo, &pr)) {
      citta_logf("SYSLOG: problemi con %s", FILE_UDATA);
      return (-1);
   }

   if(!check_magic_number(pr, MAGIC_PROP, "prop")) {
      citta_logf("SYSLOG: urna-prop %s %ld non corretto", FILE_UPROP, progressivo);
      fclose(pr);
      return (-1);
   }
   num_prop = read_prop(progressivo, nvoti, &testo);
   linea=0;
   for(j = 0; j < num_prop; j++) {
      if(*(testo + j) == NULL)
         continue;
      printf("<fg=4;b>proposta %d:</b><fg=7> %s", linea, *(testo + j));
	  if(j%10==0){
	      printf("\n", linea, *(testo + j));
	  }
   }

   return 0;
};

struct urna_dati *load_dati(unsigned long progressivo, int modo)
{
   FILE *fp;
   unsigned long int alloc;
   struct urna_dati udt;
   long int *uvot, *ucoll;
   int i;

   if(load_file(URNA_DIR, FILE_UDATA, progressivo, &fp)) {
      printf("SYSLOG: problemi con %s\n", FILE_UDATA);
      return (NULL);
   }
   /*   */

   if(!check_magic_number(fp, MAGIC_DATA, "dati")) {
      printf("SYSLOG: urna-data %s %ld non corretto\n", FILE_UDATA,
             progressivo);
      fclose(fp);
      return (NULL);
   }


   if(fread(&udt, sizeof(struct urna_dati), 1, fp) != 1) {
      printf("SYSLOG: non riesco a leggere i dati\n");
      fclose(fp);
      return (NULL);
   }

   if(udt.nvoti > MAX_UTENTI) {
      printf("SYSLOG: non riesco a leggere i dati\n");
      fclose(fp);
      return (NULL);
   }

   printf("\n\n*******DATI******\n");
   printf("stop %ld\n",udt.stop);
   printf("posticipo %d\n",udt.posticipo);
   printf("bianche %ld\n",udt.bianche);
   printf("num_voci %d\n",udt.num_voci);
   printf("nvoti %ld\n",udt.nvoti);

   alloc = udt.voti_nslots * LEN_SLOTS;
   uvot = calloc(alloc, sizeof(long));

   if(fread(uvot, sizeof(long int), alloc, fp) != alloc) {
      printf("non scrivo i votanti \n");
   } else {
		   printf("ha votato: ");
		   for(i=0;i<alloc;i++){
				   printf("%ld ",*(uvot+i));
		   }
		   printf("\n");
	}

   printf("ncoll %ld\n",udt.ncoll);

   alloc = udt.coll_nslots * LEN_SLOTS;
   ucoll = calloc(alloc, sizeof(long));

   if(fread(ucoll, sizeof(long int), alloc, fp) != alloc) {
      printf("non scrivo i collegati \n");
   } else {
		   printf("si e` collegato: ");
		   for(i=0;i<alloc;i++){
				   printf("%ld ",*(ucoll+i));
		   }
		   printf("\n");
	}

   free(ucoll);
   free(uvot);
   printf("voti_slot%ld\n",udt.voti_nslots);
   printf("coll_slot %ld\n",udt.coll_nslots);

   if(!load_voti(progressivo, udt.num_voci, modo))
      return (NULL);

   return 0;
}

int read_prop(unsigned long progressivo, int nvoti, char ***ptesto)
{
   FILE *fp;
   char c;
   char **testo;
   char proposta[MAXLEN_PROPOSTA];
   int i, j,n,prog;
   unsigned long int matricola;
   size_t lent;
   int slots, num_proposte, nextc;


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
   slots=1; 
   /*CREATE(testo, char **, slots*LEN_SLOTS, TYPE_POINTER);
	* */

   testo=(char **)calloc(sizeof( char **), slots*LEN_SLOTS);


	for(i=0;i<nvoti;i++){
       while((c=fgetc(fp))=='\n');

  	     if(c!=':')
			citta_logf("mancano i duepunti:%d",i);

			if(fscanf(fp,":%ld:%d\n",&matricola,&num_proposte)!=2){
				citta_logf("qualcosa non va nella lettura delle urne... pos %d",i);
					continue;
			}


/* 
      txt_putf(txt, "utente %d:", i);
 * matricola...
 */
			for(j=0;j<num_proposte;j++){
				fscanf(fp,"%d:",&prog);
				nextc=' ';
				/* salta tutti i bianchi */

				while(nextc==' ')
					nextc=fgetc(fp);
				if(nextc=='\n'){
					citta_logf("??, proposta vuota %d, posto %d->%d",i,j,prog);
					continue;
				}
				ungetc(nextc,fp);
				/* fscanf(fp,"%s\n",proposta); */
				if(fgets(proposta,MAXLEN_PROPOSTA,fp)==NULL){;
					citta_logf("??, proposta vuota %d, posto %d->%d",i,j,prog);
					continue;
				};
				if (prog!=j){
					citta_logf("??, proposta %d, posto %d->%d",i,j,prog);
					continue;
				}
				lent=strlen(proposta);
/*   	    		CREATE(*(testo+n), char, lent+1, TYPE_CHAR);
 */
				*(testo+n)=(char*)malloc((lent+2)*sizeof(char));
				strncpy(*(testo+n),proposta,lent+1);
				n++;
        		if(n>=slots*LEN_SLOTS){
           			slots++;
   				testo=(char **)realloc(testo, slots*LEN_SLOTS*sizeof(char*));
       			}
			}
      }

   fclose(fp);
   qsort(testo,n,sizeof(char *),(void *)compstr);
   *ptesto=testo;
   return n;
};

int compstr(char ** a, char **b){
		return strcasecmp(*a,*b);
}

