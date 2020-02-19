/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File : cml_entities.c                                                     *
*        Entity table for CML (and HTML)                                    *
****************************************************************************/
#include <string.h>
#include "cml_entities.h"

int cml_entity(const char **str);

const struct cml_entity_tab cml_entity_tab[] = {
	{ Key_NOBREAK_SPACE, "nbsp",   "\\001"},     /* Code 160 */
	{ Key_INV_EXCL_MARK, "iexcl",  "!"},
	{ Key_CENT_SIGN,     "cent",   "-c-"},
	{ Key_POUND_SIGN,    "pound",  "-L-"},
	{ Key_CURRENCY_SIGN, "curren", "CUR"},
	{ Key_YEN_SIGN,      "yen",    "YEN"},
	{ Key_BROKEN_BAR,    "brvbar", "|"},
	{ Key_SECTION_SIGN,  "sect",   "S:"},
	{ Key_DIAERESIS,     "uml",    "\""},
	{ Key_COPYRIGHT,     "copy",   "(c)"},
	{ Key_FEM_ORDINAL,   "ordf",   "-a"},
	{ Key_LEFT_GUILLEM,  "laquo",  "<<"},
	{ Key_NOT_SIGN,      "not",    "NOT"},      /* shortcut missing */
	{ Key_SOFT_HYPHEN,   "shy",    "-"},
	{ Key_REGISTERED,    "reg",    "(r)"},
	{ Key_MACRON,        "macr",   "-"},
	{ Key_DEGREE_SIGN,   "deg",    "DEG"},
	{ Key_PLUS_MINUS,    "plusmn", "+/-"},
	{ Key_SUPER_TWO,     "sup2",   "^2"},
	{ Key_SUPER_THREE,   "sup3",   "^3"},
	{ Key_ACUTE_ACC,     "acute",  "'"},
	{ Key_MICRO_SIGN,    "micro",  "u"},       /* shortcut missing */
	{ Key_PILCROW_SIGN,  "para",   "P:"},
	{ Key_MIDDLE_DOT,    "middot", "."},
	{ Key_CEDILLA,       "cedil",  ","},
	{ Key_SUPER_ONE,     "sup1",   "^1"},
	{ Key_MASC_ORDINAL,  "ordm",   "-o"},
	{ Key_RIGHT_GUILLEM, "raquo",  ">>"},
	{ Key_ONE_QUARTER,   "frac14", " 1/4"},
	{ Key_ONE_HALF,      "frac12", " 1/2"},
	{ Key_THREE_QUART,   "frac34", " 3/4"},
	{ Key_INV_QUOT_MARK, "iquest", "?"},
	{ Key_A_GRAVE,       "Agrave", "A`"},
	{ Key_A_ACUTE,       "Aacute", "A'"},
	{ Key_A_CIRCUMFLEX,  "Acirc",  "A^"},
	{ Key_A_TILDE,       "Atilde", "A"},
	{ Key_A_DIAERESIS,   "Auml",   "A:"},
	{ Key_A_RING,        "Aring",  "AA"},
	{ Key_AE,            "AElig",  "AE"},     /* shortcut missing */
	{ Key_C_CEDILLA,     "Ccedil", "C,"},
	{ Key_E_GRAVE,       "Egrave", "E`"},
	{ Key_E_ACUTE,       "Eacute", "E'"},
	{ Key_E_CIRCUMFLEX,  "Ecirc",  "E^"},
	{ Key_E_DIAERESIS,   "Euml",   "E:"},
	{ Key_I_GRAVE,       "Igrave", "I`"},
	{ Key_I_ACUTE,       "Iacute", "I'"},
	{ Key_I_CIRCUMFLEX,  "Icirc",  "I^"},
	{ Key_I_DIAERESIS,   "Iuml",   "I:"},
	{ Key_ETH,           "ETH",    "D-"},       /* shortcut missing */
	{ Key_N_TILDE,       "Ntilde", "N"},
	{ Key_O_GRAVE,       "Ograve", "O`"},
	{ Key_O_ACUTE,       "Oacute", "O'"},
	{ Key_O_CIRCUMFLEX,  "Ocirc",  "O^"},
	{ Key_O_TILDE,       "Otilde", "O~"},
	{ Key_O_DIAERESIS,   "Ouml",   "O:"},
        { Key_MULT_SIGN,     "times",  " *"},
	{ Key_O_SLASH,       "Oslash", "O/"},
	{ Key_U_GRAVE,       "Ugrave", "U`"},
	{ Key_U_ACUTE,       "Uacute", "U'"},
	{ Key_U_CIRCUMFLEX,  "Ucirc",  "U^"},
	{ Key_U_DIAERESIS,   "Uuml",   "U:"},
	{ Key_Y_ACUTE,       "Yacute", "Y'"},
	{ Key_THORN,         "THORN",  "TH"},       /* shortcut missing */
	{ Key_SHARP_S,       "szlig",  "ss"},
	{ Key_a_GRAVE,       "agrave", "a`"},
	{ Key_a_ACUTE,       "aacute", "a'"},
	{ Key_a_CIRCUMFLEX,  "acirc",  "a^"},
	{ Key_a_TILDE,       "atilde", "a"},
	{ Key_a_DIAERESIS,   "auml",   "a:"},
	{ Key_a_RING,        "aring",  "aa"},
	{ Key_ae,            "aelig",  "ae"},        /* shortcut missing */
	{ Key_c_CEDILLA,     "ccedil", "c,"},
	{ Key_e_GRAVE,       "egrave", "e`"},
	{ Key_e_ACUTE,       "eacute", "e'"},
	{ Key_e_CIRCUMFLEX,  "ecirc",  "e^"},
	{ Key_e_DIAERESIS,   "euml",   "e:"},
	{ Key_i_GRAVE,       "igrave", "i`"},
	{ Key_i_ACUTE,       "iacute", "i'"},
	{ Key_i_CIRCUMFLEX,  "icirc",  "i^"},
	{ Key_i_DIAERESIS,   "iuml",   "i:"},
	{ Key_eth,           "eth",    "d-"},       /* shortcut missing */
	{ Key_n_TILDE,       "ntilde", "n"},
	{ Key_o_GRAVE,       "ograve", "o`"},
	{ Key_o_ACUTE,       "oacute", "o'"},
	{ Key_o_CIRCUMFLEX,  "ocirc",  "o^"},
	{ Key_o_TILDE,       "otilde", "o~"},
	{ Key_o_DIAERESIS,   "ouml",   "o:"},
        { Key_DIVISION_SIGN, "divide", "%"},
	{ Key_o_SLASH,       "oslash", "o/"},
	{ Key_u_GRAVE,       "ugrave", "u`"},
	{ Key_u_ACUTE,       "uacute", "u'"},
	{ Key_u_CIRCUMFLEX,  "ucirc",  "u^"},
	{ Key_u_DIAERESIS,   "uuml",   "u:"},
	{ Key_y_ACUTE,       "yacute", "y'"},
	{ Key_thorn,         "thorn",  "th"},       /* shortcut missing */
	{ Key_y_DIAERESIS,   "yuml",   "y:"},
	{ 0,                 "",       ""}
};

