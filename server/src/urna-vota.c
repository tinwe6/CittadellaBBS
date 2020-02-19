/*
 *
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/***************************************************************************
* Cittadella/UX client            (C) M. Caldarelli and R. Vianello        *
*                                                                          *
* File : urna-vota.c                                                       *
*     funzioni per la gestione dei referendum e sondaggi.                  * 
****************************************************************************/

/* Wed Nov 12 23:32:28 CET 2003 */

#include <string.h>

#include "config.h"
#include "strutture.h"
#include "urna-strutture.h"
#include "utility.h"
#include "urna-servizi.h"
#include "urna-vota.h"
#include "urna_errori.h"
#include "parm.h"
#include "memstat.h"


int rs_vota(struct sessione *t, struct urna_conf *ucf);
void rs_add_vote(struct urna *u, struct sessione * t, char *ris);
int rs_add_proposta(struct urna *u, struct sessione* t);
void rs_add_slots(struct urna *u);
void rs_add_ques(struct urna_stat *ustat);
int rs_inizia_voto(struct sessione *t, int num);
int cerca_votante(int num, unsigned char flag);
int del_votante(int num, long int matricola);
int add_votante(int num,long int matricola);
void avverti_votante(long int matricola);
int rs_vota_ut(struct dati_ut *ut, struct urna_conf *ucf);


/* 
 * aggiunge un votante all'elenco
 * torna 0 se e` tutto ok,
 * o anche se e` gia` presente, 
 * -1 (o altro) se ci sono problemi
 * ustat.votanti (la testa) e` sempre !=NULL,
 * ustat.utente=-1
 * ustat.num=-1
 * ustat.time=-1
 * (semplifica le funzioni una cifra...
 */

int add_votante(int num,long int matricola){

		struct votanti * p, *q;


		p=ustat.votanti;
		q=p;

		while(p!=NULL && p->utente!=matricola){
				q=p;
				p=p->next;
		}

		if (p!=NULL){
				p->num=num;
				return 0;
		}

		CREATE(q->next, struct votanti,1,TYPE_VOTANTI);
		p=q->next;
		p->next=NULL;
		p->num=num;
		p->utente=matricola;
		p->inizio=time(NULL);
		return 0;
}

/*
 * di NUOVO
 * ustat.votanti e` sempre !=NULL,
 * ustat.utente=-1
 * ustat.num=-1
 * ustat.time=-1
 * (semplifica le funzioni una cifra...)
 * 
 * se num=-1
 * ritorna il numero dell'urna che t sta votando (e cancella)
 * o -1 se non sta votando
 *
 * se num>0 ritorna
 * 0 se t sta votando num (e cancella)
 * num se t non sta votando num ma qualcos'altro(e  cancella)
 * -1 se t non sta votando.
 */

int del_votante(int num, long int matricola){
		struct votanti *p,*q;
		int ret=0;

		p=ustat.votanti;
		while(p!=NULL &&p->utente!=matricola){
				q=p;
				p=p->next;
			};

		if (p==NULL)
				return -1;
		
		if(num>0 && p->num==num)
				ret=0;
		  else
				ret=p->num;

		q->next=p->next;
		Free(p);

		 return ret;
}

/*
 * cerca se qualcuno sta votando num
 * torna 0 se non vota nessuno
 * 1 se vota qualcuno
 * Se flag=1 allora avvisa i votanti
 */

int cerca_votante(int num,unsigned char flag){

		struct votanti *p;
		int ret;

		if (num==-1){
				return 0;
		}

		ret=0;
		for(p=ustat.votanti;p!=NULL;p=p->next){
				if(p->num==num){
					   if (flag){
							   ret=1;
							   avverti_votante(p->utente);
					   }
					   else
						   return 1;
				}
		}
		return ret;
};

int rs_inizia_voto(struct sessione *t, int num){

		int pos;
				struct urna_conf *ucf; 
				struct urna *u;

				if((pos=rs_trova(num))==-1){
						u_err(t,U_NOT_FOUND);
						return -1;
				}

				u=*(ustat.urna_testa+pos);
				ucf=u->conf;
				if(u->semaf&(SEM_U_CONCLUSA|SEM_U_PRECHIUSA)){
						u_err(t,URNA_CHIUSA);
						return -1;
				}

				 if(rs_vota(t, ucf))
						 return -1; 

				 if(rs_ha_votato(u, t->utente->matricola))
						 return -1; 

		 	if (add_votante(num, t->utente->matricola))
					return -1;
/*
					u->votanti++;
*/
			return 0; 
}

