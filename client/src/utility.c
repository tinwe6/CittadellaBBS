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
* File : utility.c                                                          *
*        routines varie, manipolazione stringhe, prompts,  etc etc..        *
****************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "utility.h"
#include "ansicol.h"
#include "cittaclient.h"
#include "cml.h"
#include "conn.h"
#include "edit.h"
#include "inkey.h"
#include "prompt.h"
#include "urna_comune.h"
#include "file_flags.h"
#include "user_flags.h"


/* Costanti globali */
const char *settimana[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};

const char *settimana_esteso[] = {"Domenica", "Luned&iacute;",
				  "Marted&iacute;", "Mercoled&iacute;",
				  "Gioved&iacute;", "Venerd&iacute;",
				  "Sabato"};

const char *mese[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago",
                      "Set", "Ott", "Nov", "Dic"};

const char *mese_esteso[] = { "Gennaio", "Febbraio", "Marzo", "Aprile",
	"Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre",
	"Novembre", "Dicembre"};

/******************************************************************************
******************************************************************************/
/* Le funzioni seguenti prendono una stringa in input dall'utente.
 *
 * Prende al piu' 'max' caratteri in input. Per la stringa devono essere
 * percio' allocati almeno (max+1) bytes.
 *
 * Se max < 0, allora non mostra l'input (visualizza solo asterischi).
 *
 * Suffisso: _def_ : prende un valore di default per la stringa. Se la
 *                   stringa in input e' nulla, restituisce il default.
 * Suffisso: _m: lascia com'e', _M: maiuscola inizio parole.
 *
 * Le funzioni restituiscono il numero di caratteri immessi (senza contare
 * lo 0 finale) o -1 se l'immissione e' stata abortita
 * Se l'utente sceglie di mantenere il default, le funzioni _def_
 * restituiscono 0 (che comunque e' il numero di caratteri immessi)
 *
 */
int new_str_M(char *prompt, char *str, int max)
{
        str[0] = '\0';
        cml_printf("%s", prompt);
        return c_getline(str, max, true, false);
}

int new_str_m(char *prompt, char *str, int max)
{
        str[0] = '\0';
        cml_printf("%s", prompt);
        return c_getline(str, max, false, false);
}

int new_str_def_M(char *prompt, char *str, int max)
{
        int n;
        char buf[256];

        cml_printf("%s [%s]: ", prompt, str);
        n = c_getline(buf, max, true, false);
        if (n > 0) {
	        int len = (max > 0) ? max : -max;
	        strncpy(str, buf, len + 1);
	}
	return n;
}

int new_str_def_m(char *prompt, char *str, int max)
{
        int n;
        char buf[256];

        cml_printf("%s [%s]: ", prompt, str);
        n = c_getline(buf, max, false, false);
        if (n > 0) {
	        int len = (max > 0) ? max : -max;
	        strncpy(str, buf, len + 1);
	}
	return n;
}

/*************************************************************************/

/*
 * Prompt da stdin di un numero intero, positivo se neg = 0, accetta '-'
 * come primo carattere se neg = 1. Se enter, mette un \n alla fine.
 */ 
void get_number(char *str, bool neg, bool enter)
{
        int a, b;
        const int flag = 0;  /* TODO not implemented *** */
        int max = NUM_DIGIT; /* numero massimo di cifre */

        b = strlen(str);
        do { 
                a = 0;
                a = inkey_sc(0);
                if (isdigit(a) || (a == 8) || 
                    (neg && (a == '-') && (b == 0))) {
                        if ((a == 8) && (b != 0)) {
                                delchar();
                                str[b - 1] = '\0';
                                b--;
                        }
                        if ((a != 8) && (b < max)) {
                                if (!flag)
                                        putchar(a);
                                else
                                        putchar('*');
                                str[b] = a;
                                str[b + 1] = '\0';
                                b++;
                        }
                }
        } while (!((a==13) && (str[b]!='-')) && !((a==10) && (str[b]!='-')));

        if (enter)
                putchar('\n');
}

