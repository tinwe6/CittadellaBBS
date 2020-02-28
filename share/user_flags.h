/*
 *  Copyright (C) 1999-2006 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server & client          (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: user_flags.h                                                        *
*       definizione dei flag di configurazione utenti e livelli di accesso. *
****************************************************************************/
#ifndef _USER_FLAGS_H
#define _USER_FLAGS_H   1

/* Titoli degli utenti */
#define NOME_DA_BUTTARE "Utente da Buttare"
#define NOME_OSPITE     "Utente Ospite"
#define NOME_NONVAL     "Utente non Validato"
#define NOME_ROMPIBALLE "Utente Rompiballe"
#define NOME_NORMALE    "Utente Normale"
#define NOME_HH         "Utente Helping Hands"
#define NOME_ROOMAIDE   "Room Aide"
#define NOME_FLOORAIDE  "Floor Aide"
#define NOME_AIDE       "Aide"
#define NOME_MASTER     "Aide Master"
#define NOME_SYSOP      "System Operator"

#define NOME_ROOMHELPER "Room Helper"
#define NOME_CITTA      "Cittadella/UX"

/* Livelli minimi per i comandi */
#define MINLVL_AUTOINV    LVL_NORMALE
#define MINLVL_BANNER     LVL_SYSOP /* Level to change/rehash banner file */
#define MINLVL_BIFF       LVL_NON_VALIDATO
#define MINLVL_BLOG       LVL_NORMALE
#define MINLVL_BROADCAST  LVL_AIDE
#define MINLVL_BUGREPORT  LVL_NORMALE
#define MINLVL_CHAT       LVL_NORMALE
#define MINLVL_CONFIG     LVL_AIDE
#define MINLVL_COPY       LVL_AIDE
#define MINLVL_DELFLOOR   LVL_AIDE
#define MINLVL_DELROOM    LVL_AIDE
#define MINLVL_DOING      LVL_NORMALE
#define MINLVL_DOWNLOAD   LVL_NON_VALIDATO /* download dei file */
#define MINLVL_EDITCONFIG LVL_SYSOP
#define MINLVL_EDITPRFL   LVL_NORMALE
#define MINLVL_EDITUSR    LVL_AIDE
#define MINLVL_FIND       LVL_NORMALE
#define MINLVL_FRIENDS    LVL_NORMALE
#define MINLVL_KICKUSR    LVL_AIDE
#define MINLVL_KILLUSR    LVL_SYSOP
#define MINLVL_LOCKTERM   LVL_NON_VALIDATO
#define MINLVL_MOVE       LVL_AIDE
#define MINLVL_NEWFLOOR   LVL_AIDE
#define MINLVL_NEWROOM    LVL_AIDE
#define MINLVL_NEWS       LVL_AIDE     /* Livello minimo per editare le news */
#define MINLVL_POP3       LVL_NORMALE
#define MINLVL_POST       LVL_NORMALE
#define MINLVL_PWDN       LVL_NON_VALIDATO
#define MINLVL_PWDU       LVL_AIDE     /* change password of an user         */
#define MINLVL_R_SYSLOG   LVL_SYSOP
#define MINLVL_READ_CFG   LVL_AIDE
#define MINLVL_READ_STATS LVL_SYSOP
#define MINLVL_REGISTRATION LVL_NON_VALIDATO
#define MINLVL_REFERENDUM LVL_NORMALE
#define MINLVL_REF_DEL    LVL_AIDE
#define MINLVL_REF_STOP   LVL_AIDE
#define MINLVL_ROOMLEN    LVL_AIDE
#define MINLVL_SHUTDOWN   LVL_SYSOP
#define MINLVL_SUBSHELL   LVL_NORMALE
#define MINLVL_SWAPROOM   LVL_AIDE
#define MINLVL_SYSLOG     LVL_SYSOP
#define MINLVL_SYSOPCHAT  LVL_AIDE
#define MINLVL_UPLOAD     LVL_NORMALE  /* Upload dei file */
#define MINLVL_URNA       LVL_NORMALE  /* per accedere ai servizi di voto    */
#define MINLVL_USERCFG    LVL_NON_VALIDATO
#define MINLVL_XMSG       LVL_NORMALE

/****************************************************************************/
/* NON MODIFICARE QUI SOTTO SE NON SAI ___VERAMENTE___ COSA STAI FACENDO!!! */
/****************************************************************************/

/* Livelli di accesso per gli utenti */
#define LVL_DA_BUTTARE    0
#define LVL_OSPITE        1
#define LVL_NON_VALIDATO  2
#define LVL_ROMPIPALLE    3
#define LVL_NORMALE       4
#define LVL_HELPINHANDS   5
#define LVL_ROOMAIDE      6
#define LVL_FLOORAIDE     7
#define LVL_AIDE          8
#define LVL_MASTER        9
#define LVL_SYSOP        10