/*
 * controlla se l'utente
 * puo` votare ad un sondaggio
 * 0 se si`
 * -1 (o il motivo) altrimenti
*/


int rs_vota(struct sessione *t, struct urna_conf *ucf){

   struct dati_ut *ut;
   struct room *room_posted;
   
   ut=t->utente;

   if(!(room_posted = room_findn(ucf->room_num))) {
      citta_log("room per urna  NON trovata");
      return ROOM_NOT_FOUND;
   }
   if(ut->livello < room_posted->data->rlvl) {
      citta_log("livello non valido");
      return BAD_LEVEL;
   }

	if(room_known(t,room_posted)!=1){
			return UNKNOWN_ROOM;
	}

/*
 *  attenzione: nessuno dei livelli, escluso CV_UNIVERSALE, DEV'ESSERE
 *  0 
 *  Torna 0 su successo, il numero CV su insuccesso
 */

   switch (ucf->crit) {
   case CV_UNIVERSALE:
      break;
   case CV_LIVELLO:
      if(ut->livello < ucf->val_crit.numerico)
         return CV_LIVELLO;
      break;
   case CV_ANZIANITA:
      if(ut->firstcall > ucf->val_crit.tempo)
         return CV_ANZIANITA;
      break;
   case CV_CALLS:
      if(ut->chiamate < ucf->val_crit.numerico)
         return CV_CALLS;
      break;
   case CV_POST:
      if(ut->post < ucf->val_crit.numerico)
         return CV_POST;
      break;
   case CV_X:
      if(ut->x_msg < ucf->val_crit.numerico)
         return CV_X;
      break;
   case CV_SESSO:
      if(ut->sflags[0] & SUT_SEX) {
         if(ucf->val_crit.numerico == 1)
            return CV_SESSO;
      } else {
         if(ucf->val_crit.numerico == 0)
            return CV_SESSO;
      }
      break;
   default:
      return OTHER;
   }
   return 0;
}

/*
 * IDEM, dato però l'utente e non la struttura
 * controlla se l'utente
 * puo` votare ad un sondaggio
 * 0 se si`
 * -1 (o il motivo) altrimenti
*/

int rs_vota_ut(struct dati_ut *ut, struct urna_conf *ucf)
{
   struct room *room_posted;

   if(!(room_posted = room_findn(ucf->room_num))) {
      citta_log("room per urna  NON trovata");
      return ROOM_NOT_FOUND;
   }
   if(ut->livello < room_posted->data->rlvl) {
      citta_log("livello non valido");
      return BAD_LEVEL;
   }

   /* per ora non controlla...
    */
   /*
	if(room_known(t,room_posted)!=1){
			return UNKNOWN_ROOM;
	}

	*/
/*
 *  attenzione: nessuno dei livelli, escluso CV_UNIVERSALE, DEV'ESSERE
 *  0 
 *  Torna 0 su successo, il numero CV su insuccesso
 */

   switch (ucf->crit) {
   case CV_UNIVERSALE:
      break;
   case CV_LIVELLO:
      if(ut->livello < ucf->val_crit.numerico)
         return CV_LIVELLO;
      break;
   case CV_ANZIANITA:
      if(ut->firstcall > ucf->val_crit.tempo)
         return CV_ANZIANITA;
      break;
   case CV_CALLS:
      if(ut->chiamate < ucf->val_crit.numerico)
         return CV_CALLS;
      break;
   case CV_POST:
      if(ut->post < ucf->val_crit.numerico)
         return CV_POST;
      break;
   case CV_X:
      if(ut->x_msg < ucf->val_crit.numerico)
         return CV_X;
      break;
   case CV_SESSO:
      if(ut->sflags[0] & SUT_SEX) {
         if(ucf->val_crit.numerico == 1)
            return CV_SESSO;
      } else {
         if(ucf->val_crit.numerico == 0)
            return CV_SESSO;
      }
      break;
   default:
      return OTHER;
   }
   return 0;
}

/*
 * aggiunge un voto
 * per l'utente
 */

