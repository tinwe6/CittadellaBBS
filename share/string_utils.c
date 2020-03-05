/****************************************************************************
 * Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
 *                                                                           *
 * File : string_utils.c                                                     *
 *        string utilities                                                   *
 ****************************************************************************/

#include <ctype.h>
#include "string_utils.h"
#include "iso8859.h"
#include "macro.h"

/*
 * counts the number of words (defined as cluster of characters separated by
 * whitespace) in the string str.
 */
int words_count(const char * str)
{
        int count = 0;
        int index = 0;

        do {
                /* skip whitespace */
                while (isspace(str[index])) {
                        ++index;
                }
                if (str[index] == '\0') {
                        return count;
                }

                while (!isspace(str[index]) && str[index] != '\0') {
                        ++index;
                }
                ++count;
        } while (str[index] != '\0');

        return count;
}


/*
 * removes all spaces from front and back of the string, and collapses
 * multiple spaces between words to a single space
 */
char * strip_spaces(const char * str)
{
        char *stripped;

        CREATE(stripped, char, strlen(str) + 1, 0);

        const char *src = str;
        char *dest = stripped;

        while (*src != 0 && *src == ' ') {
                src++;
        }

        while (true) {
                while (*src != 0 && *src != ' ') {
                        *dest++ = *src++;
                }
                while (*src != 0 && *src == ' ') {
                        src++;
                }
                if (*src != 0) {
                        *dest++ = ' ';
                } else {
                        break;
                }
        }
        *dest = '\0';

        return stripped;
}


/*
 * Very basic check on the real name entered by the user. We want at least
 * two words (name + surname) and allow, because we are generous, up to three
 * middle names. No characters other than letters, spaces, hyphens and
 * apostrophes are allowed.
 * Returns true iff the name is well-formed according to the above rules.
 */
bool is_valid_full_name(const char * name)
{
        int words;
        const char *c = name;

        words = words_count(name);
        if (words < 2 || words > 5) {
                return false;
        }
        bool is_first = true;
        int word_len = 0;
        while (*c != 0) {
                if (*c == ',') {
                        return (!strcmp(c + 1, " Jr.")
                                || !strcmp(c + 1, " Jr"));
                }
                if (*c == '.' && (is_first || word_len != 1
                                  || *(c + 1) != ' ')) {
                        return false;
                }
                if (*c == '-') {
                        if (c == name || *(c - 1) == '-' || *(c - 1) == ' '
                            || *(c - 1) == '\'' || *(c + 1) == ' '
                            || *(c + 1) == '\'') {
                                return false;
                        }
                }
                if (*c == '\'' && *(c + 1) == '\'') {
                        return false;
                }
                if (*c != ' ' && !is_letter(*c) && *c != '\''
                    && *c != '.' && *c != '-') {
                        return false;
                }
                if (*c == ' ') {
                        if (is_first) {
                                is_first = false;
                                if (word_len < 2) {
                                        return false;
                                }
                        }
                        word_len = 0;
                } else {
                        ++word_len;
                }
                ++c;
        }
        if (word_len < 2) {
                return false;
        }
        return true;
}


/*
 * Basic check on validity of domain names. Should catch most invalid domain
 * names.
 */
bool is_valid_domain(const char *str)
{
        int levels = 1;

        /* full domain name cannot exceed 253 chars in its textual repr. */
        if (strlen(str) > 253) {
                return false;
        }

        const char *start = str;
        const char *end = start;
        while (end != 0) {
                /* labels: only letters, digits and hyphens allowed */
                while (*end != 0 && (isalnum(*end) || *end == '-')) {
                        end++;
                }
                /* max 63 chars per label */
                if (end == start || end - start > 63) {
                        return false;
                }
                /* labels cannot start or end with a hyphen */
                if (*start == '-' || *(end - 1) == '-') {
                        return false;
                }
                if (*end == 0) {
                        break;
                }
                if (*end != '.') {
                        return false;
                }
                ++levels;
                ++end;
                start = end;
        }
        if (end == start) {
                return false;
        }

        /* The top-level domain cannot be all numeric. */
        bool ok = false;
        for ( ; start < end; ++start) {
                if (isalpha(*start)) {
                        ok = true;
                        break;
                }
        }
        if (!ok) {
                return false;
        }

        if (levels < 2 || levels > 127) {
                /* The full domain name may have up to 127 levels. */
                return false;
        }

        return true;
}

