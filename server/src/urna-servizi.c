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
* File: urna-servizi.c                                                      *
* Servizi vari                                                              *
*                                                                           *
****************************************************************************/

/* Wed Nov 12 23:23:59 CET 2003 */

#include <string.h>
#include "config.h"
#include "memstat.h"
#include "strutture.h"
#include "urna-crea.h"
#include "urna-fine.h"
#include "urna-servizi.h"
#include "urna-strutture.h"
#include "urna-vota.h"
#include "urna_errori.h"
#include "utility.h"

void posta_nuovo_quesito(struct urna_conf *ucf);
void scrivi_scelte(struct urna_conf *ucf, struct text *avviso);
void rs_expire(void);
int rs_trova(long progressivo);
struct urna *rs_trova_lettera(char strlettera[3]);
int rs_ha_votato(struct urna *u, long user);
int rs_ha_visto(struct urna *u, long user);
void chipuo(struct urna_conf *ucf, char *buf);
void cod_lettera(unsigned char lettera, char * let);
unsigned char lettera_cod(char *stringa);
void dealloca_urna(struct urna *u);
void deall_prop(struct urna_prop *upr);
void deall_dati(struct urna_dati *udt);
void deall_conf(struct urna_conf* ucf);
void rs_free_data();
void rs_free_urne();
void rs_free_stat();
void u_err(struct sessione *t,int err);
void u_err2(struct sessione *t,int err,int lev);

#if U_DEBUG
void dump_prop(struct urna_prop *upr);
void dump_conf(struct urna_conf *upr);
void dump_dati(struct urna_dati *upr);
int rs_dump_voci(char **voci, int num_voci);
int rs_dump_voti(struct urna_voti *uvt);
int rs_dump_dati(struct urna_dati *udt);
int rs_dump_prop(struct urna_prop *upr);
void dump_stat();
#endif


void rs_expire(void)
{
   struct urna *u;
   time_t ora;
   time_t ferma;
   int i;

   time(&ora);
   if(ustat.urna_testa==NULL){
         return;
   };
   for(i=0; i<ustat.urne_slots*LEN_SLOTS; i++){
      u=*(ustat.urna_testa+i);
      if(u==NULL)
            continue;
      if(u->dati==NULL)
            continue;

      ferma=u->dati->stop;
      if(cerca_votante(u->progressivo,0)!=0){
            ferma+=URNA_LASCIA_VOTARE;
      }
	  /* 
	   * se adesso e` prima di ferma 
	   * e l'urna non e` stata prechusa continua
	   */

      if((ora < ferma)&& !(u->semaf&SEM_U_PRECHIUSA)){
			  continue;
	  }

	  /* 
	   * se ci sono votanti 
	   * ma non  e` stata gia` conclusa (quindi l tempo per votare
	   * e` non e` passato)
	   * allora  la definisce conclusa
	   * (la chiude il giro dopo...)
	   */

          if(cerca_votante(u->progressivo,1)!=0 &&(!(u->semaf&SEM_U_CONCLUSA))){
             u->semaf|=SEM_U_CONCLUSA;
             continue;
        }

          u->semaf|=SEM_U_CONCLUSA;
          rs_end(u);
          rs_del(u);
        *(ustat.urna_testa+i)=NULL;
        ustat.modificata=1;
      }
}

/*
 * Restituisce la posizione del puntatore 
 * alla struttura urna del sondaggio #num.
 * 
 */

int rs_trova(long progressivo)
{
   struct urna **u;
   int i;

   if(ustat.urna_testa==NULL){
         return -1;
   };

   for(i=0;i<ustat.urne_slots*LEN_SLOTS;i++){
      u = ustat.urna_testa+i;
      if((*u)!=NULL && (*u)->progressivo == progressivo){
         return i;
     }
   }
   return -1;
}

#if 0
struct urna *rs_trova_lettera(char strlettera[3])
{
   struct urna *u;
   unsigned char lettera;
   int i;

   lettera=lettera_cod(strlettera);

   if(ustat.urna_testa==NULL){
         return NULL;
   };
   for(i=0;i<ustat.urne_slots*LEN_SLOTS;i++){
      u = *(ustat.urna_testa+i);
      if(u!=NULL && u->conf->lettera == lettera)
         return u;
   }
   return NULL;
}

#endif
/*
 * 0 se l'utente non ha votato
 * 1 se ha votato
 */

int rs_ha_votato(struct urna *u, long user)
{
   long i;

   for(i = 0; i < u->dati->nvoti ; i++) {
      if(u->dati->uvot[i] == user) {
     return 1;
      }
   }
   return 0;
}
/* 
 * torna 1 se l'utene ha visto
 * l'urna
 * 0 altrimenti
 * e l'aggiunge
 * (non controlla se l'utente puo` votare
 * ma per ora lo fa chi chiama)
 */

