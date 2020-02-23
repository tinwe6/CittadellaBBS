/****************************************************************************
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cittacfg.c                                                         *
*        Funzioni per gestire i file di configurazione.                     *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cittacfg.h"
#include "utility.h"
#include "macro.h"

#undef CFG_VERBOSE /* define for debugging purposes... */

#ifdef WINDOWS       /* Windows path for config file   */
#define CFG_FILE_USER   "/cittaclient.ini"
#define CFG_FILE_USER2  "/windows/cittaclient.ini"
#define CFG_FILE_SYSTEM "/system/cittaclient.ini"
#else
#ifdef MACOSX        /* MacOS/X path for config file   */
#define CFG_FILE_USER   "~/.cittaclientrc"
#define CFG_FILE_USER2  "~/.cittaclient/cittaclient.rc"
#define CFG_FILE_USER3  "~/Library/Preferences/cittaclient.rc"
#define CFG_FILE_USER4  "~/Library/Application Support/cittaclient/cittaclient.rc"
#define CFG_FILE_SYSTEM "/etc/cittaclient.rc"
#else                /* UNIX path for config file      */
#define CFG_FILE_USER   "~/.cittaclientrc"
#define CFG_FILE_USER2  "~/.cittaclient/cittaclient.rc"
#define CFG_FILE_SYSTEM "/etc/cittaclient.rc"
#endif
#endif

#define ERR_MSG "Error in configuration file, line %d : "

#define CFG_BOOL        1
#define CFG_CHAR        2
#define CFG_INT         3
#define CFG_LONG        4
#define CFG_STRING      5
#define CFG_PATH        6
#define CFG_FUNCTION    7

#define CFG_OK          0
#define CFG_ERR_EOF     1
#define CFG_ERR_STR     2
#define CFG_ERR_CHR     3

#define PARSE_OK        0
#define PARSE_UNKNOWN   1
#define PARSE_STRING    2

#ifdef LOCAL
/* Vettore dei path ai file di configurazione standard. Il client legge
 * il primo di questi che trova.                                        */

static const char * cfg_stdrc[] = { CFG_FILE_USER,
				    CFG_FILE_USER2,
#ifdef MACOSX
				    CFG_FILE_USER3,
				    CFG_FILE_USER4,
#endif
				    CFG_FILE_SYSTEM, NULL };

static int cfg_line_num; /* Here is stored the line number being parsed. */
#endif

typedef union {
	char *c;
	int  *i;
	long *l;
	char **s;
	void (*parse) (char *arg);
} cfg_varptr;

struct config_t {
	char *token;
	cfg_varptr var;
	char type;
};

struct cfg_opt {
	char *opt;
	struct cfg_opt *next;
};

struct client_cfg client_cfg;
static struct cfg_opt *cfg_opt = NULL, *cfg_opt_first = NULL;

