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
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : cml.c                                                              *
*        Cittadella Markup Language.                                        *
****************************************************************************/
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "ansicol.h"
#include "cittacfg.h"
#include "cml.h"
#include "cml_entities.h"
#include "configurazione.h"
#include "inkey.h"
#include "macro.h"
#include "metadata.h"

/* 
        CML Specifications:

   Character entity references: &name;
   <b> </b>  Bold
   <f> </f>  Flash
   <r> </r>  Reverse
   <u> </u>  Underline

*/

/* initial stretchy buffer length */
#define TMPBUF_LEN 256

int cml_print_max(const char *str, int max);
int cml_printf(const char *format, ...);
int cml_nprintf(size_t size, const char *format, ...);
char * cml_evalf(int *color, const char *format, ...);
char * cml_eval_max(const char *str, int *totlen, int maxlen, int *color,
                    Metadata_List *mdlist);
int cml2editor(const char *str, int *outstr, int *outcol, int *totlen,
	       int maxlen, int *color, Metadata_List *mdlist);
char * editor2cml(int *str, int *col, int *color, size_t len,
                  Metadata_List *mdlist);
char * iso2cml(char *str, size_t len);

static char * cml_parse_tag(const char **str, int *color,Metadata_List *mdlist);
static const char * cml_getnum(unsigned long *num, const char *p);
static const char * cml_getstr(char *str, const char *p);
static void cml_parse_mdtag(const char **str, Metadata_List *mdlist);

/***************************************************************************/
/*
 * Visualizza una stringa cml dopo averla interpretata.
 * Restituisce il numero di caratteri visualizzati sullo schermo.
 */
int cml_print_max(const char *str, int max)
{
	char *out;
	int len, col;
 
	col = getcolor();
	out = cml_eval_max(str, &len, max, &col, NULL);
	printf("%s", out);
	Free(out);
	putcolor(col);
	return len;
}

int cml_spoiler_print(const char *str, int max, int spoiler)
{
        if (spoiler) {
                push_color();
                setcolor(MAGENTA);
                putchar('|');
                pull_color();
        }
        return cml_print_max(str, max);
}


