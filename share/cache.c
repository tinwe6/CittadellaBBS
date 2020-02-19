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
#include <stdlib.h>

#include "cache.h"
#include "macro.h"
#include <stdio.h>

/* Prototipi funzioni in questo file */
int cache_init(Cache **cache, long size, long maxmem, char type,
	       void (*destroy)(void *));
void cache_free(Cache **cache);
Cache_Block * cache_find(Cache *cache, void *key);
void cache_insert(Cache *cache, void *key, void *data);
static void cache_enqueue(Cache *cache, Cache_Block *b);
static void cache_dequeue(Cache *cache, Cache_Block *b);

/*
 * Gli elementi vengono inseriti in fondo alla coda (cache->last). Se la
 * cache e' piena e serve un nuovo elemento, si fa posto all'inizio della
 * coda, eliminando cache->first. Quando viene utilizzato un elemento della
 * cache, questo viene spostato in fondo alla coda. In questo modo si
 * implementa una cache LRU, e si elimina sempre l'elemente della cache
 * che e' rimasto inutilizzato piu' a lungo.
 */
/***************************************************************************/
/*
 * Inizializza la cache **cache, di capienza 'size' blocchi, con una memoria
 * disponibile di maxmem bytes e con una chiave di hash del tipo HASH_STRING
 * o HASH_LONG.
 * Restituisce 0 se l'operazione va in porto, -1 altrimenti.
 */
int cache_init(Cache **cache, long size, long maxmem, char type,
	       void (*destroy)(void *))
{
	CREATE(*cache, Cache, 1, 2);
	if (hash_init(&((*cache)->hash), size/2, type) == -1) {
		Free(*cache);
		return -1;
	}
	(*cache)->size = size;
	(*cache)->maxmem = maxmem;
	(*cache)->num = 0;
	(*cache)->mem = 0;
	(*cache)->requests = 0;
	(*cache)->hits = 0;
	(*cache)->key_type = type;
	(*cache)->free = destroy;
	(*cache)->first = NULL;
	(*cache)->last = NULL;
	return 0;
}

/*
 * Libera tutta la memoria associata alla cache **cache.
 */
void cache_free(Cache **cache)
{
	Cache_Block *tmp, *next;
	
	hash_free(&((*cache)->hash));
	for (tmp = (*cache)->first; tmp; tmp = next) {
		next = tmp->next;
		(*cache)->free(tmp->data);
		Free(tmp);
	}
	Free(*cache);
	*cache = NULL;
}

/*
 * Cerca il blocco con chiave 'key' nella cache '*cache'.
 * Restituisce il puntatore al blocco se lo trova, altrimenti NULL.
 */
Cache_Block * cache_find(Cache *cache, void *key)
{
	Hash_Entry *h;
	Cache_Block *b;
	
	cache->requests++;
	if ( (h = hash_find(cache->hash, key))) {
		cache->hits++;
		/* sposta in fondo allo stack il blocco */
		b = (Cache_Block *)(h->data);
		if (b == cache->last)
			return b; /* e' gia' in fondo */
		cache_dequeue(cache, b);
		cache_enqueue(cache, b);
		return b;
	}
	return NULL;
}

/*
 * Inserisce nella cache 'cache' i dati 'data' identificati dalla chiave 'key'.
 */
void cache_insert(Cache *cache, void *key, void *data)
{
	Cache_Block *block;

	if (cache->num < cache->size) {
		CREATE(block, Cache_Block, 1, 3);
		cache->num++;
	} else {  /* Libera il blocco Least Recently Used. */
		block = cache->first;
		cache_dequeue(cache, block);
		hash_remove(cache->hash, block->key);
		/* Qui salva il disco se va salvato (da fare) */
		cache->free(block->data);
	}
	block->key = key;
	block->data = data;
	block->next = NULL;
	cache_enqueue(cache, block);
	hash_insert(cache->hash, key, block);
}

/*
 * Elimina dalla cache il blocco associato alla chiave 'key'.
 */
void cache_remove(Cache *cache, void *key)
{
	
}

static void cache_enqueue(Cache *cache, Cache_Block *b)
{
	if (cache->first == NULL) {
		cache->first = b;
		cache->last = b;
		b->prev = NULL;
	} else {
		cache->last->next = b;
		b->prev = cache->last;
		cache->last = b;
	}
	b->next = NULL;
}

static void cache_dequeue(Cache *cache, Cache_Block *b)
{
	if (cache->first == b) {
		if (cache->last == b) {
			cache->first = NULL;
			cache->last = NULL;
		} else {
			cache->first = b->next;
			cache->first->prev = NULL;
		}
	} else if (cache->last == b) {
		cache->last = b->prev;
		cache->last->next = NULL;
	} else {
		b->prev->next = b->next;
		b->next->prev = b->prev;
	}
	b->prev = NULL;
	b->next = NULL;
}

