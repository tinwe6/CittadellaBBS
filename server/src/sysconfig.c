/*
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/*****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello   *
*                                                                            *
* File: sysconfig.c                                                          *
*       comandi per visualizzare/modificare la configurazione del sistema.   *
*****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cittaserver.h"
#include "extract.h"
#include "generic.h"
#include "cache_post.h"
#include "memstat.h"
#include "room_flags.h"
#include "rooms.h"
#include "sysconfig.h"
#include "sys_flags.h"
#include "urna-comm.h"
#include "utility.h"

/*********    Prototipi delle funzioni in questo file   *************/
bool si_no(char *buf);
void legge_configurazione(void);
void sysconfig(char cmd[][256], char val[][256], int n_cmd);
void cmd_rsys(struct sessione *t, char *cmd);
void cmd_esys(struct sessione *t, char *buf);
void cmd_rslg(struct sessione *t);
void cmd_rsst(struct sessione *t);

/****************************************************************************/
/* returns true if buf is 'Si' or variant, or any positive number
 * returns false if buf is 'No' or variant, or 0.
 */
bool si_no(char *buf)
{
        if (!strncmp(buf,"SI",2)||!strncmp(buf,"Si",2)||!strncmp(buf,"si",2))
                return true;
        if (!strncmp(buf,"NO",2)||!strncmp(buf,"No",2)||!strncmp(buf,"no",2))
                return false;
        return (bool)atoi(buf);
}

/* Legge e interpreta la configurazione del server in sysconfig.rc          */

void legge_configurazione(void) 
{
        FILE *fp;
        char buf[LBUF], buf1[LBUF];
  
        /* Apre il file sysconfig */
  
        fp = fopen(FILE_SYSCONFIG, "r");
        if (fp == NULL) {
                citta_log("SYSTEM: file_sysconfig inesiste: creazione al prossimo shutdown.");
                return;
        }

        /* legge linea di input */
        while (fgets(buf, LBUF, fp) != NULL) {
                /* le righe che iniziano con '#' sono commenti */
                if (buf[0] != '#'){
                        /* elimina gli spazi alla fine */
                        while (isspace(buf[strlen(buf)-1]))
                                buf[strlen(buf)-1] = 0;

                        if (buf[0] != 0) { /* Salta le righe vuote */
                                if (!strncmp(buf, "TIC_TAC=", 8))
                                        TIC_TAC = atoi(&buf[8]);
                                else if (!strncmp(buf, "FREQUENZA=", 10))
                                        FREQUENZA = atoi(&buf[10]);
                                else if (!strncmp(buf, "citta_soft=", 11)){
                                        strncpy(citta_soft, &buf[11], 49);
                                        citta_soft[49] = 0;
                                } else if (!strncmp(buf, "citta_ver=", 10)) {
                                        strncpy(citta_ver, &buf[10], 9);
                                        citta_ver[9] = 0;
                                } else if (!strncmp(buf, "citta_ver_client=",
                                                  10)) {
                                        strncpy(citta_ver_client,&buf[10],9);
                                        citta_ver_client[9] = 0;
                                } else if (!strncmp(buf, "citta_nodo=", 11)){
                                        strncpy(citta_nodo, &buf[11], 49);
                                        citta_nodo[49] = 0;
                                } else if (!strncmp(buf, "citta_dove=", 11)){
                                        strncpy(citta_dove, &buf[11], 49);
                                        citta_dove[49] = 0;
#if 0
                                } else if (!strncmp(buf, "FILE_UTENTI=", 12)){
                                        strcpy(FILE_UTENTI, &buf[12]);
#endif
				} else if (!strncmp(buf,
					    "livello_alla_creazione=", 23))
                                        livello_alla_creazione=atoi(&buf[23]);
                                else if (!strncmp(buf,"express_per_tutti=",18))
                                        express_per_tutti = si_no(&buf[18]);
                                else if (!strncmp(buf, "min_msgs_per_x=", 15))
                                        min_msgs_per_x = atoi(&buf[15]);
                                else if (!strncmp(buf, "butta_fuori_se_idle=",
                                                  20))
                                        butta_fuori_se_idle = si_no(&buf[20]);
                                else if (!strncmp(buf,"min_idle_warning=",17))
                                        min_idle_warning = atoi(&buf[17]);
                                else if (!strncmp(buf, "min_idle_logout=", 16))
                                        min_idle_logout = atoi(&buf[16]);
                                else if (!strncmp(buf, "max_min_idle=", 13))
                                        max_min_idle = atoi(&buf[13]);
                                else if (!strncmp(buf,"login_timeout_min=",18))
                                        login_timeout_min = atoi(&buf[18]);
                                else if (!strncmp(buf, "auto_save=", 10))
                                        auto_save = si_no(&buf[10]);
                                else if (!strncmp(buf, "tempo_auto_save=", 16))
                                        tempo_auto_save = atoi(&buf[16]);
                                else if (!strncmp(buf,"nameserver_lento=",17))
                                        nameserver_lento = si_no(&buf[17]);
                                else if (!strncmp(buf, "lobby=", 6)) {
                                        strncpy(lobby,&buf[6],ROOMNAMELEN-1);
                                        lobby[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "sysop_room=", 11)) {
                                        strncpy(sysop_room, &buf[11],
                                                ROOMNAMELEN-1);
                                        sysop_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "aide_room=", 10)) {
                                        strncpy(aide_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        aide_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "ra_room=", 8)) {
                                        strncpy(ra_room, &buf[8],
                                                ROOMNAMELEN-1);
                                        ra_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "mail_room=", 10)) {
                                        strncpy(mail_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        mail_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "twit_room=", 10)) {
                                        strncpy(twit_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        twit_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "dump_room=", 10)) {
                                        strncpy(dump_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        dump_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "find_room=", 10)) {
                                        strncpy(find_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        find_room[ROOMNAMELEN-1] = 0;
                                } else if (!strncmp(buf, "blog_room=", 10)) {
                                        strncpy(blog_room, &buf[10],
                                                ROOMNAMELEN-1);
                                        blog_room[ROOMNAMELEN-1] = 0;
                                } else {
                                        strcpy(buf1,"SYSERR: linea non riconosciuta in sysconfig: ");
                                        strcat(buf1, buf);
                                        citta_log(buf1);
                                }
                        }
                }
        }
        fclose(fp);

        /* TODO eliminare TIC_TAC o frequenza dal file di configurazione */
        TIC_TAC = 1000000 / FREQUENZA; /* microsecondi tra i cicli del server */
}

