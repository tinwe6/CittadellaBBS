/*
 *  Copyright (C) 2003-2003 by Marco Caldarelli and Riccardo Vianello
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
* File: parm.c                                                              *
*       funzione per la trasmissione dei parametri dei comandi del server   *
****************************************************************************/

#include "conn.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extract.h"
#include "parm.h"
#include "utility.h"
#include "cittaclient.h"
#include "urna-strutture.h"
#include "macro.h"

/* Prototipi delle funzioni in questo file */
void parm_txt(struct parameter *t);
void parm_free(struct parameter *t);
void cmd_parm(struct urna_client *dati, char *str);

/*****************************************************************************/
/* Inizializza la ricezione di un testo.                                     */
/*****************************************************************************/

void parm_txt(struct parameter *p)
{
        if (p != NULL)
		parm_free(p);
}

/* Libera la lista dei parametri e torna in stato CON_COMANDI, non occupato. */

void parm_free(struct parameter *p)
{
	struct parameter *next;

	while (p) {
		Free(p->val);
		next = p->next;
		Free(p);
		p = next;
	}
}

/* Riceve un nuovo parametro, e lo inserisce in un nuovo elemento della lista
 * dei parametri ricevuti dal server.                                        */

void cmd_parm(struct urna_client *dati, char *str)
{
	struct parameter *newparm;

       	newparm = (struct parameter *)calloc(1, sizeof(struct parameter));
       	newparm->next = dati->parm;
       	extractn(newparm->id, str, 0, MAXLEN_PARM);
       	newparm->val = extracta(str, 1);
       	dati->parm = newparm;
}

/*
 * prende tutti i * PARM
 * si conclude con * PEND
 * se non finisce con PEND
 * torna -1
 * altrimenti il # di parametri letti
 * (al max MAXPAR)
 */
/* TODO: MAXPAR non implementato */
int get_parm(struct urna_client  *dati)
{
        int num = 0;
	char buf[LBUF];

       	serv_gets(buf);

       	if (strncmp(buf + 4, "PEND", 4) == 0) {
	        return 0;
	}

       	while(strncmp(buf + 4, "PARM", 4) == 0) {
	       	num++;
	       	cmd_parm(dati, buf + 9);
	       	serv_gets(buf);
	}

	if (strncmp(buf + 4, "PEND", 4) == 0) {
       		return num;
	} else {
       		return -1;
	}
}


/* 
 * cerca il primo parametro con id=id successivo a p (compreso)
 * ==== 0 se l'ha trovato
 * ==== -1
 * ==== p punta al parametro trovato se e` stato trovato,
 * ====altrimenti rimane quello chiamato
 */

int cercapar(struct parameter *p, char *id)
{
   struct parameter *q;

   q = p;
   while(q) {
      if(strcmp(q->id, id) == 0)
         break;
      q = q->next;
   };

   if(q == NULL) {
      return -1;
   } else {
      p = q;
      return 0;
   }
}

/*
 * conta il numero di parametri=id dopo p compreso
 */
int contapar(struct parameter *p, char *id)
{
    struct parameter *q;
    int tot = 0;

    q = p;
    while(q) {
        if(strcmp(q->id, id) == 0)
	    tot++;
	q=q->next;
    }
    return tot;
}


/*
 * copia il primo  parametro con id=id  in s (con max_char char)
 * dopo p (p compreso)
 * === p punta al val trovato o al valore di partenza. 
 * === 0 se e` stato trovato 
 * -1 se non c'e`
 * length se la lunghezza e` > max
 */

long int par2str(char *s, struct parameter *p, char *id, int max)
{
   struct parameter *q;

   q = p;
   while(q) {
      if(strcmp(q->id, id) == 0)
         break;
      q = q->next;
   };

   if(q == NULL) {
      return -1;
   };

   if(strlen(q->val) > max) {
      strncpy(s, q->val, max - 1);
      strcat(s, "");
      p = q;
      return strlen(q->val);
   }
   strcpy(s, q->val);
   p = q;
   return 0;
}

/*
 * copia tutti i parametri con id=id successivi a p (compreso) in ss,
 * che e` vettore di stringhe, fino ad un massimo di
 * max_par e ad un massimo di char_max ciascuna.
 * torna 
 *** -(par_copiati+1) (e l'ultimo non viene copiato) e p==ultimo da
 *    copiare al primo parametro che supera max_char
 ***  il numero di par copiati con par=successivo se ce
 *    ne sono ancora, p=NULL altrimenti,
 *** 0 se non l'ha trovato( p rimane lo stesso)
 */

int pars2strs(char **s, struct parameter *p, char *id, int max_char,
			   	int max_par)
{

   struct parameter *q;
   int par = 0;

   q = p;
   while(q) {
      if(strcmp(p->id, id) == 0) {
         if(strlen(q->val) > max_char) {
            p = q;
            return -(par + 1);
         }
         strcpy(*(s + par), q->val);
         par++;
         if(par > max_par) {
            p = q;
            return par;
         };
      };
      q = q->next;
   };

   if(par != 0) {
      p = NULL;
   };
   return par;
};

/*
 * copia tutti i parametri con id=id successivi a p (compreso)
 * in s, che e` un'unica stringa, separati da sep
 * fino ad un massimo di max_par e ad un massimo di char_max
 * se max_char viene raggiunto
 * torna -(par_copiati+1) (l'ultimo non viene copiato)
 * e p==ultimo da copiare
 *
 * torna il numero di par copiati
 * con par=successivo se ce ne sono ancora
 * p=NULL altrimenti
 * 0 se NON l'ha trovato (!!!!!!)
 * p rimane lo stesso
 */

