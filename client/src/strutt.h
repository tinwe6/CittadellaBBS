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
* File : strutt.h                                                           *
*        Definizioni di alcune strutture usate da cittaclient.              *
****************************************************************************/

#ifndef _STRUTT_H
#define _STRUTT_H   1

struct blocco_testo {
	char *testo;
	struct blocco_testo *prossimo;
};

struct coda_testo {
	struct blocco_testo *partenza;
	struct blocco_testo *termine;
};

struct serverinfo {
	char software[50];
	int  server_vcode;
	char versione[20];
	char nodo[50];
	char dove[50];
	int  protocol_vcode;
	int  newclient_vcode;
	char num_canali_chat;
	int maxlineebug;      /* Numero massimo di righe nei bug rep. */
	int maxlineebx;       /* Numero massimo di righe nei messaggi */
	int maxlineenews;     /* Numero massimo di righe nelle news   */
	int maxlineepost;     /* Numero massimo di righe nei post     */
	int maxlineeprfl;     /* Numero massimo di righe nei profile  */
	int maxlineeroominfo; /* Numero massimo di righe RoomInfo     */
	int flags;
        bool legacy;          /* True if connected to an older server */
};

#endif /* strutt.h */