/* Costanti che definiscono lunghezza stringhe in struttura utente. */
#define MAXLEN_UTNAME    25 /* Lunghezza massima nome utente        */
#define MAXLEN_PASSWORD  35 /* Lunghezza massima password           */
#define MAXLEN_VALKEY    10 /* Lunghezza massima validation key     */
#define MAXLEN_RNAME     30 /* Lunghezza massima nome reale         */
#define MAXLEN_VIA       30 /* Lunghezza massima via                */
#define MAXLEN_CITTA     15 /* Lunghezza massima citta'             */
#define MAXLEN_STATO     15 /* Lunghezza massima stato              */
#define MAXLEN_CAP       10 /* Lunghezza massima C.A.P.             */
#define MAXLEN_TEL       15 /* Lunghezza massima n. tel.            */
#define MAXLEN_EMAIL     32 /* Lunghezza massima email              */
#define MAXLEN_URL       64 /* Lunghezza massima URL home page      */
#define MAXLEN_LASTHOST 256 /* Lunghezza massima ultimo host        */
#define MAXLEN_DOING     27 /* Lunghezza massima 'doing'            */
#define MAXLEN_FIND      60 /* Lunghezza massima stringa di ricerca */
#define NUM_UTFLAGS       8 /* Numero bytes flags utente            */
#define NUM_UTSFLAGS      8 /* Numero bytes flags sys utente        */

/* Flags di configurazione dell'utente                                  */
/* flags[0] : generali                                                  */
#define UT_ESPERTO     1   /* 0: aiuta l'utente                         */
#define UT_EDITOR      2   /* EDITOR, 1: Esterno di default             */
#define UT_LINE_EDT_X  4   /* Usa il line editor per default per gli X  */
#define UT_OLD_EDT     8   /* Usa l'editor interno vecchio se 1         */
#define UTDEF_0        0

/* flags[1] : Terminale                                        */
#define UT_NOPROMPT    1  /* niente prompt dopo i messaggi     */
#define UT_DISAPPEAR   2  /* disappearing message prompts      */
#define UT_PAGINATOR   4  /* Pausa a ogni schermata            */
#define UT_BELL        8  /* Abilita il beep                   */
#define UT_HYPERTERM  16  /* Grassetto                         */
#define UT_ANSI       32  /* ANSI color term                   */
#define UT_ISO_8859   64  /* Use 8-bit ascii extension         */
#define UTDEF_1       (UT_DISAPPEAR | UT_PAGINATOR | UT_BELL)

/* flags[2] : Dati Personali                           */
#define UT_VNOME     1  /* NOME reale Visibile a tutti */
#define UT_VADDR     2  /* Indirizzo visibile a tutti  */
#define UT_VTEL      4  /* Numero telefono visibile    */
#define UT_VEMAIL    8  /* Indirizzo Email visibile    */
#define UT_VURL     16  /* URL home page visibile      */
#define UT_VSEX     32  /* Sesso visibile a tutti :P   */
#define UT_VPRFL    64  /* Profile personale visibile  */
#define UT_VFRIEND 128  /* Dati personali visibili agli amici  */
#define UTDEF_2      0

/* flags[3] : Xmsg e Notifiche                               */
#define UT_X         1  /* accetta Xmsg                      */
#define UT_XFRIEND   2  /* accetta X da FRIEND-list          */
#define UT_LION      4  /* LogIn/Out Notification            */
#define UT_LIONF     8  /* LogIn/Out Notification of Friends */
#define UT_FOLLOWUP 16  /* Follow Up per gli X               */
#define UT_XPOST    32  /* X arrive while reading posts      */
#define UT_NMAIL    64  /* X arrive while reading posts      */
#define UT_MYXCC   128  /* Carbon copy degli X che spedisco nel diario */
#define UTDEF_3      (UT_X | UT_LION | UT_NMAIL)

/* Flags[4] : Rooms                                                          */
#define UT_ROOMSCAN  1  /* Va nella prima room della lista con nuovi msg.    */
#define UT_LASTOLD   2  /* Legge ultimo messaggio vecchio con i nuovi        */
#define UT_AGAINREP  4  /* Fa <a>gain dopo il reply                          */
#define UT_NPOST     8  /* Notifica quando viene lasciato un post nella room */
#define UT_KRCOL    16  /* Lista Known Rooms Incolonnata                     */
#define UT_FLOOR    32  /* Vede i floor e usa la march list con i floor      */
#define UTDEF_4      (UT_LASTOLD | UT_KRCOL | UT_NPOST | UT_FLOOR)

