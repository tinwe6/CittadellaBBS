/* Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File: urna-crea.c                                                         *
*                                                                           *
* Creazione-Cancellazione-Modifica di un referendum                         *
****************************************************************************/
/* Wed Nov  5 00:11:32 CET 2003 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "debug.h"
#include "memstat.h"
#include "parm.h"
#include "urna-cost.h"
#include "urna-crea.h"
#include "urna-file.h"
#include "urna-fine.h"
#include "urna-posta.h"
#include "urna-vota.h"
#include "urna-rw.h"
#include "urna-servizi.h"
#include "urna-strutture.h"
#include "urna_comune.h"
#include "utility.h"
#include "x.h"
#include "extract.h"
#include "urna_errori.h"

int init_urna(struct sessione *t);
int rs_new(struct sessione *t, char *buf, char *lettera);
int rs_ucf_parm(struct urna_conf *ucf, struct sessione *t);
struct urna **add_urne_slot();

/* 
 * accetta la creazione di un nuovo referendum
 * cerca in ut_definende se l'utente sta gia` creando, e nel
 * caso lo sostituisce.
 * nel frattempo fa un po' di pulizia
 * 0 se tutto e` ok
 * entra nello stato get-parm
 * per ora ut_definende non viene usato, ma sembra funzioni
 */

/* 
 * per ora -- nulla
 */

int init_urna(struct sessione *t)
{
#ifdef USA_DEFINENDE
   struct definende *p, *q;

   q = NULL;
   p = ut_def;

   /*
    * * cancella quelle vecchie
    */
   while(p != NULL) {
      if(p->inizio > time(NULL) + TEMPO_PER_VOTARE) {
			  u_err(t,TEMPO_DEF_SCADUTO);
         if(q == NULL) {
            ut_def = p->next;
            Free(p);
            p = ut_def;
            ut_definende--;
            continue;
         } else {
            q->next = p->next;
            Free(p);
            p = q;
            ut_definende--;
            continue;
         }
      }
      p = p->next;
   }

   /*
    * * controlla che non ci sia gia` l'utente
    */

   q = NULL;

   for(p = ut_def; p != NULL; q = p, p = p->next) {
      if(p->sessione == t) {
         p->inizio = time(NULL);
         return (0);
      }
   }
#endif

   if(ustat.attive >= MAX_REFERENDUM)
		  return(-1);

		   
#ifdef USA_DEFINENDE

   if(ustat.attive + ut_definende > MAX_REFERENDUM) 
      return (-1);

   if(p == NULL) {
      CREATE(p, struct definende, 1, TYPE_URNA_DEFNDE);

      if(q == NULL) {
         ut_def = p;
      } else {
         q->next = p;
      }
      p->next = NULL;
      p->sessione = t;
      p->inizio = time(NULL);
      ut_definende++;

      return (0);
   }

   citta_logf("questo non puo` succedere (fine di init_urna)");
   return (-1);
#endif

   return 0;
}

/* inizializza un nuovo referendum 
 * il server ha gia` ricevuto tutti
 * i dati (rs_new viene chiamato 
 * da cmd_srne)
 */

int rs_new(struct sessione *t, char *buf, char *lettera)
{
   struct urna *u;
   struct urna **testa;
   struct sessione * punto;

   /* 
    * * Controlla innanzitutto che l'utente 
    * * abbia inizializzato l'urna
    */
#ifdef USA_DEFINENDE
   if(check_init(t)) {
      u_err(t,NON_INIT_DEF);
      return -1;
   }
#endif

   ustat.modificata = 1;

#ifdef USA_DEFINENDE
   if(ut_definende <= 0) {
      citta_logf("ustat.ut_definende<0!!!");
      u_err(t,NON_INIT_DEF);
      return (-1);
   }
#endif

   /* 
    * * cerca un posto dove allocare l'urna 
    */

   if((testa = add_urne_slot()) == NULL) {
      u_err(t,RISORSE);
      return -1;
   }

   if(num_parms(buf) < 7) {
     u_err(t, URNAPOCHIDATI);
      return -1;
   }

   /* creazione dell'urna */

   CREATE(u, struct urna, 1, TYPE_URNA);

   u->conf = NULL;
   u->dati = NULL;
   u->prop = NULL;

   /* 
	* creazione di config, dati, voti e/o proposte 
	* la gestione dell'errore passa sotto
	* (quindi va mandato....)
	*/ 

   if(!(u->conf = rs_new_ucf(buf, t))) {
      dealloca_urna(u);
      Free(u);
      return -1;
   }

   if(!(u->dati = rs_new_udt(u->conf))) {
      dealloca_urna(u);
      Free(u);
      return -1;
   }

 /*
  * aggiunge a chi ha visto gli utenti
  * collegati in quel momento
  */

   for(punto = lista_sessioni; punto; punto = punto->prossima)
      if(rs_vota(punto, u->conf)==0)
	      rs_ha_visto(u,punto->utente->matricola);

   u->prop = NULL;

   /* altri dati */

   u->progressivo = ustat.ultima_urna;
   u->semaf &= ~SEM_U_ATTIVA;
   /*
   u->votanti = 0;
   */

   ustat.attive++;
   ustat.totali++;
#ifdef USA_DEFINENDE
   ut_definende--;
#endif
   ustat.ultima_urna++;
   posta_nuovo_quesito(u->conf);
   *testa = u;
   save_stat(1);
   save_urna(u, 1);

   /*
    * trova lettera
    */

   cod_lettera(u->conf->lettera, lettera);

   u->semaf |= SEM_U_ATTIVA;
   return (u->progressivo);
}

