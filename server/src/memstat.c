/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
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
* File: memstat.c                                                           *
*       Funzioni di allocazione e liberazione memoria con statistiche       *
****************************************************************************/
#include "memstat.h"

#ifdef USE_MEM_STAT

/* definire MEMSTAT_ON per avere il memstat completo funzionante */
//#define MEMSTAT_ON

#include <string.h>
#include <time.h>
#include "blog.h"
#include "cache.h"
#include "cache_post.h"
#include "cittaweb.h"
#include "file_messaggi.h"
#include "fileserver.h"
#include "find.h"
#include "hash.h"
#include "imap4.h"
#include "parm.h"
#include "patacche.h"
#include "pop3.h"
#include "prefixtree.h"
#include "rooms.h"
#include "strutture.h"
#include "text.h"
#include "urna-strutture.h"
#include "utente.h"
#include "utility.h"

/* TODO rendere questo vettore statito e creare funzioncina per l'accesso */
unsigned long mem_stats[TYPE_NUM];

/* TODO anche questa, renderla statica */
struct mem_type type_table[] = {
	  {"char",         sizeof(char)},
	  {"text",         sizeof(struct text)},
	  {"Cache",        sizeof(Cache)},
	  {"Cache_Block",  sizeof(Cache_Block)},
#ifdef USE_CACHE_POST
	  {"Pcache_Data",  sizeof(Pcache_Data)},
#else
	  {"---",          1},
#endif
	  {"Hash_Table",   sizeof(Hash_Table)},
	  {"Hash_Entry",   sizeof(Hash_Entry)},
	  {"fm_list",      sizeof(struct fm_list)},
	  {"file_msg",     sizeof(struct file_msg)},
	  {"room_list",    sizeof(struct room_list)},
	  {"room",         sizeof(struct room)},
	  {"room_data",    sizeof(struct room_data)},
	  {"msg_list",     sizeof(struct msg_list)},
	  {"coda_testo",   sizeof(struct coda_testo)},
	  {"blocco_testo", sizeof(struct blocco_testo)},
	  {"dati_server",  sizeof(struct dati_server)},
	  {"sessione",     sizeof(struct sessione)},
	  {"dati_idle",    sizeof(struct dati_idle)},
#ifdef USE_REFERENDUM
	  {"urna",         sizeof(struct urna)},
	  {"urna_conf",    sizeof(struct urna_conf)},
	  {"urna_stat",    sizeof(struct urna_stat)},
	  {"head-voci",    sizeof(char ** )},
#else
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
#endif
	  {"dati_ut",      sizeof(struct dati_ut)},
	  {"long",         sizeof(long)},
	  {"Text_Block",   sizeof(Text_Block)},
#ifdef USE_FLOORS
	  {"floor",        sizeof(struct floor)},
	  {"floor_data",   sizeof(struct floor_data)},
	  {"f_room_list",  sizeof(struct f_room_list)},
#else
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
#endif
	  {"tm",           sizeof(struct tm)},
#ifdef USE_CITTAWEB
          {"http_sess",    sizeof(struct http_sess)},
#else
	  {"---",          1},
#endif
#ifdef USE_BADGES
          {"badge",        sizeof(struct badge)},
#else
	  {"---",          1},
#endif
          {"lista_ut",     sizeof(struct lista_ut)},
          {"parameter",    sizeof(struct parameter)},
#ifdef USE_POP3
          {"pop3_sess",    sizeof(struct pop3_sess)},
          {"pop3_msg",     sizeof(struct pop3_msg)},
#else
	  {"---",          1},
	  {"---",          1},
#endif
#ifdef USE_REFERENDUM
          {"definende",    sizeof(struct definende)},
          {"urna_dati",    sizeof(struct urna_dati)},
          {"urna_voti",    sizeof(struct urna_voti)},
          {"urna_prop",    sizeof(struct urna_prop)},
          {"pointer",      sizeof(void *)},
          {"votanti",      sizeof(struct votanti)},
#else
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
#endif
#ifdef USE_IMAP4
          {"imap4_sess",    sizeof(struct imap4_sess)},
          {"imap4_msg",     sizeof(struct imap4_msg)},
#else
	  {"---",          1},
	  {"---",          1},
#endif
#ifdef USE_FIND
	  {"msg_ref",      sizeof(struct msg_ref)},
	  {"ptref",        sizeof(struct reference)},
	  {"reflist",      sizeof(struct reflist)},
	  {"ptnode",       sizeof(struct ptnode)},
	  {"ptree",        sizeof(struct ptree)},
#else
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
	  {"---",          1},
#endif
#ifdef USE_BLOG
	  {"blog",         sizeof(struct blog)},
#else
	  {"---",          1},
#endif
	  {"dfr",          sizeof(dfr_t)},
	  {"file_booking", sizeof(struct file_booking)},
	  {"void *",       sizeof(void *)}
};

