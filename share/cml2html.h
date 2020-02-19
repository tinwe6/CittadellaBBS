/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cml2html.h                                                         *
*        Cittadella Markup Language (CML) to HTML translator.               *
****************************************************************************/

#ifndef _CML2HTML_H
#define _CML2HTML_H  1

#include "metadata.h"

/* Codifica colore carattere */
#define COLOR(FG, BG, AT) (((AT) << 8) + ((BG) << 4) + (FG))
#define CC_FG(CODE)     ((CODE) & 0x0f)
#define CC_BG(CODE)     (((CODE) >> 4) & 0x0f)
#define CC_ATTR(CODE)   (((CODE) >> 8) )

char * cml2html_max(const char *str, int *totlen, int maxlen, int *color,
                    Metadata_List *mdlist);
char * htmlstr_close_tags(char *ptr, int color);

#endif /* cml2html.h */