int get_int(bool enter)
{
        char str[NUM_DIGIT+1]="";
  
        get_number(str, false, enter);
        return strtol(str, NULL, 10);
}

long get_long(bool enter)
{
        char str[NUM_DIGIT+1]="";
  
        get_number(str, false, enter);
        return strtol(str, NULL, 10);
}

unsigned long get_ulong(bool enter)
{
        char str[NUM_DIGIT+1] = "";

        get_number(str, false, enter);
        return strtoul(str, NULL, 10);
}

int new_int(char *prompt)
{
        cml_printf("%s", prompt);
        return(get_int(true));
}

long new_long(char *prompt)
{
        cml_printf("%s", prompt);
        return(get_long(true));
}

unsigned long new_ulong(char *prompt)
{
        cml_printf("%s", prompt);
        return(get_ulong(true));
}

int new_int_def(char *prompt, int def)
{
        char str[NUM_DIGIT+1]="";
        
        cml_printf("%s [%d]: ", prompt, def);
        get_number(str, false, true);
        if (str[0] == 0)
                return(def);
        else
                return strtol(str, NULL, 10);
}

/* Chiede un numero intero _relativo_ con prompt e valore di default */
int new_sint_def(char *prompt, int def)
{
        char str[NUM_DIGIT+1]="";
  
        cml_printf("%s [%d]: ", prompt, def);
        get_number(str, true, true);
        if (str[0] == 0)
                return def;
        else
                return strtol(str, NULL, 10);
}

long new_long_def(char *prompt, long def)
{
        char str[NUM_DIGIT+1]="";
        
        cml_printf("%s [%ld]: ", prompt, def);
        get_number(str, false, true);
        if (str[0] == '\0')
                return(def);
        else
                return strtol(str, NULL, 10);
}

/*****************************************************************************/
void print_ok(void)
{
        push_color();
        setcolor(C_OK);
        printf("OK\n");
        pull_color();
}

void print_on_off(bool a)
{
	push_color();
	setcolor(C_TOGGLE);
	if (a)
		printf("On.\n");
	else
		printf("Off.\n");
	pull_color();
}

char * print_si_no(bool a)
{
        return (a ? (SI): (NO));
}

char si_no(void)
{
        char c;

        c = 0;
        while((c != 's') && (c != 'n'))
                c = inkey_sc(0);
        if (c == 's') {
                cml_printf("%s\n", print_si_no(1));
                return('s');
        } else {
                cml_printf("%s\n", print_si_no(0));
                return('n');
        }
}

bool new_si_no_def(char *prompt, bool def)
{
        char c;
        bool risultato;

        cml_printf("%s (s/n) [%s] : ", prompt, print_si_no(def));
        c = 0;
        while((c != 's') && (c != 'n') && (c != 10) && (c != 13))
                c = inkey_sc(0);
        if (c == 's')
                risultato = true;
        else if (c == 'n')
                risultato = false;
        else
                risultato = def;
        cml_printf("%s\n", print_si_no(risultato));
        return(risultato);
}


/******************************************************************************
******************************************************************************/
/*
 * Wait for a key
 */
void hit_any_key(void)
{
        char c;

	push_color();
	setcolor(C_HAK);
        cml_print(_("\r\\<<b>Premi un tasto</b>\\>"));
	pull_color();
        c = 0;
        while (c == 0)
                c = getchar();
        printf("\r                 \r");
}

int hit_but_char(char but)
{
        char c;

	push_color();
	setcolor(C_HAK);
        cml_printf(_("\r\\<<b>Premi un tasto per continuare, %c per smettere</b>\\>"),but);
	pull_color();
        c = 0;
        while (c == 0)
                c = getchar();
        printf("\r                                                           \r");
	return (c == but ? 1 : 0);
}

/*
 * Sleep for n microseconds
 */