static bool is_allowed_in_email(char c)
{
        const char allowed[] = "!#$%&'*+-/=?^_`.{|}~";

        if (isalnum(c)) {
                return true;
        }

        for (size_t i = 0; i < sizeof allowed / sizeof(char); ++i) {
                if (c == allowed[i]) {
                        return true;
                }
        }
        return false;
}

/*
 * Check if the email looks valid.
 * Based on RFC 3696 specs.
 */
bool is_valid_email(const char *str)
{
        const char *last_at_sign = NULL;
        const char *domain = NULL;
        const char *c = str;

        while (*c != 0) {
                if (*c == '@') {
                        last_at_sign = c;
                }
                ++c;
        }

        /* false if no '@' or no username */
        if (last_at_sign == NULL || last_at_sign == str) {
                return false;
        }

        domain = last_at_sign + 1;
        if (!is_valid_domain(domain)) {
                return false;
        }

        /* check the local part */
        if (last_at_sign - str > 64) { /* max 64 chars */
                return false;
        }
        c = str;
        bool quoted = false;
        bool double_quoted = false;
        while (c < last_at_sign) {
                if (quoted) {
                        quoted = false;
                } else if (*c == '\\') {
                        quoted = true;
                } else if (*c == '"') {
                        double_quoted = !double_quoted;
                } else if (double_quoted) {
                        /* noop */
                } else if (*c == '.' && (c == str || c == last_at_sign - 1
                                         || *(c + 1) == '.')) {
                        return false;
                } else {
                        if (!is_allowed_in_email(*c)) {
                                return false;
                        }
                }
                ++c;
        }

        if (double_quoted) {
                return false;
        }

        return true;
}


/****************************************************************************/
/* Tests                                                                    */
/****************************************************************************/

#ifdef TESTS

#include <assert.h>
#include <stdio.h>

static bool test_strip(void)
{
        const char test_name[] = "strip_spaces";
        typedef struct test {
                const char *in;
                const char *out;
        } test;
        test tests[] = {
                {"", ""},
                {"a", "a"},
                {" a", "a"},
                {"a ", "a"},
                {" a ", "a"},
                {"   a     ", "a"},
                {"a b", "a b"},
                {"a  b", "a b"},
                {" a  b ", "a b"},
                {"  a   b  ", "a b"},
                {" Hello   World!   ", "Hello World!"},
        };
        int passed = 0;
        int failed = 0;
        size_t ntests = sizeof tests / sizeof(test);

        for (size_t i = 0; i < ntests; ++i) {
                char *result = strip_spaces(tests[i].in);
                if (strcmp(result, tests[i].out)) {
                        printf("FAIL %s test %lu: "
                               "'%s' should give '%s' but got '%s'\n",
                               test_name, i, tests[i].in, tests[i].out,
                               result);
                        ++failed;
                } else {
                        ++passed;
                }
                Free(result);
        }
        if (failed) {
                printf("%s: %d/%lu tests failed.\n", test_name, failed,
                       ntests);
        } else {
                printf("%s: %d/%lu tests passed.\n", test_name, passed,
                       ntests);
        }

        assert(failed == 0);

        return !failed;
}


