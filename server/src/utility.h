/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
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
* File: utility.h                                                           *
*       Header File: Funzioni varie, di utilita' generale.                  *
****************************************************************************/
#ifndef _UTILITY_H
#define _UTILITY_H   1

#include "text.h"
#include "time.h"
#include <sys/time.h>

/* Prototipi funzioni in utility.c */
void citta_log(char *str);
void citta_logf(const char *format, ...) CHK_FORMAT_1;
void Perror(const char *str);
int timeval_subtract(struct timeval *result, struct timeval *x,
		     struct timeval *y);
int timeval_add(struct timeval *result, struct timeval *x,
		struct timeval *y);
size_t text2string(struct text *txt, char **strtxt);
void strdate(char *str, long ora);
char *space2under(char *stringa);
char *under2space(char *stringa);
void strdate_rfc822(char *str, size_t max, long ora);
char * str2html(char *str);
char * strstrip(const char *src);
const char * get_word(char *word, const char *src, size_t * len);
void random_init(void);
int random_int(int min, int max);
void generate_random_string_AZ(char *buf, size_t len);
void generate_random_string(char *buf, size_t len);

#endif /* utility.h */
