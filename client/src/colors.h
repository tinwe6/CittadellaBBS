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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : colors.h                                                           *
*        Definizione dei colori utilizzati dal cittaclient.                 *
****************************************************************************/
#ifndef _COLORS_H
#define _COLORS_H   1
/*
   COME MODIFICARE I COLORI DEL CLIENT:
   ------------------------------------

   In questo file vengono definiti tutti i colori della BBS.
   Potete modificarli secondo i vostri gusti molto facilmente; dovete
   prima di tutto trovare la riga corrispondente al colore che volete
   cambiare. Ad esempio, se il non vi piace l'ora di connessione in
   magenta nel <w>ho, e la volete blu, dovete trovare qui sotto la riga:

#define C_WHO_TIME      MAGENTA    / * Who: ora di connessione       * /

   Di tutta la riga, e' importante il commento finale che vi dice a che
   colore si riferisce la riga, e il terzo campo, 'MAGENTA', dove inserite
   il colore che desiderate. Percio' la riga va modificata in

#define C_WHO_TIME      BLUE       / * Who: ora di connessione       * /

   per avere l'ora di connessione in blu!

   I colori ANSI normali che avete a disposizione sono

      BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GRAY

   Poi ci sono i colori ANSI chiari (o bold):

      L_BLACK, L_RED, L_GREEN, L_YELLOW, L_BLUE, L_MAGENTA, L_CYAN, WHITE

   Potete usare uno qualunque di questi per colorare la BBS!

   NB: YELLOW appare in marrone generalmente. Se si vuole un vero giallo,
       bisogna utilizzare L_YELLOW.

   Una volta soddisfatti delle modifiche, dovete ricompilare il client.
   E' sufficiente per questo eseguire dalla direcory con i sorgenti del
   client (e questo file), il seguente comando:

       make clean; make

   NB: E' essenziale fare anche il make clean, altrimenti le modifiche non
       vengono prese in considerazione.

                                 o---O---o

   ARGOMENTI AVANZATI:
   -------------------

   Se pensate di essere limitati nelle possibilita' di scelta dei colori,
   vi sbagliate! Le possibilita' sono innumerevoli. Oltre al colore del
   carattere, potete settare il colore dello sfondo, nonche' i suoi
   "attributi", a scelta fra le seguenti possibilita':

   ATTR_RESET         : Nessun attributo
   ATTR_BOLD          : Caratteri in grassetto (o piu' luminosi)
   ATTR_HALF_BRIGHT   : Luminosita` normale
   ATTR_UNDERSCORE    : Carattere sottolineato
   ATTR_BLINK         : Lampeggia
   ATTR_REVERSE       : Inverte i colori del carattere e dello sfondo.
   ATTR_NOCHANGE      : Non modificare l'attributo dei caratteri.

   NB: Non tutti i terminali sono in grado di visualizzare tutti questi
       attributi. Ad esempio, la sottolineatura puo' risultare in un
       cambiamento di colori in alcuni casi. Sperimentate!

   L'unico costo e` che la sintassi e` lievemente diversa, e dovete
   definire il colore nel modo seguente:

   COLOR( colore_caratteri , colore_sfondo , attributi)

   Per i colori dei caratteri e dello sfondo dovrete scegliere fra i
   seguenti:

      COL_BLACK,      COL_RED,  COL_GREEN,  COL_YELLOW,
       COL_BLUE,  COL_MAGENTA,   COL_CYAN,    COL_GRAY
    COL_DEFAULT:  colore di default
   COL_NOCHANGE:  non modificare il colore

   Gli attributi si possono combinare fra loro con un '|':
     Attributo_1 | Attributo_2    ...

   Per tornare al nostro esempio precedente, supponiamo di essere molto
   esigenti e di voler visualizzare l'ora di connessione con caratteri
   gialli, su sfondo blu. Dovremmo scrivere:

#define C_WHO_TIME   COLOR(COL_YELLOW, COL_BLUE, ATTR_BOLD)

   Abbiamo messo il bold, altrimenti il giallo appare marrone.
   Se vogliamo che sia anche sottolineato, e' sufficiente fare

#define C_WHO_TIME   COLOR(COL_YELLOW, COL_BLUE, ATTR_BOLD | ATTR_UNDERSCORE)

   Se vogliamo invece mantenere il colore corrente del carattere, ma avere
   lo sfondo rosso, dobbiamo fare:

#define C_WHO_TIME   COLOR(COL_NOCHANGE, COL_RED, ATTR_RESET)

                                 o---O---o

   Spero che con l'aiuto di questo file non avrete difficolta' a cambiare
   a vostro piacimento i colori della BBS. In ogni caso, se avete delle
   difficolta' o dei commenti da fare, postateli pure in Cittadella/UX>!

   E se siete particolarmente fieri del vosto senso estetico, speditemi
   il vostro file colors.h a caldarel@yahoo.it !

   Happy BBSing,
   Ping.

*/
#include "ansicol.h"