struct config_t config[] = {
	  {"HOST",          { (void *)&client_cfg.host },          CFG_STRING},
	  {"PORT",          { (void *)&client_cfg.port },          CFG_INT },
	  {"USER",          { (void *)&client_cfg.name },          CFG_STRING},
	  {"PASSWORD",      { (void *)&client_cfg.password },      CFG_STRING},
	  {"NO_BANNER",     { (void *)&client_cfg.no_banner },     CFG_BOOL},
	  {"NRIGHE",        { (void *)&client_cfg.nrighe},         CFG_INT },
	  {"EDITOR",        { (void *)&client_cfg.editor},         CFG_PATH},
	  {"AUTOSAVE_FILE", { (void *)&client_cfg.autosave_file},  CFG_PATH},
	  {"AUTOSAVE_TIME", { (void *)&client_cfg.autosave_time},  CFG_INT },
	  {"DOING",         { (void *)&client_cfg.doing },         CFG_STRING},
	  {"AUTO_LOG_X",    { (void *)&client_cfg.auto_log_x },    CFG_BOOL},
	  {"AUTO_LOG_MAIL", { (void *)&client_cfg.auto_log_mail }, CFG_BOOL},
	  {"AUTO_LOG_CHAT", { (void *)&client_cfg.auto_log_chat }, CFG_BOOL},
	  {"DUMP_FILE",     { (void *)&client_cfg.dump_file },     CFG_PATH},
	  {"LOG_FILE_X",    { (void *)&client_cfg.log_file_x },    CFG_PATH},
	  {"LOG_FILE_MAIL", { (void *)&client_cfg.log_file_mail }, CFG_PATH},
	  {"LOG_FILE_CHAT", { (void *)&client_cfg.log_file_chat }, CFG_PATH},
	  {"TMP_DIR",       { (void *)&client_cfg.tmp_dir},        CFG_PATH},
          {"USE_COLORS",    { (void *)&client_cfg.use_colors},     CFG_BOOL},
          {"MAX_XLOG",      { (void *)&client_cfg.max_xlog},       CFG_INT },
          {"TABSIZE",       { (void *)&client_cfg.tabsize},        CFG_INT },
	  {"SHELL",         { (void *)&client_cfg.shell},          CFG_PATH},
          {"COMPRESSIONE",  { (void *)&client_cfg.compressione},   CFG_STRING},
          {"DEBUG",         { (void *)&client_cfg.debug},          CFG_BOOL},
	  {"BROWSER",       { (void *)&client_cfg.browser },       CFG_PATH},
	  {"PICTUREAPP",    { (void *)&client_cfg.picture },       CFG_PATH},
	  {"MOVIEAPP",      { (void *)&client_cfg.movie },         CFG_PATH},
	  {"AUDIOAPP",      { (void *)&client_cfg.audio },         CFG_PATH},
	  {"PDFAPP",        { (void *)&client_cfg.pdfapp },        CFG_PATH},
	  {"PSAPP",         { (void *)&client_cfg.psapp },         CFG_PATH},
	  {"DVIAPP",        { (void *)&client_cfg.dviapp },        CFG_PATH},
	  {"RTFAPP",        { (void *)&client_cfg.rtfapp },        CFG_PATH},
	  {"DOCAPP",        { (void *)&client_cfg.docapp },        CFG_PATH},
	  {"TERMBROWSER",   { (void *)&client_cfg.termbrowser },   CFG_BOOL},
	  {"DOWNLOAD_DIR",  { (void *)&client_cfg.download_dir },  CFG_PATH},
/*	{"COLOR",         { (void *)&client_cfg.parse_color }, CFG_FUNCTION},*/
	  {"", { (void *) 0 }, 0}
};

#define LEGGO_FCONF  _("Leggo file di configurazione %s\n")
#define NO_FCONF     _("Nessun file di configurazione. Uso default.\n")

/* Prototipi funzioni statiche in cittacfg.c */
static void cfg_init(void);
#ifdef LOCAL
static void cfg_free_opt(void);
static int cfg_parse(const char *buf, const size_t bufsize, int parse_err);
static void cfg_err(const char *msg);
static FILE * cfg_open(void);
static int cfg_getline(char *buf, size_t *bufsize, FILE *fp);
static char * cfg_strdup(const char *str);
static int cfg_parse_line(const char *buf, size_t toklen);
static size_t cfg_toklen(const char *buf, size_t buflen);
#endif
#ifdef CFG_VERBOSE
static void cfg_print_client_cfg(const struct client_cfg arg);
#endif
/*****************************************************************************/

/* Read, parse and apply the configuration file */
void cfg_read(char *rcfile, bool no_rc)
{
#ifdef LOCAL
	FILE *fp;
	char buf[LBUF], *filename;
	size_t bufsize = LBUF;
	int fine, ret;
#else
	IGNORE_UNUSED_PARAMETER(rcfile);
	IGNORE_UNUSED_PARAMETER(no_rc);
#endif

	cfg_init();

#ifdef LOCAL
	if (!no_rc) {
		/* Find & Open configuration file */
		if (rcfile) {
			filename = interpreta_tilde_dir(rcfile);
			fp = fopen(filename, "r");
			if (filename)
				Free(filename);
			if (fp != NULL) {
				fprintf(stderr, LEGGO_FCONF, CFG_FILE_USER);
			} else
				fprintf(stderr, _("\nFile di configurazione %s non trovato.\nUso default.\n"), rcfile);
		} else {
		        /* Cerca i file di configurazione standard. */
			fp = cfg_open();
		}

		if (fp != NULL) {
			/* Parse configuration file */
			cfg_line_num = 0;
			do {
				ret = cfg_getline(buf, &bufsize, fp);
				fine = cfg_parse(buf, bufsize, ret);
			} while (!fine);
			
			/* Close configuration file */
			fclose(fp);
		}
	}
	/* Parse command options */
	cfg_opt = cfg_opt_first;
	while (cfg_opt){
		cfg_parse(cfg_opt->opt, strlen(cfg_opt->opt), CFG_OK);
		cfg_opt = cfg_opt->next;
	};
	cfg_free_opt();
#endif

#ifdef CFG_VERBOSE
	cfg_print_client_cfg(client_cfg);
#endif
}

