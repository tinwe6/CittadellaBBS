/****************************************************************************
 * Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File : string_utils.h                                                     *
 *        string utilities                                                   *
 ****************************************************************************/

#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H 1

#include <stdbool.h>

int words_count(const char * str);
char * strip_spaces(const char * str);
bool is_valid_full_name(const char * name);
bool is_valid_domain(const char *str);
bool is_valid_email(const char *str);
void test_string_utils(void);

#endif /* string_utils.h */
