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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : name_list.c                                                        *
*        routines per la gestione di liste di nomi (utenti, room, etc..)    *
****************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cittaclient.h"
#include "cml.h"
#include "name_list.h"
#include "macro.h"
#include "utility.h"

/* Definire DEBUG_BL per info di debugging */
#undef DEBUG_NL

/*****************************************************************************/
/* Crea una nuova lista di nomi */
struct name_list * nl_create(const char * nome_lista)
{
	struct name_list *nl;
	int i;

	CREATE(nl, struct name_list, 1, 0);
	nl->num = 0;
	strncpy(nl->list_name, nome_lista, NL_LISTNAME);

	for (i = 0; i < NL_SIZE; i++) {
		nl->name[i][0] = 0;
		nl->id[i] = -1;
	} 

	return nl;
}

/* Elimina la lista di nomi */
int nl_free(struct name_list ** nl)
{
	if (*nl == NULL)
		return NL_ERR_STRUCT;

	Free(*nl);
	*nl = NULL;

	return NL_OK;
}

/* Inserisce un nome nella lista
 * Restituisce NL_OK se il nome e' stato inserito correttamente,
 * NL_ERR_STRUCT se la struttura non e' allocata,
 * NL_ERR_DOUBLE se il nome e' gia' presente.
 * NL_ERR_EMPTY se non viene fornito il nome.
 * NL_ERR_FULL se non c'e' piu' posto.
 */
int nl_insert(struct name_list * nl, char * nick, long id)
{
	if (nl == NULL)
		return NL_ERR_STRUCT;

	if (nl->num == NL_SIZE)
		return NL_ERR_FULL;

	if ((nick == NULL) || (*nick == 0))
		return NL_ERR_EMPTY;

	if (nl_in(nl, nick) != -1)
		return NL_ERR_DOUBLE;

	strncpy(nl->name[nl->num], nick, MAXLEN_UTNAME);
	nl->id[nl->num] = id;
	nl->num++;

#ifdef DEBUG_NL
	printf("$ [%s] inserted in name list [%s].\n", nick, nl->list_name);
#endif

	return NL_OK;
}

/* Elimina il nome 'str' dalla lista. */
int nl_remove(struct name_list * nl, char * str)
{
	int pos, i;

	if (nl == NULL)
		return NL_ERR_STRUCT;

	if (nl->num == 0)
		return NL_ERR_NOTIN;

	pos = nl_in(nl, str);
	if (pos == -1)
		return NL_ERR_NOTIN;

	for (i = pos+1; i < nl->num; i++)
		strncpy(nl->name[i-1], nl->name[i], MAXLEN_UTNAME);

	nl->num--;
	nl->name[nl->num][0] = '\0';
	nl->id[nl->num] = -1;

#ifdef DEBUG_NL
	printf("$ [%s] removed from name list [%s].\n", str, nl->list_name);
#endif

	return NL_OK;
}

/* Elimina il nome n-esimo dalla lista. */
int nl_removen(struct name_list * nl, int n)
{
	int i;

	if (nl == NULL)
		return NL_ERR_STRUCT;

	if ((n >= nl->num) || (n < 0))
		return NL_ERR_NOTIN;

	for (i = n+1; i < nl->num; i++)
		strncpy(nl->name[i-1], nl->name[i], MAXLEN_UTNAME);

	nl->num--;
	nl->name[nl->num][0] = '\0';
	nl->id[nl->num] = -1;

#ifdef DEBUG_NL
	printf("$ Name #%d removed from name list [%s].\n", n, nl->list_name);
#endif

	return NL_OK;
}

/* Restituisce il numero di nomi presenti nella lista. */
int nl_num(struct name_list * nl)
{
	if (nl == NULL)
		return NL_ERR_STRUCT;

	return nl->num;
}

