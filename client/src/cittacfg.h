/****************************************************************************
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cittacfg.h                                                         *
*        Funzioni per gestire i file di configurazione.                     *
****************************************************************************/
#ifndef _CITTACFG_H
#define _CITTACFG_H   1

#include "cittaclient.h"

/* Valori di default delle opzioni di configurazione */
#ifdef LOCAL                 /* Client locale        */
/* Default Host:             bbs.cittadellabbs.it    */
#define HOST_DFLT            "178.62.204.156"
#define EDITOR_DFLT          ""
#define SHELL_DFLT           ""
#ifdef MACOSX
#define DUMP_FILE_DFLT       "~/Library/Application Support/cittaclient/cittadella.dump"
#define AUTOSAVE_FILE_DFLT   "~/Library/Application Support/cittaclient/autosave.cml"
#else
#define DUMP_FILE_DFLT       "~/cittadella.dump"
#define AUTOSAVE_FILE_DFLT   "~/autosave.cml"
#endif

#else                        /* Client remoto        */

#define HOST_DFLT            "localhost"
#define EDITOR_DFLT          "/home/bbs/Cittadella/citta/client/bin/BBSeditor"
#define SHELL_DFLT           ""
#define DUMP_FILE_DFLT       ""
#define AUTOSAVE_FILE_DFLT    ""
#endif

#define PORTA_DFLT           4000
#define LOG_FILE_X_DFLT      DUMP_FILE_DFLT
#define LOG_FILE_MAIL_DFLT   DUMP_FILE_DFLT
#define LOG_FILE_CHAT_DFLT   DUMP_FILE_DFLT
#define NRIGHE_DFLT          24
#define AUTOSAVE_TIME_DFLT   (60*5)   /* Autosave in Editor every 5 minutes */
#define DOWNLOAD_DIR_DFLT    "~/"

#ifdef WINDOWS
#define TMPDIR_DFLT          "/"
#else
#define TMPDIR_DFLT          "/tmp"
#endif

#define PFX_EDIT "edit"
#define TEMP_EDIT_TEMPLATE "/" PFX_EDIT "XXXXXX"

#define MAX_XLOG_DFLT   20   /* Numero di messaggi memorizzati nel diario */
#define TABSIZE_DFLT     8   /* Numero di spazi corrispondenti a un <TAB> */

/* Macro per riferirsi alle variabili di configurazione */
#define HOST                 (client_cfg.host)
#define PORT                 (client_cfg.port)
#define USER_NAME            (client_cfg.name)
#define USER_PWD             (client_cfg.password)
#define NO_BANNER            (client_cfg.no_banner)
#define NRIGHE               (client_cfg.nrighe)
#define EDITOR               (client_cfg.editor)
#define AUTOSAVE_FILE        (client_cfg.autosave_file)
#define AUTOSAVE_TIME        (client_cfg.autosave_time)
#define DOING                (client_cfg.doing)
#define AUTO_LOG_X           (client_cfg.auto_log_x)
#define AUTO_LOG_MAIL        (client_cfg.auto_log_mail)
#define AUTO_LOG_CHAT        (client_cfg.auto_log_chat)
#define DUMP_FILE            (client_cfg.dump_file)
#define LOG_FILE_X           (client_cfg.log_file_x)
#define LOG_FILE_MAIL        (client_cfg.log_file_mail)
#define LOG_FILE_CHAT        (client_cfg.log_file_chat)
#define TMPDIR               (client_cfg.tmp_dir)
#define USE_COLORS           (client_cfg.use_colors)
#define MAX_XLOG             (client_cfg.max_xlog)
#define TABSIZE              (client_cfg.tabsize)
#define SHELL                (client_cfg.shell)
#define COMPRESSIONE         (client_cfg.compressione)
#define DEBUG_MODE           (client_cfg.debug)
#define BROWSER              (client_cfg.browser)
#define TERMBROWSER          (client_cfg.termbrowser)
#define PICTURE              (client_cfg.picture)
#define MOVIE                (client_cfg.movie)
#define AUDIO                (client_cfg.audio)
#define PDFAPP               (client_cfg.pdfapp)
#define PSAPP                (client_cfg.psapp)
#define DVIAPP               (client_cfg.dviapp)
#define RTFAPP               (client_cfg.rtfapp)
#define DOCAPP               (client_cfg.docapp)
#define DOWNLOAD_DIR         (client_cfg.download_dir)

struct client_cfg {
	char *host;
	unsigned int port;
	char *name;
	char *password;
	bool no_banner;   /* Anti-boss: se 1 non mostra le LogIn/Out banner */
	int  nrighe;
	char *editor;
        char *autosave_file;
        int  autosave_time;
	char *doing;
	bool auto_log_x;
	bool auto_log_mail;
	bool auto_log_chat;
	char *dump_file;
	char *log_file_x;
	char *log_file_mail;
	char *log_file_chat;
	char *tmp_dir;
	bool use_colors;
	int max_xlog;
	int tabsize;
	char *shell;
        char *compressione; /* Modalita' di compressione. */
	int compressing;   /* non-zero se la compressione dei dati e' attiva */
        bool debug;
        char *browser;
        char *picture;
        char *movie;
        char *audio;
        char *pdfapp;
        char *psapp;
        char *dviapp;
        char *rtfapp;
        char *docapp;
        bool termbrowser;
        char *download_dir;
};

extern struct client_cfg client_cfg;

/* Prototipi funzioni in cittacfg.c */
void cfg_read(char *rcfile, bool no_rc);
void cfg_add_opt(const char *buf);

#endif /* cittacfg.h */
