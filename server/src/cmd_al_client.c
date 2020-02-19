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
* File: cmd_al_client.c                                                     *
*       trasferimento da coda comandi a coda output etc etc                 *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cittaserver.h"

/* Prototipi delle funzioni in questo file */
void idle_cmd(struct sessione *t);

/******************************************************************************
******************************************************************************/

void idle_cmd(struct sessione *t)
{
        switch(t->idle.warning) {
        case 1 :                    /* Warning per idle prolungato */
                cprintf(t, "%d Idle warning.\n", IWRN);
                break;
        case 2 :                    /* Ordine di logout            */
                cprintf(t, "%d Ordine di logout.\n", ILGT);
                break;
        }
}
