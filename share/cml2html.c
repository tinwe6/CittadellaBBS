/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX share                                     (C) M. Caldarelli *
*                                                                           *
* File : cml2html.c                                                         *
*        Cittadella Markup Language (CML) to HTML translator.               *
****************************************************************************/
#ifdef LINUX
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "cml2html.h"
#include "cml_entities.h"

/* TODO Trovare modo di eliminare la seguente def!!! */
//#define USE_MEM_STAT

#include "macro.h"

#define LBUF 256
#define HAS_ISO8859   1

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

/*
Black = "#000000" Green = "#008000"
Silver = "#C0C0C0" Lime = "#00FF00"
Gray = "#808080" Olive = "#808000"
White = "#FFFFFF" Yellow = "#FFFF00"
Maroon = "#800000" Navy = "#000080"
Red = "#FF0000" Blue = "#0000FF"
Purple = "#800080" Teal = "#008080"
Fuchsia = "#FF00FF" Aqua = "#00FFFF"
*/

/*
const char *html_col_tab[] = {
	"Black", "Red",     "Lime", "Yellow",
	"Blue",  "Fuchsia", "Aqua", "Silver"};
*/
const char html_col_tab[] = {'K', 'R', 'G', 'Y', 'B', 'M', 'C', 'W'};

/* these are Web-safe colors: http://www.brown.edu/webmaster/chart.html    */

static char * htmlstr_open_tags(char *ptr, int color);
static int cml_parse_tag(const char **str, int *color, Metadata_List *mdlist);
static const char * cml_getnum(unsigned long *num, const char *p);
static const char * cml_getstr(char *str, const char *p);

/***************************************************************************/
/*
 * Interpreta la stringa CML in str e la traduce in una stringa ASCII
 * o ISO-8859 visualizzabile nello schermo. Colori e attributi dei
 * caratteri vengono codificati in sequenze di escape ANSI.
 * La stringa risultante, che viene restituita dalla funzione, viene
 * generata dinamicamente da malloc(), ed e' pertanto necessario
 * ricordarsi di effettuare un free quando non serve piu'.
 * Se maxlen > 0, vengono inseriti al piu' maxlen caratteri nel
 * risultato ('\0' finale compreso). Altrimenti non vi sono limiti.
 * In totlen viene restituita la lunghezza effettiva della stringa
 * una volta visualizzata.
 */
/* TODO totlen non viene mai utilizzato.. lo tolgo? */
char * cml2html_max(const char *str, int *totlen, int maxlen, int *color,
                    Metadata_List *mdlist)
{
	enum {
                ET_TEXT,
                ET_ESCAPE,
                ET_TAG,
                ET_ENDTAG,
                ET_ENTITY
	};
	register const char *p;
	register int curr_state = ET_TEXT;
	const char *tmp;
	char *out;
        char tags[2*LBUF], *tptr;
	size_t pos = 0, max = LBUF, len = 0;
        int oldcol, tag;

	if (str == NULL)
		return Strdup("");

	if (totlen != NULL)
		*totlen = maxlen;

	CREATE(out, char, max+1, TYPE_CHAR);

	for (p = str; *p; p++) {
                /* TODO mettere un check serio!!! */
		if ((max - pos) < LBUF) { /* potrebbe non bastare il posto */
			max *= 2;
			RECREATE(out, char, max+1);
		}
		switch(*p) {
		case '\\':
			switch (curr_state) {
			case ET_TEXT:
				curr_state = ET_ESCAPE;
				break;
			case ET_ESCAPE:
				out[pos++] = *p;
				len++;
				curr_state = ET_TEXT;
				break;
			}
			break;
		case '<':
			switch (curr_state) {
			case ET_TEXT:
				tmp = p;

                                oldcol = *color;

				tag = cml_parse_tag(&tmp, color, mdlist);
                                if (tag != -1)
                                        p = tmp;
                                if (tag == 0) {
                                        tags[0] = 0;
                                        tptr = tags;
                                        
                                        if ((CC_FG(*color) != CC_FG(oldcol))
                                            || (CC_BG(*color)!=CC_BG(oldcol))) {
                                                tptr = htmlstr_close_tags(tptr, oldcol);
                                                tptr = stpcpy(tptr, "</SPAN><SPAN class=\"");
                                                *(tptr++)=html_col_tab[CC_FG(*color)];
                                                *(tptr++)=html_col_tab[CC_BG(*color)];
                                                tptr = stpcpy(tptr, "\">");
                                                tptr = htmlstr_open_tags(tptr, *color);
                                        } else {
                                                tptr = htmlstr_close_tags(tptr, oldcol);
                                                tptr = htmlstr_open_tags(tptr, *color);
                                        }
                                } else if (tag > 0) {
                                        md_convert2html(mdlist, tag, tags);
                                }
                                if (tags[0]) {
                                        tptr = tags;
					while (*tptr)
						out[pos++] = *(tptr++);
				} else {
					out[pos++] = '&';
					out[pos++] = 'l';
					out[pos++] = 't';
					out[pos++] = ';';
					len++;
				}
				break;
			case ET_ESCAPE:
				out[pos++] = '&';
				out[pos++] = 'l';
				out[pos++] = 't';
				out[pos++] = ';';
				len++;
				curr_state = ET_TEXT;
				break;
			}
			break;
		case '>':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
			case ET_TEXT:
				out[pos++] = '&';
				out[pos++] = 'g';
				out[pos++] = 't';
				out[pos++] = ';';
				len++;
				break;
			}
			break;
		case '&':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				out[pos++] = '&';
				out[pos++] = 'a';
				out[pos++] = 'm';
				out[pos++] = 'p';
				out[pos++] = ';';
				len++;
				break;
			case ET_TEXT:
				out[pos++] = *p;
				len++;
				break;
			}
			break;
		default:
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
			case ET_TEXT:
				out[pos++] = *p;
				len++;
				break;
			}
		}
		if ((maxlen > 0) && (len > maxlen)) {
			out[pos-1] = '\0';
			return out;
		}
	}
	out[pos] = '\0';
	
	if (totlen != NULL)
		*totlen = len;
	return out;
}