/* Colori della BBS */
#define C_NORMAL        GRAY       /* default color                 */
#define C_HELP          RED        /* Help color                    */
#define C_ROOMPROMPT    YELLOW     /* Room prompt                   */
#define C_ERROR         RED        /* Error Color                   */
#define C_EDIT_PROMPT   COLOR(COL_YELLOW, COL_BLACK, ATTR_BOLD)
                                   /* Prompt dell'editor '>'        */
#define C_EDITOR        COLOR(COL_YELLOW, COL_BLUE, ATTR_BOLD)
                                   /* Editor interno : intestazione */
#define C_FRIEND_PROMPT COLOR(COL_YELLOW, COL_BLACK, ATTR_BOLD)
                                   /* Prompt della friendlist       */
#define C_FRIEND_TEXT   L_BLUE     /* Testo modifica friendlist     */
#define C_QUESTION      GREEN      /* Colore domande (generico)     */
#define C_OK            L_YELLOW   /* Colore per risposta 'OK'      */
/* Questi devono essere colori scuri, in quanto usa l'highlight     */
#define C_BROADCAST     RED        /* Broacast                      */
#define C_FRIENDLIST    CYAN       /* Colore friendlist             */
#define C_X             CYAN       /* X                             */
#define C_CHAT          L_BLUE     /* Chat                          */
#define C_PRFL          GREEN      /* Profile body                  */
#define C_PRFL_NOME     GREEN      /* Nome nel profile              */
#define C_PRFL_ADDR     YELLOW     /* Profile: Indirizzo utente     */
#define C_PRFL_EMAIL    GREEN      /* Email nel profile             */
#define C_PRFL_URL      GREEN      /* Home Page URL nel profile     */
#define C_PAGERPROMPT   CYAN       /* Pager                         */
#define C_MSGPROMPT     MAGENTA    /* Message prompt                */
#define C_XLOG          GREEN      /* Diario degli X                */
#define C_XLOGPROMPT    YELLOW     /* Prompt del diario             */

/* Altri colori */
#define C_PRFL_PERS     L_BLUE     /*          personale            */
#define C_TOGGLE        L_YELLOW   /* Colore 'On/Off' del toggle    */
#define C_CONFIG_H      L_MAGENTA  /* Configurazione utente: header  BOLD!! */
#define C_CONFIG_B      L_BLUE     /* Configurazione utente: body   */
#define C_MSG_HDR       GREEN      /* Header dei post               */
#define C_MSG_SUBJ      L_YELLOW   /* Subject dei post              */
#define C_MSG_BODY      C_NORMAL   /* Body del post                 */
#define C_BLOGLIST      C_NORMAL   /* List blog                     */
#define C_BLOGTIME      YELLOW     /* ora ultimo post nel blog      */
#define C_BLOGDATE      MAGENTA    /* data ultimo post nel blog     */
#define C_BLOGNUM       GRAY       /* # post presenti               */
#define C_BLOGNEW       L_RED      /* New posts flag                */
#define C_WHO           C_NORMAL   /* Who                           */
#define C_WHO_TIME      MAGENTA    /* Who: data & ora               */
#define C_NEWS          L_RED      /* News                          */
#define C_ENTRYPROMPT   GREEN      /* Entry prompt                  */
#define C_NOTIF_URG     L_RED      /* Notifiche urgenti     BOLD!   */
#define C_NOTIF_NORM    L_CYAN     /* Notifiche normali             */
#define C_KR_HDR        L_CYAN     /* Known Room: Headers           */
#define C_KR_NUM        L_MAGENTA  /*             Room Number       */
#define C_KR_ROOM       L_BLUE     /*             Room Name         */
#define C_HAK           YELLOW     /* Hit any Key prompt            */
#define C_CHAT_HDR      L_CYAN     /* Autore del messaggio in chat  */
#define C_CHAT_HDR1     CYAN       /* '>' dopo autore in chat (NOT USED) */
#define C_CHAT_MSG      L_BLUE     /* messaggio chat                */

