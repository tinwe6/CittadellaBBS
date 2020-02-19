/* * Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server (C) M. Caldarelli and R. Vianello *
* *
* File: urna-strutturee.c *
* Gestione referendum e sondaggi. *
******************************************************************************/

/* Wed Nov 12 23:28:42 CET 2003 */

#include "config.h"
#include "urna-strutture.h"

struct urna_stat ustat;
#if URNA_DEFINENDE
unsigned short ut_definende;
struct definende *ut_def;	/* urne che qualcuno sta definendo */
#endif

