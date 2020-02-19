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
* Cittadella/UX client			(C) M. Caldarelli and R. Vianello   *
*									    *
* File : urna-servizio.h
*	 funzioni per la gestione dei referendum e sondaggi.                *
****************************************************************************/
#ifndef URNA_SERVIZIO_H

#define URNA_SERVIZIO_H

#include "urna-strutture.h"


struct elenco_ref {
			long int num;
			int tipo;
			int modo;
			char titolo[LEN_TITOLO+1];
			long int stop;
			char lettera[3];
			char room[ROOMNAMELEN+1];
			char zap;
			char votabile;
			char votato;
			char posticipabile;
};

#if 0
struct all_ref {
		struct elenco_ref * elenco[MAX_REFERENDUM];
		int totali; /* totale dei ref scaricati */
		int possibili; /* numero di ref tra cui si puo` scegliere*/
		int scelto; /* posizione di quello scelto*/
		char flag[2]; /* ??? */
};


#endif
int chiedi_scelte(char *risposte[], int min_scelte, int maxlen, int max_scelte, char *prompt);
int get_elenco_ref(struct elenco_ref *elenco[], char tipo);
int new_str_m_abo(char *prompt, char *str, int max);
int scegli_ref(int n_rs, struct elenco_ref *elenco[], char *richiesta, const char *format);
int trim(char *str);
int urna_prendi_voci(struct urna_client *dati);
long get_ref(struct urna_client *dati, long num, char *ask, char vota);
void azzera_dati(struct urna_client *dati);
void crea_elenco_base(char str[], int len, int max);
void debugga_dati(struct urna_client *u, int n);
void free_dati(struct urna_client *dati);
void free_elenco_ref(struct elenco_ref *elenco[]);
void print_ref(int n_rs, struct elenco_ref *elenco[], const char *format);
void spaces(int n);
void stampa_motivo(int motivo);
void stampa_quesito(struct text *quesito);
void stampa_voci(const struct urna_client *dati);
void urna_prendi_testo(struct urna_client *dati);
void ordina_ref(int n_rs,struct elenco_ref *elenco[], char *format);
int cmp_ref(struct elenco_ref **a, struct elenco_ref **b);

#endif /* URNA_SERVIZIO_H */
