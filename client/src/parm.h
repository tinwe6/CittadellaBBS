/*
 *  Copyright (C) 2003-2003 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/*****************************************************************************
 * Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File: parm.h                                                              *
 *       funzione per la trasmissione dei parametri dei comandi del server   *
 ****************************************************************************/
#ifndef _PARM_H
#define _PARM_H   1

/*

Descrizione protocollo per inviare dei parametri dal client al server.
---------------------------------------------------------------------

- LATO CLIENT:

Vengono usati, analogamente alla trasmissione dei testi, tre comandi, uno 
di inizializzazione, che chiameremo qui INIT, il comando PARM che invia un
parametro, e il comando di terminazione che chiamiamo FINE.

Il client invia quindi la seguente sequenza di comandi:

INIT Arg1|Arg2|..|ArgN
PARM id_parametro_1|valore_parametro_1
PARM id_parametro_2|valore_parametro_2
...

FINE  Arg1|Arg2|..|ArgN

L'identificatore dei parametri e' una stringa con al piu' MAXLEN_PARM-1
caratteri (attualmete sono dieci caratteri, e comunque piu' corti sono
gli id e piu' efficiente e' il server..) In ogni caso e' facile, eliminare
il vincolo sulla lunghezza dell'id, quindi se si rivela necessario fatemelo
sapere.

Dopo l'invio di INIT, il server risponde con INVIA_PARAMETRI = "240" se tutto
e' andato bene, con ERRORE altrimenti.

Se il client ha inizializzato l'invio di parametri con un comando
tipo INIT e vuole annullare il comando, deve annullarlo inviando
il comando "Abort PaRaMeter"

  APRM

che non prende argomenti, e il Server risponde con OK=200. Se non viene
effettuata questa operazione, il client si blocca.


- LATO SERVER:

I parametri vengono inseriti nella lista t->parm, creata dinamicamente.
Ogni elemento e` del tipo "struct parameter" (vedi sotto la sua definizione).
In t->parm->id c'e' la stringa che identifica il parametro, e in
t->parm->val si trova la stringa che corrisponde al valore del dato parametro.
in t->parm->next si trova il parametro successivo.

Attenzione: i parametri vengono inseriti nella lista nell'ordine inverso in
            cui arrivano al server: nell'esempio precedente, la lista sarebbe

            ArgN -> ... -> Arg2 -> Arg1

            Non credo questo crei problemi pratici. In ogni caso, se si
            rivelasse piu' comodo avere la lista nell'ordine di arrivo,
            possiamo facilmente modificare il comando cmd_parm() per farlo.

Per implementare il comando sono necessarie le seguenti operazioni:

1) definire in questo file una costante di identificazione del comando
   che richiede l'invio dei parametri. Ad esempio:

  #define PARM_INIT 100


2) definire in comm.h una costante CON_stato_PARM che definisce lo stato
   del client (per il who) e indica che il comando ammette parametri
   (esempio: per i sondaggi usare CON_REF_PARM, gia' definita.)


3) funzione di inizializzazione

void cmd_init(struct sessione *t, char *cmd)
{
         Controlla i permessi;

         Eventuali inizializzazioni per il comando;

         t->parm_com = PARM_INIT;
         t->stato = CON_stato_PARM;

	 parm_txt(t);
}

parm_txt() inizializza l'invio dei parametri e risponde al client con
INVIA_PARAMETRI. Se il server deve inviare anche altri argomenti in risposta,
e' sufficiente sostituire parm_txt() con:

        if ((t->parm) != NULL)
		parm_free(t);
        cprintf(t, "%d %s|%s| ...\n", INVIA_PARAMETRI, arg1, arg2, ...);

4) Funzione di fine trasmissione parametri:

void cmd_fine(struct sessione *t, char *buf)
{
        if (t->parm_com != PARM_INIT)
                cprintf(t, "%d\n", ERROR);
	else {        
                effettua ulteriori controlli, se necessari;
                elabora i parametri e esegue il comando;
                Risponde OK al client.
	}
        parm_free(t);  ( Elimina i parametri. )
}

*/

#define MAXLEN_PARM 11  /* Lunghezza massima del nome del parametro */
#define MAXNUM_PARM 100 /* Massimo numero di parametri accettati dal server */

struct parameter {
	char id[MAXLEN_PARM];
	char *val;
	struct parameter *next;
};

/* Identificatori comando associato */
#define PARM_SONDAGGIO    100
#include "urna-strutture.h"

/* Prototipi delle funzioni in questo file */

void parm_txt(struct parameter *t);
void parm_free(struct parameter *t);
void cmd_parm(struct urna_client *dati, char *str);
void cmd_aprm(struct parameter *t);
void cmd_SMPO(struct parameter *t);
void cmd_bend(struct parameter *t);
int cercapar(struct parameter *p, char *id);
long int par2str(char *s, struct parameter *p, char *id, int max);
int pars2strs(char **s, struct parameter *p, char *id, int max_char, int max_par);
int pars2str(void *s, struct parameter *p, char *id, int max_par, int max_len, char *sep);
long int par2strd(char *s, struct urna_client *dati,char *id, int max_char);
int pars2strsd(char **s, struct parameter *t, char *id, int max_char, int max_par);
int pars2strd(void *s, struct parameter *t, char *id, int max_par, int max_len, char *sep);
int printpar(struct parameter *t);
int contapar(struct parameter *p, char *id);
int get_parm(struct urna_client  *dati);
#endif /* parm.h */