int cml_printf(const char *format, ...)
{
	char *out;
	va_list ap;
	size_t len = TMPBUF_LEN;
	char *str;
	int ret, totlen, ok = 0, col;

	va_start(ap, format);
	CREATE(str, char, len, 0);
	while (!ok) {
		ret = vsnprintf(str, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(str, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	va_end(ap);

	col = getcolor();
	out = cml_eval_max(str, &totlen, -1, &col, NULL);
	printf("%s", out);
	Free(str);
	Free(out);
	putcolor(col);

	return totlen;
}

int cml_sprintf(char *str, const char *format, ...)
{
	char *out, *tmp;
	va_list ap;
	size_t len = TMPBUF_LEN;
	int ret, totlen, ok = 0, col;

	va_start(ap, format);
	CREATE(tmp, char, len, 0);
	while (!ok) {
		ret = vsnprintf(tmp, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(tmp, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	va_end(ap);

	col = getcolor();
	out = cml_eval_max(tmp, &totlen, -1, &col, NULL);
	strcpy(str, out);

	Free(tmp);
	Free(out);

	return totlen;
}

int cml_nprintf(size_t size, const char *format, ...)
{
	char *out;
	va_list ap;
	size_t len = TMPBUF_LEN;
	char *str;
	int ret, totlen, ok = 0, col;

	va_start(ap, format);
	CREATE(str, char, len, 0);
	while (!ok) {
		ret = vsnprintf(str, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(str, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	va_end(ap);

	col = getcolor();
	out = cml_eval_max(str, &totlen, size, &col, NULL);
	printf("%s", out);
	Free(str);
	Free(out);
	putcolor(col);
	return totlen;
}

char * cml_evalf(int *color, const char *format, ...)
{
	va_list ap;
	size_t len = TMPBUF_LEN;
	char *str, *out;
	int ret, ok = 0;

	va_start(ap, format);
	CREATE(str, char, len, 0);
	while (!ok) {
		ret = vsnprintf(str, len, format, ap);
		if ((ret == -1) || (ret >= len)) {
			len *= 2;
			RECREATE(str, char, len);
			va_end(ap);
			va_start(ap, format);
		} else
			ok = 1;
	}
	va_end(ap);

	out = cml_eval_max(str, NULL, -1, color, NULL);
	Free(str);
	return out;
}

enum {
	ET_TEXT,
	ET_ESCAPE,
	ET_TAG,
	ET_ENDTAG,
	ET_ENTITY
};

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
char * cml_eval_max(const char *str, int *totlen, int maxlen, int *color,
                    Metadata_List *mdlist)
{
	register const char *p;
	register int curr_state = ET_TEXT;
	const char *tmp;
	char *out, *tmp2;
	size_t pos = 0, max = LBUF, len = 0;
	int ret;

	if (str == NULL)
		return strdup("");

	if (totlen != NULL)
		*totlen = maxlen;

	CREATE(out, char, max+1, 0);

	for (p = str; *p; p++) {
		if ((max - pos) < 40) { /* potrebbe non bastare il posto */
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
				tmp2 = cml_parse_tag(&tmp, color, mdlist);
				if (tmp2 != NULL) {
					p = (const char *)tmp2;
					while(*p)
						out[pos++] = *(p++);
					p = tmp;
					Free(tmp2);
				} else
					out[pos++] = '<';
				break;
			case ET_ESCAPE:
				out[pos++] = *p;
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
				out[pos++] = *p;
				len++;
				break;
			}
			break;
		case '&':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				out[pos++] = *p;
				len++;
				break;
			case ET_TEXT:
				tmp = p;
				ret = cml_entity(&tmp);
				if (ret != -1) {
					if (HAS_ISO8859) {
						out[pos++] = ret;
						len++;
					} else if (is_isoch(ret)) {
						p = cml_entity_tab[ret -
						    Key_NOBREAK_SPACE].ascii;
						while(*p) {
							out[pos++] = *(p++);
							len++;
						}
					}
					p = tmp;
					/* Si mangia il ';' finale se c'e' */
					/* Prima o poi sara` obbligatorio. */
					if (*(p+1) == ';')
						p++;
				} else {
					out[pos++] = '&';
					len++;
				}
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
		if (len > maxlen) {
			out[pos-1] = '\0';
			return out;
		}
	}
	out[pos] = '\0';

	if (totlen != NULL)
		*totlen = len;

	return out;
}

/*
 * Interpreta la stringa CML in str e la traduce in una coppia di stringhe,
 * la prima di caratteri ISO-8859 corrispondenti al testo, la seconda
 * con i rispettivi colori e attributi dei caratteri.
 * Se maxlen > 0, vengono inseriti al piu' maxlen caratteri nel
 * risultato. Altrimenti non vi sono limiti.
 * Restituisce la lunghezza in caratteri della stringa risultante.
 */
/* TODO visto che viene restituito, totlen non serve come argomento!!! */
int cml2editor(const char *str, int *outstr, int *outcol, int *totlen,
	       int maxlen, int *color, Metadata_List *mdlist)
{
	register const char *p;
	register int curr_state = ET_TEXT;
	const char *tmp;
	char *tmp2;
	size_t pos = 0, len = 0;
	int ret;

	if (str == NULL) 
		return 0;

	if (totlen != NULL)
		*totlen = maxlen;

	for (p = str; *p; p++) {
		switch(*p) {
		case '\\':
			switch (curr_state) {
			case ET_TEXT:
				curr_state = ET_ESCAPE;
				break;
			case ET_ESCAPE:
				outstr[pos] = *p;
				outcol[pos++] = *color;
				len++;
				curr_state = ET_TEXT;
				break;
			}
			break;
		case '<':
			switch (curr_state) {
			case ET_TEXT:
				tmp = p;
				tmp2 = cml_parse_tag(&tmp, color, mdlist);
				if (tmp2 != NULL) {
					p = tmp;
					Free(tmp2);
				} else {
					outstr[pos] = '<';
					outcol[pos++] = *color;
				}
				break;
			case ET_ESCAPE:
				outstr[pos] = *p;
				outcol[pos++] = *color;
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
				outstr[pos] = *p;
				outcol[pos++] = *color;
				len++;
				break;
			}
			break;
		case '&':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				outstr[pos] = *p;
				outcol[pos++] = *color;
				len++;
				break;
			case ET_TEXT:
				tmp = p;
				ret = cml_entity(&tmp);
				if (ret != -1) {
					if (HAS_ISO8859) {
						outstr[pos] = ret;
						outcol[pos++] = *color;
						len++;
					} else if (is_isoch(ret)) {
						p = cml_entity_tab[ret -
						    Key_NOBREAK_SPACE].ascii;
						while(*p) {
							outstr[pos] = *(p++);
							outcol[pos++] = *color;
							len++;
						}
					}
					p = tmp;
					/* Si mangia il ';' finale se c'e' */
					/* Prima o poi sara` obbligatorio. */
					if (*(p+1) == ';')
						p++;
				} else {
					outstr[pos] = '&';
					outcol[pos++] = *color;
					len++;
				}
				break;
			}
			break;
		default:
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
			case ET_TEXT:
				outstr[pos] = *p;
				outcol[pos++] = *color;
				len++;
				break;
			}
		}
		if (len > maxlen)
			return len;
	}

	if (totlen != NULL)
		*totlen = len;

	return len;
}

static char * cml_parse_tag(const char **str, int *color, Metadata_List *mdlist)
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
	register char *o;
	register int curr_state;
	char digit, tmp[LBUF], strbuf1[LBUF], strbuf2[LBUF];
	int tmpcol;
        unsigned long size, flags, num;

	tmpcol = *color;
	if (**str != '<')
		return NULL;
	curr_state = TAG_START;
	o = tmp;
	*(o++)='\x1b';
	*(o++)='[';
	for (p = *str+1; *p; p++) {
		switch(*p) {
		case '/':
			switch (curr_state) {
			case TAG_START:
				curr_state = TAG_NOT;
				break;
			default:
				return NULL;
			}
			break;
		case 'b':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "log user=\"", 10)) {
                                        p += 11;
                                        p = cml_getstr(strbuf1, p);
                                        if (strncmp(p, " num=", 5))
                                                return NULL;
                                        p += 5;
                                        p = cml_getnum(&num,p);
                                        if (*p != '>')
                                                return NULL;
                                        o += sprintf(o,
                       "1;33;40m%s\x1b[0;33m: \x1b[0;35m#%ld\x1b[0%d;3%d;4%dm",
                                                     strbuf1, num,
                                                     CC_ATTR(*color),
                                                     CC_FG(*color), CC_BG(*color));
                                        /*md_id = md_insert_blogpost(mdlist, strbuf1,num);*/
                                        *str = p;
                                        return strdup(tmp);
                                }
				curr_state = TAG_b;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_BOLD;
				break;
			default:
				return NULL;
			}
			break;
		case 'f':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "ile name=\"", 10)) {
                                        p += 11;
                                        p = cml_getstr(strbuf1, p);
                                        if (strncmp(p, " num=", 5))
                                                return NULL;
                                        p += 5;
                                        p = cml_getnum(&num,p);
                                        if (strncmp(p, " size=", 6))
                                                return NULL;
                                        p += 6;
                                        p = cml_getnum(&size,p);
                                        if (strncmp(p, " flags=", 7))
                                                return NULL;
                                        p += 7;
                                        p = cml_getnum(&flags,p);
                                        if (*p != '>')
                                                return NULL;
                                        o += sprintf(o,
                                               "4;32;40m%s\x1b[0%d;3%d;4%dm",
                                                     strbuf1,
                                                     CC_ATTR(*color),
                                                     CC_FG(*color), CC_BG(*color));
                                        /*md_id = md_insert_file(mdlist, strbuf1, NULL, num, size, flags);*/
                                        *str = p;
                                        return strdup(tmp);
                                }
				curr_state = TAG_f;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_BLINK;
				break;
			default:
				return NULL;
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
				return NULL;
			}
			break;
		case 'l':
			switch (curr_state) {
			case TAG_START:
                                if (strncmp(p+1, "ink=\"", 5))
                                        return NULL;
                                p += 6;
                                p = cml_getstr(strbuf1, p);
                                if (!strncmp(p, " label=\"", 8)) {
                                        p += 8;
                                        p = cml_getstr(strbuf2, p);
                                } else
                                        strbuf2[0] = 0;
                                if (*p != '>')
                                        return NULL;
                                /*md_id = md_insert_link(mdlist, strbuf1,strbuf2);*/
                                if (strbuf2[0] == 0) {
                                        strncpy(strbuf2, strbuf1, 75);
                                        if (strlen(strbuf1) > 75)
                                                strcpy(strbuf2+75, "...");
                                }

                                o += sprintf(o, "4;36;40m%s\x1b[0%d;3%d;4%dm",
                                             strbuf2, CC_ATTR(*color),
                                             CC_FG(*color), CC_BG(*color));
                                *str = p;
				return strdup(tmp);
			default:
				return NULL;
			}
			break;
                case 'p':
                        if (strncmp(p+1, "ost room=\"", 10))
                                return NULL;
                        p += 11;
                        p = cml_getstr(strbuf1, p);
                        if (strncmp(p, " num=", 5))
                                return NULL;
                        p += 5;
                        p = cml_getnum(&num,p);
                        if (*p != '>')
                                return NULL;
                        o += sprintf(o,
                                     "1;33;40m%s\x1b[0;33m> \x1b[0;35m#%ld\x1b[0%d;3%d;4%dm",
                                     strbuf1, num,
                                     CC_ATTR(*color),
                                     CC_FG(*color), CC_BG(*color));
                        /*md_id = md_insert_post(mdlist, strbuf1, num);*/
                        *str = p;
                        return strdup(tmp);
		case 'r':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "oom=\"", 5)) {
                                        p += 6;
                                        p = cml_getstr(strbuf1, p);
                                        if (*p != '>')
                                                return NULL;
                                        o += sprintf(o,
                 "1;33;40m%s\x1b[0;33m>\x1b[0%d;3%d;4%dm",
                                                     strbuf1, CC_ATTR(*color),
                                                     CC_FG(*color), CC_BG(*color));
                                        /*md_id = md_insert_room(mdlist, strbuf1);*/
                                        *str = p;
                                        return strdup(tmp);
                                }
				curr_state = TAG_REVERSE;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_REVERSE;
				break;
			default:
				return NULL;
			}
		case 'u':
			switch (curr_state) {
			case TAG_START:
                                if (!strncmp(p+1, "ser=\"", 5)) {
                                        p += 6;
                                        p = cml_getstr(strbuf1, p);
                                        if (*p != '>')
                                                return NULL;
                                        o += sprintf(o,
                 "1;34;40m%s\x1b[0%d;3%d;4%dm",
                                                     strbuf1, CC_ATTR(*color),
                                                     CC_FG(*color), CC_BG(*color));
                                        /*md_id = md_insert_user(mdlist, strbuf1);*/
                                        *str = p;
                                        return strdup(tmp);
                                }
				curr_state = TAG_UNDERSCORE;
				break;
			case TAG_NOT:
				curr_state = TAG_CLEAR_UNDERSCORE;
				break;
			default:
				return NULL;
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
				return NULL;
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
				return NULL;
			}
			break;
		case ';':
		case '>':
			switch (curr_state) {
			case TAG_b:                /* Set bold         */
				*(o++) = '1';
				*color |= (ATTR_BOLD << 8);
				break;
			case TAG_CLEAR_BOLD:       /* clear bold       */
				*(o++) = '2';
				*(o++) = '2';
				*color &= ~(ATTR_BOLD << 8);
				break;
			case TAG_f:                /* Set blink        */
				*(o++) = '5';
				*color |= (ATTR_BLINK << 8);
				break;
			case TAG_CLEAR_BLINK:      /* Clear Blink      */
				*(o++) = '2';
				*(o++) = '5';
				*color &= ~(ATTR_BLINK << 8);
				break;
			case TAG_REVERSE:          /* Set reverse      */
				*(o++) = '7';
				*color |= (ATTR_REVERSE << 8);
				break;
			case TAG_CLEAR_REVERSE:    /* Clear reverse    */
				*(o++) = '2';
				*(o++) = '7';
				*color &= ~(ATTR_REVERSE << 8);
				break;
			case TAG_UNDERSCORE:       /* Set underscore   */
				*(o++) = '4';
				*color |= (ATTR_UNDERSCORE << 8);
				break;
			case TAG_CLEAR_UNDERSCORE: /* Clear underscore */
				*(o++) = '2';
				*(o++) = '4';
				*color &= ~(ATTR_UNDERSCORE << 8);
				break;
			case TAG_FGEND:            /* Change fg color  */
				*(o++) = '3';
				*(o++) = digit;
				*color = (*color & (~0x0f)) + (digit - '0');
				break;
			case TAG_BGEND:            /* Change bg color  */
				*(o++) = '4';
				*(o++) = digit;
				*color = (*color & (~0xf0)) + ((digit-'0')<<4);
				break;
			default:
				return NULL;
			}
			if (*p == '>') {
				*str = p;
				if (!USE_COLORS)
					return strdup("");
				*(o++) = 'm';
				*(o++) = '\0';
				return strdup(tmp);
			}
			*(o++) = ';';
		        curr_state = TAG_START;	
			break;
		}
	}
	*color = tmpcol;
	return NULL;
}

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
#define MDNUM(c)      ((c >> 16) & 0xff)