int rs_ha_visto(struct urna *u, long user)
{
   long i;

   for(i = 0; i < u->dati->ncoll; i++) {
      if(u->dati->ucoll[i] == user) {
         return 1;
      }
   }
     rs_add_slots(u);
     u->dati->ucoll[u->dati->ncoll]=user;
     u->dati->ncoll++;
   return 0;
}

/*
 * da`  let, dato lettera
 */

void cod_lettera(unsigned char lettera, char * let){
      char num;
      
      if (lettera>0 && lettera<=26){
            let[0]='a'+(lettera-1);
            let[1]=0;
            let[2]=0;
            return;
      }
      num=lettera/26;
      
      let[0]='a'+(lettera%26-1);
      sprintf(let+1,"%1d",num);
      return;
}

/* 
 * e viceversa
 */

unsigned char lettera_cod(char *stringa){
      char * strnum;
      if (strlen(stringa)==1){
            return ((unsigned char)stringa[0]-'a'+1);
      };
      strnum=stringa+1;
      return (((char)stringa[0]-'a'+1)+26*atoi(strnum));
}

/* 
 * prende una struttura urna
 * cerca di deallocare il deallocabile
 */

void dealloca_urna(struct urna *u){
      deall_prop(u->prop);
      u->prop=NULL;
      deall_dati(u->dati);
      u->dati=NULL;
      deall_conf(u->conf);
      u->conf=NULL;
};

void deall_prop(struct urna_prop *upr){
      int i;
      struct urna_prop * testa;
      struct urna_prop * old;
      old=NULL;

      testa=upr;
      while(testa){
            if (testa->proposte!=NULL){
               for(i=0;i<testa->num;i++)
                     if((*(testa->proposte+i)))
                   Free(*(testa->proposte+i));

               Free (testa->proposte);
            };
            old=testa;
            testa=testa->next;
            Free(old);
      }
      /* upr e` liberato all'inizio*/
}


void deall_dati(struct urna_dati *udt){
   
      if(udt==NULL){
            return;
      }
                   
      if (udt->uvot!=NULL){
            Free(udt->uvot);
      };
      if (udt->ucoll!=NULL){
            Free(udt->ucoll);
      };

#if URNA_GIUDIZI
      for(i=0;i<udt->num_voci;i++)
            if(((udt->voti)+i)->giudizi!=NULL)
                  Free(((udt->voti)+i)->giudizi);
#endif

      Free(udt->voti);
      Free(udt);
};

void deall_conf(struct urna_conf* ucf){

      int i;
      
      if(ucf==NULL){
            return;
      }

      if (ucf->voci!=NULL){
            for(i=0;i<ucf->num_voci;i++)
                  if(*(ucf->voci+i)!=NULL)
                        Free(*(ucf->voci+i));

            Free(ucf->voci);
      };
      Free(ucf);
};

int rs_del_conf(struct urna_conf *ucf)
{
   rs_del_voci(ucf->voci, ucf->num_voci);
   Free(ucf);
   return 0;
};

int rs_del_voci(char **voci, int num_voci)
{
  int i;

 for(i=0;i<num_voci;i++){
       Free(*(voci+i));
 }
 Free(voci);
   return 0;
}

int rs_del_voti(struct urna_voti *uvt)
{
   if(uvt == NULL) {
      return 0;
   };
   Free(uvt);
   return 0;
};

int rs_del_dati(struct urna_dati *udt)
{
   Free(udt->uvot);
   Free(udt->ucoll);
#if 0
   for(i=0;i<udt->num_voci;i++){
         if((udt->voti+i)!=NULL)
               if((udt->voti+i)->giudizi!=NULL)
               Free((udt->voti+i)->giudizi);
   };
#endif
   Free(udt->voti);
   Free(udt);
   return 0;
};

int rs_del_prop(struct urna_prop *upr)
{
   struct urna_prop *testa;
   char **testapr;
   struct urna_prop *q;
   int i;

   testa = upr;
   while(testa) {
      q = testa;
      testapr = testa->proposte;
      for(i = 0; i < testa->num; i++) {
         Free(testapr + i);
      };
      testa = q->next;
      Free(q);
   };
   return 0;
};

#if U_DEBUG
void dump_stat(){
      int i;
      citta_logf("SYSLOG: stat: attive %d, totali %d, modificata %d,\n"
	   "                  voti %d, complete %d, ultima urna %d,\n"
	   "                  ultima lettera %d, urne_slots %d, testa %p",
      ustat.attive , ustat.totali, ustat.modificata, 
      ustat.voti, ustat.complete, ustat.ultima_urna, 
      ustat.ultima_lettera, ustat.urne_slots, ustat.urna_testa);
      for (i=0;i<ustat.urne_slots*LEN_SLOTS;i++){
            citta_logf("testa+%d=%p",i,*(ustat.urna_testa+i));
      }
}