#define C_ROOM          L_YELLOW   /* Room Name                     */

#define C_ME            L_CYAN     /* Mio nome                      */
#define C_USER          L_BLUE     /* Nomi utenti                   */
#define C_FRIEND        L_RED      /* Nome utente amico             */
#define C_ENEMY         L_BLACK    /* Nome utente nemico            */
#define C_HOST          GRAY       /* Host di provenienza           */
#define C_DOING         WHITE      /* Doing                         */

#define C_NUSER         L_CYAN     /* Nomi utenti nelle notifiche   */
#define C_NFRIEND       L_RED      /* Nome amico   '    '      '    */
#define C_NENEMY        L_BLACK    /* Nome nemico  '    '      '    */
#define C_NCENEMY       COLOR(COL_BLACK, COL_RED, ATTR_RESET)
                                   /* Nome nemico notifiche in chat */

#define C_MUSER         L_GREEN    /* Nomi utenti nell'header msg   */
#define C_MFRIEND       L_RED      /* Nome amico   '    '      '    */
#define C_MENEMY        L_BLACK    /* Nome nemico  '    '      '    */
#define C_MANONIMO      WHITE      /* Nome anonim  '    '      '    */
#define C_MBBS          MAGENTA    /* Nome BBS header citta_post    */

#define C_COMANDI       GREEN      /* Comandi utente                */
#define C_IDLE          L_RED      /* Notifiche idle                */
#define C_URNA          CYAN       /* Colore sondaggi               */
#define C_URNA_TITOLO   RED        /* Titolo del sondaggio          */
#define C_URNA_ROOM	WHITE	   /* stanza                        */
#define C_URNA_VOTA	L_YELLOW   /* vota                          */
#define C_URNA_VOTATA   YELLOW     /* vota                          */
#define C_URNA_LETTERA  L_RED      /* lettera                       */
#define C_URNA_NUMERO   CYAN       /* Numero                        */
#define C_URNA_ZAPPATA  L_RED      /* Titolo del sondaggio          */
#define C_QUOTE         L_CYAN     /* Colore del '>' dei quote      */
#define C_ROOM_INFO     GREEN      /* Room Info                     */
#define C_ROOM_INFO_DESC CYAN      /* Room Info: description        */
#define C_ROOM_INFO_ROOM L_YELLOW  /* Room Info: nome room          */
#define C_ROOM_INFO_FLAG L_YELLOW  /* Room Info: flag               */
#define C_ROOM_INFO_USER L_CYAN    /* Room Info: nome utenti        */
#define C_ROOM_INFO_NUM L_MAGENTA  /* Room Info: cifre              */
#define C_ROOM_INFO_X   WHITE      /* Room Info: X                  */

/* Colori User List                                                 */
#define C_USERLIST      C_NORMAL   /* User List                     */
#define C_USERLIST_LVL  L_RED      /* User Level                    */
#define C_USERLIST_LAST L_MAGENTA  /* Last call                     */
#define C_USERLIST_CALLS L_BLUE    /* Number of calls               */
#define C_USERLIST_POST L_CYAN     /* Number of posts               */
#define C_USERLIST_MAIL WHITE      /* Number of mails               */
#define C_USERLIST_XMSG L_YELLOW   /* Number of express-msg         */
#define C_USERLIST_CHAT L_GREEN    /* Number of chat messages       */

/* Colori Citta Editor */
#define C_EDITOR_DEBUG  L_RED

/* Altri colori --NOT USED-- */
#define C_PROMPT_ROOM   L_YELLOW   /* Nome room nel prompt          */
#define C_ROOM_TYPE     WHITE      /* Room Type                     */

#define C_HELP_HEADER   L_MAGENTA  /* Colore Header Help            */
#define C_HELP_HEADER2  L_CYAN     /* Colore Header Help 2          */
#define C_HELP_BODY     L_RED      /* Colore body degli help        */

/***************************************************************************/

#endif /* colors.h */
