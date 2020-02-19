/*
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX administration             (C) M. Caldarelli and R. Vianello  *
*                                                                             *
* File: termioc.c                                                             *
*       Funzioni I/O terminale per le utiliti di amministrazione              *
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "utility.h"

#define MAX_LINEE         5
#define LBUF            256
#define NUM_DIGIT 20 /* Max num cifre nei numeri presi da tastiera */
#define SIS "Si"
#define NOS "No"

/* Prototipi delle funzioni in questo file */
void get_line(char *str, int max, char maiuscole, char special);
int gettext(char (*txt)[80], char max);
void new_str_M(char *prompt, char *str, int max);
void new_str_m(char *prompt, char *str, int max);
void new_str_def_M(char *prompt, char *def, char *str, int max);
void new_str_def_m(char *prompt, char *def, char *str, int max);
void get_number(char *str, char neg);
int get_int(void);
long get_long(void);
int new_int(char *prompt);
long new_long(char *prompt);
int new_int_def(char *prompt, int def);
int new_sint_def(char *prompt, int def);
long new_long_def(char *prompt, long def);
int inkey_sc(int m);
void hit_any_key(void);
char * print_si_no(int a);
char si_no(void);
int new_si_no_def(char *prompt, int def);
void us_sleep(unsigned int n);
void delchar(void);
void delnchar(char n);

/******************************************************************************
******************************************************************************/
/*
 *  Prende una stringa dallo stdinput, lunga |max|, se max<0 visualizza
 *  asterischi al posto delle lettere, se special = 0 non accetta
 *  i caratteri speciali utilizzati dalla connessione server/client
 *  (per ora solo '|'). Se maiuscole!=0 il primo carattere di ogni parola
 *  e' automaticamente convertito in maiuscola.
 */
void get_line(char *str, int max, char maiuscole, char special)
{
        int a, b;
        int flag = 0;

        if (max == 0)
                return;

        if (max <0) {
                flag = 1;
                max = (0 - max);
        }

        do { 
                a = 0;
                a = inkey_sc(0);
                if ((isascii(a) && isprint(a) && a!='|') || (a == 8)
                    || (special && a=='|')) {
                        b = strlen(str);
                        if ((a == 8) && (b != 0)) {
                                delchar();
                                str[b - 1] = '\0';
                        }
                        if ((a != 8) && (b < max)) {
                                if (maiuscole && ((b==0)||(str[b - 1]==' '))) 
                                        if (islower(a))
                                                a = toupper(a);
                                if (!flag)
                                        putchar(a);
                                else
                                        putchar('*');
                                str[b] = a;
                                str[b + 1] = '\0';
                        }
                }
        } while ((a != 13) && (a != 10));
        
        putchar('\n');
}

int gettext(char (*txt)[80], char max)
{
        int a, b = 1;
        int riga = 0;
        int pos, maxchar = 79;
        char str[80];
        char parola[80];
        int ch;
  
        if (max>maxchar)
                max = maxchar;
  
        strcpy(str, "");
  
        do {
                printf(">%s", str);
                pos = strlen(str);
                do {
                        ch = 0;
                        ch = inkey_sc(0);
                        if ((isascii(ch) && isprint(ch)) || (ch == 8)) {
                                b = strlen(str);
                                if ((ch == 8) && (b != 0)) {
                                        delchar();
                                        str[b - 1] = '\0';
                                        pos--;
                                } 
                                if (ch != 8) {
                                        putchar(ch);
                                        str[b] = ch;
                                        str[b + 1] = '\0';
                                        pos++;
                                }
                        }
                } while (((ch != 10) && (ch != 13)) && (pos < (max - 1)));

                strcpy(parola, "");
                
                if (pos == (max - 1)) {
                        do {
                                pos--;
                        } while ((str[pos] != ' ') && (pos >= 0));
                        if (pos >= 0) {
                                strcpy(parola, &str[pos + 1]);
                                str[pos + 1] = '\0';
                                delnchar(strlen(parola));
                        }
                }
    
                strcpy(txt[riga], str);
                a = strlen(txt[riga]);
                printf("\n");
                strcpy(str, parola);
                riga++;
        } while ((a > 0) && (riga < (MAX_LINEE - 1)));
  
        if ((a == 0) && (riga == 1)) b = 0; /* testo nullo */
        else if (a > 0) {
                printf(">%s", str);
                pos = strlen(str);
                do {
                        ch = 0;
                        ch = inkey_sc(0);
                        if ((isascii(ch) && isprint(ch)) || (ch == 8)) {
                                b = strlen(str);
                                if ((ch == 8) && (b != 0)) {
                                        delchar();
                                        str[b - 1] = '\0';
                                        pos--;
                                } 
                                if ((ch != 8) && (pos < (max - 1))) {
                                        putchar(ch);
                                        str[b] = ch;
                                        str[b + 1] = '\0';
                                        pos++;
                                }
                        }
                } while ((ch != 10) && (ch != 13));
                strcpy(txt[riga], str);
                printf("\n");
                riga++;
        }    
        return(b);
}

