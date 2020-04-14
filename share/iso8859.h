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
* Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : iso8859.h                                                          *
*        gestione input da utente                                           *
****************************************************************************/
#ifndef _ISO8859_H
#define _ISO8859_H   1

/* Codici ASCII */

#define Ctrl(c) ((c) - 'A' + 1)
#define Key_BS             8
#define Key_TAB            9
#define Key_LF            10
#define Key_CR            13
#define Key_ESC           27
#define Key_DEL          127

/* ISO 8859-1 Characters */
#define Key_NOBREAK_SPACE 160 /* &nbsp   */
#define Key_INV_EXCL_MARK 161 /* &iexcl  */
#define Key_CENT_SIGN     162 /* &cent   */
#define Key_POUND_SIGN    163 /* &pound  */
#define Key_CURRENCY_SIGN 164 /* &curren */
#define Key_YEN_SIGN      165 /* &yen    */
#define Key_BROKEN_BAR    166 /* &brvbar */
#define Key_SECTION_SIGN  167 /* &sect   */
#define Key_DIAERESIS     168 /* &uml    */
#define Key_COPYRIGHT     169 /* &copy   */
#define Key_FEM_ORDINAL   170 /* &ordf   */
#define Key_LEFT_GUILLEM  171 /* &laquo  */
#define Key_NOT_SIGN      172 /* &not    */
#define Key_SOFT_HYPHEN   173 /* &shy    */
#define Key_REGISTERED    174 /* &reg    */
#define Key_MACRON        175 /* &macr   */
#define Key_DEGREE_SIGN   176 /* &deg    */
#define Key_PLUS_MINUS    177 /* &plusmn */
#define Key_SUPER_TWO     178 /* &sup2   */
#define Key_SUPER_THREE   179 /* &sup3   */
#define Key_ACUTE_ACC     180 /* &acute  */
#define Key_MICRO_SIGN    181 /* &micro  */
#define Key_PILCROW_SIGN  182 /* &para   */
#define Key_MIDDLE_DOT    183 /* &middot */
#define Key_CEDILLA       184 /* &cedil  */
#define Key_SUPER_ONE     185 /* &sup1   */
#define Key_MASC_ORDINAL  186 /* &ordm   */
#define Key_RIGHT_GUILLEM 187 /* &raquo  */
#define Key_ONE_QUARTER   188 /* &frac14 */
#define Key_ONE_HALF      189 /* &frac12 */
#define Key_THREE_QUART   190 /* &frac34 */
#define Key_INV_QUOT_MARK 191 /* &iquest */
#define Key_A_GRAVE       192 /* &Agrave */
#define Key_A_ACUTE       193 /* &Aacute */
#define Key_A_CIRCUMFLEX  194 /* &Acirc  */
#define Key_A_TILDE       195 /* &Atilde */
#define Key_A_DIAERESIS   196 /* &Auml   */
#define Key_A_RING        197 /* &Aring  */
#define Key_AE            198 /* &AElig  */
#define Key_C_CEDILLA     199 /* &Ccedil */
#define Key_E_GRAVE       200 /* &Egrave */
#define Key_E_ACUTE       201 /* &Eacute */
#define Key_E_CIRCUMFLEX  202 /* &Ecirc  */
#define Key_E_DIAERESIS   203 /* &Euml   */
#define Key_I_GRAVE       204 /* &Igrave */
#define Key_I_ACUTE       205 /* &Iacute */
#define Key_I_CIRCUMFLEX  206 /* &Icirc  */
#define Key_I_DIAERESIS   207 /* &Iuml   */
#define Key_ETH           208 /* &ETH    */
#define Key_N_TILDE       209 /* &Ntilde */
#define Key_O_GRAVE       210 /* &Ograve */
#define Key_O_ACUTE       211 /* &Oacute */
#define Key_O_CIRCUMFLEX  212 /* &Ocirc  */
#define Key_O_TILDE       213 /* &Otilde */
#define Key_O_DIAERESIS   214 /* &Ouml   */
#define Key_MULT_SIGN     215 /* &times  */
#define Key_O_SLASH       216 /* &Oslash */
#define Key_U_GRAVE       217 /* &Ugrave */
#define Key_U_ACUTE       218 /* &Uacute */
#define Key_U_CIRCUMFLEX  219 /* &Ucirc  */
#define Key_U_DIAERESIS   220 /* &Uuml   */
#define Key_Y_ACUTE       221 /* &Yacute */
#define Key_THORN         222 /* &THORN  */
#define Key_SHARP_S       223 /* &szlig  */
#define Key_a_GRAVE       224 /* &agrave */
#define Key_a_ACUTE       225 /* &aacute */
#define Key_a_CIRCUMFLEX  226 /* &acirc  */
#define Key_a_TILDE       227 /* &atilde */
#define Key_a_DIAERESIS   228 /* &auml   */
#define Key_a_RING        229 /* &aring  */
#define Key_ae            230 /* &aelig  */
#define Key_c_CEDILLA     231 /* &ccedil */
#define Key_e_GRAVE       232 /* &egrave */
#define Key_e_ACUTE       233 /* &eacute */
#define Key_e_CIRCUMFLEX  234 /* &ecirc  */
#define Key_e_DIAERESIS   235 /* &euml   */
#define Key_i_GRAVE       236 /* &igrave */
#define Key_i_ACUTE       237 /* &iacute */
#define Key_i_CIRCUMFLEX  238 /* &icirc  */
#define Key_i_DIAERESIS   239 /* &iuml   */
#define Key_eth           240 /* &eth    */
#define Key_n_TILDE       241 /* &ntilde */
#define Key_o_GRAVE       242 /* &ograve */
#define Key_o_ACUTE       243 /* &oacute */
#define Key_o_CIRCUMFLEX  244 /* &ocirc  */
#define Key_o_TILDE       245 /* &otilde */
#define Key_o_DIAERESIS   246 /* &ouml   */
#define Key_DIVISION_SIGN 247 /* &divide */
#define Key_o_SLASH       248 /* &oslash */
#define Key_u_GRAVE       249 /* &ugrave */
#define Key_u_ACUTE       250 /* &uacute */
#define Key_u_CIRCUMFLEX  251 /* &ucirc  */
#define Key_u_DIAERESIS   252 /* &uuml   */
#define Key_y_ACUTE       253 /* &yacute */
#define Key_thorn         254 /* &thorn  */
#define Key_y_DIAERESIS   255 /* &yuml   */

/* char is signed long on some platforms, we first cast it to unsigned char */
#define is_isoch(c) (((unsigned int)(c) >= Key_NOBREAK_SPACE)          \
		     && ((unsigned int)(c) <= Key_y_DIAERESIS))

#define is_letter(c) (((c >= 'A') && (c <= 'Z'))                       \
                      || ((c >= 'a') && (c <= 'z'))		       \
                      || (((unsigned int)(c) >= Key_A_GRAVE)           \
                          && ((unsigned int)(c) <= Key_O_DIAERESIS ))  \
                      || (((unsigned int)(c) >= Key_O_SLASH)           \
                          && ((unsigned int)(c) <= Key_o_DIAERESIS))   \
                      || ((unsigned int)(c) >= Key_o_SLASH)            \
                      )

#endif /* iso8859.h */
