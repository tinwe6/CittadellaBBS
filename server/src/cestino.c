/*
 *  Copyright (C) 2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cestino.c                                                          *
*        Funzioni per la gestione del cestino (del/undel dei post).         *
****************************************************************************/
#include "config.h"
#ifdef USE_CESTINO
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "cittaserver.h"
#include "rooms.h"
#include "mail.h"
#include "memstat.h"
#include "file_messaggi.h"
#include "fileserver.h"
#include "ut_rooms.h"
#include "utility.h"

/* Prototipi delle funzioni in rooms.c */
void msg_dump(struct room *room, long msgnum);
int msg_undelete(struct room *room, long msgnum, char *roomname);
void msg_load_dump(void);
void msg_save_dump(void);
void msg_dump_free(void);
void msg_dump_resize(long oldsize, long newsize);

/* Informazioni per undelete dei post. */
static long *dump_undelete;

/***************************************************************************/

/* Carica i dati per l'undelete dal Cestino. */
void msg_load_dump(void)
{
	struct room *room;
        FILE *fp;
	int i;

	room = room_find(dump_room);
	if (room == NULL) { /* IN FUTURO CREARE LA DUMPROOM IN QUESTO CASO */
		citta_log("SYSERR: Non trovo la DUMP ROOM!!");
		return;
	}
	CREATE(dump_undelete, long, room->data->maxmsg, TYPE_LONG);

	/* Legge da file i vettori */
	fp = fopen(FILE_DUMP_DATA, "r");
	if (!fp) {
		citta_log("SYSERR: no dump data per Cestino.");
		/* Inizializza dump data */
		for (i = 0; i < room->data->maxmsg; i++)
			dump_undelete[i] = 0;
		return;
	}
	fread(dump_undelete, sizeof(long), room->data->maxmsg, fp);
	fclose(fp);
}

/* Salva i dati per l'undelete dal Cestino. */
void msg_save_dump(void)
{
	struct room *room;
        FILE *fp;
	char bak[LBUF];

	room = room_find(dump_room);
	if (room == NULL) { /* IN FUTURO CREARE LA DUMPROOM IN QUESTO CASO */
		citta_log("SYSERR: Non trovo la DUMP ROOM!!");
		return;
	}
	sprintf(bak, "%s.bak", FILE_DUMP_DATA);
	rename(FILE_DUMP_DATA, bak);

	fp = fopen(FILE_DUMP_DATA, "w");
        if (!fp) {
		citta_logf("SYSERR: Cannot write msg dump data");
		rename(bak, FILE_DUMP_DATA);
		return;
        }
	fwrite(dump_undelete, sizeof(long), room->data->maxmsg, fp);
	fclose(fp);
	unlink(bak);
}

/*
 *  Sposta il messaggio corrente nel cestino
 */
void msg_dump(struct room *room, long msgnum)
{
	struct room *dest;
	long msgpos, locnum;
	long pos, i;

	for (i = 0; i < room->data->maxmsg; i++)
		if (room->msg->num[i] == msgnum) {
			/* Elimina il post dalla room corrente */
			pos = i;
			msgpos = room->msg->pos[i];
			locnum = room->msg->local[i];
			for (i = pos; i > 0; i--) {
				room->msg->num[i] = room->msg->num[i-1];
				room->msg->pos[i] = room->msg->pos[i-1];
				room->msg->local[i] = room->msg->local[i-1];
                                /* NON USO FM multipli 
				if (room->msg->fmnum)
				  room->msg->fmnum[i] = room->msg->fmnum[i-1];
                                */
			}
			room->msg->num[0] = -1L;
			room->msg->pos[0] = -1L;
			room->msg->local[0] = -1L;
                        /* NON USO FM multipli 
			if (room->msg->fmnum)
			        room->msg->fmnum[0] = -1L;
                        */

			/* inserisci il post nel cestino */
			if (dump_undelete == NULL) {
				citta_log("SYSERR: dump_undelete NULL!");
				return;
			}

			dest = room_find(dump_room);
			if (dest == NULL) {
				citta_log("SYSERR: Non trovo il Cestino!");
				return;
			}
			i = 1;
			do {
				dest->msg->num[i-1]= dest->msg->num[i];
				dest->msg->pos[i-1]= dest->msg->pos[i];
				dest->msg->local[i-1]= dest->msg->local[i];
                                /* NON USO FM multipli 
				dest->msg->fmnum[i-1]= dest->msg->fmnum[i];
                                */
				dump_undelete[i-1] = dump_undelete[i];
				i++;
			} while ((i < dest->data->maxmsg)
                                 && (dest->msg->num[i] < msgnum));
			dest->msg->num[i-1]= msgnum;
			dest->msg->pos[i-1]= msgpos;
			dest->msg->local[i-1]= locnum;
                        /* NON USO FM multipli 
			dest->msg->fmnum[i-1]= room->data->fmnum;
                        */
			dump_undelete[i-1] = room->data->num;

                        /* Elimina eventuali file allegati */
                        fs_delete_file(msgnum);

			return;
		}	
}

void msg_dump_free(void)
{
	Free(dump_undelete);
}

/*
 * Undelete del messaggio corrente dal cestino.
 * Copia in roomname il nome della room di origine del messaggio.
 */
int msg_undelete(struct room *room, long msgnum, char *roomname)
{
	struct room *dest;
	long pos, i;

	for (i = 0; i < room->data->maxmsg; i++)
		if (room->msg->num[i] == msgnum) {
			/* Rimette il post a posto */
			pos = i;
			dest = room_findn(dump_undelete[i]);
			if (dest == NULL) { /* La room non esiste piu' */
				return 0;
			}
			msg_move(dest, room, msgnum);
			strcpy(roomname, dest->data->name);
			for (i = pos; i > 0; i--)
				dump_undelete[i] = dump_undelete[i-1];
			return 1;
		}
	return 0;
}

/*
 * Ridimensiona il numero di messaggi nel dumproom.
 */
void msg_dump_resize(long oldsize, long newsize)
{
	long *newdump;
	long i, j;

	if (newsize <= 0)
		return;

	CREATE(newdump, long, newsize, TYPE_LONG);
	for(i = oldsize-1, j = newsize-1;
	    (i >= 0) && (j >= 0); i--, j--) {
		newdump[j] = dump_undelete[i];
	}
	msg_dump_free();
	dump_undelete = newdump;
}

#endif