/*
 * elimina urna e tutti 
 * i suoi dati
 */

int rs_del(struct urna *u)
{
   int num;

   if(u == NULL)
      return (0);

   num = u->progressivo;

   /*
    * * poi c'e` un 
    * * dealloca
    * * andrebbe studiata...
    */

   dealloca_urna(u);

   canc_file(URNA_DIR, FILE_UDATI, num);
   canc_file(URNA_DIR, FILE_UCONF, num);

/* 
 * finche non e` a posto read_prop
 canc_file(URNA_DIR, FILE_UPROP, num);
*/

   canc_file(URNA_DIR, FILE_UVOTI, num);

   Free(u);
   ustat.attive--;
   ustat.modificata = 1;

   return (0);
}

/*
 * postpone la scadenza
 */

void rs_postpone(struct sessione *t, struct urna_dati *udt, time_t stop)
{


   if(udt->posticipo > MAX_POSTICIPI) {
      u_err(t, TROPPI_POSTICIPI);
      return;
   }

   if(stop < time(0)) {
      u_err(t, E_NEL_PASSATO);
      return;
   }

   if(stop < udt->stop) {
      u_err(t, E_UN_ANTICIPO);
      return;
   }

   udt->stop = stop;
   udt->posticipo++;

}

/*
 * Cancella un Sondaggio.  
 * Un sondaggio puo' venire cancellato
 * soltanto da utenti di livello MINLVL_REF_DEL 
 * o superiore, oppure
 * dal proponente del sondaggio.  
 * Se qualcuno sta votando...  fatti
 * suoi (nel voto ci sara` l'indicazione che e` stata
 * chiusa in anticipo)
 * urna_vota
 */

void cmd_sdel(struct sessione *t, char *buf)
{
   struct urna **u;
   long num;
   int pos;

   ustat.modificata = 1;

   num = extract_long(buf, 0);

   if((pos = rs_trova(num)) == -1) {
      u_err(t,U_NOT_FOUND);
      return;
   }
   u = ustat.urna_testa + pos;

   if((t->utente->livello < MINLVL_REF_DEL)
      && (t->utente->matricola != (*u)->conf->proponente)) {
      u_err(t,NOT_AUTH_DEL);
      return;
   }

   rs_del(*u);
   *u = NULL;
   cprintf(t, "%d\n", OK);
   return;
}

/*
 * Posticipa un sondaggio Il sondaggio puo' venire posticipato
 * soltanto da utenti di livello sufficiente o dal proponente del
 * sondaggio.  non lo fa se l'urna e` stata prechiusa
 */

void cmd_spst(struct sessione *t, char *buf)
{
   struct urna *u;
   long n;
   time_t stop;
   int pos;
   struct room *rm;

   n = extract_long(buf, 0);
   stop = extract_long(buf, 1);

   if((pos = rs_trova(n)) == -1) {
      u_err(t,U_NOT_FOUND);
      return;
   }

   u = *(ustat.urna_testa + pos);
   if(u->semaf & SEM_U_PRECHIUSA) {
      u_err(t,NOT_AUTH_SPS);
      return;
   }

   rm=room_findn(u->conf->room_num);
   if(rm==NULL)
	   rm=room_find(lobby);

   if((t->utente->livello < MINLVL_REF_DEL)
      && (t->utente->matricola != u->conf->proponente)
		    && (t->utente->matricola != rm->data->master)) {
      u_err(t,NOT_AUTH_SPS);
      return;
   }
   rs_postpone(t, u->dati, stop);       /* errori gestiti in rs_Post */
   cprintf(t, "%d %ld\n", OK, n);
   return;
}

