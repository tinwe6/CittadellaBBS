/*
 * Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: urnafile.h                                                            *
*       Gestione dei file di sondaggio. Load, Save, Canc, Backup              *
******************************************************************************/

/* Sun Aug 24 00:02:00 CEST 2003 */

#ifndef _URNA_FILE_H
#define _URNA_FILE_H 1

#ifdef USE_REFERENDUM
#include <stdio.h>

int esiste(const char *dir, const char *file, int progressivo);
int backup(const char *file);
int append_file(const char *dir, const char *file, const int est, FILE ** fpp);
int save_file(const char *dir, const char *file, const int est, FILE ** fpp);
int load_file(const char *dir, const char *file, const int est, FILE ** fpp);
int canc_file(const char *dir, const char *file, const int est);
int write_magic_number(FILE * fp, char code, char * nome_file);
int check_magic_number(FILE * fp, char code, char * nome_file);

#endif                          /* use referendum */
#endif                          /* urnafile.h */
