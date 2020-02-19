/*
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
* File :    urna-fine.c                                                    *
*     funzioni per la chiusura d'un urna                                   *
*     (ma il risultati li scrive in urna-posta- da vedere)                 *
****************************************************************************/

/* Wed Nov 12 23:29:48 CET 2003 */

#include "config.h"
#include "urna-strutture.h"
#include "urna-vota.h"
#include "urna-servizi.h"
#include "urna-file.h"
#include "urna-cost.h"
#include "urna-rw.h"
#include "utility.h"
#include <string.h>

int rs_end(struct urna *u);

#ifdef USA_RES_URNA
int res_proposta(struct urna *u, int nvoti, struct text *txt, FILE * fp_res);
int res_votazione(struct urna *u, int nvoti, struct text *txt,
                  int l_max, FILE * fp_res);
int res_scelta_multipla(struct urna *u, int nvoti, struct text *txt,
                        int l_max, FILE * fp_res);
int res_scelta(struct urna *u, int nvoti, struct text *txt,
               int l_max, FILE * fp_res);
int res_si_no(struct urna_voti *uvt, int voti_validi, struct text *txt,
              FILE * fp_res);
#else
int res_proposta(struct urna *u, int nvoti, struct text *txt);
int res_votazione(struct urna *u, int nvoti, struct text *txt, int l_max);
int res_scelta_multipla(struct urna *u, int nvoti,
                        struct text *txt, int l_max);
int res_scelta(struct urna *u, int nvoti, struct text *txt, int l_max);
int res_si_no(struct urna_voti *uvt, int voti_validi, struct text *txt);
#endif