/*
 * chiude un Sondaggio in anticipo 
 * Il sondaggio puo' venire
 * interrotto soltanto da utenti di livello 
 * sufficiente o dal proponente del sondaggio.
 * 
 * Cambiata totalmente. 
 * Ora interrompe semplicemente mettendo il flag
 * CONCLUSA e lasciando al server 
 * il compito di chiuderlo a prossimo * ciclo
 * (comybqe si p` anticipare metendo un a chiamaa
 * a urna_...
 */

void cmd_sstp(struct sessione *t, char *buf)
{
   struct urna **u;
   long n;
   int pos;

   ustat.modificata = 1;
   n = extract_long(buf, 0);

   if((pos = rs_trova(n)) == -1) {
      u_err(t,U_NOT_FOUND);
      return;
   }
   u = ustat.urna_testa + pos;

   if((t->utente->livello < MINLVL_REF_STOP)
      && (t->utente->matricola != (*u)->conf->proponente)) 
      u_err(t,NOT_AUTH_SPS);

   (*u)->semaf |= SEM_U_PRECHIUSA;
/*
 * rs_expire();
 */
   cprintf(t, "%d\n", OK);
}

/* 
 * crea una nuova conf
 */

struct urna_conf *rs_new_ucf(char *buf, struct sessione *t)
{

  struct urna_conf *ucf;
  int let;
  char * s;

   CREATE(ucf, struct urna_conf, 1, TYPE_URNA_CONF);

   ucf->progressivo = ustat.ultima_urna;

   if((ucf->lettera = lettera_successiva()) == 255) {
      Free(ucf);
	  u_err(t,RISORSE);
      return NULL;
   }

   ucf->proponente = t->utente->matricola;
   ucf->room_num = t->room->data->num;

   ucf->start = time(NULL);
   ucf->stop = extract_long(buf, POS_STOP);
   ucf->tipo = extract_int(buf, POS_TIPO);
   ucf->voci = NULL;
   ucf->modo = extract_int(buf, POS_MODO);
   ucf->num_voci = extract_int(buf, POS_NUM_VOCI);
   ucf->max_voci = extract_int(buf, POS_MAX_VOCI);
   ucf->num_prop = extract_int(buf, POS_NUM_PROP);
   ucf->maxlen_prop = extract_int(buf, POS_MAXLEN_PROP);
   ucf->bianca = extract_int(buf, POS_BIANCA);
   ucf->astensione = extract_int(buf, POS_ASTENSIONE_VOTO);
   ucf->crit = extract_int(buf, POS_CRITERIO);

   if(ucf->crit == CV_ANZIANITA) {
      ucf->val_crit.tempo = (time_t) extract_long(buf, POS_VAL_CRITERIO);
   } else {
      ucf->val_crit.numerico = extract_long(buf, POS_VAL_CRITERIO);
   }

   if(ucf->stop < ucf->start + TEMPO_MINIMO) {
      u_err(t, TROPPOVICINO);
      Free(ucf);
      return NULL;
   }

   if(ucf->tipo != TIPO_REFERENDUM && ucf->tipo != TIPO_SONDAGGIO) {
      u_err(t,PROTO);
      Free(ucf);
      return NULL;
   }

   if(ucf->modo < 0 && ucf->modo > NUM_MODI) {
      u_err(t, URNADATIERRATI);
      Free(ucf);
      return NULL;
   }

   if(ucf->num_voci > MAX_VOCI) {
      u_err(t, TROPPEVOCI);
      Free(ucf);
      return (0);
   }

   /* 
    * *non documentato - se bianca/astensione non sono 0=> sono 1
    */

   if(ucf->bianca != 0)
      ucf->bianca = 1;

   if(ucf->astensione != 0)
      ucf->astensione = 1;

   if((ucf->crit == CV_LIVELLO) && (ucf->val_crit.numerico > LVL_SYSOP))
      ucf->val_crit.numerico = LVL_SYSOP;

   if((ucf->modo == MODO_SCELTA_MULTIPLA) && ucf->num_voci < 3) {
      u_err(t, POCHEVOCI);
      Free(ucf);
      return NULL;
   }

