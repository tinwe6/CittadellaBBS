/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: socket_citta.h                                                        *
*       Gestione sockets di Cittadella                                        *
******************************************************************************/
#ifndef _SOCKET_CITTA_H
#define _SOCKET_CITTA_H   1

#define MAX_NOMEHOST    256

#include "strutture.h"

int iniz_socket(int porta);
int nuovo_descr(int s, char *host);
int nuova_conn(int s);
int scrivi_a_desc(int desc, char *txt);
int scrivi_a_desc_iobuf(int desc, struct iobuf *buf);
int elabora_input(struct sessione *t);
void nonblock(int s);

int scrivi_a_client(struct sessione *t, char *testo);
size_t scrivi_a_client_iobuf(struct sessione *t, struct iobuf *buf);
int scrivi_tutto_a_client(struct sessione *t, struct iobuf *buf);

#endif /* socket_citta.h */
