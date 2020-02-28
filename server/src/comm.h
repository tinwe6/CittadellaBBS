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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: comm.h                                                              *
*       costanti per il protocollo di comunicazione client/server.          *
****************************************************************************/

#ifndef _COMM_H
#define _COMM_H   1

/* Identificatori stringhe dal server */

/*                   N.B.  DA RISTRUTTURARE!!!                           */

#define ERROR            100
#define GIA_COLLEGATO      1
#define ESCI              10
#define ACC_PROIBITO      20
#define NO_MAIL            1      /* L'utente non ha accesso ai mail */
#define NO_ADMIN           2      /* No admin post                   */
#define TOO_MANY           3      /* Hai lasciato troppi post!       */
#define POST_PROIBITO      4      /* SUT_NOPOST                      */
#define MAIL_PROIBITO      5      /* SUT_NOMAIL                      */
#define ANONYM_PROIBITO    6      /* SUT_NOANONYM                    */
#define PASSWORD          30
#define ARG_NON_VAL       40
#define ARG_VUOTO         41
#define NO_SUCH_USER      50
#define NO_SUCH_FILE      60
#define NO_SUCH_ROOM      70
#define ERR_BASIC_ROOM    71
#define ERR_VIRTUAL_ROOM  72
#define NO_SUCH_MSG       75
#define NO_SUCH_FM        80
#define WRONG_STATE       90

/* File Server */
#define FS_NO_BOOK        10
#define FS_WRONG_BOOK     20
#define FS_NOT_HERE       30
#define FS_TOO_BIG        40
#define FS_USERSPACE      50
#define FS_TOO_MANY_FILES 60
#define FS_FULL           70

/* Errori X */
#define X_ACC_PROIBITO           1
#define X_NO_SUCH_USER           2
#define X_EMPTY                  3
#define X_TXT_NON_INIT           4
#define X_UTENTE_NON_COLLEGATO   5
#define X_SELF                  10
#define X_RIFIUTA               11
#define X_SCOLLEGATO            12
#define X_OCCUPATO              13
#define X_UTENTE_CANCELLATO     14
#define X_NO_DEST               15

/* Nuovo protocollo di trasmissione (Ancora da decidere...)
#define ERROR            100
 10*d tipo: ROOM, FM, MSG, USER
    d  err: NO_SUCH, PROIBITO
#define GIA_COLLEGATO      1
#define PASSWORD          10
#define BXCHAT            20
#define NO_SUCH_USER      30
#define NO_SUCH_FILE      40
#define NO_SUCH_ROOM      50
#define NO_SUCH_FM        60
#define NO_SUCH_MSG       70
#define RIFIUTA           90
#define ESCI              10
#define ACC_PROIBITO      20
#define ARG_NON_VAL       40
*/

#define OK               200
#define SEGUE_LISTA      210
#define NUOVO_UTENTE     220
#define UT_OSPITE        221
#define NON_VALIDATO     222
#define UT_VALIDATO      223
#define PRIMO_UT          10 /* Primo utente a collegarsi -> Sysop */
#define INVIA_TESTO      230 /* Ok, invia testo con TEXT           */
#define INVIA_PARAMETRI  240 /* Ok, invia parametri con PARM       */
#define SEGUE_BINARIO    250 /* Seguono dati in binario            */
#define INVIA_BINARIO    260 /* Ok, invia dati in binario          */

#define SET_BINARY       300 /* Inizia l'invio di dati binari      */
#define END_BINARY       310 /* Fine invio dati in binario         */


/* 100-199 OK */

/* 200-799 ERRORE : 200: USER 300: ROOM 400: MSG 500: FM 600: FLOOR */

/* 800-899 Comandi di esecuzione immediata                     */
#define TOUT             800 /* login TimeOUT                  */
#define ITOU             810 /* Idle TimeOUt                   */
#define KOUT             820 /* Kick OUT: utente cacciato      */
#define SDWN             830 /* Shutdown attivato              */
#define SDWC             831 /* Shutdown cancellato            */
#define RHST             840 /* Richiede l'host remoto         */
#define LOBD             850 /* Room cancellata -> va in Lobby */
#define PTOU             860 /* Post TimeOUt                   */