void new_str_M(char *prompt, char *str, int max)
{
        strcpy(str, "");
        printf("%s", prompt);
        get_line(str, max, 1, 0);
}

void new_str_m(char *prompt, char *str, int max)
{
        strcpy(str, "");
        printf("%s", prompt);
        get_line(str, max, 0, 0);
}

void new_str_def_M(char *prompt, char *def, char *str, int max)
{
        char buf[LBUF];
  
        strcpy(buf,def);
        strcpy(str, "");
        printf("%s [%s]: ", prompt, buf);
        get_line(str, max, 1, 0);
        if (str[0]==0)
                strcpy(str,buf);
}

void new_str_def_m(char *prompt, char *def, char *str, int max)
{
        char buf[LBUF];
  
        strcpy(buf,def);
        strcpy(str, "");
        printf("%s [%s]: ", prompt, buf);
        get_line(str, max, 0, 0);
        if (str[0]==0)
                strcpy(str,buf);
}

/*************************************************************************/

/*
 * Prompt da stdin di un numero intero, positivo se neg = 0, accetta '-'
 * come primo carattere se neg = 1.
 */ 
void get_number(char *str, char neg)
{
        int a, b;
        int flag = 0;
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
        
        putchar('\n');
}

int get_int(void)
{
        char str[NUM_DIGIT]="";
  
        get_number(str,0);
        return atoi(str);
}

long get_long(void)
{
        char str[NUM_DIGIT]="";
  
        get_number(str,0);
        return atol(str);
}

int new_int(char *prompt)
{
        printf("%s", prompt);
        return(get_int());
}

long new_long(char *prompt)
{
        printf("%s", prompt);
        return(get_long());
}

int new_int_def(char *prompt, int def)
{
        char str[NUM_DIGIT]="";
        
        printf("%s [%d]: ", prompt, def);
        get_number(str,0);
        if (str[0]==0)
                return(def);
        else
                return atoi(str);
}

/* Chiede un numero intero _relativo_ con prompt e valore di default */
int new_sint_def(char *prompt, int def)
{
        char str[NUM_DIGIT]="";
  
        printf("%s [%d]: ", prompt, def);
        get_number(str, 1);
        if (str[0] == 0)
                return(def);
        else
                return atoi(str);
}

long new_long_def(char *prompt, long def)
{
        char str[NUM_DIGIT]="";
        
        printf("%s [%ld]: ", prompt, def);
        get_number(str,0);
        if (str[0]==0)
                return(def);
        else
                return atol(str);
}

/*****************************************************************************/

char * print_si_no(int a)
{
  if (a==0) return (NOS);
  else return (SIS);
}

char si_no(void)
{
        char c;

        c=0;
        while((c!='s')&&(c!='S')&&(c!='n')&&(c!='N'))
                c=inkey_sc(0);
        if ((c=='S')||(c=='s')) {
                printf("%s\n",print_si_no(1));
                return('s');
        } else {
                printf("%s\n",print_si_no(0));
                return('n');
        }
}

int new_si_no_def(char *prompt, int def)
{
        char c;
        int risultato;  

        printf("%s (s/n) [%s] : ",prompt,print_si_no(def));
        c=0;
        while((c!='s')&&(c!='S')&&(c!='n')&&(c!='N')&&(c!=10)&&(c!=13))
                c=inkey_sc(0);
        if ((c=='S')||(c=='s')) 
                risultato = 1;
        else if((c=='N')||(c=='n'))
                risultato = 0;
        else
                risultato = def;
        printf("%s\n", print_si_no(risultato));
        return(risultato);
}


/******************************************************************************
******************************************************************************/

int inkey_sc(int esegue)
{
        int a = 0;
        int b;
        char buf[LBUF];
        fd_set input_set;
        struct timeval tv;

        fflush(stdout);

        do {
                do {
                        FD_ZERO(&input_set);
                        FD_SET(0, &input_set);         /* Input dall'utente */
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
                } while (!(b = select(1, &input_set, NULL, NULL, &tv))); 
        
                if (b < 0) {
                        Perror("Select");
                        exit(1);
                }
    
                if (FD_ISSET(0, &input_set)) {
                        read(0, buf, 1);
                        a = (unsigned int)buf[0];
                }
                
        } while (a==0);
  
        return(a);
}

/*
 * Wait for a key
 */
void hit_any_key(void)
{
        char c;
  
        printf("\r<Premi un tasto>");
        c=0;
        while (c==0)
                c=getchar();
        /*  while (c==0) c=inkey_sc(0); */
        printf("\r                 \r");
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
void delnchar(char n)
{
        char i;

        for(i=0; i<n; i++)
                delchar();
}