/* Restituisce il puntatore all'i-esimo nome della lista. */
const char * nl_get(struct name_list * nl, int i)
{
	if (nl == NULL)
		return NULL;

	if ((i < 0) || (i >= nl->num))
		return NULL;

	return nl->name[i];
}

/* Restituisce l'id dell'i-esimo nome della lista, -1 se non presente. */
long nl_getid(struct name_list * nl, int i)
{
	if (nl == NULL)
		return -1;

	if ((i < 0) && (i >= nl->num))
		return -1;

	return nl->id[i];
}

/* Setta l'id dell'n-esimo nome nella lista. */
int nl_setid(struct name_list * nl, int n, long id)
{
	if (nl == NULL)
		return NL_ERR_STRUCT;

	if ((n >= nl->num) || (n < 0))
		return NL_ERR_NOTIN;

	nl->id[n] = id;

	return NL_OK;
}

/* Elimina tutti gli elementi con id "id" dalla lista. */
int nl_purgeid(struct name_list * nl, long id)
{
	int i;

	if (nl == NULL)
		return NL_ERR_STRUCT;

	if (nl->num == 0)
		return NL_ERR_EMPTY;

	for (i = nl->num; i >= 0; i--)
                if (nl->id[i] == id)
                        nl_removen(nl, i);

	return NL_OK;
}

/* Controlla se il nome 'nome' e' presente nella lista 'nl'.
 * Restituisce -1 se non c'e', la sua posizione (i+1) se c'e'. */
int nl_in(struct name_list * nl, const char * name)
{
	int i;

	if (nl == NULL)
		return -1;

	if (name == NULL)
		return -1;

	for (i = 0; i < nl->num; i++) {
		if (!strncmp(nl->name[i], name, MAXLEN_UTNAME))
			return i;
	}
	return -1;
}

/* Restituisce il puntatore all'i-esimo nome della lista. */
const char * nl_name(struct name_list * nl)
{
	if (nl == NULL)
		return NULL;

	return nl->list_name;
}

/* Scambia due nomi nella lista */
int nl_swap(struct name_list * nl, int a, int b)
{
	char tmp[MAXLEN_UTNAME];
	long id;

	if (nl == NULL)
		return NL_ERR_STRUCT;

	if ((a >= nl->num) || (b >= nl->num)
	    || (a < 0) || (b < 0))
		return NL_ERR_NOTIN;

	id = nl->id[a];
	nl->id[a] = nl->id[b];
	nl->id[b] = id;

	strncpy(tmp, nl->name[a], MAXLEN_UTNAME);
	strncpy(nl->name[a], nl->name[b], MAXLEN_UTNAME);
	strncpy(nl->name[b], tmp, MAXLEN_UTNAME);

#ifdef DEBUG_NL
	printf("$ Elements #%d and #%d in name list [%s] swapped.\n", a, b,
               nl->list_name);
#endif

	return NL_OK;
}

void nl_print(const char * str, struct name_list * nl)
{
	int i;
	size_t len, plen, tlen;
	char buf[LBUF];

	if (nl == NULL) {
		cml_print("La lista &egrave; vuota.\n");
                return;
        }
	plen = printf("%s", str);
	len = plen;
	for (i = 0; i < nl->num; i++) {
		tlen = cml_sprintf(buf, "%d. <b>%s</b>, ", i + 1, nl->name[i]);
		len += tlen;
		if (len > 78) {
			delchar();
			putchar('\n');
			putnspace(plen);
			len = plen + tlen;
		}
		printf("%s", buf);
	}
	delchar();
	delchar();
	putchar('.');
        putchar('\n');
}

void nl_fprint(FILE * fp, struct name_list * nl)
{
	int i;

	if (nl == NULL) {
		fprintf(fp, "La lista e` vuota.\n");
                return;
        }
	for (i = 0; i < nl->num; i++) 
		fprintf(fp, "%d. %s, ", i + 1, nl->name[i]);
        fprintf(fp, "\n");
}
