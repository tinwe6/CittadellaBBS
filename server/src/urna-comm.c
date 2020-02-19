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
 * Cittadella/UX client 		(C) M. Caldarelli and R. Vianello   *
 *                                                                          *
 *  File : urna_comm.c                                                      *
 *  comunicazione col client                                                *
 *                                                                          *
 ***************************************************************************/

/* Wed Nov 12 23:31:24 CET 2003 */

#include "config.h"
#include "strutture.h"
#include "cittaserver.h"
#include "urna-strutture.h"
#include "extract.h"
#include "urna-vota.h"
#include "urna-servizi.h"
#include "sys_flags.h"
#include "utility.h"
#include <string.h>
#include <stdio.h>
#include "memstat.h"
#include "urna_comune.h"
#include "urna-comm.h"
#include "urna_errori.h"

void cmd_slst(struct sessione *t, char *arg);
void cmd_scvl(struct sessione *t, char *arg);
void cmd_sinf(struct sessione *t, char *buf);
int crea_stringa_per_client(char *string, struct urna *u);
void urna_vota(struct sessione *t, char *buf);
void send_ustat(struct sessione *t);

/* 
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
 * n=6 manda tutti i ref votabili e da votare nelle room _NON_ zappate creati
 *     dall'utente. 
 * n=12 manda tutti i ref/sondaggi nelle room _NON_ zappate
 *     modificabili dall'utente
 *
 *
 * ---- per la parte zap il codice si basa su 2%2=0%2=0   
 *
 * Restituisce 
 * numero, tipo, titolo, modo, termine, lettera, stanza, zappata,
 * votabile, votato, posticipabile, proponente
 *
 * se n=0,1,4,12: votabile, votato sono non prevedibili
 * se n=2 o n=4 o n=6 manda solo i ref nelle  stanze non zappate
 * (ovvero non c'e` modo di sapere se ho votato se non 
 * unzappo la room!)
 * 
 */

void cmd_slst(struct sessione *t, char *arg)
{
   struct urna *u;
   struct urna_conf *ucf;
   struct urna_dati *udt;
   struct room *rm;
   char lettera[3];
   char votabile = 0, votato = 0;
   int zap = 0;
   int i, room_zap, tipo, room_aide;
   char posticipabile = 0;

   cprintf(t, "%d\n", SEGUE_LISTA);

   if(ustat.urna_testa == NULL) {
      cprintf(t, "000\n");
      return;
   };

   tipo = extract_int(arg, 0);
   zap = tipo % 2;
   room_zap = 0;

   for(i = 0; i < ustat.urne_slots * LEN_SLOTS; i++) {
      u = *(ustat.urna_testa + i);
      if(u == NULL) {
         continue;
      }
      ucf = u->conf;
      udt = u->dati;
	  if (u->semaf&(SEM_U_PRECHIUSA|SEM_U_CONCLUSA)){
			  continue;
	  };
      rm = room_findn(ucf->room_num);

/* 
 * se
 * l'urna e`
 * in una stanza
 * cancellata |-> Lobby
 */

      if(rm == NULL) {
         citta_logf("room %ld cancellata, uso Lobby??", ucf->room_num);
         rm = room_find(lobby);
      }

      /*
       * * e` etico controllare il criterio livello e non il resto?
       */

      if((ucf->crit == CV_LIVELLO) &&
         (t->utente->livello < ucf->val_crit.numerico)) {
         continue;
      };

      /* 
       * se l'utente non conosce la stanza salta
        */


      if(room_known(t, rm) == 0) {
         continue;
      };

      if(room_known(t, rm) == 2) {
         if(zap == 0)
            continue;
         room_zap = 1;
      };

      /*
       * numero, tipo, titolo, modo, termine, lettera, stanza, zappata,
       * votabile, votato, posticipabile, proponente
       */

	  room_aide=rm->data->master;
      /* proponente = (u->dati->posticipo < MAX_POSTICIPI); */
      cod_lettera(ucf->lettera, lettera);
      posticipabile = (u->dati->posticipo < MAX_POSTICIPI);

      if(tipo == 2 || tipo == 6) {
         votabile = (rs_vota(t, ucf) == 0);
         votato = rs_ha_votato(u, t->utente->matricola);
		 rs_ha_visto(u,t->utente->matricola);
      };

       if(tipo == 4)
 			  if ((ucf->proponente != t->utente->matricola) &&
                         (t->utente->livello < LVL_AIDE) &&
 						(t->utente->matricola != room_aide))
         continue;

      if(tipo == 6 && votabile == 0)
         continue;

      if(tipo == 6 && votato == 1)
         continue;

      if(tipo == 12 && ((ucf->proponente != t->utente->matricola) ||
                        (t->utente->livello < LVL_AIDE)
         )) {
         continue;
      };

      cprintf(t, "%d %ld|%d|%s"
              "|%d|%ld|%s"
              "|%s|%d"
              "|%d|%d|%d\n",
              OK, ucf->progressivo, ucf->tipo, ucf->titolo,
              ucf->modo, udt->stop, lettera,
              rm->data->name, room_zap, votabile, votato, posticipabile);
   }
   cprintf(t, "000\n");
};


