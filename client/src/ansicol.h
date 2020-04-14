/*
 *  Copyright (C) 1999-2002 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : ansicol.h                                                          *
*        Colori ANSI                                                        *
****************************************************************************/
#ifndef _ANSICOL_H
#define _ANSICOL_H   1

/* Terminal modes */
#define CMODE_PLAIN 0
#define CMODE_BOLD  1
#define CMODE_ANSI  2

/* Codifica colore carattere */
#define COLOR(FG, BG, AT) (((AT) << 8) + ((BG) << 4) + (FG))
#define CC_FG(CODE)     ((CODE) & 0x0f)
#define CC_BG(CODE)     (((CODE) >> 4) & 0x0f)
#define CC_ATTR(CODE)   (((CODE) >> 8) & 0xff)

/* Definizione Colori */

/* Colori normali ANSI, non grassetto in modalita' bold */
#define COL_BLACK         0x0
#define COL_RED           0x1
#define COL_GREEN         0x2
#define COL_YELLOW        0x3      /* visualizzato come marrone se non bold */
#define COL_BLUE          0x4
#define COL_MAGENTA       0x5
#define COL_CYAN          0x6
#define COL_GRAY          0x7
/* #define COL_DEFAULT       0x9*/ /* Colore di default - Unused */
#define COL_NOCHANGE      0xf

/* Default background color */

/* Background nero */
#ifndef WHITEBG
#define COL_DEFFG    COL_GRAY
#define COL_DEFBG    COL_BLACK
#else
/* Per background bianco (LK, it's for you!) */
#define COL_DEFFG    COL_BLACK
#define COL_DEFBG    COL_GRAY
#endif

/* Attribute bits */
#define ATTR_DEFAULT          0x00
#define ATTR_MASK           ((1<<5)-1)

#define ATTR_BOLD            (1<<0) /* Questi 5 si possono combinare fra */
#define ATTR_HALF_BRIGHT     (1<<1) /* di loro con l'OR.                 */
#define ATTR_UNDERSCORE      (1<<2)
#define ATTR_BLINK           (1<<3)
#define ATTR_REVERSE         (1<<4)

#define ATTR_NOCHANGE        (1<<5)
#define ATTR_RESET            0x00

#if 0
/* Color Attributes */
#define CA_OFF                          (1 << 4)         /* 'not' bit */
#define CA_MASK                               7

#define CA_RESET                              0
#define CA_BOLD                               1
#define CA_HALF_BRIGHT                        2
#define CA_UNDERSCORE                         4
#define CA_BLINK                              5
#define CA_REVERSE                            7

#define CA_NOCHANGE                 (6 & CA_OFF)

#define CA_NORMAL_INT  (CA_HALF_BRIGHT & CA_OFF)
#define CA_BOLD_OFF               CA_NORMAL_INT
#define CA_UNDERLINE_OFF (CA_UNDERLINE & CA_OFF)
#define CA_BLINK_OFF         (CA_BLINK & CA_OFF)
#define CA_REVERSE_OFF     (CA_REVERSE & CA_OFF)
#endif

/* Colori ANSI normali */
#define BLACK       COLOR(  COL_BLACK, COL_DEFBG, ATTR_RESET)
#define RED         COLOR(    COL_RED, COL_DEFBG, ATTR_RESET)
#define GREEN       COLOR(  COL_GREEN, COL_DEFBG, ATTR_RESET)
#define YELLOW      COLOR( COL_YELLOW, COL_DEFBG, ATTR_RESET)
#define BLUE        COLOR(   COL_BLUE, COL_DEFBG, ATTR_RESET)
#define MAGENTA     COLOR(COL_MAGENTA, COL_DEFBG, ATTR_RESET)
#define CYAN        COLOR(   COL_CYAN, COL_DEFBG, ATTR_RESET)
#define GRAY        COLOR(   COL_GRAY, COL_DEFBG, ATTR_RESET)