void rs_add_vote(struct urna *u, struct sessione * t, char *ris)
{
   struct urna_dati *udt;
   long j;
   long tot_voti, matricola;
   int modo;
#if URNA_GIUDIZI
   long voto;
#endif

   matricola=t->utente->matricola;

   modo = u->conf->modo;
   ustat.voti++;
   udt = u->dati;
   *(udt->uvot+(udt->nvoti)) = matricola;
   udt->nvoti++;
   /*
   u->votanti--;
   */


   if(ris[0] == '=') {
      udt->bianche++;
      return;
   }

   tot_voti = 0;

   /* 
	* considera a parte il caso 0 e il caso =,
	* se il voto e` zero il numero di voti per
	* la voce 'j' non aumente
	*
	* tot_voti non ha importanza tranne che
	* per il caso delle votazioni
	*/
   for(j = 0; j < udt->num_voci; j++) {
      if(ris[j] != '=' && ris[j] != '0') {
         (udt->voti + j)->num_voti++;
         (udt->voti + j)->tot_voti += ris[j] - '0';
         tot_voti++;
         if(modo == MODO_VOTAZIONE) {
#if URNA_GIUDIZI
            voto = ris[j] - '0';
            if(voto <= 9) {
               (*(((udt->voti + j)->giudizi)+voto))++;
            }
#endif
         }
      }
      if((ris[j] == '=') && (modo == MODO_VOTAZIONE)) {
         (udt->voti + j)->astensioni++;
      }
	/*
	* nel caso di votazioni il numero di voti
	* deve aumentare se il voto e` 0.
	*/
      if((ris[j] == '0') && (modo == MODO_VOTAZIONE)) {
         (udt->voti + j)->num_voti++;
         tot_voti++;
      }
   }
}

int rs_add_proposta(struct urna *u, struct sessione* t)
{
 struct urna_prop * upr;
 struct urna_dati * udt;
 int num_prop;

 num_prop=contapar(t->parm,"prop");
 
  
 CREATE(upr, struct urna_prop, 1, TYPE_URNA_PROP);
 CREATE(upr->proposte, char *, num_prop, TYPE_POINTER);
 upr->matricola=t->utente->matricola;
 upr->num=num_prop;
 if(pars2strsd(upr->proposte,t,"prop",MAXLEN_PROPOSTA,-num_prop)!=num_prop){
   citta_log("SYSLOG: errori nelle proposte");
   return -1;
 };


/*
 * andrebbero
 * unificati
 * con add_vote
 */

   ustat.voti++;
   udt = u->dati;
   *(udt->uvot+(udt->nvoti)) = t->utente->matricola;
   udt->nvoti++;
   /*
   u->votanti--;
   */
   upr->next=u->prop;
   u->prop=upr;

 /* controllino?
  */
return 0;
}


/* 
 * crea slots nuovi per voti/coll
 * e riempie con -1
 * perche' c'e` l'utente con matricola = 0!!!!!!!!!!!!!!!
 */

void rs_add_slots(struct urna *u)
{
	int i;
	int n;
	n=u->dati->voti_nslots;
   if(u->dati->nvoti >= n * LEN_SLOTS){
           RECREATE(u->dati->uvot, long, (n+1) * LEN_SLOTS);
           for(i=0;i<LEN_SLOTS;i++)
   		   	       *(u->dati->uvot+(n*LEN_SLOTS)+i)=-1;
	       u->dati->voti_nslots++;
   }

   n=u->dati->coll_nslots;
   if(u->dati->ncoll >= n * LEN_SLOTS){
		   RECREATE(u->dati->ucoll, long, (n+1) * LEN_SLOTS);
		   for(i=0;i<LEN_SLOTS;i++)
					*(u->dati->ucoll+(n*LEN_SLOTS)+i)=-1;
		   u->dati->coll_nslots++;
   }
}
/*
 * un po' inefficiente
 * visto che fa il giro delle
 * sessioni per ogni
 * votante.(forse anderebbe usato t->stato con searc_vote)
 */

void avverti_votante(long int matricola){

	char buf[]="861\n";
	int buflen;
	struct sessione *punto;

	buflen=strlen(buf);
	for (punto = lista_sessioni; punto; punto = punto->prossima)
		if ((punto->utente) &&punto->utente->matricola==matricola) {
			metti_in_coda(&punto->output, buf, buflen);
			punto->bytes_out += buflen;
		}
}	
