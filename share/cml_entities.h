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
* File : cml_entities.h                                                     *
*        Entity table for CML (and HTML)                                    *
****************************************************************************/

#ifndef _CML_ENTITIES_H
#define _CML_ENTITIES_H  1

#include "iso8859.h"

struct cml_entity_tab {
        int code;
        char *str;
        char *ascii;
};

int cml_entity(const char **str);

extern const struct cml_entity_tab cml_entity_tab[];

#endif /* cml_entities.h */
