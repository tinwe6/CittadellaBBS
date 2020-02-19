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
* File : hash.h                                                             *
*        Funzioni per gestire tabelle di hash.                              *
****************************************************************************/
#ifndef _HASH_H
#define _HASH_H   1

/* Tipi di tabelle di hash */
#define HASH_STRING  0
#define HASH_LONG    1

typedef struct Hash_Entry_ {
	void *key;
	void *data;
	struct Hash_Entry_ *next;
} Hash_Entry;

typedef struct Hash_Table_ {
	char type;       /* Tipo di funzione di hash utilizzata */
	long size;       /* Numero di chiavi di hash            */
	long num;        /* Numero di elementi nella tabella    */
	long collisions; /* Numero di collisioni avvenute (per statistica) */
	long (*hash)(const void *, long);       /* Hash function    */
	int (*cmp)(const void *, const void *); /* Compare function */
	Hash_Entry **table;
} Hash_Table;

/* Prototipi funzioni in hash.c */
int hash_init(Hash_Table **ht, long size, char type);
void hash_free(Hash_Table **ht);
Hash_Entry * hash_find(Hash_Table *ht, void *key);
int hash_insert(Hash_Table *ht, void *key, void *data);
void * hash_remove(Hash_Table *ht, const void *key);

#endif /* cache.h */