int rs_end(struct urna *u)
{
   struct text *txt;
   struct text *txt_parz;
   struct urna_conf *ucf;
   struct urna_dati *udt;
   struct urna_voti *uvt;
   /* struct urna_prop *upr; */
   long matricola;
   long room_num;
   char *ptxt;
   struct room *room;
   long voti_validi;
   long nvoti;
   long bianche;
   long progressivo;
   char **voce;
   char subj[LBUF], buf[LBUF], buf1[LBUF];
   long i, elettori = 0;
   long l_max;
   int parte_res = 1;
   int numlin=0;
   struct lista_ut *punto;

#ifdef USA_RES_URNA
   FILE *fp_res;
#endif
   FILE *fp_room_like;
   char prechiusa;

   if(u == NULL)
      return (0);

   ucf = u->conf;
   udt = u->dati;
   uvt = udt->voti;
   /* upr = u->prop; */
   matricola = ucf->proponente;
   room_num = ucf->room_num;
   prechiusa = u->semaf & SEM_U_PRECHIUSA;

   /*
    * totale dei possibili votanti al momento -- in realta  non   
    * e` preciso... se qualcuno si è cancellato? 
    */

    for (punto = lista_utenti; punto; punto = punto->prossimo) {
	            if (rs_vota_ut(punto->dati, u->conf) == 0)
			    elettori++;
    }

   progressivo = u->progressivo;

#ifdef USA_RES_URNA
   if(save_file(URNA_DIR, FILE_URES, progressivo, &fp_res)) {
      citta_logf("SYSERR: non posso salvare i risultati %d", progressivo);
      fclose(fp_res);
      return (-1);
   }

   if(write_magic_number(fp_res, MAGIC_RES, "risultati")) {
      citta_logf("SYSERR: non posso salvare i risultati %d", progressivo);
      fclose(fp_res);
      return (-1);
   }
#endif

   if(save_file(URNA_DIR, FILE_ROOM_LIKE, progressivo, &fp_room_like)) {
      citta_logf("SYSERR: non posso salvare i risultati %ld", progressivo);
      fclose(fp_room_like);
      return (-1);
   }

   ustat.complete++;
   voce = ucf->voci;
   nvoti = udt->nvoti;
   bianche = udt->bianche;
   voti_validi = nvoti - bianche;
   l_max = 0;
   for(i = 0; i < ucf->num_voci; i++) {
      l_max = l_max > strlen(*(voce + i)) ? l_max : strlen(*(voce + i));
   }

   l_max += 3;
#ifdef USA_RES_URNA
   fwrite(&progressivo, sizeof(long), 1, fp_res);
   fwrite(ucf, sizeof(struct urna_conf), 1, fp_res);
#endif

   if(ucf->tipo)
      sprintf(subj, "Risultati Referendum #%ld", progressivo);
   else
      sprintf(subj, "Risultati Sondaggio #%ld", progressivo);

   /*   */

   txt = txt_create();
   txt_putf(txt, "*** <b>Quesito</b>: <b>%s</b> ***", ucf->titolo);

   for(i = 0; i < MAXLEN_QUESITO; i++) {
      if(strlen((ucf->testo)[i]) != 0)
         txt_putf(txt, "<b>></b>%s", (ucf->testo)[i]);
#ifdef USA_RES_URNA
      fwrite(ucf->testo, sizeof(char), LEN_TESTO, fp_res);
#endif
   }
   if(prechiusa) {
      txt_put(txt, "(&Egrave; stato chiuso in anticipo)");
   };

   txt_put(txt, " ");
   txt_put(txt, "*** Risultati:");
   if(voti_validi == 0) {
      citta_logf("SYSLOG: nessun voto valido!");
      txt_putf(txt, "    <b>Non ha votato nessuno!</b>");
      txt_put(txt, " ");
      txt_putf(txt, "(Il quesito &egrave stato proposto da <b>%s</b>.)",
               nome_utente_n(ucf->proponente));
      room = room_findn(ucf->room_num);
      if(room == NULL) {
         citta_log("room non trovata, uso Lobby");
         room = room_find(lobby);
      }
      citta_post(room, subj, txt);
      txt_rewind(txt);
      while((ptxt = txt_get(txt)))
         fprintf(fp_room_like, "%s\n", ptxt);

      txt_free(&txt);
#ifdef USA_RES_URNA
      fwrite("0", sizeof(char), 1, fp_res);
#endif
      fclose(fp_room_like);
#ifdef USA_RES_URNA
      fclose(fp_res);
#endif
      return (0);
   }

   switch (ucf->modo) {
   case MODO_SI_NO:
#ifdef USA_RES_URNA
      res_si_no(uvt, voti_validi, txt, fp_res);
#else
      res_si_no(uvt, voti_validi, txt);
#endif
      break;

   case MODO_SCELTA_SINGOLA:
#ifdef USA_RES_URNA
      res_scelta(u, voti_validi, txt, l_max, fp_res);
#else
      res_scelta(u, voti_validi, txt, l_max);
#endif
      break;

   case MODO_SCELTA_MULTIPLA:
#ifdef USA_RES_URNA
      res_scelta_multipla(u, voti_validi, txt, l_max, fp_res);
#else
      res_scelta_multipla(u, voti_validi, txt, l_max);
#endif
      break;

   case MODO_VOTAZIONE:
#ifdef USA_RES_URNA
      res_votazione(u, voti_validi, txt, l_max, fp_res);
#else
      res_votazione(u, voti_validi, txt, l_max);
#endif
      break;
   case MODO_PROPOSTA:
#ifdef USA_RES_URNA
      res_proposta(u, voti_validi, txt, fp_res);
#else
      res_proposta(u, voti_validi, txt);
#endif
      break;
   }

   if(ucf->bianca) {
      txt_put(txt, " ");
      txt_putf(txt, "Hanno votato scheda bianca:"
               " <b>%3ld</b> utenti.", bianche);
#ifdef USA_RES_URNA
      fwrite("**", sizeof(long), 3, fp_res);
      fwrite(&bianche, sizeof(long), 1, fp_res);
#endif
   }

   txt_put(txt, " ");
   txt_putf(txt,
            "Hanno votato <b>%ld</b> utenti "
            "su <b>%ld</b> aventi diritto al voto.", udt->nvoti, elettori);
   txt_putf(txt,
            "( di cui <b>%ld</b> si sono collegati durante le votazioni)",
            udt->ncoll);
   strncpy(buf, "Avevano diritto al voto tutti gli utenti", LBUF);
   chipuo(ucf, buf1);
   if (strlen(buf1) > 30) {
     txt_putf(txt, "%s", buf);
     txt_putf(txt, "%s", buf1);
   } else {
     strncat(buf,buf1,LBUF);
     txt_put(txt, buf);
   };
   txt_put(txt, " ");
   txt_putf(txt, "(Il quesito &egrave stato proposto da <b>%s</b>.)",
            nome_utente_n(matricola));
   room = room_findn(room_num);
   if(room == NULL) {
      citta_log("room non trovata, uso Lobby");
      room = room_find(lobby);
   }

   /* 
    * ci vorrebbe un txt_move generico (copia tra testi)
	* o una gestione generica
	* (subj formattato, header, footer)
	* dei post lunghi...
    */

   if(txt_len(txt) > MAXLINEEPOST - 2) {

      /*
       * * primo blocco
       */

   txt_rewind(txt);
      txt_parz = txt_create();

      while((ptxt = txt_get(txt)) && (numlin < MAXLINEEPOST - 10)) {
	      txt_putf(txt_parz, "%s", ptxt);
	      numlin++;
      }

      txt_put(txt_parz,"");
      txt_putf(txt_parz, "continua...");
      txt_put(txt_parz, "");
	  txt_rewind(txt_parz);
      citta_post(room, subj, txt_parz);
	  numlin=0;

      /*
       * * blocchi successivi
       * * 
       */
      txt_clear(txt_parz);

      txt_putf(txt_parz, "*** <b>Quesito</b>: <b>%s</b>(-%d-) ***",
               ucf->titolo, parte_res);
      while((ptxt = txt_get(txt))) {
         txt_put(txt_parz,ptxt);
         numlin++;
         if(numlin > MAXLINEEPOST - 10) {
            if(ucf->tipo==TIPO_REFERENDUM)
               sprintf(subj, "Risultati Referendum #%ld (%d)", progressivo,
                       parte_res);
            else
               sprintf(subj, "Risultati Sondaggio #%ld (%d)", progressivo,
                       parte_res);
            citta_post(room, subj, txt_parz);
            parte_res++;
            txt_clear(txt_parz);
            txt_putf(txt_parz, "*** <b>Quesito</b>: <b>%s</b>(-%d-) ***",
                     ucf->titolo, parte_res);
            numlin = 0;
         }
      }
	  if (numlin!=0){
            if(ucf->tipo==TIPO_REFERENDUM)
               sprintf(subj, "Risultati Referendum #%ld (%d)", progressivo,
                       parte_res);
            else
               sprintf(subj, "Risultati Sondaggio #%ld (%d)", progressivo,
                       parte_res);
      citta_post(room, subj, txt_parz);
	  }
      txt_clear(txt_parz);
   } else {
      citta_post(room, subj, txt);
   }

   txt_rewind(txt);
   while((ptxt = txt_get(txt))) 
      fprintf(fp_room_like, "%s\n", ptxt);

   txt_free(&txt);
   /* send to user ? */
   fclose(fp_room_like);
#ifdef USA_RES_URNA
   fclose(fp_res);
#endif
   return (0);
}

