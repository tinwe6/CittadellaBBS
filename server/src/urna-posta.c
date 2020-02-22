/*  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: urna-posta.c                                                        *
* mette i messaggi nelle stanze                                             *
*                                                                           *
****************************************************************************/

/* Wed Nov 12 23:21:13 CET 2003 -- piu  o meno */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "urna-strutture.h"
#include "urna-servizi.h"
#include "utility.h"

void posta_nuovo_quesito(struct urna_conf *ucf);
void scrivi_scelte(struct urna_conf *ucf, struct text *avviso);
void chipuo(struct urna_conf *ucf, char *buf);

void posta_nuovo_quesito(struct urna_conf *ucf)
{
   char subject[LBUF];
   struct room *room;
   struct tm *tmst;
   char nome_tipo[11];
   char* lettera;
   struct text *avviso;
   char data_fine[LBUF];
   char u_possono[LBUF];
   int i, progressivo;

   progressivo = ucf->progressivo;
   CREATE(lettera, char, 3, TYPE_CHAR);
   cod_lettera(ucf->lettera,lettera);

   if(ucf->tipo == TIPO_REFERENDUM)
      strcpy(subject, "Nuovo referendum ");
   else
      strcpy(subject, "Nuovo sondaggio ");

   strncat(subject, lettera, 3);
   strcat(subject,":");
   if(strlen(ucf->titolo) > (MAXLEN_SUBJECT - 21))
      strcat(subject, " [...]");
   else
      strcat(subject, ucf->titolo);

   if(ucf->tipo == TIPO_REFERENDUM)
      strcpy(nome_tipo, "referendum");
   else
      strcpy(nome_tipo, "sondaggio");

   avviso = txt_create();
   txt_putf(avviso, "C'&egrave un nuovo <b>%s</b>, "
              "(%s, il numero <b>%d</b>) da votare,",
              nome_tipo, lettera, progressivo);

   txt_putf(avviso, "dal titolo <b>%s</b>.", ucf->titolo);
   txt_putf(avviso, " ");
   txt_putf(avviso, "&Egrave stato proposto da <b>%s</b>.\n"
            , nome_utente_n(ucf->proponente));

   for(i = 0; i < MAXLEN_QUESITO; i++){
		   if (((ucf->testo[i])[0])){
              txt_putf(avviso, "<b>></b>%s", ucf->testo[i]);
		   }
   }

   txt_putf(avviso, " ");

   switch (ucf->modo) {
   case MODO_SI_NO:
      txt_putf(avviso, "Si risponde <b>S&iacute/No</b>.");
      break;
   case MODO_SCELTA_SINGOLA:
      txt_putf(avviso, " Si deve scegliere <b>una sola voce"
               "</b> tra le seguenti");
      scrivi_scelte(ucf,avviso);
      break;
   case MODO_SCELTA_MULTIPLA:
      txt_putf(avviso,
               " Si devono scegliere fino a <b>%d voci</b>"
               " tra le seguenti ", ucf->max_voci);
      scrivi_scelte(ucf,avviso);
      break;
   case MODO_VOTAZIONE:
      txt_putf(avviso, " Si deve dare un <b>voto</b> alle " "seguenti voci");
      scrivi_scelte(ucf,avviso);
   case MODO_PROPOSTA:
      txt_putf(avviso, " Si possono proporre fino a <b>%d</b> voci.",
               ucf->num_prop);
      break;
   }

   /*   */

   txt_putf(avviso, " ");

   if(ucf->bianca)
      txt_putf(avviso, "Si pu&ograve votare scheda bianca.");

   if((ucf->modo == MODO_VOTAZIONE) && (ucf->astensione))
      txt_putf(avviso, "Ci si pu&ograve astenere su alcune voci.");
   txt_putf(avviso, " ");

   chipuo(ucf, u_possono);

   txt_putf(avviso, "Possono votare gli utenti%s",
            u_possono);

   tmst = localtime(&(ucf->stop));
   sprintf(data_fine, " %d/%d/%d </b>alle<b> %02d:%02d",
           tmst->tm_mday, tmst->tm_mon + 1, 1900 + tmst->tm_year,
           tmst->tm_hour, tmst->tm_min);

   txt_putf(avviso, "La votazione si concluder&agrave il <b>%s</b>.\n",
            data_fine);
   room = room_findn(ucf->room_num);
   if(room == NULL) {
      citta_logf("Room per urna non trovata, uso Lobby!");
      room = room_find(lobby);
   }

   citta_post(room, subject, avviso);

   txt_free(&avviso);
#ifdef DEBUG
   citta_logf("postato in room %ld, subj %s", ucf->room_num, subject);
#endif
}

void scrivi_scelte(struct urna_conf *ucf, struct text *avviso)
{
   int i;

   for(i = 0; i < ucf->num_voci; i++)
      txt_putf(avviso, "--- <b>%s</b>", *(ucf->voci + i));
}

void chipuo(struct urna_conf *ucf, char *buf)
{
   switch (ucf->crit) {
   case CV_UNIVERSALE:
      strcpy(buf, ".");
      break;
   case CV_LIVELLO:
      sprintf(buf, " <b>di livello %ld</b>.", ucf->val_crit.numerico);
      break;
   case CV_ANZIANITA:
      {
         struct tm *tmst;

         tmst = localtime(&(ucf->val_crit.tempo));
         sprintf(buf,
                 " <b>registrati prima del %d/%d/%d</b>.",
                 tmst->tm_mday, tmst->tm_mon + 1, 1900 + tmst->tm_year);
      }
      break;
   case CV_CALLS:
      sprintf(buf,
              " <b>con un minimo di %ld chiamate all'attivo</b>.",
              ucf->val_crit.numerico);
      break;
   case CV_POST:
      sprintf(buf,
              " <b>che hanno lasciato almeno %ld post</b>.", ucf->val_crit.numerico);
      break;
   case CV_X:
      sprintf(buf,
              " </b>che hanno inviato almeno %ld Xmsg</b>.", ucf->val_crit.numerico);
      break;
   case CV_SESSO:
      if(ucf->val_crit.numerico)
         sprintf(buf, " <b>maschi</b>.");
      else
         sprintf(buf, " <b>femmine</b>.");
   }
}
