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
* Cittadella/UX Library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: extract.h                                                           *
*       Header File: Funzioni per estrarre argomenti dalle stringhe.        *
****************************************************************************/
#ifndef _EXTRACT_H
#define _EXTRACT_H   1

#include <stdbool.h>
#include <string.h>

/* Prototipi funzioni in questo file */
int num_parms(const char *source);
void extract(char *dest, const char *source, int parmnum);
void extractn(char *dest, const char *source, int parmnum, size_t len);
char * extracta(const char *source, int parmnum);
bool extract_bool(const char *source, int parmnum);
int extract_int(const char *source, int parmnum);
long extract_long(const char *source, int parmnum);
unsigned long extract_ulong(const char *source, int parmnum);
void clear_parm(char *source, int parmnum);

/****************************************************************************/
#endif /* extract.h */