#ifdef USA_RES_URNA
int res_si_no(struct urna_voti *uvt, int voti_validi, struct text *txt,
              FILE * fp_res)
#else
int res_si_no(struct urna_voti *uvt, int voti_validi, struct text *txt)
#endif
{

   int voti_voce;
   double long voti_perc;

#ifdef URNA_GIUDIZI
   if(uvt->giudizi == NULL) {
      return (-1);
   }
#endif
   voti_voce = (uvt + CODIFICA_SI)->num_voti;
   voti_perc = 100. * (double)voti_voce / (double)voti_validi;
   txt_putf(txt,
            "Hanno risposto <b>S&iacute</b>:<b> %3d</b> utenti"
            " (<b>%.1Lf</b>%%)", voti_voce, voti_perc);
#ifdef USA_RES_URNA
   fwrite("SI", sizeof(char), 3, fp_res);
   fwrite(&voti_voce, sizeof(long), 1, fp_res);
#endif

   voti_voce = (uvt + CODIFICA_NO)->num_voti;
   voti_perc = 100. * (double)voti_voce / (double)voti_validi;
   txt_putf(txt, "Hanno risposto <b>No</b>:<b> %3d</b>"
            " utenti (<b>%.1Lf</b>%%)", voti_voce, voti_perc);
#ifdef USA_RES_URNA
   fwrite("NO", sizeof(char), 3, fp_res);
   fwrite(&voti_voce, sizeof(long), 1, fp_res);
#endif
   return 0;
}

#ifdef USA_RES_URNA
int res_scelta(struct urna *u, int nvoti, struct text *txt,
               int l_max, FILE * fp_res)
#else
int res_scelta(struct urna *u, int nvoti, struct text *txt, int l_max)
#endif
{
   int j;
   int voti_voce;
   double long voti_perc;
   struct urna_voti *uvt;
   char **voce;
   int num_voci;

   num_voci = u->conf->num_voci;
   uvt = u->dati->voti;
   voce = u->conf->voci;

      txt_putf(txt, " # %*s: %*s  voti  "
               "perc", l_max/2,"voce",l_max/2,"" );
   for(j = 0; j < num_voci; j++) {
      voti_voce = (uvt + j)->num_voti;
      voti_perc = 100. * (double)voti_voce / (double)nvoti;
      txt_putf(txt, "(<b>%c</b>) %*s:  <b>%3d</b> "
               "  <b>%5.2Lf</b>%%",
               'a' + j, l_max, *(voce + j), voti_voce, voti_perc);
#ifdef USA_RES_URNA
      fwrite(*(voce + j), sizeof(char), strlen(*(voce + j)), fp_res);
#endif
   }
#ifdef USA_RES_URNA
   fwrite(&voti_voce, sizeof(long), 1, fp_res);
#endif
   return 0;

}

#ifdef USA_RES_URNA
int res_scelta_multipla(struct urna *u, int nvoti, struct text *txt,
                        int l_max, FILE * fp_res)
