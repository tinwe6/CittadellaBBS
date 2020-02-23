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
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : histo.c                                                            *
*        Funzioni per tracciare istogrammi in ascii.                        *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "histo.h"
#include "macro.h"

#define LBUF 256

/*
 * Visualizza l'istogramma dei dati forniti nel vettore data in ascii.
 * all = 1 --> visualizza fino al 100%, 0 --> visulizza fino al massimo
 * nrow = # righe disponibili per visualizzare l'istogramma
 * nlabel = # di etichette sull'asse delle ordinate
 * xtick = vettore con i label per ogni tick dell'asse delle ascisse.
 *         Ogni label deve essere al piu' lungo 3 caratteri.
 */
void print_histo(long * data, int num, int nrow, int nlabel, char all,
                 char * xlab, char * xtick[])
{
	const char vuoto[]    = "    ";
	const char unterzo[]  = " .  ";
	const char dueterzi[] = " +  ";
	const char pieno[]    = " #  ";
	const char ascisse[]  = "-+--";
        int i, j, labstep;
        long sum = 0;
        double *nvec, min, min1, min2, max, dmax = 0;
        char str[LBUF], ax[LBUF];

	if ((nlabel == 0) || (nrow == 0)) {
		printf("*** Error: non posso tracciare l'istogramma.\n");
		return;
	}
        CREATE(nvec, double, num, 0);
        for (i = 0; i < num; i++) {
                sum += data[i];
                if (data[i] > dmax)
                        dmax = data[i];
        }
	if (sum == 0) {
		printf("*** Vettore dati per l'istogramma nullo.\n");
		return;
	}
        printf(" %%  ^\n    |\n");

        for (i = 0; i < num; i++)
                nvec[i] = (double)data[i]/sum;

        labstep = nrow / nlabel;
        for (i = 0; i < nrow; i++) {
                strcpy(str, "");
                if (all) {
                        min  = 1 - (double)(i+1)/nrow;
                        min1 = 1 - (double)(i+2/3)/nrow;
                        min2 = 1 - (double)(i+1/3)/nrow;
                        max  = 1 - (double)i/nrow;
                } else {
                        min  = (double)dmax/sum * (1 - (double)(i+1)/nrow);
                        min1 = (double)dmax/sum * (1 - (double)(i+.66)/nrow);
                        min2 = (double)dmax/sum * (1 - (double)(i+.33)/nrow);
                        max  = (double)dmax/sum * (1 - (double)i/nrow);
                }
                if (i % labstep == 0)
                        sprintf(ax, "%3d + ", (int)(max*100));
                else
                        sprintf(ax, "    | ");
                for (j = 0; j < num; j++)
			if (nvec[j] >= min2)
                                strcat(str, pieno);
		        else if (nvec[j] >= min1)
                                strcat(str, dueterzi);
		        else if (nvec[j] > min)
                                strcat(str, unterzo);
                        else
                                strcat(str, vuoto);
                printf("%s%s\n", ax, str);
        }
        strcpy(str, "  0 |-");
        for (j = 0; j < num; j++)
                strcat(str, ascisse);

        printf("%s-> %s\n", str, xlab);
        strcpy(str, "      ");
        for (j = 0; j < num; j++) {
                strcat(str, xtick[j]);
                strcat(str, " ");
        }
        printf("%s\n", str);

        Free(nvec);
}

void print_histo_num(long * data, int num, int nrow, int nlabel, char all,
                 char * xlab)
{
        int i, j, labstep;
        long sum = 0;
        double *nvec, min, min1, min2, max, dmax = 0;
        char str[LBUF], str1[10], ax[LBUF];
        const char vuoto[]    = "   ";
        const char unterzo[]  = " . ";
        const char dueterzi[] = " + ";
        const char pieno[]    = " # ";
        const char ascisse[]  = "-+-";

	if ((nlabel == 0) || (nrow == 0)) {
		printf("\n*** Error: non posso tracciare l'istogramma.\n");
		return;
	}
        CREATE(nvec, double, num, 0);
        for (i = 0; i < num; i++) {
                sum += data[i];
                if (data[i] > dmax)
                        dmax = data[i];
        }

	if (sum == 0) {
		printf("\n*** Vettore dati per l'istogramma nullo.\n");
		return;
	}
	printf(" %%  ^\n    |\n");

        for (i = 0; i < num; i++)
                nvec[i] = (double)data[i]/sum;

        labstep = nrow / nlabel;
        for (i = 0; i < nrow; i++) {
                strcpy(str, "");
                if (all) {
                        min  = 1 - (double)(i+1)/nrow;
                        min1 = 1 - (double)(i+2/3)/nrow;
                        min2 = 1 - (double)(i+1/3)/nrow;
                        max  = 1 - (double)i/nrow;
                } else {
                        min  = (double)dmax/sum * (1 - (double)(i+1)/nrow);
                        min1 = (double)dmax/sum * (1 - (double)(i+.66)/nrow);
                        min2 = (double)dmax/sum * (1 - (double)(i+.33)/nrow);
                        max  = (double)dmax/sum * (1 - (double)i/nrow);
                }
		if (i % labstep == 0)
                        sprintf(ax, "%3d + ", (int)(max*100));
                else
                        sprintf(ax, "    | ");
                for (j = 0; j < num; j++)
			if (nvec[j] >= min2)
                                strcat(str, pieno);
		        else if (nvec[j] >= min1)
                                strcat(str, dueterzi);
		        else if (nvec[j] > min)
                                strcat(str, unterzo);
                        else
                                strcat(str, vuoto);
		printf("%s%s\n", ax, str);
         }
        strcpy(str, "  0 |-");
        for (j = 0; j < num; j++)
                strcat(str, ascisse);

        printf("%s> %s\n", str, xlab);
        strcpy(str, "      ");
        for (j = 0; j < num; j++) {
                sprintf(str1, "%2d ", j);
                strcat(str, str1);
        }
        printf("%s\n", str);

        Free(nvec);
}

/* Programma di esempio

int main(void)
{
        long dati[] = {123, 23, 453, 233, 234, 345, 198, 45, 0, 100, 400, 255};
        char * mesi[] = {"Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug",
                       "Ago", "Set", "Ott", "Nov", "Dic"};
        long ore[] = {123, 23, 453, 233, 234, 345, 198, 45, 0, 100, 400,
                       255, 123, 23, 453, 233, 234, 345, 198, 45, 0, 100, 400,
                       255};
        print_histo(dati, 12, 20, 5, 0, "Mese", mesi);
        printf("\n");
        print_histo(dati, 12, 20, 5, 1, "Mese", mesi);
        print_histo_num(ore, 24, 20, 5, 0, "Ora");
        print_histo_num(ore, 24, 20, 5, 1, "Ora");
        return 0;
}

*/
