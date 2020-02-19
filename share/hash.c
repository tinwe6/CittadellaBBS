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
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : hash.c                                                             *
*        Funzioni per gestire tabelle di hash.                              *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>   /* per debugging only */
#include <string.h>
#include <math.h>
#include "hash.h"
#include "macro.h"

#define SEZ_AUREA ((sqrt(5.0) - 1.0) / 2.0)

/* Prototipi funzioni in questo file */
int hash_init(Hash_Table **ht, long size, char type);
void hash_free(Hash_Table **ht);
Hash_Entry * hash_find(Hash_Table *ht, void *key);
int hash_insert(Hash_Table *ht, void *key, void *data);
void * hash_remove(Hash_Table *ht, const void *key);

static long hash_string(const void *chiave, const long size);
static long hash_long(const void *chiave, const long size);
static int hash_cmp_long(const void *key1, const void *key2);
static int hash_cmp_string(const void *key1, const void *key2);

/***************************************************************************/
/*
 * Inizializza una tabella di hash ht, con size codici di hash e con
 * chiavi di tipo type.
 * Ritorna 0 se l'inizzializzazione e' avvenuta con successo, altrimenti -1.
 */
int hash_init(Hash_Table **ht, long size, char type)
{
	Hash_Entry **tmp;
	long i;
	
	CREATE(*ht, Hash_Table, 1, 5);
	switch(type) {
	 case HASH_STRING:
		(*ht)->hash = hash_string;
		(*ht)->cmp = hash_cmp_string;
		break;
	 case HASH_LONG:
		(*ht)->hash = hash_long;
		(*ht)->cmp = hash_cmp_long;
		break;
	 default:
		Free(*ht);
		return -1;
	}
	(*ht)->type = type;
	(*ht)->size = size;
	(*ht)->num = 0;
	(*ht)->collisions = 0;
	CREATE(tmp, Hash_Entry *, size, 6);
	(*ht)->table = tmp;
	for (i = 0; i < size; i++) {
		(*ht)->table[i] = NULL;
	}
	return 0;
}

/*
 * Funzione per liberare la tabella di hash ht.
 */
void hash_free(Hash_Table **ht)
{
	long i;
	Hash_Entry *tmp, *next;
	
	for (i = 0; i< (*ht)->size; i++) {
		tmp = (*ht)->table[i];
		while (tmp) {
			next = tmp->next;
			Free(tmp);
			tmp = next;
		}
	}
	Free((*ht)->table);
	Free(*ht);
	*ht = NULL;
}

/*
 * Cerca l'elemento con chiave key nella tabella di hash ht.
 * Se trova l'elemento, restituisce il puntatore all'elemento, altrimenti NULL.
 */
Hash_Entry * hash_find(Hash_Table *ht, void *key)
{
	Hash_Entry *tmp;
	long code;

	code = ht->hash((const char *) key, ht->size);
	for (tmp = ht->table[code]; tmp != NULL; tmp = tmp->next) {
		if (ht->cmp(tmp->key, key))
			return tmp;
	}
	return NULL;
}

/*
 * Inserisce il puntatore ai dati data con chiave key nella tabella di hash.
 * Se l'elemento e' gia' presente restituisce -1, altrimenti 0.
 * 
 * ATTENZIONE: Hasho due volte la chiave, la funzione si puo' migliorare!
 */
int hash_insert(Hash_Table *ht, void *key, void *data)
{
	Hash_Entry *entry;
	long code;
	
	if (hash_find(ht, key))
		return -1;
	
	CREATE(entry, Hash_Entry, 1, 6);
	entry->key = key;
	entry->data = data;

	code = ht->hash((const char *) key, ht->size);
	if ((ht->table)[code])
		ht->collisions++;

	entry->next = (ht->table)[code];
	(ht->table)[code] = entry;

	ht->num++;
	return 0;
}

/*
 * Inserisce il puntatore ai dati data con chiave key nella tabella di hash.
 * Se l'elemento e' gia' presente restituisce -1, altrimenti 0.
 */
void * hash_remove(Hash_Table *ht, const void *key)
{
	Hash_Entry *tmp, *tmp1;
	void *data;
	long code;

	code = ht->hash((const char *) key, ht->size);
	tmp = ht->table[code];
	if (tmp == NULL)
		return NULL;
	if (ht->cmp(key, tmp->key)) {
		ht->num--;
		data = tmp->data;
		ht->table[code] = tmp->next;
		Free(tmp);
		return data;
	}
	while (tmp->next) {
		if (ht->cmp(key, tmp->next->key)) {
			ht->num--;
			data = tmp->next->data;
			tmp1 = tmp->next;
			tmp->next = tmp->next->next;
			Free(tmp1);
			return data;
		}
		tmp = tmp->next;
	}
	return NULL;
}

/*
 * Funzione di hash per le stringhe:
 * data una chiave, restituisce il codice di hash.
 * Implementa l'algoritmo di Weinberger.
 */
static long hash_string(const void *chiave, const long size)
{
	const char *ptr;
	int val, tmp, code;
	
	val = 0;
	ptr = (const char *)chiave;
	while (*ptr) {
		val <<= 4;
		val += *ptr;
		if ( (tmp = (val & 0xf0000000))) {
			val ^= (tmp >> 24);
			val ^= tmp;
		}
		ptr++;
	}
	code = val % size;
	if (code < 0)
		code = -code;
#if 0   /* Debug */
	fprintf(stderr, "Key %s, code %ld\n", (const char *)chiave, code);
	fflush(stderr);
#endif
	return (code);
}

/*
 * Funzione di hash per interi long stringhe:
 * data una chiave, restituisce il codice di hash.
 * Utilizza il metodo per moltiplicazione.
 */
static long hash_long(const void *chiave, const long size)
{
	double tmp;
	
	tmp = *(const long *)chiave * SEZ_AUREA;
	tmp -= floor(tmp);
	return ((long)(size * tmp));
}

/*
 * Funzione per confrontare chiavi di tipo stringa.
 * Restituisce 0 se le stringhe non coincidono.
 */
static int hash_cmp_string(const void *key1, const void *key2)
{
	return (!strcmp((char *)key1, (char *)key2));
}

/*
 * Funzione per confrontare chiavi di tipo long.
 * Restituisce 0 se gli interi non coincidono.
 */
static int hash_cmp_long(const void *key1, const void *key2)
{
	return (*((long *)key1) == *((long *)key2)) ? 1 : 0;
}