/* 900-999 Comandi che il client esegue quando puo`   */
#define BHDR             900 /* Broadcast HeaDeR      */
#define BTXT             902 /* Broadcast TeXT        */
#define BEND             903 /* Broadcast END         */
#define XHDR             901 /* Xmsg HeaDeR           */
#define XTXT             902 /* Xmsg TeXT             */
#define XEND             903 /* Xmsg END              */
#define CHDR             904 /* Chat HeaDeR           */
#define CTXT             905 /* Chat TeXT             */
#define CEND             906 /* Chat END              */
#define DEST             907 /* Aggiunge destinatario */
#define LGIN             910 /* Notifica Login        */
#define LGOT             911 /* Notifica Logout       */
#define LGIC             912 /* Not. entrata in chat  */
#define LGOC             913 /* Not. uscita chat      */
#define DROP             914 /* Dropped link          */
#define LOID             915 /* Logout: idle timeout  */
#define COUT             916 /* Chat timeOUT          */
#define LOIC             917 /* Logout: Idle Chat     */
#define IWRN             920 /* Idle Warning          */
#define ILGT             921 /* Idle Logout (Ordine)  */
#define NPST             930 /* Notifica nuovo Post nella room corrente */
#define NNML             931 /* Notifica nuovo mail   */
#define NPMS             932 /* Nuovo Post mentre scrivevi     */
#define NBLG             933 /* Nuovo messaggio nel blog       */
#define RFLG             940 /* Forza la rilettura dei flag    */

/*****************************************************************************/

/* Stato della connessione */
#define CON_LIC          (1<<1)       /* Login in corso                   */
#define CON_COMANDI      (1<<2)       /* In attesa di un comando          */
#define CON_LOCK         (1<<3)       /* Connessione lockata              */
#define CON_SUBSHELL     (1<<4)       /* Utente nella subshell            */
#define CON_REG          (1<<5)       /* Modifica registrazione           */
#define CON_CONF         (1<<6)       /* Edit configurazione              */
#define CON_PROFILE      (1<<7)       /* Edit profile personale           */
#define CON_BROAD        (1<<8)       /* Invia un broadcast               */
#define CON_XMSG         (1<<9)       /* Invia un X                       */
#define CON_CHAT        (1<<10)       /* Invia chat message               */
#define CON_POST        (1<<11)       /* Scrive un messaggio              */
#define CON_ROOM_EDIT   (1<<12)       /* Editing room                     */
#define CON_ROOM_INFO   (1<<13)       /* Edita le room info               */
#define CON_FLOOR_EDIT  (1<<14)       /* Editing room                     */
#define CON_FLOOR_INFO  (1<<15)       /* Edita le room info               */
#define CON_BANNER      (1<<16)       /* Edita banner                     */
#define CON_NEWS        (1<<17)       /* Edita le news                    */
#define CON_REFERENDUM  (1<<18)       /* Edit new referendum              */
#define CON_REF_PARM    (1<<19)       /* Edit refererendum con invio parm */
#define CON_BUG_REP     (1<<20)       /* Bug Report                       */
#define CON_EUSR        (1<<21)       /* Edit user                        */
#define CON_CHIUSA      (1<<22)       /* Connessione chiusa               */
#define CON_REF_VOTO    (1<<23)       /* Vota un referendum               */
#define CON_UPLOAD      (1<<24)       /* Upload file in corso             */
#define CON_CONSENT     (1<<25)       /* Waiting for user consent         */
#define CON_NOCHECK        (~0)       /* Non fare check sullo stato       */

#define CON_MASK_COM   ( CON_BROAD | CON_XMSG | CON_CHAT)
#define CON_MASK_USER  ( CON_CONF | CON_PROFILE | CON_REG)
#define CON_MASK_ADMIN ( CON_ROOM_EDIT | CON_ROOM_INFO | CON_BANNER \
                        | CON_NEWS | CON_FLOOR_EDIT | CON_FLOOR_INFO \
                        | CON_EUSR )
#define CON_MASK_REF   ( CON_REFERENDUM | CON_REF_PARM )
#define CON_MASK_TABC  ( CON_COMANDI | CON_XMSG | CON_POST)

/* Stato comandi che richiedono invio testo con TEXT */
#define CON_MASK_TEXT  ( CON_MASK_COM | CON_PROFILE | CON_ROOM_INFO \
		        | CON_BANNER | CON_NEWS | CON_BUG_REP       \
                        | CON_REFERENDUM | CON_POST | CON_FLOOR_INFO)

/* Stato comandi che richiedono invio dei parametri con PARM */
#define CON_MASK_PARM  ( CON_REF_PARM | CON_REF_VOTO)

#endif /* comm.h */
