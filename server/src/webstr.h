/*
 *  Copyright (C) 2003-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                      (C) M. Caldarelli *
*                                                                             *
* File: webstr.h                                                              *
*       templates per la costruzione delle pagine del cittaweb.               *
******************************************************************************/
#ifndef _WEBSTR_H
#define _WEBSTR_H  1

#include "config.h"
#ifdef USE_CITTAWEB

/* Strutture per la gestione delle lingue. */
#define HTTP_LANG_NUM 4
extern const char * http_lang[];
extern const char * http_lang_name[];

/* Templates */
extern const char HTML_HEADER[];
extern const char HTML_404[];
extern const char HTML_NOT_AVAIL[];

extern const char HTML_START_CSS[];
extern const char HTML_END_CSS[];
extern const char HTML_USERLIST_CSS[];
extern const char HTML_BLOGLIST_CSS[];
extern const char HTML_INDEX_CSS[];
extern const char HTML_ROOM_CSS[];

extern const char HTML_META[];
extern const char HTML_TAIL[];
extern const char HTML_TAILINDEX[];
extern const char HTML_INDEX_BODY[];

extern const char * HTML_INDEX_TITLE[HTTP_LANG_NUM];
extern const char * HTML_INDEX_PREAMBLE[HTTP_LANG_NUM];
extern const char * HTML_INDEX_ROOM_LIST[HTTP_LANG_NUM];
extern const char * HTML_INDEX_USERLIST[HTTP_LANG_NUM];
extern const char * HTML_INDEX_BLOGLIST[HTTP_LANG_NUM];
extern const char * HTML_INDEX_LASTMSG[HTTP_LANG_NUM];
extern const char * HTML_INDEX_LASTMAIL[HTTP_LANG_NUM];
extern const char * HTML_INDEX_READIT[HTTP_LANG_NUM];
extern const char * HTML_INDEX_NOTAVAIL[HTTP_LANG_NUM];
extern const char * HTML_INDEX_NONEW[HTTP_LANG_NUM];

extern const char * HTML_ROOM_PREAMBLE[HTTP_LANG_NUM];

/* Prototipi funzioni in webstr.h */

void webstr_init(void);

#endif /* USE_CITTAWEB */
#endif /* webstr.h*/
