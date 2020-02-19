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
 * Cittadella/UX administration           (C) M. Caldarelli and R. Vianello *
 *                                                                          *
 * File:  fm_admin.c                                                        *
 *        Programma di prova per le funzioni in file_messaggi.c             *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "strutture.h"
#include "file_messaggi.h"
#include "terminale.h"

#define MAX_LINEE          5
#define LBUF             256

/* Prototipi funzioni in questo file */
void print_menu(void);
void print_fmdata(void);
void crea_file_messaggi(void);
void cancella_fm(void);
void leggi_msg(void);
void scrivi_msg(void);
void print_tmpfile(void);
void print_text(struct text *txt);
void visualizza_headers(void);
void cancella_msg(void);
void ridimensiona_fm(void);

/* Prototipi altre funzioni esterne */
void new_str_m(char *prompt, char *str, int max);
long new_long(char *prompt);
void get_line(char *str, int max, char maiuscole, char special);
int gettext(char (*txt)[80], char max);

/* Variabili globali */
char *filename = "./tmp/fm_admin.tmp";
struct dati_server dati_server;

/****************************************************************************
****************************************************************************/

void print_menu(void)
{
        printf("\n(1) Vedi File Messaggi Presenti.\n");
        printf("(2) Crea un nuovo file messaggi.\n");
        printf("(3) Cancella un file messaggi.\n");
        printf("(4) Leggi da un file messaggi.\n");
        printf("(5) Scrivi su un file messaggi.\n");
        printf("(6) Visualizza headers dei messaggi in un fm.\n");
        printf("(7) Cancella un messaggio da un file messaggi.\n");
        printf("(8) Ridimensiona un file dei messaggi.\n");
        printf("(9) Esci.\n");
}

void print_fmdata(void)
{
      struct fm_list *punto;

      if (lista_fm == NULL) {
              printf("Non sono presenti file messaggi.\n\n");
              return;
      }

      for (punto = lista_fm; punto; punto = punto->next) {
              printf("File Messaggi n.%ld.\n", (punto->fm).num);
              printf("     Size      : %ld\n", (punto->fm).size);
              printf("     Lowest    : %ld\n", (punto->fm).lowest);
              printf("     Highest   : %ld\n", (punto->fm).highest);
              printf("     Posizione : %ld\n", (punto->fm).posizione);
              printf("     Flags     : %ld\n", (punto->fm).flags);
              printf("     Descr.    : %s\n\n", (punto->fm).desc);
      }
}

void crea_file_messaggi(void)
{
        long size, fmnum;
	char desc[LBUF];
	
        printf("Crea un nuovo file messaggi.\n\n");
        size = new_long("Dimensioni del nuovo file messaggi: ");
        new_str_m("Descrizione file messaggi: ", desc, 63);
	if ((fmnum = fm_create(size, desc)))
                printf("\nOk! Il file messaggi n.%ld e' stato creato con successo.\n\n", fmnum);
        else
                printf("Errore nella creazione del nuovo file messaggi!!\n\n");
}

void cancella_fm(void)
{
        long fmnum;

        printf("Cancella un file dei messaggi.\n\n");
        fmnum = new_long("Numero file messaggi: ");
        if (fm_delete(fmnum))
                printf("Ok, file messaggi cancellato.\n");
        else
                printf("File messaggi inesistente.\n");
}

void leggi_msg(void)
{
        long fmnum, msgnum, msgpos;
        char header[LBUF], ret;
	struct text *txt;
	
        printf("Leggi un post dal file dei messaggi.\n\n");
        fmnum = new_long("Numero file messaggi   : ");
        msgnum = new_long("Numero del messaggio   : ");
        msgpos = new_long("Posizione del messaggio: ");
        txt = txt_create();
	if ((ret=fm_get(fmnum, msgnum, msgpos, txt, header))) {
                printf("Errore n.%d nella lettura del messaggio.\n", ret);
                return;
        }
        printf("\nHeader:\n%s\nMessage:\n", header);
        print_text(txt);
	txt_free(&txt);
}

