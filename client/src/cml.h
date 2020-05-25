/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : cml.c                                                              *
*        Cittadella Markup Language.                                        *
****************************************************************************/

#ifndef _CML_H
#define _CML_H   1

#include "cml_entities.h"
#include "metadata.h"

void render_link(char *link, char *label, size_t maxlen);
int cml_print_max(const char *str, int max);
int cml_spoiler_print(const char *str, int max, int spoiler);
int cml_printf(const char *format, ...);
int cml_sprintf(char *str, const char *format, ...);
int cml_nprintf(size_t size, const char *format, ...);
char * cml_evalf(int *color, const char *format, ...);
char * cml_eval_max(const char *str, int *totlen, int maxlen, int *color,
                    Metadata_List *mdlist);
int cml2editor(const char *str, int *outstr, int *outcol, int *totlen,
	       int maxlen, int *color, Metadata_List *mdlist);
char * editor2cml(int *str, int *col, int *color, size_t len,
                  Metadata_List *mdlist);
char * iso2cml(char *str, size_t len);
void cml_extract_md(const char *str, Metadata_List *mdlist);

#define cml_print(STR) cml_print_max((STR), -1)
#define cml_eval(STR, COLOR) cml_eval_max((STR), NULL, -1, (COLOR))

/***************************************************************************/
#endif /* cml.h */
