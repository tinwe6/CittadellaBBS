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
#include <stdlib.h>
#include "extract.h"
#include "memstat.h"
#include "parm.h"
#include "utility.h"
#include <string.h>

/* Prototipi delle funzioni in questo file */
void parm_txt(struct sessione *t);
void parm_free(struct sessione *t);
void cmd_parm(struct sessione *t, char *str);

/*****************************************************************************/
/* Inizializza la ricezione di un testo.                                     */
void parm_txt(struct sessione *t)
{
        if ((t->parm) != NULL)
		parm_free(t);
        cprintf(t, "%d\n", INVIA_PARAMETRI);
}

/* Libera la lista dei parametri e torna in stato CON_COMANDI, non occupato. */
void parm_free(struct sessione *t)
{
	struct parameter *next;

	while (t->parm) {
		Free(t->parm->val);
		next = t->parm->next;
		Free(t->parm);
		t->parm = next;
	}
	t->parm_com = 0;
	t->parm_num = 0;
        t->stato = CON_COMANDI;
        t->occupato = 0;
}

/* Riceve un nuovo parametro, e lo inserisce in un nuovo elemento della lista
 * dei parametri ricevuti dal server.                                        */
void cmd_parm(struct sessione *t, char *str)
{
	struct parameter *newparm;

        if ((t->utente) && (t->parm_com) && (t->parm_num < MAXNUM_PARM)) {
		CREATE(newparm, struct parameter, 1, TYPE_PARAMETER);
		newparm->next = t->parm;
		extractn(newparm->id, str, 0, MAXLEN_PARM);
		newparm->val = extracta(str, 1);
		t->parm = newparm;
		t->parm_num++;
        } else
		cprintf(t, "%d\n", ERROR);
}

/* Abort PaRaMeter: interrompe l'invio dei parametri per il comando corrente.*/
void cmd_aprm(struct sessione *t)
{
	cprintf(t, "%d\n", OK);
	parm_free(t);
}

#if 0
/*
 * Inizializza l'invio di un broadcast
 */
void cmd_SMPO(struct sessione *t)
{
	init_txt(t);
	t->text_com = TXT_BROADCAST;
	t->text_max = MAXLINEEBX;
	t->stato = CON_BROAD;
	t->occupato = 1;
}

/*
 * Fine broadcast: se tutto e' andato bene lo invia.
 */
void cmd_bend(struct sessione *t)
{
        char buf[LBUF], header[LBUF];
        int a, righe;

	if (t->text_com != TXT_BROADCAST) {
                cprintf(t, "%d\n", ERROR+X_TXT_NON_INIT);
	} else if ((righe = txt_len(t->text)) == 0) {
                cprintf(t, "%d\n", ERROR+X_EMPTY);
        } else {
                cprintf(t, "%d\n", OK);
		__bx_header(t, header, BHDR);
		__broadcast(t, header);
		txt_rewind(t->text);
		for (a = 0; a < righe; a++) {
			sprintf(buf,"%d %s\n", BTXT, txt_get(t->text));
			__broadcast(t, buf);
		}
		sprintf(buf, "%d\n", BEND);
		__broadcast(t, buf);
		dati_server.broadcast++;
	}
	__reset_text(t);
}
#endif



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
   int tot=0;

   q = p;
   while(q){
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
 * copiati (non quello che fa sballare il max, ma gli cambia id!)
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
 * altrimenti gli cambia id
 */

long int par2strd(char *s, struct sessione *t, char *id, int max_char)
{

   struct parameter *q;
   struct parameter *r;         /*  struct prec */
   char newid[MAXLEN_PARM];

   q = t->parm;
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
	  strncpy(newid,"bad-", MAXLEN_PARM);
	  strncat(newid,q->id, MAXLEN_PARM-5);
	  strncpy(q->id,newid,MAXLEN_PARM);
      return strlen(q->val);
   }
   strcpy(s, q->val);
   if(r == NULL) {
      t->parm = q->next;
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


int pars2strsd(char **ss, struct sessione *t, char *id, 
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

   q = t->parm;
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
				 CREATE(*(ss+par),char,len+1,TYPE_CHAR);

         strcpy(*(ss + par), q->val);
#if U_DEBUG
   citta_logf("SYSLOG: in parm %d: %s",par,*(ss+par));
#endif
         par++;
         if(r == NULL) {
            t->parm = q->next;
			Free(q->val);
			Free(q);
			q=t->parm;
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

int pars2strd(void *s, struct sessione *t, char *id, int max_par, int max_len, char *sep)
{

   struct parameter *q;
   struct parameter *r;
   int len = 0;
   int tot_len = 0;
   int par = 0;
   int len_sep = 0;

   q = t->parm;
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
            t->parm = q->next;
			Free(q->val);
			Free(q);
			q=t->parm;
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

int printpar(struct sessione *t)
{
        struct parameter *p;
        int n;
	
        n=1;	
        p=t->parm;
        while (p){
                citta_logf("%d:%s->%s\n",n,p->id,p->val);
                p=p->next;
                n++;
        }
        return n;
}