#else
int res_scelta_multipla(struct urna *u, int nvoti, struct text *txt,
                        int l_max)
#endif
{
   int j;
   struct urna_voti *uvt;
   char **voce;
   int voti_voce;
   double long voti_perc;
   int num_voci;

   num_voci = u->conf->num_voci;
   uvt = u->dati->voti;
   voce = u->conf->voci;

      txt_putf(txt, " # %*s: voti %*s "
               "percentuali votanti", l_max/2,"voce",l_max/2,"" );
   for(j = 0; j < num_voci; j++) {
      voti_voce = (uvt + j)->num_voti;
      voti_perc = 100. * (double)voti_voce / (double)nvoti;
      txt_putf(txt, "(%c) %*s: <b>%d</b> "
               " <b>%5.2Lf</b>%% ",
               'a' + j, l_max, *(voce + j), voti_voce, voti_perc);
#ifdef USA_RES_URNA
      fwrite(*(voce + j), sizeof(char), strlen(*(voce + j)), fp_res);
#endif
   }
#ifdef USA_RES_URNA
   fwrite(&voti_voce, sizeof(long), 1, fp_res);
#endif
   return 0;

};

#ifdef USA_RES_URNA
int res_votazione(struct urna *u, int nvoti, struct text *txt,
                  int l_max, FILE * fp_res)
#else
int res_votazione(struct urna *u, int nvoti, struct text *txt, int l_max)
#endif
{
   int j;
   long int voti_voce, tot_voce;
   double long voti_media;
   struct urna_voti *uvt;
   char **voce;
   int num_voci;

   uvt = u->dati->voti;
   voce = u->conf->voci;
   num_voci=u->conf->num_voci;

   txt_putf(txt, " # %*s          media   totale   votanti   astenuti ", l_max, "voce");
   for(j = 0; j < num_voci; j++) {
      voti_voce = (uvt + j)->num_voti;
      tot_voce = (uvt + j)->tot_voti;
      if(voti_voce == 0) {
         voti_media = 0;
      } else {
         voti_media = (double)tot_voce / (double)voti_voce;
      }
      txt_putf(txt,
               "(%c) %*s:           <b>%.1Lf</b>       <b>%ld</b>"
               "        <b>%ld</b>       <b>%ld</b>",
               'a' + j, l_max, *(voce + j), voti_media, tot_voce,
               voti_voce, (uvt+j)->astensioni);
#ifdef USA_RES_URNA
      fwrite(*(voce + j), sizeof(char), strlen(*(voce + j)), fp_res);
      fwrite(&voti_voce, sizeof(long), 1, fp_res);
      fwrite("**", sizeof(char), 3, fp_res);
      fwrite(&(uvt + j)->astensioni, sizeof(long), 1, fp_res);
#endif
   }
   return 0;
}

#ifdef USA_RES_URNA
int res_proposta(struct urna *u, int nvoti, struct text *txt, FILE * fp_res)
#else
int res_proposta(struct urna *u, int nvoti, struct text *txt)
#endif
{
   FILE *pr;
   int progressivo;
   int j;
   int num_prop, num_cifre;
   int linea;
   char **testo;

   progressivo = u->progressivo;
   testo=NULL;

   /*
    * * salva quel che potrebbe essere
    * * rimasto
    */

   save_prop(u->prop, progressivo);
   u->prop=NULL;

   if(load_file(URNA_DIR, FILE_UPROP, progressivo, &pr)) {
      citta_logf("SYSLOG: problemi con %s", FILE_UDATA);
      return (-1);
   }

   if(!check_magic_number(pr, MAGIC_PROP, "prop")) {
      citta_logf("SYSLOG: urna-prop %s %d non corretto", FILE_UPROP, progressivo);
      fclose(pr);
      return (-1);
   }
   num_prop = read_prop(progressivo, nvoti, &testo);
   if (num_prop==0){
      txt_putf(txt, "non ci sono state proposte");
      return 0;
   }

   txt_put(txt, "");

   linea=0;
   num_cifre=1;
   if(num_prop>9)
		   num_cifre=2;
   if(num_prop>99)
		   num_cifre=3;
   if(num_prop>999)
		   num_cifre=4;
   if(num_prop>9999)
		   num_cifre=5;

  /* 
   * e poi?
   */ 

   for(j = 0; j < num_prop; j++) {
      if(*(testo + j) == NULL)
         continue;
      txt_putf(txt, "<fg=4><b>proposta %*d:</b><fg=7> %s",
					  num_cifre,linea+1, *(testo + j));
	  if ((linea+1)%10==0)
			  txt_put(txt,"");
	  linea++;
      free(*(testo + j));
   }

   if(testo!=NULL)
	   free(testo);

   return 0;
};
