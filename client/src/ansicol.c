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
* File : ansicol.c                                                          *
*        Colori ANSI                                                        *
****************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ansicol.h"
#include "cittacfg.h"
#include "macro.h"

/* Variabili globali */
int ansicol_mode;

const char *hbold;
const char *hstop;
const char hbold_def[] = "\x1b[1m";
const char hstop_def[] = "\x1b[22m";
const char hbgdfl[]    = "\x1b[49m";
const char hfgdfl[]    = "\x1b[39m";
const char hreset[]    = "\x1b[0;38;49m";
const char hcoldfl[]   = "\x1b[38;49m";
const char hempty[]    = "";

struct col_stack {
	int fg, bg, at;
	struct col_stack *next;
};

static struct col_stack ansicol_curr;/* Variabile globale con colore corrente*/
static struct col_stack * ansicol_stack;   /* memoria colore                 */

/* Prototipi delle funzioni statiche in questo file */
static void ansi_update_color(int fg, int bg, int at);

/*****************************************************************************/
/*
  Implements ECMA-48 Set Graphics Rendition

       The ECMA-48 SGR sequence ESC [ <parameters> m sets display
       attributes.  Several attributes can be  set  in  the  same
       sequence.

       par   result
       0     reset all attributes to their defaults
       1     set bold
       2     set half-bright (simulated with color on a color display)
       4     set underscore (simulated with color on a color display)
             (the colors used to simulate dim or underline are set
             using ESC ] ...)
       5     set blink
       7     set reverse video
       10    reset selected mapping, display control flag,
             and toggle meta flag.
       11    select null mapping, set display control flag,
             reset toggle meta flag.
       12    select null mapping, set display control flag,
             set toggle meta flag. (The toggle meta flag
             causes the high bit of a byte to be toggled
             before the mapping table translation is done.)
       21    set normal intensity (this is not compatible with ECMA-48)
       22    set normal intensity
       24    underline off
       25    blink off
       27    reverse video off
       30    set black foreground
       31    set red foreground
       32    set green foreground
       33    set brown foreground
       34    set blue foreground
       35    set magenta foreground
       36    set cyan foreground
       37    set white foreground
       38    set underscore on, set default foreground color
       39    set underscore off, set default foreground color
       40    set black background
       41    set red background
       42    set green background
       43    set brown background
       44    set blue background
       45    set magenta background
       46    set cyan background
       47    set white background
       49    set default background color
*/
/*****************************************************************************/
void ansi_init(void)
{
	ansicol_mode = CMODE_PLAIN;
	ansicol_stack = NULL;
	ansicol_curr.fg = COL_GRAY;
	ansicol_curr.bg = COL_BLACK;
	ansicol_curr.at = ATTR_DEFAULT;
	ansicol_curr.next = NULL;

	if (USE_COLORS)
		color_mode(CMODE_ANSI);
	else
		color_mode(CMODE_PLAIN);
	setcolor(C_NORMAL);
}

/* Restituisce il colore corrente del cursore. */
int getcolor(void)
{
	return COLOR(ansicol_curr.fg, ansicol_curr.bg, ansicol_curr.at);
}

/* Ridefinisce il colore corrente del cursore. */
void putcolor(int col)
{
	if (!USE_COLORS)
		return;

	ansicol_curr.fg = CC_FG(col);
	ansicol_curr.bg = CC_BG(col);
	ansicol_curr.at = CC_ATTR(col);
}

void setcolor(int col)
{
	int fg, bg, at;

	if (!USE_COLORS)
		return;

	fg = CC_FG(col);
	if (fg == COL_NOCHANGE)
		fg = ansicol_curr.fg;

	bg = CC_BG(col);
	if (bg == COL_NOCHANGE)
		bg = ansicol_curr.bg;

	at = CC_ATTR(col);
        if (at & ATTR_NOCHANGE)
                at = ansicol_curr.at;

	if ((fg == ansicol_curr.fg) && (bg == ansicol_curr.bg)
	    && (at == ansicol_curr.at))
		return;

	ansi_update_color(fg, bg, at);
}

void color2cml(char *str, int col)
{
	int fg, bg, at;

	fg = CC_FG(col);
	bg = CC_BG(col);
	at = CC_ATTR(col);

        sprintf(str, "<fg=%d;", fg);
        if (bg != COL_NOCHANGE)
                sprintf(str+strlen(str), "bg=%d;", bg);
        if (at & ATTR_BOLD)
                sprintf(str+strlen(str), "b;");
        else
                sprintf(str+strlen(str), "/b;");
        if (at & ATTR_UNDERSCORE)
                sprintf(str+strlen(str), "u;");
        else
                sprintf(str+strlen(str), "/u;");
        if (at & ATTR_BLINK)
                sprintf(str+strlen(str), "f;");
        else
                sprintf(str+strlen(str), "/f;");
        sprintf(str+strlen(str)-1, ">");
}

