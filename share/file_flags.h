/*
 *  Copyright (C) 2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client & server                           (C) M. Caldarelli *
*                                                                           *
* File : file_flags.h                                                       *
*        flags e costanti associate al file server                          *
****************************************************************************/
#ifndef _FILE_FLAGS_H
#define _FILE_FLAGS_H   1

#define MAXLEN_FILENAME 64


/* Tipo di file testo richiesto al server: messaggio o help    */
typedef enum {
        STDMSG_MESSAGGI = 0,
        STDMSG_HELP,
        STDMSG_COUNT,   /* This must always be the last entry! */
} Stdmsg_type;

typedef enum {
        STDMSGID_HELLO = 0,
        STDMSGID_BAD_NICKS,
        STDMSGID_NEW_USER,
        STDMSGID_REGISTRATION,
        STDMSGID_DISCLAIMER,
        STDMSGID_GOODBYE,
        STDMSGID_CHANGE_PWD,
        STDMSGID_BUG_REPORT,
        STDMSGID_NEWS,
        STDMSGID_PRIVACY,
        STDMSGID_DATA_PROTECTION,
        STDMSGID_INTRO_FIRST_USER,
        STDMSGID_INTRO_GUEST,
        STDMSGID_INTRO_NEW,
        STDMSGID_INTRO_JUST_VALIDATED,
} Stdmsg_id;

/* Tipo file */
#define FILE_UNKNOWN   0
#define FILE_IMAGE     1
#define FILE_AUDIO     2
#define FILE_VIDEO     4
#define FILE_PDF       8
#define FILE_PS       16
#define FILE_DVI      32
#define FILE_HTML     64
#define FILE_TEXT    128
#define FILE_RTF     256
#define FILE_DOC     512

#define FILE_DOCUMENT_TYPE (FILE_PDF  | FILE_PS  | FILE_DVI | FILE_HTML \
                          | FILE_TEXT | FILE_RTF | FILE_DOC)

/* Flags dei files */



#endif /* file_flags.h */
