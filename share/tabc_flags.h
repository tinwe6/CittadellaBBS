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
* Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : tabc_flags.h                                                       *
*        defines per Tab Completion dei nomi (utenti, room, etc.)           *
****************************************************************************/
#ifndef TABC_FLAGS_H
#define TABC_FLAGS_H   1

#define TC_MODE_X          0
#define TC_MODE_MAIL       1  /* Fare check mailbox da server */
#define TC_MODE_EMOTE      2
#define TC_MODE_ROOMKNOWN  3
#define TC_MODE_ROOMNEW    4
#define TC_MODE_FLOOR      5
#define TC_MODE_USER       6
#define TC_MODE_BLOG       7
#define TC_MODE_ROOMBLOG   8  /* Cerca sia nelle room che nei blog */

#endif /* tabc_flags.h */