/*
 * invia le informazioni
 * su un referendum
 * vedi file protocollo
 */

void cmd_sinf(struct sessione *t, char *buf)
{
   struct urna *u;
   struct urna_conf *ucf;
   int i, motivo=0;
   int pos, havotato;
   struct room *rm;
   char *str;

   pos = rs_trova(extract_long(buf, 0));

   if(pos == -1) {
		   u_err(t,U_NOT_FOUND);
      return;
   }

   if(ustat.urna_testa + pos == NULL) {
		   u_err(t,U_NOT_FOUND);
      return;
   }

   u = *(ustat.urna_testa + pos);

   if(u == NULL) {
		   u_err(t,U_NOT_FOUND);
      return;
   }

   ucf = u->conf;

   motivo = rs_vota(t, ucf);
   if(motivo == -1){
		   u_err(t,U_NOT_FOUND);
      return;
   }

   if(motivo!=0){
		   u_err2(t,CANNOT_VOTE,motivo);
      return;
   }

   rm = room_findn(ucf->room_num);

   if(rm == NULL) {
      citta_logf("room %ld cancellata, uso Lobby??", ucf->room_num);
      rm = room_find(lobby);
   }

   CREATE(str, char, (MAX_POS + 1) * (MAX_POSLEN) + 1, TYPE_CHAR);

   if(crea_stringa_per_client(str, u)) {
      Free(str);
	  u_err(t,RISORSE);
      return;
   }

   cprintf(t, "%d %s\n", OK, str);
   Free(str);

   havotato = rs_ha_votato(u, t->utente->matricola);
   cprintf(t, "%d PARM havotato|%d\n", OK, havotato);
   cprintf(t, "%d PARM titolo|%s\n", OK, ucf->titolo);

   /*
    * * al contrario
    */

   for(i = MAXLEN_QUESITO - 1; i >= 0; i--) {
      if(ucf->testo[i] != NULL) {
         cprintf(t, "%d PARM testo|%s\n", OK, ucf->testo[i]);
      }
   }

   for(i = 0; i < ucf->num_voci; i++)
      cprintf(t, "%d PARM voce-%d|%s\n", OK, i, *(ucf->voci + i));

   cprintf(t, "%d PEND\n", OK);

}

int crea_stringa_per_client(char *string, struct urna *u)
{
   char *sep = "|";
   char *(client[MAX_POS]);
   struct urna_conf *ucf;
   struct urna_dati *udt;
   struct room *rm;
   int i;

   ucf = u->conf;
   udt = u->dati;

   rm = room_findn(ucf->room_num);

   if(rm == NULL) {
      citta_logf("room %ld clancellata, uso Lobby??", ucf->room_num);
      rm = room_find(lobby);
   }

   for(i = 0; i < MAX_POS; i++) {
      CREATE(client[i], char, MAX_POSLEN + 1, TYPE_CHAR);
   }

   sprintf(client[POS_NUM], "%ld", u->progressivo);
   sprintf(client[POS_TIPO], "%d", ucf->tipo);
   sprintf(client[POS_PROPONENTE], "%s", nome_utente_n(ucf->proponente));
   sprintf(client[POS_NOME_ROOM], "%s", rm->data->name);
   sprintf(client[POS_NUM_VOCI], "%d", ucf->num_voci);
   sprintf(client[POS_MAX_VOCI], "%d", ucf->max_voci);
   sprintf(client[POS_BIANCA], "%d", ucf->bianca);
   sprintf(client[POS_ASTENSIONE_VOTO], "%d", ucf->astensione);
   sprintf(client[POS_MODO], "%d", ucf->modo);
   sprintf(client[POS_START], "%ld", ucf->start);
   sprintf(client[POS_STOP], "%ld", udt->stop);
   sprintf(client[POS_CRITERIO], "%d", ucf->crit);
   sprintf(client[POS_VAL_CRITERIO], "%ld", ucf->val_crit.numerico);
   sprintf(client[POS_ANZIANITA], "%ld", ucf->val_crit.tempo);
   sprintf(client[POS_NUM_PROP], "%d", ucf->num_prop);
   sprintf(client[POS_MAXLEN_PROP], "%d", ucf->maxlen_prop);
   sprintf(client[POS_LETTERA], "%d", ucf->lettera);

   strcpy(string, client[0]);
   Free((client[0]));
   citta_logf("stringa: %s", string);
   for(i = 1; i < MAX_POS; i++) {
      strcat(string, sep);
      strcat(string, client[i]);
      Free((client[i]));
   }
   citta_logf("stringa: %s", string);
   return (0);
}

