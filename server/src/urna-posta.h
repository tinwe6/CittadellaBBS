/*  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: urna-posta.h                                                        *
* mette i messaggi nelle stanze                                             *
*                                                                           *
****************************************************************************/

/* Wed Oct 15 00:36:27 CEST 2003 */

void posta_nuovo_quesito(struct urna_conf *ucf);
void scrivi_scelte(struct urna_conf *ucf, struct text *avviso);
void chipuo(struct urna_conf *ucf, char *buf);