void sysconfig(char cmd[][256], char val[][256], int n_cmd)
{
        FILE *fp, *fp1;
        char buf[LBUF];
        char bak[LBUF];
        char fatto[20], ok;
        int n=0;

        /* Backup del file di configurazione */
        sprintf(bak,"%s.bak",FILE_SYSCONFIG);
        sprintf(buf,"touch %s",FILE_SYSCONFIG);  /* DA SISTEMARE!!! */
        system(buf);
        sprintf(buf,"mv %s %s",FILE_SYSCONFIG,bak);
        system(buf);

        fp = fopen(FILE_SYSCONFIG,"w");
        if (fp==NULL) {
                citta_log("SYSERR: Non posso aprire in scrittura sysconfig.rc");
                return;
        }
        fp1 = fopen(bak,"r");
        if (fp1 == NULL)
                citta_log("SYSERR: Non posso aprire in lettura sysconfig.rc.bak");

        for(n = 0; n < n_cmd; n++)
                fatto[n] = 0;

        /* Ricopia riga per riga il file di backup e sostituendo il valore */
        /* nuovo del parametro da cambiare.                                */
        while (fgets(buf, LBUF, fp1) != NULL) {
                ok = 0;
                for (n = 0; n < n_cmd; n++){
                        if(!strncmp(buf, cmd[n], strlen(cmd[n]))){
                                fprintf(fp, "%s%s\n", cmd[n], val[n]);
                                fatto[n] = 1;
                                ok = 1;
                        }
                }
                if (!ok)
/* &&(buf[0]!=13)&&(buf[0]!=10)) Per cancellare le righe vuote */ 
                        fprintf(fp, "%s", buf);
        }

        /* se il parametro non era presente nel file lo aggiunge in fondo */
        for (n = 0; n < n_cmd; n++)
                if (!fatto[n])
                        fprintf(fp, "%s%s\n", cmd[n], val[n]);
        
        /* chiude i files */
        fclose(fp);
        fclose(fp1);
}