/*
 * accetta i voti
 * spedisce gli errori al client
 */

void urna_vota(struct sessione *t, char *buf)
{
   struct urna *u;
   long num;
   int i, j, pos;
   int nvoci, num_voti, num_ast, lbuf, modo;
   int motivo;
   char ris[MAX_VOCI + 1], risu[2];

   num = extract_long(buf, 0);


   if(del_votante(num,t->utente->matricola)){
		   u_err(t,WASNVOTING);
		   return;
   }

   if((pos = rs_trova(num)) == -1) {
		   u_err(t,U_NOT_FOUND);
      return;
   };

   u = *(ustat.urna_testa + pos);
   modo = u->conf->modo;

   if((motivo=rs_vota(t, u->conf))!=0) {
	  u_err2(t,LIVELLO,motivo);
      return;
   }

   if(rs_ha_votato(u,t->utente->matricola)) {
	  u_err(t,VOTED);
      return;
   }

   if(modo == MODO_PROPOSTA) {
      if(rs_add_proposta(u, t))
			  u_err(t,VOTI_ERRATI);
      else
         cprintf(t, "%d\n", OK);
      return;
   }

   for(i = 0; i <= MAX_VOCI; i++) {
      ris[i] = 0;
   }

   if(u == NULL) {
		   u_err(t,U_NOT_FOUND);
      return;
   }

   if((rs_vota(t, u->conf) != 0)) {
			  u_err(t,VOTI_ERRATI);
      return;
   }

   if(rs_ha_votato(u, t->utente->matricola)) {
			  u_err(t,VOTED);
      return;
   }

   lbuf = strlen(buf);

   j = 0;

   /*
    * * cancella tutti gli spazi
    */

   for(i = 0; i <= lbuf; i++) {
      buf[j] = buf[i];
      if(buf[i] != ' ')
         j++;
   }

   for(i = j + 1; i <= lbuf; i++) {
      buf[i] = 0;
   }

   nvoci = u->conf->num_voci;
   for(i = 0; i < nvoci; i++) {
      extractn(risu, buf, i + 1, 2);
      risu[1] = 0;
      ris[i] = risu[0];

      /* 
       * * totale (il resto viene ignorato)
       * * * nel caso si /no e nel caso
       * * * voto singolo / multiplo
       * * * 0 indica non votato (o `\000')
       * * * 1 indica votato.
       * * *
       * * * se il modo e` votazione
       * * * il valore e` il valore votato,
       * * * la a astensione sulla singola voce
       */
   }

   if(ris[0] == '=' && !(u->conf->bianca)) {
			  u_err(t,NOTWHITE);
      return;
   }

   /* 
	* se il voto e` A chiama subito
	* add_vote
	*/

   if(ris[0] == '=' && (u->conf->bianca)) {
      rs_add_vote(u, t, ris);
      cprintf(t, "%d\n", OK);
      return;
   }

   num_voti = 0;
   num_ast = 0;
   for(j = 0; j < nvoci; j++) {
      /* normalizza gli 0  */
      if(ris[j] == 0) {
         ris[j] = '0';
      }

      if(ris[j] != '=' && (ris[j] < '0' || ris[j] > '9')) {
			  u_err(t,BAD_VOTE);
         return;
      }

      if(ris[j] == '=') {
         num_ast++;
      }

      if(ris[j] != '=' && ris[j] != '0') {
         num_voti++;
      }
   }

   if(num_voti > u->conf->max_voci) {
      u_err(t,TROPPIVOTI);
      return;
   }

   if((num_voti == 0) && (modo != MODO_VOTAZIONE || modo != MODO_PROPOSTA)) {
      u_err(t,POCHIVOTI);
      return;
   }

   if((num_ast > 0) && (modo != MODO_VOTAZIONE)) {
      u_err(t, ASTENSIONESUVOCE);
      return;
   }

   if((num_ast > 0) && (modo == MODO_VOTAZIONE)
      && (!u->conf->astensione)) {
      u_err(t, ASTENSIONESCELTA);
      return;
   }

   rs_add_vote(u, t, ris);

   cprintf(t, "%d\n", OK);
   return;
}

/* Invia le statistiche dei sondaggi al client (per il comando cmd_rsst() ) */
void send_ustat(struct sessione *t)
{
   cprintf(t, "%d %d|%ld|%ld|%ld|%ld|%ld\n", OK, SYSSTAT_REFERENDUM,
           ustat.totali, ustat.attive, ustat.complete, ustat.voti,
           ustat.urne_slots
		  );
}