/* Numero di slot allocate alla volta */
#define MEMSTATS_STEP 256

struct memstats {
        size_t num;          /* Numero slot utilizzate      */
        size_t alloc;        /* Numero slot allocate        */
        unsigned long mem_tot;
        unsigned long mem_max;
        unsigned long n_alloc;
        unsigned long n_realloc;
        unsigned long n_free;
        unsigned int *tipo; /* Tipo dato allocato          */
        size_t *n;          /* Numero di elementi allocati */
        void **addr;        /* Vettore indirizzi           */
};

struct memstats memstats;

/* Prototipi funzioni in questo file */
void mem_init(void);
void mem_log(void);
void * Malloc(unsigned long size, int tipo);
void * Calloc(size_t num, unsigned long size, int tipo);
void Free(void *ptr);
void * Realloc(void *ptr, size_t size);
char * Strdup(const char *str);
unsigned long mem_tot(void);
unsigned long mem_max(void);
#ifdef MEMSTAT_ON
static void mem_ins(void *addr, unsigned int tipo, size_t n);
static bool mem_del(void *addr);
static unsigned int mem_tipo(void *addr);
#endif

/*****************************************************************************
*****************************************************************************/
/*
 * Inizializza le statistiche sulla memoria.
 */
void mem_init(void)
{
	int i;
	
        memstats.alloc = MEMSTATS_STEP;
        memstats.num = 0;
        memstats.mem_tot = 0;
        memstats.mem_max = 0;
        memstats.n_alloc = 0;
        memstats.n_realloc = 0;
        memstats.n_free = 0;

#ifdef MEMSTAT_ON
        memstats.addr = (void **) malloc(MEMSTATS_STEP * sizeof(void *));
        memstats.tipo = (unsigned int *) malloc(MEMSTATS_STEP
                                                * sizeof(unsigned int));
        memstats.n = (size_t *) malloc(MEMSTATS_STEP * sizeof(size_t));
#endif

        //        citta_logf("MEMSTAT addr=%x tipo=%x n=%x", memstats.addr, memstats.tipo, memstats.n);

	for (i = 0; i < TYPE_NUM; i++)
		mem_stats[i] = 0UL;
}

void mem_log(void)
{
#ifdef MEMSTAT_ON
	int i;
	
	citta_logf("MEM Currently allocated %ld bytes, Max %ld.", memstats.mem_tot,
              memstats.mem_max);
	for (i = 0; i < TYPE_NUM; i++) {
		if (mem_stats[i])
			citta_logf("MEM STRUCT %-15s : %ld -- %ld bytes",
                              type_table[i].type, mem_stats[i],
                              mem_stats[i] * type_table[i].size);
                citta_logf("MEMSTAT addr=%x tipo=%ld n=%d", memstats.addr[i], memstats.tipo[i], memstats.n[i]);
        }
#endif
}

void * Malloc(unsigned long size, int tipo)
{
	void * mem;
	
	if (size == 0)
		return NULL;

	if ( (mem = malloc(size)) == NULL)
		return NULL;

#ifdef MEMSTAT_ON
        mem_ins(mem, tipo, size / type_table[tipo].size);
        memstats.n_alloc++;
#endif

	return mem;
}

void * Calloc(size_t num, unsigned long size, int tipo)
{
	void * mem;
	
	if (size == 0)
		return NULL;
	
	if ( (mem = calloc(1, num*size)) == NULL)
		return NULL;

#ifdef MEMSTAT_ON
        mem_ins(mem, tipo, num*size / type_table[tipo].size);
        memstats.n_alloc++;
#endif

	return mem;
}

void Free(void *ptr)
{
	if (ptr == NULL)
		return;
#ifdef MEMSTAT_ON
        if (mem_del(ptr)) {
                free(ptr);
                memstats.n_free++;
        } else
                citta_logf("SYSERR Free: trying to free non allocated mem.");
#else
        free(ptr);
#endif
}

void * Realloc(void *ptr, size_t size)
{
#ifdef MEMSTAT_ON
        unsigned long tipo;
#endif

	if (size == 0)
		return NULL;

#ifdef MEMSTAT_ON
        tipo = mem_tipo(ptr);

        if (!mem_del(ptr)) {
                citta_logf("SYSERR Realloc: trying to realloc non allocated mem.");
                return NULL;
        }
#endif	
	if ( (ptr = realloc(ptr, size)) == NULL)
		return NULL;

#ifdef MEMSTAT_ON
        mem_ins(ptr, tipo, size / type_table[tipo].size);
        memstats.n_realloc++;
#endif

	return ptr;
}

