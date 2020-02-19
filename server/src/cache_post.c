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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: cache_post.c                                                        *
*       Gestione della memoria per i post, con sistema di cache.            *
****************************************************************************/
#include "config.h"

#ifdef USE_CACHE_POST

#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "cache_post.h"
#include "file_messaggi.h"
#include "memstat.h"
#include "utility.h"

#define LBUF 256

/* Puntatore alla cache dei post (globale) */
Cache *post_cache;

/* Prototipi funzioni in questo file */
int cp_init(void);
void cp_destroy(void);
#ifdef USE_STRING_TEXT
int cp_get(long fmnum, long msgnum, long msgpos, char **txt, size_t *len,
	   char **header);
int cp_put(long fmnum, long msgnum, long msgpos, char *txt, size_t len,
	   char *header);
#else
int cp_get(long fmnum, long msgnum, long msgpos, struct text **txt,
	   char **header);
int cp_put(long fmnum, long msgnum, long msgpos, struct text *txt,
	   char *header);
#endif
void cp_free(void *data);

/*****************************************************************************
*****************************************************************************/
/*
 * Inizializza la cache dei post.
 */
int cp_init(void)
{
	return cache_init(&post_cache, POST_CACHE_SIZE, POST_CACHE_MAXMEM,
			  HASH_LONG, cp_free);
}

void cp_destroy(void)
{
	if (post_cache->requests)
		citta_logf("CACHE post: %ld elmts, %ld requests, %ld hits (%d%%).",
		     post_cache->num, post_cache->requests, post_cache->hits,
		     (int)(100.0*post_cache->hits/post_cache->requests));
	cache_free(&post_cache);
}

/*
 * Legge un post. Prima verifica se e' gia' nella cache, se non lo trova lo
 * legge dal file messaggi con fm_get() e lo inserisce nella cache.
 * Se il post e' stato recuperato, restituisce 0, altrimenti il codice
 * di errore corrispondente a fm_get().
 */
#ifdef USE_STRING_TEXT
int cp_get(long fmnum, long msgnum, long msgpos, char **txt, size_t *len,
	   char **header)
#else
int cp_get(long fmnum, long msgnum, long msgpos, struct text **txt,
	   char **header)
#endif
{
	Cache_Block *block;
	Pcache_Data *data;
	int ret;
	
	if ( (block = cache_find(post_cache, &msgnum))) {
		data = (Pcache_Data *)block->data;
		if ((fmnum == data->fmnum) && (msgnum == data->msgnum)
		    && (msgpos == data->msgpos)) {
			*header = data->header;
			*txt = data->txt;
#ifdef USE_STRING_TEXT
			*len = data->len;
#endif
			return 0;
		}
	}
	*header = (char *) Malloc(LBUF, TYPE_CHAR);
#ifdef USE_STRING_TEXT
	ret = fm_get(fmnum, msgnum, msgpos, txt, len, *header);
#else
	*txt = txt_create();
	ret = fm_get(fmnum, msgnum, msgpos, *txt, *header);
#endif
	if (ret == 0) {
		/* TODO : si puo' evitare il realloc se fm_get() restituisce
		 * anche la lunghezza di header, che conosce.              */
		*header = (char *) Realloc(*header, strlen(*header) + 1);
		CREATE(data, Pcache_Data, 1, TYPE_PCACHE_DATA);
		data->fmnum = fmnum;
		data->msgnum = msgnum;
		data->msgpos = msgpos;
		data->header = *header;
#ifdef USE_STRING_TEXT
		data->len = *len;
#endif
		data->txt = *txt;
		cache_insert(post_cache, &(data->msgnum), data);
	}
	return ret;
}

/*
 * Inserisce un post nella cache.
 */
#ifdef USE_STRING_TEXT
int cp_put(long fmnum, long msgnum, long msgpos, char *txt, size_t len,
	   char *header)
#else
int cp_put(long fmnum, long msgnum, long msgpos, struct text *txt,
	   char *header)
#endif
{
	Cache_Block *block;
	Pcache_Data *data;

	/* Verifico che non ci sono problemi */
	if ( (block = cache_find(post_cache, &msgnum))) {
		data = (Pcache_Data *)block->data;
		if ((fmnum == data->fmnum) && (msgnum == data->msgnum)
		    && (msgpos == data->msgpos))
			return -1;
	}
	CREATE(data, Pcache_Data, 1, TYPE_PCACHE_DATA);
	data->fmnum = fmnum;
	data->msgnum = msgnum;
	data->msgpos = msgpos;
	data->header = Strdup(header);
#ifdef USE_STRING_TEXT
	data->len = len;
#endif
	data->txt = txt;
	cache_insert(post_cache, &(data->msgnum), data);
	return 0;
}

void cp_free(void *data)
{
#ifdef USE_STRING_TEXT
	Free(((Pcache_Data *)data)->txt);
#else
	txt_free((struct text **)&((Pcache_Data *)data)->txt);
#endif
	Free(((Pcache_Data *)data)->header);
	Free((Pcache_Data *)data);
}

#endif /* USE_CACHE_POST */
