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
* File : info_port.h                                                        *
*        Gestisce la porta di informazioni di Cittadella/UX                 *
****************************************************************************/
#ifndef _INFO_PORT_H
#define _INFO_PORT_H    1

#define INFOPORT 4001

/* Prototipi funzioni in questo file */
void info_port(void);
void close_info_port(void);

#endif /* info_port.h */
