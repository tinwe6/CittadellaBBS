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
* Cittadella/UX Library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : terminale.c                                                        *
*        settaggio del terminale per eliminazione echo e altre vaccate      *
****************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include "terminale.h"

/* Prototipi delle funzioni in questo file */
void reset_term(void);
void term_mode(void);
void term_save(void);

/* Variabili globali */
struct termios save_attr;   /* Settaggi originali del terminale */
struct termios term_attr;   /* Settaggi attuali                 */

/******************************************************************************
******************************************************************************/

void reset_term(void)
{
        tcsetattr(STDIN_FILENO, TCSANOW, &save_attr);
}

void term_mode(void)
{
        /* Make sure that stdin is a terminal */
        if (!isatty(STDIN_FILENO)) {
                fprintf(stderr, "Non e' un terminale.\n");
                exit(EXIT_FAILURE);
        }

        tcgetattr(STDIN_FILENO, &term_attr);
        /* Clear IXON to disable start/stop control on output, so that
           Ctrl-S / Ctrl-Q do not start/stop the terminal. */
        term_attr.c_iflag &= ~(IXON);
        /* Disable IEXTEN so that we can use Ctrl-V for PageDown */
        term_attr.c_lflag &= ~(ICANON|ECHO|IEXTEN);
        term_attr.c_cc[VMIN] = 1;
        term_attr.c_cc[VTIME] = 0;
        term_attr.c_cc[VLNEXT] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attr);
}

void term_save(void)
{
        tcgetattr(STDIN_FILENO, &save_attr);
        atexit(reset_term);
}
