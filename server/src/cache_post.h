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
#ifndef _CACHE_POST_H
# define _CACHE_POST_H   1

# include "config.h"
# ifdef USE_CACHE_POST

#  include "string.h"
#  include "cache.h"
#  include "text.h"

typedef struct Pcache_Data_ {
	long fmnum;
	long msgnum;
	long msgpos;
	char *header;
#  ifdef USE_STRING_TEXT
	char *txt;
	size_t len;
#  else
	struct text *txt;
#  endif
} Pcache_Data;

extern Cache *post_cache;

/* Prototipi funzioni in cache_post.c */
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

# endif /* Use cache post */
#endif /* cache_post.h */