char * editor2cml(int *str, int *col, int *color, size_t len,
                  Metadata_List *mdlist)
{
	int start, i, j, pos, currcol, flag, max, old_at, new_at, md_id;
        size_t mdlen;
	char *out, *tmp, mdstr[LBUF];

	max = len;
	CREATE(out, char, max+1, 0);

        start = 0;
        if ( (md_id = MDNUM(*color))) {
                /* Elimina la fine del md della riga precedente */
                while (MDNUM(col[start]) == md_id)
                        start++;
        }

	currcol = *color;
	pos = 0;
	for (i = start; i < len; i++) {
                while ( (md_id = MDNUM(col[i]))) {
                        /* Inizio del metadata */
                        mdlen = md_convert2cml(mdlist, md_id, mdstr);
                        while ((max - pos) < mdlen) {
                                max *= 2;
                                RECREATE(out, char, max+1);
                        }
                        for (j = 0; j < mdlen; j++)
                                out[pos++] = mdstr[j];
                        do
                                i++;
                        while (MDNUM(col[i]) == md_id);
                }
                while ((max - pos) < 24) {
                        max *= 2;
                        RECREATE(out, char, max+1);
                }
                if (col[i] != currcol) {
			flag = 0;
			out[pos++] = '<';
			new_at = CC_ATTR(col[i]);
			old_at = CC_ATTR(currcol);
			if (new_at != old_at) {
				if ((old_at & ATTR_BOLD) !=
				    (new_at & ATTR_BOLD)) {
					if (old_at & ATTR_BOLD)
						out[pos++] = '/';
					out[pos++] = 'b';
					flag = 1;
				}
				if ((old_at & ATTR_UNDERSCORE) !=
				    (new_at & ATTR_UNDERSCORE)) {
					if (flag)
						out[pos++] = ';';
					if (old_at & ATTR_UNDERSCORE)
						out[pos++] = '/';
					out[pos++] = 'u';
					flag = 1;
				}
				if ((old_at & ATTR_BLINK) !=
				    (new_at & ATTR_BLINK)) {
					if (flag)
						out[pos++] = ';';
					if (old_at & ATTR_BLINK)
						out[pos++] = '/';
					out[pos++] = 'f';
					flag = 1;
				}
				if ((old_at & ATTR_REVERSE) !=
				    (new_at & ATTR_REVERSE)) {
					if (flag)
						out[pos++] = ';';
					if (old_at & ATTR_REVERSE)
						out[pos++] = '/';
					out[pos++] = 'r';
					flag = 1;
				}
			}
			if (CC_FG(col[i]) != CC_FG(currcol)){
				if (flag)
					out[pos++] = ';';
				out[pos++] = 'f';
				out[pos++] = 'g';
				out[pos++] = '=';
				out[pos++] = CC_FG(col[i])+'0';
				flag = 1;
			}
			if (CC_BG(col[i]) != CC_BG(currcol)){
				if (flag)
					out[pos++] = ';';
				out[pos++] = 'b';
				out[pos++] = 'g';
				out[pos++] = '=';
				out[pos++] = CC_BG(col[i])+'0';
			}
			out[pos++] = '>';
			currcol = col[i];
		}
		if (str[i] > 127) { /* Extended 8-bit ascii char */
			for (j = 0; cml_entity_tab[j].code; j++)
				if (str[i] ==  cml_entity_tab[j].code) {
					out[pos++] = '&';
					for (tmp = cml_entity_tab[j].str; *tmp;
					     out[pos++] = *(tmp++));
					out[pos++] = ';';
					break;
				}
		} else {
			if ((str[i] == '\\') || (str[i] == '>') ||
			    (str[i] == '<') || (str[i] == '&'))
				out[pos++] = '\\';
			out[pos++] = str[i];
		}
	}
	*color = currcol;
	out[pos] = '\0';
	return out;
}

