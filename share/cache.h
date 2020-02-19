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
* File : cache.c                                                            *
*        Funzioni per gestire strutture di cache.                           *
****************************************************************************/
#ifndef _CACHE_H
#define _CACHE_H   1

#include "hash.h"

typedef struct Cache_Block_ {
	void * key;
	void * data;
	struct Cache_Block_ *prev, *next;
} Cache_Block;

typedef struct Cache_ {
	long size;          /* Numero massimo di elementi nella cache   */
	long maxmem;        /* Memoria massima disponibile per la cache */
	long num;           /* Numero di elementi nella cache           */
	long mem;           /* Memoria correntemente utilizzata         */
	long requests;      /* Numero di richieste fatte alla cache     */
	long hits;          /* Numero di richieste di elementi gia' presenti */
	char key_type;      /* Tipo di chiave di identificazione utilizzata  */
	Hash_Table *hash;   /* Tabella di hash per trovare gli elementi  */
	void (*free)(void *); /* Funzione per liberare i dati            */
	Cache_Block *first; /* Puntatore al primo elemento della cache   */
	Cache_Block *last;  /* Puntatore all'ultimo elemento della cache */
} Cache;

/* Prototipi delle funzioni in cache.c */
int cache_init(Cache **cache, long size, long maxmem, char type,
	       void (*destroy)(void *));
void cache_free(Cache **cache);
Cache_Block * cache_find(Cache *cache, void *key);
void cache_insert(Cache *cache, void *key, void *data);

#endif /* cache.h */
