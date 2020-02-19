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
* File : prompt.h                                                           *
*        Comandi per la visualizzazione dei prompt.                         *
****************************************************************************/
#ifndef _PROMPT_H
#define _PROMPT_H   1

#define P_ROOM            1
#define P_MSG             2
#define P_PAG             3
#define P_EDT             4
#define P_ENT             5
#define P_XLOG            6
#define P_EDITOR          7
#define P_VOTA            8
#define P_FRIEND          9
#define P_WRAPPER_URNA   10

/* Variabili globali */
extern char prompt_curr;      /* Prompt correntemente visualizzato.          */
extern char *prompt_txt;      /* Puntatore alla stringa che si sta editando. */
extern long prompt_pager_max, prompt_pager_n; /* #linee per prompt pager.    */
extern long prompt_xlog_n;    /* Numero messaggio diario corrente.           */
extern long prompt_xlog_max;  /* Numero messaggi nel diario.                 */
extern long prompt_attach;    /* Numero allegati presenti nel post corrente  */

/* prototipi delle funzioni in prompt.h */
void room_prompt(char *pr);
void msg_prompt(char *prompt);
void pager_prompt(char *prompt);
void edit_prompt(char *prompt);
void entry_prompt(char *prompt);
void print_prompt(void);
void del_prompt(void);

#endif /* prompt.h */