int pars2str(void *s, struct parameter *p, char *id, int max_par, int max_len, char *sep)
{

   struct parameter *q;
   int len = 0;
   int tot_len = 0;
   int par = 0;
   int len_sep = 0;

   q = p;
   len_sep = strlen(sep);

   while(q) {
      if(strcmp(q->id, id) == 0) {
         len = strlen(p->val);
         if(tot_len + len + len_sep > max_len) {
            return -(par + 1);
         }
         strcat(s, sep);
         strcat(s, p->val);
         tot_len += len + len_sep;
         par++;
         if(par > max_par) {
            return par;
         };
      }
      q = q->next;
   }
   if(par != 0) {
      p = NULL;
   };
   return par;
};

/*
 * stesse funzioni (tranne la prima)
 * ma cancella tutti i parametri
 * copiati (non quello che fa sballare il max!)
 * inoltre bisogna usare sessione *t, 
 * perche' se e` il primo a dover essere cancellato
 * serve il "next"
 */

/*
 * copia il primo  parametro con id=id  in s (con max_char char)
 * === 0 se e` stato trovato 
 * -1 se non c'e`
 * length se la lunghezza e` > max
 * se la lunghezza e` giusta cancella il parametro
 */

long int par2strd(char *s, struct urna_client *dati,char *id, int max_char)
{

   struct parameter *q;
   struct parameter *r;         /*  struct prec */

   q=dati->parm;
   r = NULL;

   while(q) {
      if(strcmp(q->id, id) == 0)
         break;
      r = q;
      q = q->next;
   };

   if(q == NULL) {
      return -1;
   };

   if(strlen(q->val) > max_char) {
      strncpy(s, q->val, max_char - 1);
      strcat(s, "");
      return strlen(q->val);
   }

   strcpy(s, q->val);

   if(r == NULL) {
      dati->parm = q->next;
   } else {
      r->next = q->next;
   };
   Free(q->val);
   Free(q);
   return 0;
}

/*
 * copia tutti i parametri con id=id  in ss,
 * che e` vettore di stringhe, fino ad un massimo di
 * max_par e ad un massimo di char_max ciascuna.
 * torna 
 *** -(par_copiati+1) (e l'ultimo non viene copiato) 
 *   se c'e` un parametro che supera max_char
 *   
 ***  max_par+1 se ce ne sono ancora,
 *** il numero di par copiati se tutto va bene
 *
 *** 0 se non l'ha trovato
 * cancella tutti i parametri che sono stati copiati correttamente
 * se max_par<0 crea gli ss (ma ss deve essere gia` creato)
 */


int pars2strsd(char **ss, struct parameter *p, char *id, 
				int max_char, int max_par)
{
   struct parameter *q;
   struct parameter *r;
   int par = 0;
   int mp;
   int len;
   mp=max_par;
   if(mp<0){
		   mp=-mp;
   };

   q = p;
   r = NULL;
   while(q) {
      if(strcmp(q->id, id) == 0) {
		 if(par >= mp)
			 return mp+1;
		 len=strlen(q->val);
         if(len > max_char) {
            return -(par + 1);
         }
		 if(max_par<0)
				 *(ss+par)=(char*)calloc(len+1,sizeof(char));

         strcpy(*(ss + par), q->val);
         par++;
         if(r == NULL) {
            p = q->next;
			Free(q->val);
			Free(q);
			q=p;
			continue;
         } else {
            r->next = q->next;
			Free(q->val);
			Free(q);
			q=r->next;
         };
      };
      r = q;
      q = q->next;
   };

   return par;
};

/*
 * copia tutti i parametri con id=id successivi a p (compreso)
 * in s, che e` un'unica stringa, separati da sep
 * fino ad un massimo di max_par e ad un massimo di char_max
 * se max_char viene raggiunto
 * torna -(par_copiati+1) (l'ultimo non viene copiato)
 * e p==ultimo da copiare
 *
 * torna il numero di par copiati
 * con par=successivo se ce ne sono ancora
 * p=NULL altrimenti
 * 0 se NON l'ha trovato (!!!!!!)
 * p rimane lo stesso
 * stesso discorso per del
 */

int pars2strd(void *s, struct parameter *p, char *id, int max_par, int max_len, char *sep)
{

   struct parameter *q;
   struct parameter *r;
   int len = 0;
   int tot_len = 0;
   int par = 0;
   int len_sep = 0;

   q = p;
   r = NULL;
   len_sep = strlen(sep);

   while(q) {
      if(strcmp(q->id, id) == 0) {
         if(par >= max_par) 
            return max_par;
         len = strlen(q->val);
         if(tot_len + len + len_sep > max_len) {
            return -(par + 1);
         }
         strcat(s, sep);
         strcat(s, q->val);
         tot_len += len + len_sep;
         par++;
         if(r == NULL) {
            p = q->next;
			Free(q->val);
			Free(q);
			q=p;
			continue;
         } else {
            r->next = q->next;
			Free(q->val);
			Free(q);
			q=r->next;
         };
      }
      q = q->next;
   }
   return par;
};

int printpar(struct parameter *p){
		int n;
		struct parameter *q;
	
		q=p;
		n=1;	
		printf("*****************\n");
		while (q){
				printf("%d:%s->%s\n",n,q->id,q->val);
				q=q->next;
				n++;
		}
		printf("*****************\n");
		return n;
}
