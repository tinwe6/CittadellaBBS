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
* Cittadella/UX config                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: floor_flags.h                                                       *
*       definizione dei flag di configurazione dei floors.                  *
****************************************************************************/
#ifndef _FLOOR_FLAGS_H
#define _FLOOR_FLAGS_H    1

#define FLOORNAMELEN 20

/* Floor Flags */
#define FF_BASE        1  /* E' il floor di base: non puo' essere modificato */
#define FF_MAIL        2  /* Mail room                                   */
#define FF_ENABLED     4  /* Se non e' settato, il floor non e' attivo   */
#define FF_INVITO      8  /* Se e' settato serve un invito per l'accesso */
#define FF_SUBJECT    16  /* Subject per i messaggi abilitati            */
#define FF_GUESS      32  /* Guess Room                                  */
#define FF_ANONYMOUS  64  /* Tutti i post sono anonimi                   */
#define FF_ASKANONYM 128  /* Chiede se il post deve essere anonimo       */
#define FF_REPLY     256  /* Possibilita' di fare un reply a un post     */
#define FF_AUTOINV   512  /* Possibilita' di invito automatico           */
/* Flags di default per le nuove room                                     */
#define FF_DEFAULT    (FF_ENABLED)
/* Flags che non possono venire modificati dagli utenti                   */
#define FF_RESERVED   (FF_BASE | FF_MAIL)

#endif /* floor_flags.h */