/*
 * Add a command line option.
 */
void cfg_add_opt(const char *buf)
{
	struct cfg_opt *opt;

	CREATE(opt, struct cfg_opt, 1, 0);
	opt->opt = strdup(buf);
	opt->next = NULL;
	if (cfg_opt_first == NULL) {
		cfg_opt_first = opt;
		cfg_opt = opt;
	} else {
		cfg_opt->next = opt;
		cfg_opt = opt;
	}
}

/****************************************************************************/

static void cfg_init(void)
{
	client_cfg.host          = strdup(HOST_DFLT);
	client_cfg.port          = PORTA_DFLT;
	client_cfg.name          = strdup("");
	client_cfg.password      = strdup("");
	client_cfg.no_banner     = false;
	client_cfg.dump_file     = interpreta_tilde_dir(DUMP_FILE_DFLT);
	client_cfg.log_file_x    = interpreta_tilde_dir(LOG_FILE_X_DFLT);
	client_cfg.log_file_mail = interpreta_tilde_dir(LOG_FILE_MAIL_DFLT);
	client_cfg.log_file_chat = interpreta_tilde_dir(LOG_FILE_CHAT_DFLT);
	client_cfg.auto_log_x    = false;
	client_cfg.auto_log_mail = false;
	client_cfg.auto_log_chat = false;
	client_cfg.doing         = strdup("");
	client_cfg.tmp_dir       = interpreta_tilde_dir(TMPDIR_DFLT);
	client_cfg.nrighe        = NRIGHE_DFLT;
	client_cfg.editor        = strdup(EDITOR_DFLT);
	client_cfg.autosave_file = interpreta_tilde_dir(AUTOSAVE_FILE);
	client_cfg.autosave_time = AUTOSAVE_TIME_DFLT;
	client_cfg.use_colors    = false;
	client_cfg.max_xlog      = MAX_XLOG_DFLT;
	client_cfg.tabsize       = TABSIZE_DFLT;
	client_cfg.shell         = strdup(SHELL_DFLT);
	client_cfg.compressione  = strdup("");
	client_cfg.browser       = strdup("");
	client_cfg.picture       = strdup("");
	client_cfg.movie         = strdup("");
	client_cfg.audio         = strdup("");
	client_cfg.pdfapp        = strdup("");
	client_cfg.psapp         = strdup("");
	client_cfg.dviapp        = strdup("");
	client_cfg.rtfapp        = strdup("");
	client_cfg.docapp        = strdup("");
	client_cfg.termbrowser   = false;
	client_cfg.download_dir  = interpreta_tilde_dir(DOWNLOAD_DIR_DFLT);
}

#ifdef LOCAL
static void cfg_free_opt(void)
{
	struct cfg_opt *punto, *next;

	for (punto = cfg_opt_first; punto; punto = next) {
		next = punto->next;
		Free(punto->opt);
		Free(punto);
	}
}

static int cfg_parse(const char *buf, const size_t bufsize, int parse_err)
{
	switch (parse_err) {
	case CFG_OK:
		switch (cfg_parse_line(buf, bufsize)) {
		case PARSE_UNKNOWN:
			cfg_err("Unknown token");
			break;
		case PARSE_STRING:
			cfg_err("Argument must be a string");
			break;
		}
		break;
	case CFG_ERR_EOF:
		return 1;
		break;
	case CFG_ERR_STR:
		cfg_err("Corrupted string");
		break;
	case CFG_ERR_CHR:
		cfg_err("Forbidden character");
		break;
	}
#ifdef CFG_VERBOSE
#ifdef MACOSX
	printf("%-60s Token Len: %ld\n", buf, cfg_toklen(buf, bufsize));
#else
	printf("%-60s Token Len: %d\n", buf, cfg_toklen(buf, bufsize));
#endif
#endif
	return 0;
}

static void cfg_err(const char *msg)
{
	fprintf(stderr, ERR_MSG "%s.\n", cfg_line_num, msg);
}

/*
 * Searches and opens a configuration file. It looks, in order, for
 * ~/.cittaclientrc - ~/.cittaclient/cittaclient.rc - /etc/cittaclient.rc
 * Returns file pointer to the file, if the file is opened, NULL otherwise.
 */