/* Funzione strdup() della libreria standard che utilizza Malloc */
char * Strdup(const char *str)
{
	size_t len;
	void * clone;
	
	len = strlen(str) + 1;
	if ( (clone = Malloc(len, TYPE_CHAR)) == NULL)
		return NULL;
	return (char *) memcpy(clone, str, len);
}

unsigned long mem_tot(void)
{
        return memstats.mem_tot;
}

unsigned long mem_max(void)
{
        return memstats.mem_max;
}

/***************************************************************************/
#ifdef MEMSTAT_ON

/* Inserisce i dati di un allocazione nel vettore */
static void mem_ins(void *addr, unsigned int tipo, size_t n)
{
        int tmp, start, end;

        if (memstats.num == memstats.alloc) {
                citta_logf("MEMSTAT addr=%x tipo=%x n=%x BEFORE", (int)memstats.addr,
                      (int)memstats.tipo, (int)memstats.n);
                memstats.alloc += MEMSTATS_STEP;
                memstats.tipo = (unsigned int *) realloc(memstats.tipo,
                                 memstats.alloc * sizeof(unsigned int));
                memstats.n = (size_t *)  realloc(memstats.n,
                                memstats.alloc * sizeof(size_t));
                memstats.addr = (void **) realloc(memstats.addr,
                                  memstats.alloc * sizeof(void *));
                citta_logf("MEMSTAT addr=%x tipo=%x n=%x AFTER", (int)memstats.addr,
                      (int)memstats.tipo, (int)memstats.n);
        }

        start = 0;
        end = memstats.num - 1;

        do {
                tmp = (end + start) / 2;
                if (addr < memstats.addr[tmp])
                        end = tmp;
                else
                        start = tmp;
        } while ((end-start) > 1);

        memmove(memstats.addr+end+1, memstats.addr+end,
                (memstats.num - end)*sizeof(void *));
        memmove(memstats.tipo+end+1, memstats.tipo+end,
                (memstats.num - end)*sizeof(unsigned int));
        memmove(memstats.n+end+1, memstats.n+end,
                (memstats.num - end)*sizeof(size_t));

        memstats.addr[end] = addr;
        memstats.tipo[end] = tipo;
        memstats.n[end] = n;

        memstats.num++;
        memstats.mem_tot += type_table[tipo].size * n;
        if (memstats.mem_tot > memstats.mem_max)
                memstats.mem_max = memstats.mem_tot;

        mem_stats[tipo]++;
}

/* Elimina i dati di un'allocazione da un vettore. Restituisce false se
 * i dati non si trovano.                                                 */
static bool mem_del(void *addr)
{
        int tmp, start, end;

        start = 0;
        end = memstats.num - 1;

        if (memstats.addr[start] == addr)
                end = start;
        else if (memstats.addr[end] == addr)
                start = end;

        do {
                tmp = (end + start) / 2;
                if (memstats.addr[tmp] == addr) {
                        memstats.mem_tot -= memstats.n[tmp]
                                * type_table[memstats.tipo[tmp]].size;
                        mem_stats[memstats.tipo[tmp]]--;

                        memmove(memstats.addr+tmp, memstats.addr+tmp+1,
                                (memstats.num - tmp - 1)*sizeof(void *));
                        memmove(memstats.tipo+tmp, memstats.tipo+tmp+1,
                                (memstats.num - tmp - 1)*sizeof(unsigned int));
                        memmove(memstats.n+tmp, memstats.n+tmp,
                                (memstats.num - end)*sizeof(size_t));

                        memstats.num--;

                        return true;
                } else if (addr < memstats.addr[tmp])
                        end = tmp;
                else
                        start = tmp;
        } while ((end-start) > 1);

        return false;
}


/* Restituisce il tipo dei dati allocati in addr */
static unsigned int mem_tipo(void *addr)
{
        int tmp, start, end;

        start = 0;
        end = memstats.num - 1;

        do {
                tmp = (end + start) / 2;
                if (memstats.addr[tmp] == addr)
                        return memstats.tipo[tmp];
                else if (addr < memstats.addr[tmp])
                        end = tmp;
                else
                        start = tmp;
        } while ((end-start) > 1);
        if (memstats.addr[end] == addr)
                return memstats.tipo[end];
        if (memstats.addr[start] == addr)
                return memstats.tipo[start];

        return 0;
}

#endif

#endif /* Use Mem_Stat */
