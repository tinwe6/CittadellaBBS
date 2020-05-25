/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client & server          (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : msg_flags.h                                                        *
*        Comandi Server/Client per leggere, scrivere, cancellare i post.    *
****************************************************************************/
#ifndef _MSG_FLAGS_H
#define _MSG_FLAGS_H   1

#define MAXLEN_SUBJECT  64
#define MAXLEN_FILENAME 64
/* Maximum width in characters for messages, posts,... */
#define MSG_WIDTH       (80 - 1)

/* Flags dei messaggi */
#define MF_MAIL        1    /* E' un mail              */
#define MF_ANONYMOUS   2    /* E' un messaggio anonimo */
#define MF_RH          4    /* Posted as Room Helper   */
#define MF_RA          8    /* Posted as Room Aide     */
#define MF_AIDE       16    /* Posted as Aide          */
#define MF_SYSOP      32    /* Posted as Sysop         */
#define MF_CITTA      64    /* Posted by Cittadella    */
#define MF_MOVED     128    /* E' stato spostato       */
#define MF_REPLY     256    /* E' la risposta ad un altro messaggio     */
#define MF_BLOG      512    /* E' un messaggio in un blog               */
#define MF_SPOILER  1024    /* Messaggio con spoiler                    */
#define MF_LINK     2048    /* Ha del metadata associato                */
#define MF_FILE     4096    /* Ha un file associato (in Directory room) */
#define MF_IMAGE    8192    /* Ha un'immagine associata                 */
#define MF_MOVIE   16384    /* Ha un film associato (in Directory room) */
#define MF_SOUND   32768    /* Ha un suono associato (in Directory room)*/


#define MF_ADMINPOST ( MF_RH | MF_RA | MF_AIDE | MF_SYSOP | MF_CITTA )

/* Errori standard file messaggi */
#define FMERR_OK           0  /* tutto ok, nessun errore :-)               */
#define FMERR_NOFM         1  /* file messaggi inesistente                 */
#define FMERR_ERASED_MSG   2  /* il messaggio e' stato sovrascritto        */
#define FMERR_NO_SUCH_MSG  3  /* il messaggio e' inesistente               */
#define FMERR_OPEN         4  /* non riesco ad aprire il file dei messaggi */
#define FMERR_DELETED_MSG  5  /* il messaggio e' stato cancellato          */
#define FMERR_CHKSUM       6  /* messaggio corrotto (chksum errato)        */
#define FMERR_MSGNUM       7  /* il msgnum fornito non corrisponde con     */
                              /* quello del messaggio                      */
#endif /* msg_flags.h */
