/*
 *  Copyright (C) 2005 by Marco Caldarelli
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
* File : metadata_ops.h                                                     *
*        operazioni utente sul metadata                                     *
****************************************************************************/
#ifndef METADATA_OPS_H
#define METADATA_OPS_H
#include "metadata.h"

#include <stdbool.h>

/* Prototipi funzioni definite in metadata_ops.c */
bool mdop(Metadata_List *mdlist);
void mdop_print_list(Metadata_List *mdlist);
void mdop_delete(Metadata_List *mdlist, int mdnum);
void mdop_upload_files(Metadata_List *mdlist);

#endif /* metadata_ops.h */