void us_sleep(unsigned int n)
{
        struct timeval timeout;
        
        timeout.tv_sec = 0;
        timeout.tv_usec = n;
        select(0, NULL, NULL, NULL, &timeout);
}

/*
 * Cancella un carattere
 * DA IMPLEMENTARE COME MACRO
 */
void delchar(void)
{
        putchar(8);
        putchar(32);
        putchar(8);
}

/*
 * Cancella n caratteri
 * DA IMPLEMENTARE COME MACRO
 */
void delnchar(int n)
{
        int i;

        for(i = 0; i < n; i++)
                delchar();
}

/*
 * Stampa n spazi
 */
void putnspace(int n)
{
        int i;

        for(i = 0; i < n; i++)
                putchar(' ');
}

void cleanline(void)
{
        printf("\r%79s\r", "");
}


/* TODO comune con quella del server: unificare in share */
/* Trasforma gli spazi in underscore */
char *space2under(char *stringa)
{
    int i;
        
    for(i = 0; stringa[i] != '\0'; i++)
            if (stringa[i] == ' ') stringa[i] = '_';

    return stringa;
}


/*
 * Extracts the filename from a file path.
 * Returns the pointer to the starting character of the filename inside
 * 'path'. Additionally, if 'filename' is not NULL, it copies the filename
 * into it. 'size' is the size of the buffer 'filename'.
 */
char * find_filename(char *path, char *filename, size_t size)
{
        char *ptr;
        size_t pathlen, i;

	assert(size > MAXLEN_FILENAME);

        ptr = path;
        pathlen = strlen(path);
        for (i = 0; i < pathlen; i++) {
	        if (path[i] == '/') {
                        ptr = path + i + 1;
		}
        }
        if (filename) {
	        snprintf(filename, size, "%s", ptr);
	}

        return ptr;
}


/*
 * Extracts the file extension from a file path.
 * Returns the pointer to the starting character of the extension inside
 * 'path'. Additionally, if 'extension' is not NULL, it copies the extension
 * into it. 'size' is the size of the buffer 'extension'.
 */
char * find_extension(char *path, char *extension, size_t size)
{
        char *ptr;
        size_t pathlen, i;

	assert(size > MAXLEN_FILENAME);

        ptr = path;
        pathlen = strlen(path);
        for (i = 0; i < pathlen; i++) {
	        if (path[i] == '.') {
                        ptr = path + i + 1;
		} else if (path[i] == '/') {
		        ptr = path;
		}
        }
        if (extension) {
	        snprintf(extension, size, "%s", ptr);
	}
        return ptr;
}

/*****************************************************************************/
/*
 * Data in italiano
 */
int stampa_data (time_t ora)
{
        struct tm *tmst;

        tmst = localtime (&ora);
        return cml_printf ("<b>%d %s %d</b> alle <b>%2.2d</b>:<b>%2.2d</b>",
                           tmst->tm_mday, mese[tmst->tm_mon],
                           1900 + tmst->tm_year, tmst->tm_hour,
                           tmst->tm_min);
}

int stampa_data_breve(time_t ora)
{
        struct tm *tmst;

        tmst = localtime (&ora);
        return cml_printf ("<b>%d</b>/<b>%d</b>/<b>%d</b> alle <b>%2.2d</b>:<b>%2.2d</b>",
                           tmst->tm_mday, tmst->tm_mon,
                           1900 + tmst->tm_year, tmst->tm_hour,
                           tmst->tm_min);
}

void strdate(char *str, long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	sprintf(str, "%d %s %d alle %2.2d:%2.2d", tmst->tm_mday,
		mese[tmst->tm_mon], 1900+tmst->tm_year, tmst->tm_hour,
		tmst->tm_min);
}

/*
 * Data in italiano con mese lungo
 */
void stampa_datal(long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	printf("%d %s %d alle %2.2d:%2.2d", tmst->tm_mday, mese_esteso[tmst->tm_mon],
	       1900+tmst->tm_year, tmst->tm_hour, tmst->tm_min);
}

