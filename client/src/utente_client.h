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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : utente_client.h                                                    *
*        funzioni di gestione degli utenti per il client.                   *
****************************************************************************/
#ifndef _UTENTE_CLIENT_H
#define _UTENTE_CLIENT_H   1

extern char *titolo[];

/* prototipi delle funzioni in utente_client.c */
void lista_utenti(void);
void registrazione(bool nuovo);
char profile(char *nome_def);
void caccia_utente(void);
void elimina_utente(void);
void edit_user(void);
void chpwd(void);
void chupwd(void);
void enter_profile(void);

#endif /* utente_client.h */