void dump_urna(struct urna *u){

      citta_logf("SYSLOG: urna %d",u->progressivo);
      dump_conf(u->conf);
      dump_dati(u->dati);
      dump_prop(u->prop);
	  return;
};

void dump_prop(struct urna_prop *upr){
      int i;
      struct urna_prop * testa;
      struct urna_prop *  old;
      old=NULL;

      testa=upr;
      if(testa==NULL){
            citta_logf("SYSLOG: proposte nulle");
      };
      while(testa){
            if (testa->proposte!=NULL){
                  citta_logf("SYSLOG:\t p %d",testa->matricola);
               for(i=0;i<testa->num;i++){
                if((*testa->proposte+i))
                  citta_logf("SYSLOG:\t\t i:%d,%s",i,(*testa->proposte+i));
               }
            };
            old=testa;
            testa=testa->next;
      }
}


void dump_dati(struct urna_dati *udt){
      int i;
      struct urna_voti *uvt;
   
      if(udt==NULL){
            log("SYSLOG: data nullo");
            return;
      }

      citta_log("SYSLOG: data");
      citta_logf("SYSLOG: stop %ld, posticipo %d," 
                  " bianche %d, nvoti %d, voti_nslot %d,"
               "ncoll %d, udt->coll_nslot %d", udt->stop , udt->posticipo,
               udt->bianche , udt->nvoti, udt->voti_nslots, udt->ncoll, 
            udt->coll_nslots);

      citta_logf("SYSLOG: Votanti");
      if (udt->uvot!=NULL)
            for(i=0;i<udt->nvoti;i++)
            citta_logf("SYSLOG:%d", *((udt->uvot)+i));

      citta_logf("SYSLOG: Vedenti");
      if (udt->ucoll!=NULL)
            for(i=0;i<udt->ncoll;i++)
            citta_logf("SYSLOG:%d", *((udt->uvot)+i));

      for(i=0;i<udt->num_voci;i++){
            uvt=udt->voti+i;
            citta_logf("SYSLOG: Voti %i: num_voti %d, tot_voti %d, astensioni %d",
                        i, uvt->num_voti,uvt->tot_voti,
                        uvt->astensioni);
      }
};

void dump_conf(struct urna_conf* ucf){

      int i;
      
      if(ucf==NULL){
      log("SYSLOG: data");
            return;
      }

      citta_logf("SYSLOG: progressivo %ld, lettera %d, proponente %ld,\n"
	   "		 room_num %ld, start %ld, stop %ld, titolo %s, ",
	   ucf->progressivo , ucf->lettera , ucf->proponente , 
	   ucf->room_num , ucf->start, ucf->stop, ucf->titolo);

      citta_logf("SYSLOG:num_prop %d, num_voci %d, max_voci %d,\n"
	   "		  maxlen_prop %d, tipo %d, modo %d, bianca %d, ",
	   ucf->num_prop, ucf->num_voci , ucf->max_voci, 
	   ucf->maxlen_prop, ucf->tipo, ucf->modo, ucf->bianca);

      citta_logf("SYSLOG:astensione %d, crit %d, val_crit %ld",
	   ucf->astensione, ucf->crit, ucf->val_crit.numerico);

      citta_log("SYSLOG: testo");
      for(i=0;i<MAXLEN_QUESITO;i++){
      citta_logf("SYSLOG: testo %s", ucf->testo[i]);
      };

      if (ucf->voci!=NULL)
            for(i=0;i<ucf->num_voci;i++)
                  citta_logf("SYSLOG: voce %d %s",i,*(ucf->voci+i));
};
#endif

void rs_free_data(){
		rs_free_urne();
		rs_free_stat();
};

void rs_free_urne(){

	int i;
   struct urna *punto;

   for(i = 0; i < LEN_SLOTS * (ustat.urne_slots); i++) {
      punto = *(ustat.urna_testa + i);
	  if (punto==NULL)
			  continue;
      dealloca_urna(punto);
	  Free(punto);
	}
}

void rs_free_stat(){
		struct votanti *p,*q;

		Free(ustat.urna_testa);
		p=ustat.votanti;
		while(p!=NULL){
				q=p->next;
				Free(p);
				p=q;
		}
};

/*
*ERRORI
*/




void u_err(struct sessione *t,int err){
		cprintf(t,"%d %d\n",ERROR,err);
}

void u_err2(struct sessione *t,int err, int lev){
        /* cprintf(t,"%d %d\n",ERROR,err); */
		cprintf(t,"%d %d %dlev\n",ERROR,err,lev);
}