static FILE * cfg_open(void)
{
	FILE *fp;
	char *filename;
	int i = 0;

	while (cfg_stdrc[i]) {
		filename = interpreta_tilde_dir(cfg_stdrc[i]);
		fp = fopen(filename, "r");
		if (filename)
			Free(filename);
		if (fp != NULL) {
			fprintf(stderr, LEGGO_FCONF, cfg_stdrc[i]);
			return fp;
		}
		i++;
	}

	fprintf(stderr, NO_FCONF);
	return NULL; /* No config file found */

#if 0 /* questo si puo` eliminare... */
	filename = interpreta_tilde_dir(CFG_FILE_USER);
	fp = fopen(filename, "r");
	if (filename)
		Free(filename);
        if (fp != NULL) {
		fprintf(stderr, LEGGO_FCONF, CFG_FILE_USER);
		return fp;
	}

	filename = interpreta_tilde_dir(CFG_FILE_SYSTEM);
	fp = fopen(filename, "r");
	if (filename)
		Free(filename);
        if (fp != NULL) {
		fprintf(stderr, LEGGO_FCONF, CFG_FILE_SYSTEM);
		return fp;
	}

	fprintf(stderr, NO_FCONF);
	return NULL; /* No config file found */
#endif
}

/*
 * Length of the first token in string buf[].
 * The token ends with a space or 
 */
static size_t cfg_toklen(const char *buf, size_t buflen)
{
	size_t toklen = 0;
	register const char *p;

	p = buf;
	while ((*p != ' ') && (*p != 0) && (toklen < buflen)) {
		p++;
		toklen++;
	}
	return toklen;
}

/* 
 * Evaluates the token in in buf and assigns the value in the array config[]
 * Returns PARSE_OK, or an error code PARSE_.
 */
static int cfg_parse_line(const char *buf, size_t bufsize)
{
	const char *arg;
	char *tmp;
	size_t toklen;
	int i = 0;
	bool path;

	toklen = cfg_toklen(buf, bufsize);
	arg = buf + toklen + 1;
	while (*config[i].token != '\0') {
		path = false;
		if (!strncmp(config[i].token, buf, toklen)) {
			switch (config[i].type) {
			case CFG_CHAR:
				*((char *) config[i].var.c) = *arg;
				return PARSE_OK;
			case CFG_INT:
				*((int *) config[i].var.i)=strtol(arg,NULL,10);
				return PARSE_OK;
			case CFG_LONG:
				*((long *)config[i].var.i)=strtol(arg,NULL,10);
				return PARSE_OK;
			case CFG_PATH:
				path = true;
			case CFG_STRING:
				if (*((char **) config[i].var.s) != NULL)
					Free(*((char **) config[i].var.s));
#ifdef CFG_VERBOSE
				printf("ARG %s\n", arg);
#endif
				tmp = cfg_strdup(arg);
				if (path) { /* BRUTTO!!! */
					*((char **)config[i].var.s) =
						interpreta_tilde_dir(tmp);
				} else
					*((char **)config[i].var.s) =
						strdup(tmp);
#ifdef CFG_VERBOSE
				printf("TMP %s CONFIG %s\n", tmp, *((char **)config[i].var.s));
#endif
				Free(tmp);
#ifdef CFG_VERBOSE
				printf("CONFIG %s\n", *((char **)config[i].var.s));
#endif
				if (config[i].var.s == NULL)
					return PARSE_STRING;
				return PARSE_OK;
			case CFG_FUNCTION:
				return PARSE_OK;
			case CFG_BOOL:
				*((bool *) config[i].var.i) = true;
				return PARSE_OK;
			}
		}
		i++;
	}
	return PARSE_UNKNOWN;
}

/* bah... questa non mi piace... */
static char * cfg_strdup(const char *str)
{
	char *tmp;
	size_t len;

	len = strlen(str);
	if ((str[0] != '\"') || (str[len-1] != '\"'))
		return strdup("");
	tmp = strdup(str+1);
	tmp[len-2] = '\0';
	return tmp;
}

/*
 * Gets a line from the input. Strips unnecessary spaces.
 * '\' means continue in next line
 * '#' is a comment.
 * Returns -1 if an error is found input line is found.
 */
