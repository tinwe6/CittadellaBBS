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
* File: macro.h                                                             *
*       utility: macro per il server di Cittadella/UX                       *
****************************************************************************/

#ifndef _MACRO_H
#define _MACRO_H   1

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define ABS(x)   ((x) > 0 ? (x) : -(x))

/* This macro allows to silence the -Wunused-parameter compiler
   warning for the parameter 'param.                            */
#define IGNORE_UNUSED_PARAMETER(param) assert(true || param)

#ifndef  _MEMSTAT_H

#  define TYPE_CHAR            0
#  define TYPE_TEXT            1
#  define TYPE_CACHE           2
#  define TYPE_CACHE_BLOCK     3
#  define TYPE_PCACHE_DATA     4
#  define TYPE_HASH_TABLE      5
#  define TYPE_HASH_ENTRY      6
#  define TYPE_FM_LIST         7
#  define TYPE_FILE_MSG        8
#  define TYPE_ROOM_LIST       9
#  define TYPE_ROOM           10
#  define TYPE_ROOM_DATA      11
#  define TYPE_MSG_LIST       12
#  define TYPE_CODA_TESTO     13
#  define TYPE_BLOCCO_TESTO   14
#  define TYPE_DATI_SERVER    15
#  define TYPE_SESSIONE       16
#  define TYPE_DATI_IDLE      17
#  define TYPE_URNA           18
#  define TYPE_URNA_CONF      19
#  define TYPE_URNA_STAT      20
#  define TYPE_HEAD_VOCI      21
#  define TYPE_DATI_UT        22
#  define TYPE_LONG           23
#  define TYPE_TEXT_BLOCK     24
#  define TYPE_FLOOR          25
#  define TYPE_FLOOR_DATA     26
#  define TYPE_F_ROOM_LIST    27
#  define TYPE_TM             28
#  define TYPE_HTTP_SESSIONE  29
#  define TYPE_BADGE          30
#  define TYPE_LISTA_UT       31
#  define TYPE_PARAMETER      32
#  define TYPE_POP3_SESSIONE  33
#  define TYPE_POP3_MSG       34
#  define TYPE_URNA_DEFNDE    35
#  define TYPE_URNA_DATI      36
#  define TYPE_URNA_VOTI      37
#  define TYPE_URNA_PROP      38
#  define TYPE_POINTER        39
#  define TYPE_VOTANTI        40
#  define TYPE_IMAP4_SESSIONE 41
#  define TYPE_IMAP4_MSG      42
#  define TYPE_MSG_REF        43
#  define TYPE_PTREF          44
#  define TYPE_REFLIST        45
#  define TYPE_PTNODE         46
#  define TYPE_PTREE          47
#  define TYPE_BLOG           48
#  define TYPE_DFR            49
#  define TYPE_VOIDSTAR       50
#  define TYPE_FILE_BOOKING   51
#  define TYPE_NUM            52 /* Questo deve stare in fondo */

extern char * citta_strdup(const char *str);

#ifdef USE_MEM_STAT
extern void * Calloc(size_t num, unsigned long size, int tipo);
extern void * Realloc(void *ptr, size_t size);
extern char * Strdup(const char *str);
extern void Free(void *ptr);

#define CREATE(result, type, num, tnum) do {                          \
      if (!((result) = (type *) Calloc((num), sizeof(type), (tnum)))) \
       { Perror("Calloc failure"); abort(); } } while(0)

#define RECREATE(result, type, number) do {                               \
  if (!((result) = (type *) Realloc ((result), sizeof(type) * (number)))) \
                { Perror("Realloc failure"); abort(); } } while(0)

extern void Perror(const char *str);

#else
#include <string.h>

static inline void * Calloc(size_t num, unsigned long size, int tipo)
{
        IGNORE_UNUSED_PARAMETER(tipo);
	return calloc(num, size);
}

static inline void * Realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

static inline void Free(void *ptr)
{
	free(ptr);
}

static inline char * Strdup(const char *ptr)
{
	return citta_strdup(ptr);
}

static inline void Perror(const char *str) {
        perror(str);
}

#  define CREATE(result, type, num, tnum)   do {                      \
      if (!((result) = (type *) calloc((num), sizeof(type)))) \
	{ Perror("calloc failure"); abort(); (void *)tnum; } } while(0)

#  define RECREATE(result, type, number)   do {                      \
      if (!((result) = (type *) realloc(result,number * sizeof(type)))) \
       { Perror("realloc failure"); abort(); } } while(0)

#endif /* USE_MEM_STAT*/
#endif /* MEM_STAT_H */


#define IS_SET(flag,bit)  ((bool)((flag) & (bit)))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')
#define RIGA(a) (80*a)

#define SESSO(UT) (((UT)->sflags[0] & SUT_SEX) &&            \
                    ((UT)->flags[2] & UT_VSEX)) ? 'F' : 'M'

#define SESSON(UT) (((UT)->sflags[0] & SUT_SEX) &&            \
                    ((UT)->flags[2] & UT_VSEX)) ? 1 : 0

#endif /* macro.h */