char * iso2cml(char *str, size_t len)
{
	int i, j, pos, max;
	char *out, *tmp;

	max = len;
	len = strlen(str);
	CREATE(out, char, max+1, 0);

	pos = 0;
	for (i = 0; i < len; i++) {
		while ((max - pos) < 24) { /* potrebbe non bastare il posto */
			max *= 2;
			RECREATE(out, char, max+1);
		}
		if (str[i] < 0) { /* Extended 8-bit ascii char */
			for (j = 0; cml_entity_tab[j].code; j++)
				if ((unsigned char)str[i] ==  cml_entity_tab[j].code) {
					out[pos++] = '&';
					for (tmp = cml_entity_tab[j].str; *tmp;
					     out[pos++] = *(tmp++));
					out[pos++] = ';';
					break;
				}
		} else {
			if ((str[i] == '\\') || (str[i] == '>') ||
			    (str[i] == '<') || (str[i] == '&'))
				out[pos++] = '\\';
			out[pos++] = str[i];
		}
	}
	out[pos] = '\0';
	return out;
}

/***************************************************************************/
void cml_extract_md(const char *str, Metadata_List *mdlist)
{
	const char *p;
	int curr_state = ET_TEXT;
	int ret;

	if (str == NULL)
		return;

	for (p = str; *p; p++) {
		switch(*p) {
		case '\\':
			switch (curr_state) {
			case ET_TEXT:
				curr_state = ET_ESCAPE;
				break;
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				break;
			}
			break;
		case '<':
			switch (curr_state) {
			case ET_TEXT:
				cml_parse_mdtag(&p, mdlist);
				break;
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				break;
			}
			break;
		case '>':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
			}
			break;
		case '&':
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
				break;
			case ET_TEXT:
				ret = cml_entity(&p);
				if (ret != -1) {
					/* Si mangia il ';' finale se c'e' */
					/* Prima o poi sara` obbligatorio. */
					if (*(p+1) == ';')
						p++;
				}
				break;
			}
			break;
		default:
			switch (curr_state) {
			case ET_ESCAPE:
				curr_state = ET_TEXT;
			}
		}
	}
}