static int cfg_getline(char *buf, size_t *bufsize, FILE *fp)
{
	enum {  CFG_AFTERSPACE,
		CFG_COMMENT,
		CFG_STR,
		CFG_ENDSTR,
		CFG_TOKEN,
		CFG_CONTINUE,
		CFG_END        };

#define CFG_CONTEXT_VERIFY_CHAR(c)   ( ( ((c) >= 'a') && ((c) <= 'z') ) || \
                                       ( ((c) >= 'A') && ((c) <= 'Z') ) || \
                                       ( ((c) >= '0') && ((c) <= '9') ) || \
           ((c) == '-') || ((c) == '_') || ((c) == '.') || ((c) == '~') || \
           ((c) == '+') || ((c) == '=') || ((c) == '(') || ((c) == ')') || \
           ((c) == '[') || ((c) == ']') || ((c) == '{') || ((c) == '}') || \
           ((c) == '!') || ((c) == '@') || ((c) == '#') || ((c) == '$') || \
           ((c) == '%') || ((c) == '^') || ((c) == '&') || ((c) == '*') || \
           ((c) == '`') || ((c) == '<') || ((c) == '>') || ((c) == ',') || \
           ((c) == '\\')|| ((c) == ':') || ((c) == ';') || ((c) == '/') || \
           ((c) == '\'')|| ((c) == '?') || ((c) == 9) )

	register char *p, *i;
	register int curr_state;
	size_t n = 0; /* num char written in buf */
	char line[LBUF];

	i = buf;

	/* Prende dal file una riga */
	while( fgets(line, LBUF, fp) != NULL) {
		cfg_line_num++;
		curr_state = CFG_TOKEN;
		for (p = line;
		     (*p) && (n < *bufsize - 1)
		       && (curr_state != CFG_END)
		       && (curr_state != CFG_CONTINUE);
		     p++) {
			switch (*p) {
			case '#':
			case '\r':
			case '\n':
				curr_state = CFG_END;
				break;
			case ' ':
			case 9: /* Tab = space */
				if (curr_state == CFG_STR) {
					*i++ = ' ';
					n++;
					break;
				}
				if (curr_state != CFG_AFTERSPACE) {
					*i++ = ' ';
					n++;
					curr_state = CFG_AFTERSPACE;
				}
				break;
			case '"':
				if (curr_state == CFG_STR)
					curr_state = CFG_ENDSTR;
				else
					curr_state = CFG_STR;
				*i++ = '"';
				n++;
				break;
			case '\\':
				if (curr_state == CFG_STR) {
					*i++ = '\\';
					n++;
				} else
					curr_state = CFG_CONTINUE;
				break;
			default:
				if (curr_state == CFG_ENDSTR)
					return CFG_ERR_STR;

				if ( CFG_CONTEXT_VERIFY_CHAR(*p) ) {
					if (*p == 9) /* TAB -> ' ' */
						*p = ' ';
					*i++ = *p;
					n++;
					if (curr_state != CFG_STR)
						curr_state = CFG_TOKEN;
				} else
					return CFG_ERR_CHR;
			}
		}
		if ((curr_state != CFG_CONTINUE) && (n > 0)) {
			*i = '\0';
			return CFG_OK;
		}
	}
	return CFG_ERR_EOF;
}

#endif /* LOCAL */

/***************************************************************************/
#ifdef CFG_VERBOSE

/* Print contents of a cfg_client struct */
static void cfg_print_client_cfg(const struct client_cfg arg)
{
	printf("host = '%s'\n", arg.host);
	printf("port = %d\n", arg.port);
	printf("name = '%s'\n", arg.name);
	printf("password = '%s'\n", arg.password);
	printf("no_banner = %d\n", arg.no_banner);
	printf("dump_file = '%s'\n", arg.dump_file);
	printf("log_file_x = '%s'\n", arg.log_file_x);
	printf("log_file_mail = '%s'\n", arg.log_file_mail);
	printf("auto_log_x = %d\n", arg.auto_log_x);
	printf("auto_log_mail = %d\n", arg.auto_log_mail);
	printf("doing = '%s'\n", arg.doing);
	printf("tmp_dir = '%s'\n", arg.tmp_dir);
	printf("editor = '%s'\n", arg.editor);
	printf("download_dir = '%s'\n", arg.download_dir);
}
#endif
#ifdef CFG_TEST
int main(void)
{
	printf("\nTesting the configuration parser.\n\n");
	cfg_read();
	printf("\nResulting client configuration:\n");
	cfg_print_client_cfg(client_cfg);
	printf("\n");
	return 1;
}

#endif /* CFG_TEST */