/*
 * Data in italiano con mese lungo
 */
void stampa_giorno(long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	cml_printf("<b>%d %s %d</b>", tmst->tm_mday, mese_esteso[tmst->tm_mon],
                   1900+tmst->tm_year);
}

/*
 * Data in italiano con mese e giorno della settimana lunghi
 */
void stampa_datall(long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	cml_printf("%s %d %s %d alle %2.2d:%2.2d",
		   settimana_esteso[tmst->tm_wday],
		   tmst->tm_mday, mese_esteso[tmst->tm_mon],
		   1900+tmst->tm_year, tmst->tm_hour, tmst->tm_min);
}

/*
 * Data in italiano con mese e giorno della settimana brevi
 */
void stampa_data_smb(long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	printf("%s %d %s %d alle %2.2d:%2.2d", settimana[tmst->tm_wday],
	       tmst->tm_mday, mese[tmst->tm_mon],
	       1900+tmst->tm_year, tmst->tm_hour, tmst->tm_min);
}

/*
 * Data in italiano con mese e giorno della settimana brevi
 */
void stampa_ora(long ora)
{
        struct tm *tmst;
	
	tmst = localtime(&ora);
	cml_printf("%s %d %s %d - %2.2d:%2.2d:%2.2d",
	       settimana_esteso[tmst->tm_wday], tmst->tm_mday,
	       mese_esteso[tmst->tm_mon], 1900+tmst->tm_year,
	       tmst->tm_hour, tmst->tm_min, tmst->tm_sec);
}

/****************************************************************************/
/*         USEFUL STRING FUNCTIONS                                          */
/****************************************************************************/
/* Creates dynamically (unconstrained allocation with malloc) a string
 * which is the concatenation of str1 and str2.
 * Remember to Free() the result!                                           */
char * astrcat(char *str1, char *str2)
{
	char *dup;

	CREATE(dup, char, strlen(str1) + strlen(str2) + 1, 0);
	strcat(strcpy(dup, str1), str2);

	return(dup);
}


int bisestile(int anno){
	if ((anno/4)*4!=anno){
		return (0);
	}
	if ((anno/400)*400==anno){
		return (1);
	}
	if ((anno/4)*4==anno && (anno/100)*100!=anno){
		return (1);
	}
	return(0);
}

/*  Se chiamata con pos=0 accetta ogni data 
 *  se chiamata con pos>0 accetta date successive a oggi 
 *  (con delta TIME_DELTA minimo di scarto) 
 *  se pos<0 accetta date precedenti.
 *   date comprese tutte tra
 *  "The Epoch" e quanto parole di 32 bits permettono
 *  inoltre dà dei valori di default
 *  pari a 
 *
 *  -- primo = 1 settimana
 *  -- giorno = +7 giorni
 *  -- ora = questa ora
 *  -- minuti = questi minuti
 *  -- secondi = questi secondi
 *  -- anno quest'anno(anno prossimo)
 */