/**********************************************************************/

static const char str_cute[] = "cute";
static const char str_ilde[] = "ilde";
static const char str_irc[] = "irc";
static const char str_ml[] = "ml";
static const char str_rave[] = "rave";

/*
 * Riconosce un character entity reference rappresentante un carattere
 * ISO 8859-1 nella stringa *str. Restituisce -1 se non c'e' stato il
 * riconoscimento, altrimenti restituisce il codice ascii 8-bit, e fa
 * puntare *str all'ultimo carattere dell'entity reference.
 */
int cml_entity(const char **str)
{
	enum {
		ENTITY_A, ENTITY_E, ENTITY_I, ENTITY_N, ENTITY_O, ENTITY_U,
		ENTITY_a, ENTITY_ac, ENTITY_c, ENTITY_ce, ENTITY_d, ENTITY_e,
		ENTITY_i, ENTITY_m, ENTITY_mi, ENTITY_n, ENTITY_o, ENTITY_p,
		ENTITY_r, ENTITY_s, ENTITY_t, ENTITY_u, ENTITY_y, ENTITY_NONE
	};
	register const char *p;
	register int curr_state = ENTITY_NONE;

	if (**str != '&')
		return -1;
	for (p = *str+1; *p; p++) {
		switch(*p) {
		case 'A':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_A;
				break;
			default:
				return -1;
			}
			break;
		case 'C':
			switch (curr_state) {
			case ENTITY_NONE: /* &Ccedil */
				if (strncmp(p+1, "cedil", 5))
					return -1;
				*str = p + 5;
				return Key_C_CEDILLA;
			default:
				return -1;
			}
			break;
		case 'E':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_E;
				break;
			case ENTITY_A: /* &AElig */
				if (strncmp(p+1, "lig", 3))
					return -1;
				*str = p + 3;
				return Key_AE;
			default:
				return -1;
			}
			break;
		case 'I':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_I;
				break;
			default:
				return -1;
			}
			break;
		case 'N':
			switch (curr_state) {
			case ENTITY_NONE: /* &Ntilde */
				if (strncmp(p+1, "tilde", 5))
					return -1;
				*str = p + 5;
				return Key_N_TILDE;
			default:
				return -1;
			}
			break;
		case 'O':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_O;
				break;
			default:
				return -1;
			}
			break;
		case 'T':
			switch (curr_state) {
			case ENTITY_NONE: /* &THORN */
				if (strncmp(p+1, "HORN", 4))
					return -1;
				*str = p + 4;
				return Key_THORN;
			case ENTITY_E: /* &ETH */
				if (*(p+1) != 'H')
					return -1;
				*str = p + 1;
				return Key_ETH;
			default:
				return -1;
			}
			break;
		case 'U':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_U;
				break;
			default:
				return -1;
			}
			break;
		case 'Y':
			switch (curr_state) {
			case ENTITY_NONE: /* &Yacute */
				if (strncmp(p+1, "acute", 5))
					return -1;
				*str = p + 5;
				return Key_Y_ACUTE;
			default:
				return -1;
			}
			break;
		case 'a':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_a;
				break;
			case ENTITY_a: /* &aacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_a_ACUTE;
			case ENTITY_e: /* &eacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_e_ACUTE;
			case ENTITY_i: /* &iacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_i_ACUTE;
			case ENTITY_m: /* &macr */
				if (strncmp(p+1, "cr", 2))
					return -1;
				*str = p + 2;
				return Key_MACRON;
			case ENTITY_o: /* &oacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_o_ACUTE;
			case ENTITY_p: /* &para */
				if (strncmp(p+1, "ra", 2))
					return -1;
				*str = p + 2;
				return Key_PILCROW_SIGN;
			case ENTITY_r: /* &raquo */
				if (strncmp(p+1, "quo", 3))
					return -1;
				*str = p + 3;
				return Key_RIGHT_GUILLEM;
			case ENTITY_u: /* &uacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_u_ACUTE;
			case ENTITY_y: /* &yacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_y_ACUTE;
			case ENTITY_A: /* &Aacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_A_ACUTE;
			case ENTITY_E: /* &Eacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_E_ACUTE;
			case ENTITY_I: /* &Iacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_I_ACUTE;
			case ENTITY_O: /* &Oacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_O_ACUTE;
			case ENTITY_U: /* &Uacute */
				if (strncmp(p+1, str_cute, 4))
					return -1;
				*str = p + 4;
				return Key_U_ACUTE;
			default:
				return -1;
			}
			break;
		case 'b':
			switch (curr_state) {
			case ENTITY_NONE: /* &brvbar */
				if (strncmp(p+1, "rvbar", 5))
					return -1;
				*str = p + 5;
				return Key_BROKEN_BAR;
			case ENTITY_n: /* &nbsp */
				if (strncmp(p+1, "sp", 2))
					return -1;
				*str = p + 2;
				return Key_NOBREAK_SPACE;
			default:
				return -1;
			}
			break;
		case 'c':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_c;
				break;
			case ENTITY_a:
				curr_state = ENTITY_ac;
				break;
			case ENTITY_c: /* &ccedil */
				if (strncmp(p+1, "edil", 4))
					return -1;
				*str = p + 4;
				return Key_c_CEDILLA;
			case ENTITY_e: /* &ecirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_e_CIRCUMFLEX;
			case ENTITY_i: /* &icirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_i_CIRCUMFLEX;
			case ENTITY_mi: /* &micro */
				if (strncmp(p+1, "ro", 2))
					return -1;
				*str = p + 2;
				return Key_MICRO_SIGN;
			case ENTITY_o: /* &ocirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_o_CIRCUMFLEX;
			case ENTITY_u: /* &ucirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_u_CIRCUMFLEX;
			case ENTITY_A: /* &Acirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_A_CIRCUMFLEX;
			case ENTITY_E: /* &Ecirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_E_CIRCUMFLEX;
			case ENTITY_I: /* &Icirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_I_CIRCUMFLEX;
			case ENTITY_O: /* &Ocirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_O_CIRCUMFLEX;
			case ENTITY_U: /* &Ucirc */
				if (strncmp(p+1, str_irc, 3))
					return -1;
				*str = p + 3;
				return Key_U_CIRCUMFLEX;
			default:
				return -1;
			}
			break;
		case 'd':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_d;
				break;
			case ENTITY_ce: /* &cedil */
				if (strncmp(p+1, "il", 2))
					return -1;
				*str = p + 2;
				return Key_CEDILLA;
			case ENTITY_mi: /* &middot */
				if (strncmp(p+1, "dot", 3))
					return -1;
				*str = p + 3;
				return Key_MIDDLE_DOT;
			default:
				return -1;
			}
			break;
		case 'e':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_e;
				break;
			case ENTITY_a: /* &aelig */
				if (strncmp(p+1, "lig", 3))
					return -1;
				*str = p + 3;
				return Key_ae;
			case ENTITY_c:
				curr_state = ENTITY_ce;
				break;
			case ENTITY_d: /* &deg */
				if (*(p+1) != 'g')
					return -1;
				*str = p + 1;
				return Key_DEGREE_SIGN;
			case ENTITY_i: /* &iexcl */
				if (strncmp(p+1, "xcl", 3))
					return -1;
				*str = p + 3;
				return Key_INV_EXCL_MARK;
			case ENTITY_r: /* &reg */
				if (*(p+1) != 'g')
					return -1;
				*str = p + 1;
				return Key_REGISTERED;
			case ENTITY_s: /* &sect */
				if (strncmp(p+1, "ct", 2))
					return -1;
				*str = p + 2;
				return Key_SECTION_SIGN;
			case ENTITY_y: /* &yen */
				if (*(p+1) != 'n')
					return -1;
				*str = p + 1;
				return Key_YEN_SIGN;
			default:
				return -1;
			}
			break;
		case 'f':
			switch (curr_state) {
			case ENTITY_NONE: /* &frac (12, 14, 34) */
				if (strncmp(p+1, "rac", 3))
					return -1;
				switch (*(p+4)) {
				case '1': /* &frac1 (2,4) */
					if (*(p+5)=='2') { /* &frac12 */
						*str = p + 5;
						return Key_ONE_HALF;
					} else if (*(p+5)=='4') { /* &frac14 */
						*str = p + 5;
						return Key_ONE_QUARTER;
					}
					return -1;
				case '3': /* &ordm */
					if (*(p+5)=='4') { /* &frac34 */
						*str = p + 5;
						return Key_THREE_QUART;
					}
					return -1;
				}
				return -1;
			default:
				return -1;
			}
			break;
		case 'g':
			switch (curr_state) {
			case ENTITY_a: /* &agrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_a_GRAVE;
			case ENTITY_e: /* &egrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_e_GRAVE;
			case ENTITY_i: /* &igrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_i_GRAVE;
			case ENTITY_o: /* &ograve */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_o_GRAVE;
			case ENTITY_u: /* &ugrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_u_GRAVE;
			case ENTITY_A: /* &Agrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_A_GRAVE;
			case ENTITY_E: /* &Egrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_E_GRAVE;
			case ENTITY_I: /* &Igrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_I_GRAVE;
			case ENTITY_O: /* &Ograve */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_O_GRAVE;
			case ENTITY_U: /* &Ugrave */
				if (strncmp(p+1, str_rave, 4))
					return -1;
				*str = p + 4;
				return Key_U_GRAVE;
			default:
				return -1;
			}
			break;
		case 'h':
			switch (curr_state) {
			case ENTITY_s: /* &shy */
				if (*(p+1) != 'y')
					return -1;
				*str = p + 1;
				return Key_SOFT_HYPHEN;
			case ENTITY_t: /* &thorn */
				if (strncmp(p+1, "orn", 3))
					return -1;
				*str = p + 3;
				return Key_thorn;
			default:
				return -1;
			}
			break;
		case 'i':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_i;
				break;
			case ENTITY_ac: /* &acirc */
				if (strncmp(p+1, "rc", 2))
					return -1;
				*str = p + 2;
				return Key_a_CIRCUMFLEX;
			case ENTITY_d: /* &divide */
				if (strncmp(p+1, "vide", 4))
					return -1;
				*str = p + 4;
				return Key_DIVISION_SIGN;
			case ENTITY_m:
				curr_state = ENTITY_mi;
				break;
			case ENTITY_t: /* &times */
				if (strncmp(p+1, "mes", 3))
					return -1;
				*str = p + 3;
				return Key_MULT_SIGN;
			default:
				return -1;
			}
			break;
		case 'l':
			switch (curr_state) {
			case ENTITY_NONE: /* &laquo */
				if (strncmp(p+1, "aquo", 4))
					return -1;
				*str = p + 4;
				return Key_LEFT_GUILLEM;
			case ENTITY_p: /* &plusmn */
				if (strncmp(p+1, "usmn", 4))
					return -1;
				*str = p + 4;
				return Key_PLUS_MINUS;
			default:
				return -1;
			}
			break;
		case 'm':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_m;
				break;
			case ENTITY_u: /* &uml */
				if (*(p+1) != 'l')
					return -1;
				*str = p + 1;
				return Key_DIAERESIS;
			default:
				return -1;
			}
			break;
		case 'n':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_n;
				break;
			case ENTITY_ce: /* &cent */
				if (*(p+1) != 't')
					return -1;
				*str = p + 1;
				return Key_CENT_SIGN;
			default:
				return -1;
			}
			break;
		case 'o':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_o;
				break;
			case ENTITY_c: /* &copy */
				if (strncmp(p+1, "py", 2))
					return -1;
				*str = p + 2;
				return Key_COPYRIGHT;
			case ENTITY_n: /* &not */
				if (*(p+1) != 't')
					return -1;
				*str = p + 1;
				return Key_NOT_SIGN;
			case ENTITY_p: /* &pound */
				if (strncmp(p+1, "und", 3))
					return -1;
				*str = p + 3;
				return Key_POUND_SIGN;
			default:
				return -1;
			}
			break;
		case 'p':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_p;
				break;
			default:
				return -1;
			}
			break;
		case 'q':
			switch (curr_state) {
			case ENTITY_i: /* &iquest */
				if (strncmp(p+1, "uest", 4))
					return -1;
				*str = p + 4;
				return Key_INV_QUOT_MARK;
			default:
				return -1;
			}
			break;
		case 'r':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_r;
				break;
			case ENTITY_a: /* &aring */
				if (strncmp(p+1, "ing", 3))
					return -1;
				*str = p + 3;
				return Key_a_RING;
			case ENTITY_o: /* &ordm &ordf */
				if (*(p+1)!= 'd')
					return -1;
				switch (*(p+2)) {
				case 'f': /* &ordf */
					*str = p + 2;
					return Key_FEM_ORDINAL;
				case 'm': /* &ordm */
					*str = p + 2;
					return Key_MASC_ORDINAL;
				}
				return -1;
			case ENTITY_A: /* &Aring */
				if (strncmp(p+1, "ing", 3))
					return -1;
				*str = p + 3;
				return Key_A_RING;
			default:
				return -1;
			}
			break;
		case 's':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_s;
				break;
			case ENTITY_o: /* &oslash */
				if (strncmp(p+1, "lash", 4))
					return -1;
				*str = p + 4;
				return Key_o_SLASH;
			case ENTITY_O: /* &Oslash */
				if (strncmp(p+1, "lash", 4))
					return -1;
				*str = p + 4;
				return Key_O_SLASH;
			default:
				return -1;
			}
			break;
		case 't':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_t;
				break;
			case ENTITY_a: /* &atilde */
				if (strncmp(p+1, str_ilde, 4))
					return -1;
				*str = p + 4;
				return Key_a_TILDE;
			case ENTITY_e: /* &eth */
				if (*(p+1) != 'h')
					return -1;
				*str = p + 1;
				return Key_eth;
			case ENTITY_n: /* &ntilde */
				if (strncmp(p+1, str_ilde, 4))
					return -1;
				*str = p + 4;
				return Key_n_TILDE;
			case ENTITY_o: /* &otilde */
				if (strncmp(p+1, str_ilde, 4))
					return -1;
				*str = p + 4;
				return Key_o_TILDE;
			case ENTITY_A: /* &Atilde */
				if (strncmp(p+1, str_ilde, 4))
					return -1;
				*str = p + 4;
				return Key_A_TILDE;
			case ENTITY_O: /* &Otilde */
				if (strncmp(p+1, str_ilde, 4))
					return -1;
				*str = p + 4;
				return Key_O_TILDE;
			default:
				return -1;
			}
			break;
		case 'u':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_u;
				break;
			case ENTITY_a: /* &auml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_a_DIAERESIS;
			case ENTITY_ac: /* &acute */
				if (strncmp(p+1, "te", 2))
					return -1;
				*str = p + 2;
				return Key_ACUTE_ACC;
			case ENTITY_c: /* &curren */
				if (strncmp(p+1, "rren", 4))
					return -1;
				*str = p + 4;
				return Key_CURRENCY_SIGN;
			case ENTITY_e: /* &euml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_e_DIAERESIS;
			case ENTITY_i: /* &iuml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_i_DIAERESIS;
			case ENTITY_o: /* &ouml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_o_DIAERESIS;
			case ENTITY_s: /* &sup (1,2,3) */
				if (*(p+1)!= 'p')
					return -1;
				switch (*(p+2)) {
				case '1': /* &sup1 */
					*str = p + 2;
					return Key_SUPER_ONE;
				case '2': /* &sup2 */
					*str = p + 2;
					return Key_SUPER_TWO;
				case '3': /* &sup3 */
					*str = p + 2;
					return Key_SUPER_THREE;
				}
				return -1;
			case ENTITY_u: /* &uuml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_u_DIAERESIS;
			case ENTITY_y: /* &yuml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_y_DIAERESIS;
			case ENTITY_A: /* &Auml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_A_DIAERESIS;
			case ENTITY_E: /* &Euml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_E_DIAERESIS;
			case ENTITY_I: /* &Iuml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_I_DIAERESIS;
			case ENTITY_O: /* &Ouml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_O_DIAERESIS;
			case ENTITY_U: /* &Uuml */
				if (strncmp(p+1, str_ml, 2))
					return -1;
				*str = p + 2;
				return Key_U_DIAERESIS;
			default:
				return -1;
			}
			break;
		case 'y':
			switch (curr_state) {
			case ENTITY_NONE:
				curr_state = ENTITY_y;
				break;
			default:
				return -1;
			}
			break;
		case 'z':
			switch (curr_state) {
			case ENTITY_s: /* &szlig */
				if (strncmp(p+1, "lig", 3))
					return -1;
				*str = p + 3;
				return Key_SHARP_S;
			default:
				return -1;
			}
			break;
		}
	}
	return -1;
}