char * htmlstr_close_tags(char *ptr, int color)
{
        if (CC_ATTR(color) & ATTR_BOLD)
                ptr = stpcpy(ptr, "</STRONG>");
        if (CC_ATTR(color) & ATTR_BLINK)
                ptr = stpcpy(ptr, "</EM>");
        if (CC_ATTR(color) & ATTR_UNDERSCORE)
                ptr = stpcpy(ptr, "</I>");
        return ptr;
}

static char * htmlstr_open_tags(char *ptr, int color)
{
        if (CC_ATTR(color) & ATTR_UNDERSCORE)
                ptr = stpcpy(ptr, "<I>");
        if (CC_ATTR(color) & ATTR_BLINK)
                ptr = stpcpy(ptr, "<EM>");
        if (CC_ATTR(color) & ATTR_BOLD)
                ptr = stpcpy(ptr, "<STRONG>");
        return ptr;
}

/* Parsing dei tag cml. Ritorna -1 se il tag non era ben formato, 0 se e'
 * cambiato il colore o qualche attributo del carattere, l'id dell'allegato
 * se era un allegato.                                                      */
static int cml_parse_tag(const char **str, int *color, Metadata_List *mdlist)
{
        /* Riconosce i seguenti tag:
           <b> </b> <bg=dig> <fg=dig> <u> </u> <r> </r> <f> </f>
           <link="str"> <user="str"> <room="str">
           <post room="str" num=num> <blog user="str" num=num>
        */
	enum { TAG_START, TAG_NOT, TAG_CLEAR_BOLD, TAG_CLEAR_BLINK,
	       TAG_CLEAR_REVERSE, TAG_CLEAR_UNDERSCORE, TAG_b, TAG_f,
	       TAG_REVERSE, TAG_UNDERSCORE, TAG_BG, TAG_BGEQ, TAG_BGEND,
	       TAG_FG, TAG_FGEQ, TAG_FGEND };

	register const char *p;
	register int curr_state;
	char digit, strbuf1[LBUF], strbuf2[LBUF];
	int tmpcol, md_id;
        unsigned long size, flags, num;

	if (**str != '<')
		return -1;

	tmpcol = *color;
	curr_state = TAG_START;

	for (p = *str+1; *p; p++) {
		switch(*p) {
		case '/':
			switch (curr_state) {
			case TAG_START:
				curr_state = TAG_NOT;
				break;
			default:
				return -1;
			}
			break;
		case 'b':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "log user=\"", 10)) {
                                        p += 11;
                                        p = cml_getstr(strbuf1, p);
                                        if (strncmp(p, " num=", 5))
                                                return -1;
                                        p += 5;
                                        p = cml_getnum(&num,p);
                                        if (*p != '>')
                                                return -1;
                                        *str = p;
                                        return md_insert_blogpost(mdlist, strbuf1,num);
                                }
				curr_state = TAG_b;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_BOLD;
				break;
			default:
				return -1;
			}
			break;
		case 'f':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "ile name=\"", 10)) {
                                        p += 11;
                                        p = cml_getstr(strbuf1, p);
                                        if (strncmp(p, " num=", 5))
                                                return -1;
                                        p += 5;
                                        p = cml_getnum(&num,p);
                                        if (strncmp(p, " size=", 6))
                                                return -1;
                                        p += 6;
                                        p = cml_getnum(&size,p);
                                        if (strncmp(p, " flags=", 7))
                                                return -1;
                                        p += 7;
                                        p = cml_getnum(&flags,p);
                                        if (*p != '>')
                                                return -1;
                                        *str = p;
                                        return md_insert_file(mdlist, strbuf1, NULL, num, size, flags);
                                }
				curr_state = TAG_f;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_BLINK;
				break;
			default:
				return -1;
			}
			break;
		case 'g':
			switch (curr_state) {
			case TAG_b:
				curr_state = TAG_BG;
				break;
			case TAG_f:
				curr_state = TAG_FG;
				break;
			default:
				return -1;
			}
			break;
		case 'l':
			switch (curr_state) {
			case TAG_START:
                                if (strncmp(p+1, "ink=\"", 5))
                                        return -1;
                                p += 6;
                                p = cml_getstr(strbuf1, p);
                                if (!strncmp(p, " label=\"", 8)) {
                                        p += 8;
                                        p = cml_getstr(strbuf2, p);
                                } else
                                        strbuf2[0] = 0;
                                if (*p != '>')
                                        return -1;
                                md_id = md_insert_link(mdlist, strbuf1,strbuf2);
                                if (strbuf2[0] == 0) {
                                        strncpy(strbuf2, strbuf1, 75);
                                        if (strlen(strbuf1) > 75)
                                                strcpy(strbuf2+75, "...");
                                }
                                *str = p;
				return md_id;
			default:
				return -1;
			}
			break;
                case 'p':
                        if (strncmp(p+1, "ost room=\"", 10))
                                return -1;
                        p += 11;
                        p = cml_getstr(strbuf1, p);
                        if (strncmp(p, " num=", 5))
                                return -1;
                        p += 5;
                        p = cml_getnum(&num,p);
                        if (*p != '>')
                                return -1;
                        *str = p;
                        return md_insert_post(mdlist, strbuf1, num);
		case 'r':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "oom=\"", 5)) {
                                        p += 6;
                                        p = cml_getstr(strbuf1, p);
                                        if (*p != '>')
                                                return -1;
                                        *str = p;
                                        return md_insert_room(mdlist, strbuf1);
                                }
				curr_state = TAG_REVERSE;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_REVERSE;
				break;
			default:
				return -1;
			}
		case 'u':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "ser=\"", 5)) {
                                        p += 6;
                                        p = cml_getstr(strbuf1, p);
                                        if (*p != '>')
                                                return -1;
                                        *str = p;
                                        return md_insert_user(mdlist, strbuf1);
                                }
				curr_state = TAG_UNDERSCORE;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_UNDERSCORE;
				break;
			default:
				return -1;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			digit = *p;
			switch (curr_state) {
			case TAG_BGEQ:
				curr_state = TAG_BGEND;
				break;
			case TAG_FGEQ:
				curr_state = TAG_FGEND;
				break;
			default:
				return -1;
			}
			break;
		case '=':
			switch (curr_state) {
			case TAG_BG:
				curr_state = TAG_BGEQ;
				break;
			case TAG_FG:
				curr_state = TAG_FGEQ;
				break;
			default:
				return -1;
			}
			break;
		case ';':
		case '>':
			switch (curr_state) {
			case TAG_b:                /* Set bold         */
				*color |= (ATTR_BOLD << 8);
				break;
			case TAG_CLEAR_BOLD:       /* clear bold       */
				*color &= ~(ATTR_BOLD << 8);
				break;
			case TAG_f:                /* Set blink        */
				*color |= (ATTR_BLINK << 8);
				break;
			case TAG_CLEAR_BLINK:      /* Clear Blink      */
				*color &= ~(ATTR_BLINK << 8);
				break;
			case TAG_REVERSE:          /* Set reverse      */
				*color |= (ATTR_REVERSE << 8);
				break;
			case TAG_CLEAR_REVERSE:    /* Clear reverse    */
				*color &= ~(ATTR_REVERSE << 8);
				break;
			case TAG_UNDERSCORE:       /* Set underscore   */
				*color |= (ATTR_UNDERSCORE << 8);
				break;
			case TAG_CLEAR_UNDERSCORE: /* Clear underscore */
				*color &= ~(ATTR_UNDERSCORE << 8);
				break;
			case TAG_FGEND:            /* Change fg color  */
				*color = (*color & (~0x0f)) + (digit - '0');
				break;
			case TAG_BGEND:            /* Change bg color  */
				*color = (*color & (~0xf0)) + ((digit-'0')<<4);
				break;
			default:
				return -1;
			}
			if (*p == '>') {
				*str = p;
				return 0;
			}
		        curr_state = TAG_START;	
			break;
		}
	}
	*color = tmpcol;
	return -1;
}

/* TODO mettere in un file comune in share */
static const char * cml_getnum(unsigned long *num, const char *p)
{
        int i = 0;

        *num = 0;
        while (isdigit(p[i])) {
                *num = (*num * 10) + (p[i] - '0');
                i++;
        }

        return p+i;
}

static const char * cml_getstr(char *str, const char *p)
{
        int i = 0;

        while (p[i] != '\"') {
                if (p[i] == 0) {
                        str[0] = 0;
                        return p;
                }
                str[i] = p[i];
                i++;
        }
        str[i] = 0;
        return p+i+1;
}

/**********************************************************************/