   if(ucf->modo == MODO_SCELTA_SINGOLA && ucf->num_voci < 2) {
      u_err(t, POCHEVOCI);
      Free(ucf);
      return NULL;
   }

   if(ucf->modo == MODO_PROPOSTA && ucf->num_prop < 1) {
      u_err(t, POCHEVOCI);
      Free(ucf);
      return NULL;
   }

   if(ucf->max_voci > ucf->num_voci) {
      ucf->max_voci = ucf->num_voci;
   }

   if(ucf->max_voci == 0 && ucf->modo != MODO_PROPOSTA) {
      ucf->max_voci = 1;
   }

   if(ucf->modo == MODO_PROPOSTA && ucf->maxlen_prop > MAXLEN_PROPOSTA) {
      ucf->maxlen_prop = MAXLEN_PROPOSTA;
   }

   if(rs_ucf_parm(ucf, t)) {
      u_err(t,POCHIDATI);
      return NULL;
   }
/*
 * controlla che il titolo 
 * contenga almeno 3 lettere
 */
  let=0;
  for(s=ucf->titolo;*s;s++)
	if (*s>32 && *s<127)
		let++;
  
   if(let<3){
      citta_log("titolo corto");
      u_err(t,URNA_TITOLO_CORTO);
	return NULL;
	}

	
   return ucf;
}

/*
 * legge i parametri
 * (titolo/testo/voci)
 */

int rs_ucf_parm(struct urna_conf *ucf, struct sessione *t)
{

   int j, i = 0;
   char stri[3];
   char nome_voce[8];
   int l=0;

   if((l=par2strd(ucf->titolo, t, "titolo", LEN_TITOLO)) > 1) {
      citta_logf("titolo troncato, %d-%d",l,LEN_TITOLO);
   }


   for(i = MAXLEN_QUESITO - 1; i >= 0; i--)
      if((l=(par2strd((ucf->testo)[i], t, "testo", LEN_TESTO)))>1)
         citta_logf("testo troncato %d-%d",l,LEN_TESTO);

   /* 
    * * * rinormalizza
    */

   j = 0;
   for(i = 0; i < MAXLEN_QUESITO; i++) {
      if(strlen(ucf->testo[i]) == 0) {
         continue;
      }
      strncpy((ucf->testo)[j], (ucf->testo)[i], LEN_TESTO);
      j++;
   }

   for(; j < MAXLEN_QUESITO; j++)
      strcpy(ucf->testo[j], "");

   if(ucf->modo == MODO_SI_NO) {
      ucf->num_voci = 2;
      CREATE((ucf->voci), char *, 2, TYPE_HEAD_VOCI);
      CREATE(*(ucf->voci + CODIFICA_SI), char, strlen(SI) + 1, TYPE_CHAR);
      CREATE(*(ucf->voci + CODIFICA_NO), char, strlen(NO) + 1, TYPE_CHAR);

      strcpy(*(ucf->voci + CODIFICA_SI), SI);
      strcpy(*(ucf->voci + CODIFICA_NO), NO);
      return 0;
   }

   CREATE((ucf->voci), char *, (ucf->num_voci), TYPE_HEAD_VOCI);

   for(i = 0; i < ucf->num_voci; i++) {
      CREATE(*(ucf->voci + i), char, MAXLEN_VOCE + 1, TYPE_CHAR);

      sprintf(stri, "%d", i);
      strcpy(nome_voce, "voce");
      strcat(nome_voce, "-");
      strcat(nome_voce, stri);
      par2strd(*(ucf->voci + i), t, nome_voce, MAXLEN_VOCE);
   }
   return 0;
}

/* 
 *crea la struttura urna_dati
 */

struct urna_dati *rs_new_udt(struct urna_conf *ucf)
{

   struct urna_dati *udt;
   int i;
   struct urna_voti *voto;

   CREATE(udt, struct urna_dati, 1, TYPE_URNA_DATI);

   udt->stop = ucf->stop;
   udt->posticipo = 0;
   udt->bianche = 0;
   udt->nvoti = 0;
   udt->num_voci = ucf->num_voci;
   udt->voti_nslots = 1;
   udt->coll_nslots = 1;

   CREATE(udt->uvot, long, LEN_SLOTS, TYPE_LONG);
   CREATE(udt->ucoll, long, LEN_SLOTS, TYPE_LONG);

   for(i = 0; i < LEN_SLOTS; i++) {
      *(udt->uvot + i) = -1;
      *(udt->ucoll + i) = -1;
   }

