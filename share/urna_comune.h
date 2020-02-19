/*
 * Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX share                     (C) M. Caldarelli and R. Vianello *
*                                                                           *
* File: urna_comune.h                                                       *
*       Gestione referendum e sondaggi.                                     *
****************************************************************************/

#ifndef URNA_COMUNE
#define URNA_COMUNE   1

#define POS_PROPONENTE 1
#define POS_NOME_ROOM 2
#define POS_TIPO 3
#define POS_MODO 4
#define POS_BIANCA 5
#define POS_ASTENSIONE_VOTO 6
#define POS_STOP 7
#define POS_NUM 8
#define POS_CRITERIO 9
#define POS_VAL 10
#define POS_ANZIANITA 11
#define POS_MAX_VOCI 12
#define POS_NUM_VOCI 13
#define POS_VAL_CRITERIO 14
#define POS_START  15
#define POS_NUM_PROP 16
#define POS_LETTERA 17
#define POS_MAXLEN_PROP 18
#define MAX_POS 19
#define MAX_POSLEN 51
#define URNA_LASCIA_VOTARE 1000

#define CODIFICA_SI     0
#define CODIFICA_NO     1

#define MODO_SI_NO      0
#define MODO_SCELTA_SINGOLA  1
#define MODO_SCELTA_MULTIPLA  2
#define MODO_VOTAZIONE  3
#define MODO_PROPOSTA  4
#define NUM_MODI	4

#define TEMPO_MINIMO   7200 
#define TEMPO_PER_VOTARE 2000
#define MAXLEN_QUESITO   4
#define MAXLEN_PROPOSTA (80 +1)
#define LEN_TESTO	80
#define LEN_TITOLO	(50+1)
#define MAXLEN_VOCE 	(50+1)
#define MAX_VOCI	20
#define MAX_POSTICIPI	2

#define TIPO_SONDAGGIO  0
#define TIPO_REFERENDUM 1
#define NOME_TIPO(a) (a==TIPO_SONDAGGIO?"sondaggio" : "referendum")

/* Criteri di voto */

#define CV_UNIVERSALE 0		/* Suffragio universale */
#define CV_LIVELLO 1		/* Livello minimo */
#define CV_ANZIANITA 2		/* Anzianita' minima */
#define CV_CALLS 3		/* Numero minimo di chiamate */
#define CV_POST 4		/* Numero minimo di post */
#define CV_X 5			/* Numero minimo di Xmsg */
#define CV_SESSO 6		/* Sesso */
#define OTHER 7			/* altro  */

/* Errori di voto 
enum errori_urna { START_URNA=
	    700, ASKSYSOP, URNAPOCHIDATI, TROPPOVICINO, TROPPEVOCI,
	HAIVOTATO, VOTONONVALIDO, TROPPIVOTI, POCHIVOTI, ASTENSIONESUVOCE,
	ASTENSIONESCELTA, TROPPI_POSTICIPI, E_UN_ANTICIPO, E_NEL_PASSATO,
	POCHEVOCI, TROPPESCELTE, TROPPI_SOND, URNA_NON_AIDE, NON_BIANCA, END_URNA,
			NONINIT, URNADATIERRATI
};
*/

#endif /* urna_comune.h */