void scrivi_msg(void)
{
        char header[LBUF];
        char txt[MAX_LINEE][80];
        long fmnum;
        int i;
        FILE *fp;

        for (i = 0; i< MAX_LINEE; i++)
                *txt[i] = 0;
        printf("Salva messaggio su un file messaggi.\n\n");
        fmnum = new_long("Numero file messaggi: ");
        printf("Inserisci l'header del messaggio:\n");
        *header = 0;
        get_line(header, 255, 0, 0);
        printf("Inserisci il messaggio:\n");
        gettext(txt, 79);
        fp = fopen(filename, "w");
        for (i=0; i<MAX_LINEE; i++)
                if (txt[i][0])
                        fprintf(fp, "%s\n", txt[i]);
        fclose(fp);
/*        fm_putf(fmnum, filename, header); */
}

void print_tmpfile(void)
{
        char buf[LBUF];
        FILE *fp;

        fp = fopen(filename, "r");
        if (fp == NULL)
                return;
        while (fgets(buf, LBUF, fp) != NULL)
                printf("%s", buf);
        fclose(fp);
}

void print_text(struct text *txt)
{
	char *riga;
	
	if (txt == NULL) {
		printf("txt == NULL --> abort print.\n");
		return;
	}
	txt_rewind(txt);
	riga = txt_get(txt);
	while (riga != NULL) {
                printf("%s", riga);
		riga = txt_get(txt);
	}
}

void visualizza_headers(void)
{
        long fmnum;
        char ret;
	struct text *txt;
	
        printf("Visualizza gli header dei messaggi presenti.\n\n");
	txt = txt_create();
        fmnum = new_long("Numero file messaggi: ");
        ret = fm_headers(fmnum, txt);
        if (ret==0)
                print_text(txt);
	else
                printf("Errore n.%d da chiamata a fm_headers().\n", ret);
	txt_free(&txt);
}

void cancella_msg(void)
{
        long fmnum, msgnum, msgpos;
        char ret;

        printf("Cancella un post dal file dei messaggi.\n\n");
        fmnum = new_long("Numero file messaggi   : ");
        msgnum = new_long("Numero del messaggio   : ");
        msgpos = new_long("Posizione del messaggio: ");
        if ((ret = fm_delmsg(fmnum, msgnum, msgpos)))
                printf("Errore n.%d da chiamata a fm_delmsg().\n",ret);
        else
                printf("Messaggio cancellato.\n");
}

void ridimensiona_fm(void)
{
        long fmnum, newsize;
        char ret;

        printf("Ridimensiona un file dei messaggi.\n\n");
        fmnum = new_long("Numero file messaggi: ");
        newsize = new_long("Nuova dimensione    : ");
        if ((ret = fm_resize(fmnum, newsize)))
                printf("Errore n.%d da chiamata a fm_resize().\n",ret);
        else
                printf("Ok, file messaggi ridimensionato.\n");
}

/***************************************************************************/

int main(void)
{
        char ch;

        term_save();
        term_mode(1);
        printf("\n\n   Cittadella/UX  -  File Message Administration Tool.\n");
        print_menu();
        dati_server.fm_num = 2;
        dati_server.fm_curr = 2;

        fm_load_data();
        while (1) {
                printf("\nScelta (? per menu) : ");
                ch = 0;
                while (!isdigit(ch) && !(ch == '?'))
                        ch = getchar();
                printf("%c\n", ch);
                switch (ch) {
                case '1':
                        print_fmdata();
                        break;
                case '2':
                        crea_file_messaggi();
                        break;
                case '3':
                        cancella_fm();
                        break;
                case '4':
                        leggi_msg();
                        break;
                case '5':
                        scrivi_msg();
                        break;
                case '6':
                        visualizza_headers();
                        break;
                case '7':
                        cancella_msg();
                        break;
                case '8':
                        ridimensiona_fm();
                        break;
                case '9':
                        fm_save_data();
                        printf("\nCiao...\n\n");
                        reset_term();
                        exit(0);
                        break;
                case '?':
                        print_menu();
                        break;
               }                
        }
        return 0;
}
