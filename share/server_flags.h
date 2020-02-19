/*
 *  Copyright (C) 1999-2004 by Marco Caldarelli and Riccardo Vianello
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
* File: server_flags.h                                                      *
*       definizione dei flag di configurazione del server.                  *
****************************************************************************/
#ifndef _SERVER_FLAGS_H
#define _SERVER_FLAGS_H    1

#define SF_VALKEY        1  /* Utilizza la chiave di validazione           */
#define SF_FLOORS        2  /* Ci sono i floors                            */
#define SF_REFERENDUM    4  /* Tool per referendum presenti                */
#define SF_MEMSTAT       8  /* Vengono tenute statistico uso memoria       */
#define SF_FIND         16  /* Cittagoogle disponibile                     */
#define SF_BLOG         32  /* Blog rooms disponibili                      */

#ifdef _CITTACLIENT_
#define SERVER_VALKEY     (serverinfo.flags & SF_VALKEY)
#define SERVER_FLOORS     (serverinfo.flags & SF_FLOORS)
#define SERVER_REFERENDUM (serverinfo.flags & SF_REFERENDUM)
#define SERVER_MEMSTAT    (serverinfo.flags & SF_MEMSTAT)
#define SERVER_FIND       (serverinfo.flags & SF_FIND)
#define SERVER_BLOG       (serverinfo.flags & SF_BLOG)
#endif

#endif /* server_flags.h */
