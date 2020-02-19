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
* File : name_list.c                                                        *
*        routines per la gestione di liste di nomi (utenti, room, etc..)    *
****************************************************************************/
#ifndef NAME_LIST_H
#define NAME_LIST_H   1

#include "user_flags.h"

#define NL_LISTNAME 30   /* Lunghezza massima nome della lista      */
#define NL_SIZE     10   /* Numero massimo di elementi nella lista. */

struct name_list {
	char list_name[NL_LISTNAME];         /* Nome della lista           */
	char name[NL_SIZE][MAXLEN_UTNAME];   /* Nomi                       */
	long id[NL_SIZE];                    /* numeri matricola corrisp.  */
	int num;                             /* Numero di nomi nella lista */
};

#define NL_OK         0
#define NL_ERR_EMPTY  1
#define NL_ERR_FULL   2
#define NL_ERR_NOTIN  3
#define NL_ERR_DOUBLE 4
#define NL_ERR_STRUCT 5

/* Prototipi delle funzioni in name_list.c */
struct name_list * nl_create(const char * nome_lista);
int nl_free(struct name_list ** nl);
int nl_insert(struct name_list * nl, char * nome, long id);
int nl_remove(struct name_list * nl, char * str);
int nl_removen(struct name_list * nl, int n);
int nl_num(struct name_list * nl);
const char * nl_get(struct name_list * nl, int i);
long nl_getid(struct name_list * nl, int i);
int nl_setid(struct name_list * nl, int n, long id);
int nl_purgeid(struct name_list * nl, long id);
int nl_in(struct name_list * nl, const char * nome);
const char * nl_name(struct name_list * nl);
int nl_swap(struct name_list * nl, int n1, int n2);
void nl_print(const char * str, struct name_list * nl);
void nl_fprint(FILE * fp, struct name_list * nl);

#endif /* name_list.h */