static void cml_parse_mdtag(const char **str, Metadata_List *mdlist)
{
	const char *p;
	char strbuf1[LBUF], strbuf2[LBUF];
	/* int md_id; */
        unsigned long num, size, flags;

	if (**str != '<')
		return;
	p = *str+1;
        if (!strncmp(p, "blog user=\"", 11)) {
                p += 11;
                p = cml_getstr(strbuf1, p);
                if (!strncmp(p, " num=", 5)) {
                        p += 5;
                        p = cml_getnum(&num,p);
                }
                if (*p != '>')
                        return;
                /* md_id = md_insert_blogpost(mdlist, strbuf1,num); */
                *str = p;
                return;
        } else if (!strncmp(p, "file name=\"", 11)) {
                p += 11;
                p = cml_getstr(strbuf1, p);
                if (strncmp(p, " num=", 5))
                        return;
                p += 5;
                p = cml_getnum(&num,p);

                if (strncmp(p, " size=", 6))
                        return;
                p += 6;
                p = cml_getnum(&size,p);
                if (strncmp(p, " flags=", 7))
                        return;
                p += 7;
                p = cml_getnum(&flags,p);

                if (*p != '>')
                        return;
                /* md_id = md_insert_file(mdlist, strbuf1, NULL, num, size, flags); */
                *str = p;
                return;
        } else if (!strncmp(p, "link=\"", 6)) {
                p += 6;
                p = cml_getstr(strbuf1, p);
                if (!strncmp(p, " label=\"", 8)) {
                        p += 8;
                        p = cml_getstr(strbuf2, p);
                } else
                        strbuf2[0] = 0;
                if (*p != '>')
                        return;
                /* md_id = md_insert_link(mdlist, strbuf1, strbuf2); */
                *str = p;
                return;
        } else if (!strncmp(p, "post room=\"", 11)) {
                p += 11;
                p = cml_getstr(strbuf1, p);
                if (!strncmp(p, " num=", 5)) {
                        p += 5;
                        p = cml_getnum(&num,p);
                }
                if (*p != '>')
                        return;
                /* md_id = md_insert_post(mdlist, strbuf1, num); */
                *str = p;
                return;
        } else if (!strncmp(p, "room=\"", 6)) {
                p += 6;
                p = cml_getstr(strbuf1, p);
                if (*p != '>')
                        return;
                /* md_id = md_insert_room(mdlist, strbuf1); */
                *str = p;
                return;
        } else if (!strncmp(p, "user=\"", 6)) {
                p += 6;
                p = cml_getstr(strbuf1, p);
                if (*p != '>')
                        return;
                /* md_id = md_insert_user(mdlist, strbuf1); */
                *str = p;
                return;
        }
	return;
}