static bool test_words_count(void)
{
        const char test_name[] = "words_count";
        typedef struct test {
                const char *in;
                const int out;
        } test;
        test tests[] = {
                {"", 0},
                {"a", 1},
                {"a  ", 1},
                {"  a", 1},
                {" a ", 1},
                {"abc", 1},
                {"Abc", 1},
                {"Hello World!", 2},
                {"  word1, word2, and word3  ", 4},
        };
        int passed = 0;
        int failed = 0;
        size_t ntests = sizeof tests / sizeof(test);

        for (size_t i = 0; i < ntests; ++i) {
                int result = words_count(tests[i].in);
                if (result != tests[i].out) {
                        printf("FAIL %s test %lu: "
                               "'%s' should give %d but got %d\n",
                               test_name, i, tests[i].in, tests[i].out,
                               result);
                        ++failed;
                } else {
                        ++passed;
                }
        }
        if (failed) {
                printf("%s: %d/%lu tests failed.\n", test_name, failed,
                       ntests);
        } else {
                printf("%s: %d/%lu tests passed.\n", test_name, passed,
                       ntests);
        }

        assert(failed == 0);

        return !failed;
}


static bool test_is_valid_full_name(void)
{
        const char test_name[] = "is_valid_full_name";
        typedef struct test {
                const char *in;
                const bool out;
        } test;
        test tests[] = {
                {"", false},
                {"A", false},
                {"A B", false},
                {"Tizio", false},
                {"A Caio", false},
                {"Sempronio B", false},
                {"Caio, Jr. Sempronio", false},
                {"Caio Sempronio, Jr.", true},
                {"Tizio C. Sempronio", true},
                {"Tizio C.. Sempronio", false},
                {"Tizio C.C Sempronio", false},
                {"Tizio .C Sempronio", false},
                {"Tizio Caio. Sempronio", false},
                {"Tizio .Caio Sempronio", false},
                {"T. Caio Sempronio", false},
                {"Tizio Caio S.", false},
                {"Tizio Caio S. ", false},
                {"Tizio-Caio Sempronio", true},
                {"Tizio--Caio Sempronio", false},
                {"-Tizio Caio Sempronio", false},
                {"Tizio- Caio Sempronio", false},
                {"Tizio - Caio Sempronio", false},
                {"Tizio -Caio Sempronio", false},
                {"Tizio Caio- Sempronio", false},
                {"Tizio 'Caio", true},
                {"Tizio ''Caio", false},
                {"Tizio Ca''io", false},
                {"Paperon de' Paperoni", true},
                {"Albert", false},
                {"Subrahmanyan Chandrasekhar", true},
                {"Sir Isaac Newton", true},
                {"Alexei Alexeyevich Abrikosov", true},
                {"Stephen L. Adler", true},
                {"Stephen L Adler", true},
                {"Paul A. M. Dirac", true},
                {"Paul A M Dirac", true},
                {"Mathias d'Arras", true},
                {"Norman Foster Ramsey, Jr.", true},
                {"John Lennard-Jones", true},
                {"Hannes Olof", true},
                {"Gerardus 't Hooft", true},
                {"Yuval Ne'eman", true},
                {"Yakov Borisovich Zel'dovich", true},
                {"Johannes Diderik van der Waals", true},
                {"Martinus J. G. Veltman", true},
                {"John Richard Gott III", true},
        };
        int passed = 0;
        int failed = 0;
        size_t ntests = sizeof tests / sizeof(test);

        for (size_t i = 0; i < ntests; ++i) {
                bool result = is_valid_full_name(tests[i].in);
                if (result != tests[i].out) {
                        printf("FAIL %s test %lu: "
                               "'%s' should give %d but got %d\n",
                               test_name, i, tests[i].in, tests[i].out,
                               result);
                        ++failed;
                } else {
                        ++passed;
                }
        }
        if (failed) {
                printf("%s: %d/%lu tests failed.\n", test_name, failed,
                       ntests);
        } else {
                printf("%s: %d/%lu tests passed.\n", test_name, passed,
                       ntests);
        }

        assert(failed == 0);

        return !failed;
}