   CREATE(udt->voti, struct urna_voti, udt->num_voci, TYPE_URNA_VOTI);

   for(i = 0; i < udt->num_voci; i++) {
      voto = udt->voti + i;
      voto->num_voti = 0;
      voto->tot_voti = 0;
      voto->astensioni = 0;
#if URNE_GIUDIZI
      /* urna giudizi memorizza i voti dati da ongi
       * utente
       */
      voto->giudizi = 0;
      if(ucf->modo == MODO_VOTAZIONE)
         CREATE(voto->giudizi, long, NUM_GIUDIZI, TYPE_LONG);
#endif
   }

   return udt;
}

/*
 * alla partenza urna_prop
 * e` nulla
 */

struct urna_prop *rs_new_prop()
{
   struct urna_prop *upr;

   upr = NULL;
   return upr;
}

unsigned char lettera_successiva()
{
   /* */
   struct urna *testa;
   unsigned char lettera;
   int i;
   unsigned char usate[32] = "";        /* 
                                         * * * elenco delle lettere usate       v9...a1 d c b a
                                         * * * lo 0 e il 255 non sono usati
                                         */

   for(i = 0; i < ustat.urne_slots * LEN_SLOTS; i++) {
      testa = *(ustat.urna_testa + i);
      if(testa != NULL) {
         lettera = testa->conf->lettera;
         usate[lettera / 8] |= 1 << (lettera % 8);
      }
   }


   /* 
    * * lettera vale 'a', ma se l'ultima lettera
    * * utilizzata e` una lettera singola <z, allora
    * * e` la successiva.
    */

   lettera = 1;

   if(ustat.ultima_lettera <= 25) {
      lettera = ustat.ultima_lettera + 1;
   }

   /* 
    * * ciclo su tutto l'alfabeto, partendo da ultima_lettera+1
    */

   for(; lettera <= 26; lettera++)
      if((usate[lettera / 8] & (1 << (lettera % 8))) == 0) {
         ustat.ultima_lettera = lettera;
         return lettera;
      }

   /* 
    * *superata la z si torna da capo (da a) e si attraversa
    * * tutto da a...z a1...z1.... a9  u9
    */

   for(lettera = 1; lettera < 255; lettera++)
      if((usate[lettera / 8] & (1 << (lettera % 8))) == 0) {
         ustat.ultima_lettera = lettera;
         return lettera;
      }

   citta_log("finite le lettere!!!");
   return 255;
}

/* 
 * controlla che l'utente
 * abbia inizializzato
 * la creazione di un urna
 * non usato, per ora
 */
#ifdef USA_DEFINENDE
int check_init(struct sessione *t)
{
   struct definende *q;
   struct definende *p;

   q = NULL;
   for(p = ut_def; p != NULL; q = p, p = p->next) {
      if(p->sessione == t)
         break;
   }

   if(p == NULL) {
      err(t, NONINIT);
#if U_DEBUG
      citta_logf("SYSLOG: in rs_new, Non inizializzata");
#endif
      return (-1);
   }

   if(q != NULL) {
      q->next = p->next;
   } else {
      ut_def = p->next;
   }

   Free(p);
   return 0;
}
#endif  /* DEFINENDE */

/* 
 *aggiunge  uno slot
 * per le urne
 * e ritorna la prima locazione disponibile
 */

struct urna **add_urne_slot()
{

   int slots, pos;
   struct urna **testa;

   if(ustat.urne_slots == 0) {
      ustat.urne_slots = 1;
      CREATE(ustat.urna_testa, struct urna *, LEN_SLOTS, TYPE_POINTER);

      return ustat.urna_testa;
   }

   slots = ustat.urne_slots * LEN_SLOTS;
   testa = ustat.urna_testa;

   for(pos = 0; pos < slots; pos++) {
      if(*testa == NULL)
         break;
      testa++;
   }

   if(pos == slots) {
      if(ustat.urne_slots == MAX_SLOT) {
         citta_logf("troppe urne");
         return (NULL);
      }

      ustat.urne_slots++;
      slots = ustat.urne_slots * LEN_SLOTS;
      RECREATE(ustat.urna_testa, struct urna *, slots);
      memset(ustat.urna_testa + pos, 0, LEN_SLOTS * sizeof(struct urna **));
      testa = (ustat.urna_testa + pos);
   }

   if(*testa != NULL) {
      citta_log("questo non puo` succedere (urna-crea.c)");
      return NULL;
   }
   return testa;
}
