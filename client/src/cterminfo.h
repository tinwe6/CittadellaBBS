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
* File : c_terminfo.h                                                       *
*        funzioni e wrappers per l'uso del database terminfo                *
****************************************************************************/
#ifndef _CTERMINFO_H
#define _CTERMINFO_H   1

#define HAVE_CURSES_H   1

#ifdef HAVE_CURSES_H
#define HAVE_CTI 1
#include <curses.h>
#include <term.h>
#endif

#include <stdbool.h>

extern int term_nrows;
extern int term_ncols;
extern struct winsize target_term_size;

# define NCOL          term_ncols
# define NRIGHE        term_nrows

/* ANSI escape sequence to erase all text from cursor pos to end of line */
#define ERASE_TO_EOL "\x1b[K"

#ifdef HAVE_CTI
extern char *ctistr_ll;
extern int cti_bce;   /* Flag background color erase */
#endif

#define cti_visual_bell()       putp(flash_screen)
#define make_cursor_invisible() putp(cursor_invisible)
#define make_cursor_visible()   putp(cursor_normal)
#define cti_mv(hpos, vpos)      putp(tparm(cursor_address, (vpos), (hpos)))
//#define cti_ll()               putp(ctistr_ll)
#define cti_ll()                cti_mv(0, NRIGHE - 1)
#define cti_scroll_ureg()       set_scroll_region(0, NRIGHE-1)
#define scroll_up()             putp(scroll_forward)
#define scroll_down()           putp(scroll_reverse)

/* Sequenze di escape caratteri estesi */
/*
extern char *key_left, *key_right, *key_up, *key_down, *key_home, *key_end;
extern char *next_page, *prev_page, *key_ins;
extern char *key_f0, *key_f1, *key_f2, *key_f3, *key_f4, *key_f5, *key_f6;
extern char *key_f7, *key_f8, *key_f9;
*/

/* Prototipi delle funzioni in cterminfo.c */
void cti_init(void);
void cti_term_init(void);
void cti_term_exit(void);
bool cti_record_term_change(void);
void cti_clear_screen(void);
void set_scroll_full(void);
void reset_scroll_region(void);
void init_window(void);
void window_push(int first_row, int last_row);
void window_pop(void);

int debug_get_winstack_index(void);
void debug_get_winstack(int index, int *first, int *last);
void debug_get_current_win(int *first, int *last);

#endif /* cterminfo.h */