void push_color(void)
{
	struct col_stack * col;

	if (USE_COLORS) {
		CREATE(col, struct col_stack, 1, 0);
		col->fg = ansicol_curr.fg;
		col->bg = ansicol_curr.bg;
		col->at = ansicol_curr.at;
		col->next = ansicol_stack;
		ansicol_stack = col;
	}
}

void pull_color(void)
{
	if (USE_COLORS) {
		if (ansicol_stack == NULL) {
			assert(false);
			return;
		}
		ansi_update_color(ansicol_stack->fg, ansicol_stack->bg,
				  ansicol_stack->at);

		struct col_stack *col = ansicol_stack;
		ansicol_stack = col->next;
		Free(col);
	}
}

static void ansi_update_color(int fg, int bg, int at)
{
	bool flag = false;

        if ((fg == ansicol_curr.fg) && (bg == ansicol_curr.bg)
            && (at == ansicol_curr.at))
                return;

	putchar('\x1b');
	putchar('[');
	if (at != ansicol_curr.at) {
		/* bold */
		if ((at & ATTR_BOLD) && !(ansicol_curr.at & ATTR_BOLD)) {
			putchar('1');
			flag = true;
		} else if (!(at & ATTR_BOLD) &&
			   (ansicol_curr.at & ATTR_BOLD)) {
			printf("22");
			flag = true;
		}
		/* underscore */
		if ((at & ATTR_UNDERSCORE) &&
		    !(ansicol_curr.at & ATTR_UNDERSCORE)) {
			if (flag)
				putchar(';');
			else
				flag = true;
			putchar('4');
		} else if (!(at & ATTR_UNDERSCORE) &&
			   (ansicol_curr.at & ATTR_UNDERSCORE)) {
			if (flag)
				putchar(';');
			else
				flag = true;
			printf("24");
		}
		/* blink */
		if ((at & ATTR_BLINK) && !(ansicol_curr.at & ATTR_BLINK)){
			if (flag)
				putchar(';');
			else
				flag = true;
			putchar('5');
		} else if (!(at & ATTR_BLINK) &&
			   (ansicol_curr.at & ATTR_BLINK)) {
			if (flag)
				putchar(';');
			else
				flag = true;
			printf("25");
		}
		/* reverse */
		if ((at & ATTR_REVERSE) &&
		    !(ansicol_curr.at & ATTR_REVERSE)) {
			if (flag)
				putchar(';');
			else
				flag = true;
			putchar('7');
		} else if (!(at & ATTR_REVERSE) &&
			   (ansicol_curr.at & ATTR_REVERSE)) {
			if (flag)
				putchar(';');
			else
				flag = true;
			printf("27");
		}
		ansicol_curr.at = at;
	}
	if (fg != ansicol_curr.fg) {
		if (flag)
			putchar(';');
		else
			flag = true;
		printf("3%d", fg);
		ansicol_curr.fg = fg;
	}
	if (bg != ansicol_curr.bg) {
		if (flag)
			putchar(';');
		else
			flag = true;
		printf("4%d", bg);
		ansicol_curr.bg = bg;
	}
	putchar('m');
        fflush(stdout);
}

void ansi_reset(void) /* deprecated : use setcolor(C_DEFAULT); instead! */
{
	setcolor(C_DEFAULT);
	printf("%s", hreset);
}

void ansi_color_reset(void)
{
	printf("%s", hcoldfl);
}

void ansi_clear_bold(void)
{
	printf("%s", hstop_def);
}

void ansi_set_bold(void)
{
	printf("%s", hstop_def);
}

void color_mode(int mode)
{
	switch(mode) {
	case CMODE_PLAIN:
		if (ansicol_mode == CMODE_ANSI)
			ansi_reset();
		else if (ansicol_mode == CMODE_BOLD)
			ansi_clear_bold();
		hstop = hempty;
		hbold = hempty;
		break;
	case CMODE_BOLD:
		if (ansicol_mode == CMODE_ANSI)
			ansi_reset();
	case CMODE_ANSI:
		hbold = hbold_def;
		hstop = hstop_def;
		break;
	default:
		return;
	}
	ansicol_mode = mode;
}

void ansi_print(void)
{
	printf("FG: %d, BG: %d, AT: %d.\n", ansicol_curr.fg, ansicol_curr.bg,
	       ansicol_curr.at);
}

#if 0
void col_bg(int col)
{
	printf("\x1b[1;4%dm", col);
}

void printcol(int code)
{
	printf("\x1b[0%d;3%d;4%dm", CC_ATTR(code), CC_FG(code), CC_BG(code));
}

void col_attr(int attr)
{
	printf("\x1b[1;0%dm", attr);
}

void col_set(int bg, int fg, int attr)
{
	printf("\x1b[1;4%d;3%d;0%dm", bg, fg, attr);
}
#endif