static bool test_is_valid_domain(void)
{
        const char test_name[] = "is_valid_domain";
        typedef struct test {
                const char *in;
                const bool out;
        } test;
        test tests[] = {
                {"", false},
                {"a", false},
                {"a.", false},
                {"a.", false},
                {".a", false},
                {"a.1", false},
                {"1.a", true},
                {"a.a", true},
                {".a.a", false},
                {"aaa..bbb", false},
                {"aaa.bbb..", false},
                {"with-hyphen.com", true},
                {"no-ending-.com", false},
                {"-no-startgin.com", false},
                {"domain.-.wrong", false},
                {"123.digitsonly", true},
                {"digitsonly.123.ok", true},
                {"wrong-ending.digitsonly.123", false},
                {"digits.d123", true},
                {"digits.123d456", true},
                {"digits.123d", true},
{"012345678901234567890123456789012345678901234567890123456789abc.63chars",
                 true},
{"012345678901234567890123456789012345678901234567890123456789abcd.64chars",
                 false},
        };
        int passed = 0;
        int failed = 0;
        size_t ntests = sizeof tests / sizeof(test);

        for (size_t i = 0; i < ntests; ++i) {
                bool result = is_valid_domain(tests[i].in);
                if (result != tests[i].out) {
                        printf("FAIL %s test %lu: "
                               "'%s' should give  %d but got %d\n",
                               test_name, i, tests[i].in, tests[i].out,
                               result);
                        ++failed;
                } else {
                        ++passed;
                }
        }
        if (failed) {
                printf("%s: %d/%lu tests failed.\n", test_name, failed,
                       ntests);
        } else {
                printf("%s: %d/%lu tests passed.\n", test_name, passed,
                       ntests);
        }

        assert(failed == 0);

        return !failed;
}



static bool test_is_valid_email(void)
{
        const char test_name[] = "is_valid_email";
        typedef struct test {
                const char *in;
                const bool out;
        } test;
        test tests[] = {
                {"", false},
                {"cittadella.bbs", false},
                {"bbs@cittadellabbs.it", true},
                {"a@a", false},
                {"a@a.a", true},
                {"NotAnEmail", false},
                {"@NotAnEmail", false},
                {".bbs@cittadella.it", false},
                {"bbs.@cittadella.it", false},
                {"bbs.cittadella@cittadella.it", true},
                {"bbs..cittadella@cittadella.it", false},
                {"bbs cittadella@cittadella.it", false},
                {"\"bbs@cittadella\"@cittadella.it", true},
                {".@allowed.no", false},
                {"Abc\\@def@example.com", true},
                {"Fred\\ Bloggs@example.com", true},
                {"Joe.\\Blow@example.com", true},
                {"\"Abc@def\"@example.com", true},
                {"\"Fred Bloggs\"@example.com", true},
                {"user+mailbox@example.com", true},
                {"customer/department=shipping@example.com", true},
                {"$A12345@example.com", true},
                {"!def!xyz%abc@example.com", true},
                {"_somename@example.com", true},
                {"foo bar@foobar.com", false},
                {"\"test\\blah\"@example.com", true},
                {"\"control chars\b\r\n\t\"@are.allowed", true},
                {"\\\"notgood\r\\\"@fake.news", false},
                {"~@funny.email", true},
                {"\"no\"good\"@email.com", false},
                {"\"this\\\"good\"@email.com", true},
{"abcd012345678901234567890123456789012345678901234567890123456789@64.ok.org",
                 true},
{"abcde012345678901234567890123456789012345678901234567890123456789@65.bad.io",
                 false},
        };
        int passed = 0;
        int failed = 0;
        size_t ntests = sizeof tests / sizeof(test);

        for (size_t i = 0; i < ntests; ++i) {
                bool result = is_valid_email(tests[i].in);
                if (result != tests[i].out) {
                        printf("FAIL %s test %lu: "
                               "'%s' should give %d but got %d\n",
                               test_name, i, tests[i].in, tests[i].out,
                               result);
                        ++failed;
                } else {
                        ++passed;
                }
        }
        if (failed) {
                printf("%s: %d/%lu tests failed.\n", test_name, failed,
                       ntests);
        } else {
                printf("%s: %d/%lu tests passed.\n", test_name, passed,
                       ntests);
        }

        assert(failed == 0);

        return !failed;
}


void test_string_utils(void)
{
        test_strip();
        test_words_count();
        test_is_valid_full_name();
        test_is_valid_domain();
        test_is_valid_email();
}
#endif /* TESTS */

#if 0
int main(void) {
        test_string_utils();
}
#endif