/* Colori ANSI chiari (o bold) */
#define L_BLACK     COLOR(  COL_BLACK, COL_DEFBG, ATTR_BOLD)
#define L_RED       COLOR(    COL_RED, COL_DEFBG, ATTR_BOLD)
#define L_GREEN     COLOR(  COL_GREEN, COL_DEFBG, ATTR_BOLD)
#define L_YELLOW    COLOR( COL_YELLOW, COL_DEFBG, ATTR_BOLD)
#define L_BLUE      COLOR(   COL_BLUE, COL_DEFBG, ATTR_BOLD)
#define L_MAGENTA   COLOR(COL_MAGENTA, COL_DEFBG, ATTR_BOLD)
#define L_CYAN      COLOR(   COL_CYAN, COL_DEFBG, ATTR_BOLD)
#define WHITE       COLOR(   COL_GRAY, COL_DEFBG, ATTR_BOLD)

/* Colori ANSI normali - Non modifica BG */
#define FG_BLACK       COLOR(  COL_BLACK, COL_NOCHANGE, ATTR_RESET)
#define FG_RED         COLOR(    COL_RED, COL_NOCHANGE, ATTR_RESET)
#define FG_GREEN       COLOR(  COL_GREEN, COL_NOCHANGE, ATTR_RESET)
#define FG_YELLOW      COLOR( COL_YELLOW, COL_NOCHANGE, ATTR_RESET)
#define FG_BLUE        COLOR(   COL_BLUE, COL_NOCHANGE, ATTR_RESET)
#define FG_MAGENTA     COLOR(COL_MAGENTA, COL_NOCHANGE, ATTR_RESET)
#define FG_CYAN        COLOR(   COL_CYAN, COL_NOCHANGE, ATTR_RESET)
#define FG_GRAY        COLOR(   COL_GRAY, COL_NOCHANGE, ATTR_RESET)

/* Colori ANSI chiari (o bold) - Non modifica BG */
#define FG_L_BLACK     COLOR(  COL_BLACK, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_RED       COLOR(    COL_RED, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_GREEN     COLOR(  COL_GREEN, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_YELLOW    COLOR( COL_YELLOW, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_BLUE      COLOR(   COL_BLUE, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_MAGENTA   COLOR(COL_MAGENTA, COL_NOCHANGE, ATTR_BOLD)
#define FG_L_CYAN      COLOR(   COL_CYAN, COL_NOCHANGE, ATTR_BOLD)
#define FG_WHITE       COLOR(   COL_GRAY, COL_NOCHANGE, ATTR_BOLD)

/* Carica la definizione dei colori della bbs */
/* #define C_DEFAULT       COLOR(COL_DEFAULT, COL_DEFAULT, ATTR_RESET) */
#define C_DEFAULT       COLOR(COL_DEFFG, COL_DEFBG, ATTR_RESET)
#define C_BOLD          COLOR(COL_DEFFG, COL_DEFBG, ATTR_BOLD)
#define C_RES_BG_AT     COLOR(COL_NOCHANGE, COL_DEFBG, ATTR_NOCHANGE)
#include "colors.h"

/* Variabili globali */
extern int  ansicol_mode;
extern const char *hbold;
extern const char *hstop;

/* Prototipi delle funzioni in ansicol.c */
void ansi_init(void);
void color_mode(int mode);
int getcolor(void);
void putcolor(int col);
void setcolor(int col);
void color2cml(char *str, int col);
void push_color(void);
void pull_color(void);
void ansi_reset(void);
void ansi_print(void);

#define ansiset_fg(col) setcolor(COLOR((col), COL_NOCHANGE, ATTR_NOCHANGE))
#define ansiset_bg(col) setcolor(COLOR(COL_NOCHANGE, (col), ATTR_NOCHANGE))
#define ansiset_attr(at) setcolor(COLOR(COL_NOCHANGE, COL_NOCHANGE, (at)))

/* macro definitions */
#define HAS_ANSI (ansicol_mode == CMODE_ANSI)
#define HAS_BOLD (ansicol_mode >= CMODE_BOLD)

/***************************************************************************/

#endif /* ansicol.h */