/* Spedisce al client la configurazione del server */

void cmd_rsys(struct sessione *t, char *cmd)
{
	/* 1 - Configurazione del sistema */
	if ((cmd[0] == '0') || (cmd[0] == '1'))
		cprintf(t, "%d|1|%ld|%d|%d|%d|%d|%d|%d|%d\n", OK, TIC_TAC,
			FREQUENZA, livello_alla_creazione, express_per_tutti,
			min_msgs_per_x, auto_save, tempo_auto_save,
			nameserver_lento);
	/* 2 - Info sul server */
	if ((cmd[0] == '0') || (cmd[0] == '2'))
		cprintf(t, "%d|2|%s|%s|%s|%s|%s\n", OK, citta_soft, citta_ver,
			citta_ver_client, citta_nodo, citta_dove);
	/* 3 - Path dei files */
	if ((cmd[0] == '0') || (cmd[0] == '3'))
		cprintf(t, "%d|3|%s\n", OK, FILE_UTENTI);
	/* 4 - Idle */
	if ((cmd[0] == '0') || (cmd[0] == '4'))
		cprintf(t, "%d|4|%d|%d|%d|%d|%d\n", OK,	butta_fuori_se_idle,
			min_idle_warning, min_idle_logout, max_min_idle,
			login_timeout_min);
	/* 5 - Nome rooms principali */
	if ((cmd[0] == '0') || (cmd[0] == '5'))
		cprintf(t, "%d|5|%s|%s|%s|%s|%s|%s|%s|%s|%s\n", OK, lobby,
			sysop_room, aide_room, ra_room,	mail_room, twit_room,
			dump_room, find_room, blog_room);
}

