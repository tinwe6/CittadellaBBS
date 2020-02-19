/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX config                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: room_flags.h                                                        *
*       definizione dei flag di configurazione delle room.                  *
****************************************************************************/
#ifndef _ROOMS_FLAGS_H
#define _ROOMS_FLAGS_H    1

#define ROOMNAMELEN         20
#define DFLT_MSG_PER_ROOM  150
#define MAXROOMLEN        1000             /* Max number of slots per room */

/* Room Flags */
#define RF_BASE       (1<<0) /* Room di base: non puo' essere modificata    */
#define RF_MAIL       (1<<1) /* Mail room                                   */
#define RF_ENABLED    (1<<2) /* Se non e' settato, la room non e' attiva    */
#define RF_INVITO     (1<<3) /* Se e' settato serve un invito per l'accesso */
#define RF_SUBJECT    (1<<4) /* Subject per i messaggi abilitati            */
#define RF_GUESS      (1<<5) /* Guess Room                                  */
#define RF_ANONYMOUS  (1<<6) /* Tutti i post sono anonimi                   */
#define RF_ASKANONYM  (1<<7) /* Chiede se il post deve essere anonimo       */
#define RF_REPLY      (1<<8) /* Possibilita' di fare un reply a un post     */
#define RF_AUTOINV    (1<<9) /* Possibilita' di invito automatico           */
#define RF_FLOOR     (1<<10) /* Floor Lobby                                 */
#define RF_SONDAGGI  (1<<11) /* Tutti possono postare sondaggi              */
#define RF_CITTAWEB  (1<<12) /* La room viene pubblicata nel cittaweb       */
#define RF_BLOG      (1<<13) /* E` la blog-room                             */
#define RF_POSTLOCK  (1<<14) /* Un solo utente alla volta puo` postare      */
#define RF_L_TIMEOUT (1<<15) /* Applica il timeout se c'e' lock dei post    */
#define RF_FIND      (1<<16) /* Ricerca sempre nei post di questa room      */
#define RF_DYNROOM   (1<<17) /* TRUE se virtual room ha room data dinamico  */
#define RF_DYNMSG    (1<<18) /* TRUE se virtual room ha msglist dinamica    */
#define RF_SPOILER   (1<<19) /* Possibilita' di messaggi con spoiler        */
#define RF_DIRECTORY (1<<20) /* Possibilita' upload/download files          */

/* Flags di default per le nuove room                                       */
#define RF_DEFAULT    (RF_SUBJECT | RF_REPLY | RF_ENABLED | RF_FIND)

/* Flags che non possono venire modificati dagli utenti                    */
#define RF_RESERVED   (RF_BASE | RF_MAIL | RF_FLOOR)

/* Flags di default delle room di base dei floor                           */
#define RF_BASE_DEF  (RF_DEFAULT | RF_BASE)

/* Flags di default delle room di base dei floor                           */
#define RF_FLOOR_DEF  (RF_DEFAULT | RF_FLOOR)

/* Tipo di room di destinazione per un GOTO */
#define GOTO_ROOM  0
#define GOTO_FLOOR 1
#define GOTO_BLOG  2

#endif /* room_flags.h */