/* Flags[5] : Messaggi                                             */
#define UT_MYMAILCC    1  /* Carbon copy delle mail che spedisco    */
#define UT_MAIL2EMAIL  2  /* Forward dei mail che ricevo via email  */
#define UT_NEWSLETTER  8  /* Ricevo annunci BBS via Email.          */
#define UTDEF_5      (UT_MYMAILCC | UT_NEWSLETTER)

/* Flags[6] : Editor                                         */
#define UT_         1  /* log degli X su file                */
#define UT_XED      2  /* Ricevo X nell'editor               */
#define UT_CED      4  /* Ricevo Chat-msg nell'editor        */
#define UT_NOTED    8  /* Ricevo Notifiche nell'editor       */
#define UTDEF_6     (UT_CED | UT_NOTED)

/* Flags[7] : Blog                                                 */
#define UT_BLOG_ON       1  /* Blog attivato                       */
#define UT_BLOG_ALL      2  /* Tutti possono scrivere sul blog     */
#define UT_BLOG_FRIENDS  4  /* Gli amici possono scrivere sul blog */
#define UT_BLOG_ENEMIES  8  /* I nemici non possono scrivere       */
#define UT_BLOG_WEB     16  /* Pubblica blog su web                */
#define UT_BLOG_LOBBY   32  /* Finisci il giro delle room nel blog */
#define UTDEF_7          0

/* Flags non modificabili dall'utente direttamente */
/* sflags[0] : variabili di configurazione */
#define SUT_SEX           1  /* 0: maschio, 1: femmina                    */
#define SUT_REGISTRATO    2  /* 1: se l'utente e' registrato              */
#define SUT_NEWS          4  /* 1: non ha ancora letto nuove news         */
#define SUT_UPGRADE       8  /* 1: primo collegamento con nuovo client    */
#define SUT_CONSENT      16  /* 1: l'utente ha accettato le condizioni    */
#define SUT_NEED_CONSENT 32  /* 1: l'utente deve accettare le condiz.     */
#define SUT_DELETEME     64  /* 1: l'utente ha richiesto la cancellazione */
#define SUTDEF_0          0

/* sflags[1] : divieti */
#define SUT_NOX         1  /* 1: X disabilitati per l'utente */
#define SUT_NOCHAT      2  /* 1: Chat disabilitata           */
#define SUT_NOPOST      4  /* 1: Divieto di post             */
#define SUT_NOMAIL      8  /* 1: Divieto di inviare mail     */
#define SUT_NOANONYM   16  /* 1: Divieto di postare anonimo  */
#define SUTDEF_1        0

#if 0 /* These flags are not used! */
/* sflags[2] : Tipo utente */
#define SUT_HELPING_HANDS  1 /* L'utente e' un Helping Hands */
#define SUT_DEVELOPER      2 /* L'utente e' uno sviluppatore */
#define SUTDEF_2           0

/* sflags[3] : Avvisi       */
#define SUT_AVVISO_NONCOL  1 /* Primo avviso di non collegamento */
#define SUT_AVVISO_CANC_1  2 /* Primo avviso di cancellazione    */
#define SUT_AVVISO_CANC_2  4 /* Cancellazione imminente!         */
#define SUTDEF_3           0

/* sflags[4] : Varie       */
#define SUT_RAFFREDDORE    1 /* Se l'utente e` raffreddato   */
#define SUT_IMMUNIZZATO    2 /*   immunizzato al raffreddore */
#endif

/* Flag utenti per le room */
#define UTR_KNOWN       1   /* Room conosciuta             */
#define UTR_ZAPPED      2   /* Room zappata                */
#define UTR_ROOMAIDE    4   /* L'utente e' Room Aide       */
#define UTR_ROOMHELPER  8   /* L'utente e' Room Helper     */
#define UTR_READ       16   /* Permesso di lettura         */
#define UTR_WRITE      32   /* Permesso di scrittura       */
#define UTR_ADVERT     64   /* Primo avvertimento          */
#define UTR_KICKOUT   128   /* L'utente e' stato cacciato  */
#define UTR_INVITED   256   /* L'utente e' stato invitato  */
#define UTR_NEWGEN    512   /* Se settato e' la prima volta che l'utente */
                            /* accede a questa generazione della room.   */
#define UTR_DEFAULT (UTR_KNOWN | UTR_READ | UTR_WRITE | UTR_NEWGEN)

#define STEP_POST 5         /* Cost. necessaria per il secondi_per_post  */

#endif /* user_flags.h */