int new_date(struct tm *data,int pos)
{
	char c;
	int giorno, month, anno, ora, minuto;
	time_t adesso,scelta,minimo,time_def;
	struct tm *oggi, *domani;
	int defm,defanno,defora;

	do {
                adesso = time(NULL);
                minimo = adesso+TEMPO_MINIMO;
                time_def= adesso+
                        pos*(DELTA_TIME+TEMPO_DEF+
                             ((long long int) TEMPO_RAND*rand()/(RAND_MAX+1.0)));
		c = 'n';
		if (pos>0)
			printf(_("La data dev'essere nel futuro (almeno il "));
		else if (pos<0)
			printf(_("La data dev'essere nel passato (al massimo il "));
                
		if (pos!=0){
                        stampa_data(minimo);
                        printf(")\n");
		}

		scelta=time_def;

                memcpy(data,gmtime(&scelta),sizeof(struct tm));

                oggi=gmtime(&adesso);
                time_def=adesso+5*TEMPO_MINIMO;
                domani=gmtime(&time_def);
		  
		if(pos!=0){
                        printf(_("Ti va bene %d %s %d alle %2.2d:%2.2d? (s/n) "),
                               data->tm_mday, mese[data->tm_mon], 
                               1900+data->tm_year, data->tm_hour, data->tm_min);
                        c = si_no();
                        if (c=='s'){
                                break;
                        }
		};

		month = -1;
		anno = -1;
		giorno = -1;
		while (anno < 0) {
			while (month < 0) {
				printf("\n");
				while((giorno < 1) || (giorno > 31))
					giorno = new_int_def(" Giorno: ",
                                                             domani->tm_mday);
				defm=oggi->tm_mon+1; /* +1 perche' gennaio=0!*/
                                
				if ((giorno-oggi->tm_mday)*pos<0){
                                        defm+=pos;
				}
				while((month < 1) || (month > 12))
					month = new_int_def(_(" Mese: "),
                                                            defm);
                                
				if (((month == 4) || (month == 6) 
				     || (month == 9) || (month == 11))
				    && (giorno>30)) {
					printf(_("Burlone: %s non ha %d giorni!\n"),
					       mese_esteso[month-1], giorno);
					month = -1;
					giorno = -1;
				}
				if (((month == 2) && (giorno > 29))) {
					printf(_("Burlone: febbraio non ha %d giorni!\n"),
					       giorno);
					month = -1;
					giorno = -1;
				}
			}
                        
			defanno=oggi->tm_year+1900;
			if ((pos*(month-(oggi->tm_mon)))<0)
					defanno+=pos;

			if((month==oggi->tm_mon)&& ((giorno-(oggi->tm_mday))*pos<0))
							defanno+=pos;

					while ((anno<1971) || (anno>2036))
					anno = new_int_def(_(" Anno (tra 1970 e 2036): "),defanno);
			if ((month == 2) && (giorno == 29)) {
				if (!bisestile(anno)) {
					cml_printf(_("Burlone: febbraio non ha 29 giorni,"
					   " %d non &egrave; bistestile!\n"), anno);
					anno = -1;
					month = -1;
					giorno = -1;
				}
			}
		}

		defora=oggi->tm_hour;
		if((giorno==oggi->tm_mday)&&(month==oggi->tm_mon)&&
						anno==oggi->tm_year){
				defora=oggi->tm_hour+5;
				if (defora>=24)
						defora=23;
		};
		ora = -1;
		while((ora < 0) || (ora > 23))
			ora = new_int_def(_(" Ora: "),defora);

		minuto = -1;
		while((minuto < 0) || (minuto > 59))
		minuto = new_int(_(" Minuti: "));

		data->tm_mday = giorno;
		data->tm_mon = month - 1;
		data->tm_year = anno - 1900;
		data->tm_hour =ora;
		data->tm_min = minuto;
		data->tm_sec = 0;
		data->tm_isdst = -1;
		scelta = mktime(data);
		printf(_("Hai scelto %d %s %d alle %2.2d:%2.2d, va bene? "),
		       data->tm_mday, mese[data->tm_mon], 
		       1900+data->tm_year, data->tm_hour, data->tm_min);
		c = si_no();
		if ((adesso-scelta)*pos>-TEMPO_MINIMO+100){
			printf(_("Data troppo vicina\n"));
		}
	}  while( (c != 's') || 
		  ( (pos == 0) || 
		    ( (adesso-scelta)*pos>-DELTA_TIME+100  || 
		 (adesso-scelta)*pos>-TEMPO_MINIMO+100)   
		)) ;
	return(0);
}

int min_lungh(char *str , int min) {
        int l = 0;
	char *s;

	for(s = str; *s; s++)
	        if (*s > 32 && *s < 127)
		        l++;
		
	return (l >= min);
}