/* Questo comando permette di cambiare la configurazione di sistema  */
/* Si chiama con "ESYS %d|..." dove il primo intero corrisponde alla */
/* parte di configurazione da cambiare.                              */
void cmd_esys(struct sessione *t, char *buf)
{
        char aaa[LBUF];
        long aa;
        int a;
        /* in questa tabella i comandi di modifica di sysconf */
        char cmd[20][256];
        /* Nuovi valori delle variabili da modificare */
        char val[20][256];
        int n_cmd, n_file;

        n_cmd = 0;
        switch(buf[0]){
	 case '1':
                /* 1 - Configurazione del sistema */
                a = extract_int(buf,2);
                if (a!=FREQUENZA){
                        sprintf(cmd[n_cmd], "FREQUENZA=");
                        sprintf(val[n_cmd], "%d",a);
                        n_cmd++;
                        FREQUENZA=a;
                }
                //aa=extract_long(buf,1);
                aa = 1000000 / FREQUENZA;
                if (aa != TIC_TAC) {
                        sprintf(cmd[n_cmd], "TIC_TAC=");
                        sprintf(val[n_cmd], "%ld",aa);
                        n_cmd++;
                        TIC_TAC = aa;
                }
                a = extract_int(buf,3);
                if (a != livello_alla_creazione) {
                        sprintf(cmd[n_cmd], "livello_alla_creazione=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        livello_alla_creazione = a;
                }
                a = extract_bool(buf,4);
                if (a != express_per_tutti){
                        sprintf(cmd[n_cmd], "express_per_tutti=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        express_per_tutti = a;
                }
                a = extract_int(buf,5);
                if (a!=min_msgs_per_x){
                        sprintf(cmd[n_cmd], "min_msgs_per_x=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        min_msgs_per_x = a;
                }
                a = extract_bool(buf,6);
                if (a != auto_save){
                        sprintf(cmd[n_cmd], "auto_save=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        auto_save = a;
                }
                a = extract_int(buf,7);
                if (a!=tempo_auto_save){
                        sprintf(cmd[n_cmd], "tempo_auto_save=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        tempo_auto_save = a;
                }
                a = extract_bool(buf, 8);
                if (a != nameserver_lento){
                        sprintf(cmd[n_cmd], "nameserver_lento=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        nameserver_lento = a;
                }
                break;
                
	 case '2':
                /* 2 - Info sul server */
                extract(aaa,buf,1);
                if (strcmp(aaa,citta_soft)){
                        sprintf(cmd[n_cmd],"citta_soft=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(citta_soft,aaa);
                }
                extract(aaa,buf,2);
                if (strcmp(aaa,citta_ver)){
                        sprintf(cmd[n_cmd],"citta_ver=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(citta_ver,aaa);
                }
                extract(aaa,buf,3);
                if (strcmp(aaa,citta_ver_client)){
                        sprintf(cmd[n_cmd],"citta_ver_client=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(citta_ver_client,aaa);
                }
                extract(aaa,buf,4);
                if (strcmp(aaa,citta_nodo)){
                        sprintf(cmd[n_cmd],"citta_nodo=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(citta_nodo,aaa);
                }
                extract(aaa,buf,5);
                if (strcmp(aaa,citta_dove)){
                        sprintf(cmd[n_cmd],"citta_dove=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(citta_dove,aaa);
                }
                break;
	 case '3':
                /* 3 - Path dei files */
                n_file = extract_int(buf,1);
                extract(aaa,buf,2);
                switch(n_file){
                case 1: /* File Utenti */
                        if (strcmp(aaa,FILE_UTENTI)){
                                sprintf(cmd[n_cmd],"FILE_UTENTI=");
                                strcpy(val[n_cmd],aaa);
                                n_cmd++;
                                strcpy(FILE_UTENTI,aaa);
                        }
                }
                break;
	 case '4':
                /* 4 - Idle */
                a = extract_bool(buf,1);
                if (a != butta_fuori_se_idle){
                        sprintf(cmd[n_cmd], "butta_fuori_se_idle=");
                        sprintf(val[n_cmd], "%d", a);
                        n_cmd++;
                        butta_fuori_se_idle = a;
                }
                a = extract_int(buf,2);
                if (a!=min_idle_warning){
                        sprintf(cmd[n_cmd],"min_idle_warning=");
                        sprintf(val[n_cmd],"%d",a);
                        n_cmd++;
                        min_idle_warning=a;
                }
                a = extract_int(buf,3);
                if (a!=min_idle_logout){
                        sprintf(cmd[n_cmd],"min_idle_logout=");
                        sprintf(val[n_cmd],"%d",a);
                        n_cmd++;
                        min_idle_logout=a;
                }
                a = extract_int(buf,4);
                if (a!=max_min_idle){
                        sprintf(cmd[n_cmd],"max_min_idle=");
                        sprintf(val[n_cmd],"%d",a);
                        n_cmd++;
                        max_min_idle=a;
                }
                a = extract_int(buf,5);
                if (a!=login_timeout_min){
                        sprintf(cmd[n_cmd],"login_timeout_min=");
                        sprintf(val[n_cmd],"%d",a);
                        n_cmd++;
                        login_timeout_min=a;
                }
                break;
	 case '5':
                /* 5 - Nome rooms principali */
                extract(aaa,buf,1);
                if (strcmp(aaa,lobby)){
                        sprintf(cmd[n_cmd],"lobby=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(lobby,aaa);
                }
                extract(aaa,buf,2);
                if (strcmp(aaa,sysop_room)){
                        sprintf(cmd[n_cmd],"sysop_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(sysop_room,aaa);
                }
                extract(aaa,buf,3);
                if (strcmp(aaa,aide_room)){
                        sprintf(cmd[n_cmd],"aide_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(aide_room,aaa);
                }
                extract(aaa,buf,4);
                if (strcmp(aaa,ra_room)){
                        sprintf(cmd[n_cmd],"ra_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(ra_room,aaa);
                }
                extract(aaa,buf,5);
                if (strcmp(aaa,mail_room)){
                        sprintf(cmd[n_cmd],"mail_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(mail_room,aaa);
                }
                extract(aaa,buf,6);
                if (strcmp(aaa,twit_room)){
                        sprintf(cmd[n_cmd],"twit_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(twit_room,aaa);
                }
                extract(aaa,buf,7);
                if (strcmp(aaa,dump_room)){
                        sprintf(cmd[n_cmd],"dump_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(dump_room,aaa);
                }
                extract(aaa,buf,8);
                if (strcmp(aaa,find_room)){
                        sprintf(cmd[n_cmd],"find_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(find_room,aaa);
                }
                extract(aaa,buf,9);
                if (strcmp(aaa,blog_room)){
                        sprintf(cmd[n_cmd],"blog_room=");
                        strcpy(val[n_cmd],aaa);
                        n_cmd++;
                        strcpy(blog_room,aaa);
                }
                break;
        }
        if (n_cmd>0) /* ci sono delle modifiche da salvare */
                sysconfig(cmd,val,n_cmd);
	cprintf(t, "%d\n", OK);
}

/* Spedisce al client il file di log del sistema */

void cmd_rslg(struct sessione *t)
{
	legge_file(t,"./syslog");
}

/*
 * Read Server STatistics
 */
void cmd_rsst(struct sessione *t)
{
        char buf[LBUF];
	int i;

	cprintf(t, "%d\n", SEGUE_LISTA);
	cprintf(t,"%d %d|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld|%ld\n",
		OK, SYSSTAT_GENERAL, dati_server.server_runs,
		dati_server.matricola, dati_server.login, dati_server.ospiti,
		dati_server.nuovi_ut, dati_server.validazioni,
		dati_server.connessioni, dati_server.local_cl,
		dati_server.remote_cl, dati_server.max_users,
		dati_server.max_users_time);
#ifdef USE_CITTAWEB
	cprintf(t,"%d %d|%ld|%ld|%ld|%ld|%ld|%ld|%ld\n", OK, SYSSTAT_CITTAWEB,
		dati_server.http_conn, dati_server.http_conn_ok,
		dati_server.http_room, dati_server.http_userlist,
		dati_server.http_help, dati_server.http_profile,
		dati_server.http_blog);
#endif	
	cprintf(t,"%d %d|%ld|%ld|%ld|%ld|%ld|%ld\n",
		OK, SYSSTAT_MSG, dati_server.X, dati_server.broadcast,
		dati_server.mails, dati_server.posts, dati_server.chat,
		dati_server.blog);
#ifdef USE_REFERENDUM
	send_ustat(t);
#endif
	cprintf(t,"%d %d|%ld|%ld|%ld|%ld\n",
		OK, SYSSTAT_ROOMS, dati_server.room_num,
		dati_server.room_curr, dati_server.utr_nslot,
		dati_server.mail_nslot);
#ifdef USE_FLOORS
	cprintf(t, "%d %d|%ld|%ld\n", OK, SYSSTAT_FLOORS,
		dati_server.floor_num, dati_server.floor_curr);
#endif	
	cprintf(t,"%d %d|%ld|%ld|%ld\n", OK, SYSSTAT_FILEMSG,
		dati_server.fm_num, dati_server.fm_curr, dati_server.fm_basic);
#ifdef USE_CACHE_POST
	cprintf(t, "%d %d|%ld|%ld|%ld\n", OK, SYSSTAT_CACHE, post_cache->num,
		post_cache->requests, post_cache->hits);
#endif	
#ifdef USE_MEM_STAT
	cprintf(t, "%d %d|%ld|%ld\n", SEGUE_LISTA, SYSSTAT_MEMORY, mem_tot(),
		mem_max());
	for (i = 0; i < TYPE_NUM; i++)
		if (mem_stats[i])
			cprintf(t, "%d %-12s : %6ld - %6ld bytes\n", OK,
				type_table[i].type, mem_stats[i],
				mem_stats[i]*type_table[i].size);
	cprintf(t, "000\n");
#endif	
	sprintf(buf, "%d %d|", OK, SYSSTAT_CONN_DAY);
	for (i = 0; i < 24; i++)
		sprintf(buf + strlen(buf), "%ld|",
			dati_server.stat_ore[i]);
	cprintf(t, "%s\n", buf);
	sprintf(buf, "%d %d|", OK, SYSSTAT_CONN_WM);
	for (i = 0; i < 7; i++)
		sprintf(buf + strlen(buf), "%ld|",
			dati_server.stat_giorni[i]);
	for (i = 0; i < 12; i++)
		sprintf(buf + strlen(buf), "%ld|",
			dati_server.stat_mesi[i]);
	cprintf(t, "%s\n", buf);
	cprintf(t, "000\n");
}
