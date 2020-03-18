/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : cittaclient.c                                                      *
*        main(), ciclo base del client                                      *
****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <pwd.h>
#include <setjmp.h>
#include <stddef.h>

#ifdef DEBUG
#include "memkeep.h"
#endif

#ifndef LOCAL
#include <termios.h>
#endif

#include "aide.h"
#include "ansicol.h"
#include "blog_cmd.h"
#include "cittacfg.h"
#include "cittaclient.h"
#include "cmd_da_server.h"
#include "cml.h"
#include "comandi.h"
#include "comunicaz.h"
#include "configurazione.h"
#include "conn.h"
#include "cti.h"
#include "decompress.h"
#include "extract.h"
#include "floor_cmd.h"
#include "friends.h"
#include "generic_cmd.h"
#include "inkey.h"
#include "login.h"
#include "messaggi.h"
#include "metadata.h"
#include "prompt.h"
#include "room_cmd.h"
#include "strutt.h"
#include "signals.h"
#include "string_utils.h"
#include "sysconf_client.h"
#include "sysop.h"
#include "terminale.h"
#include "urna-crea.h"
#include "urna-gestione.h"
#include "urna-leggi.h"
#include "urna-servizio.h"
#include "urna-vota.h"
#include "user_flags.h"
#include "utente_client.h"
#include "utility.h"
#include "versione.h"
#include "macro.h"
#ifdef LOCAL
#include "local.h"
#else
#include "remote.h"
#endif

/* variabili globali in cittaclient.c */
struct serverinfo serverinfo;
struct coda_testo comandi;

/* client */
bool local_client;                 /* TRUE se client locale, FALSE: remoto */
bool login_eseguito;
int  riga;
char *home_dir;                    /* Home directory dell'utente     */

char postref_room[ROOMNAMELEN+1];  /* Per i post reference del metadata */
long postref_locnum;

/* user data */
char nome[MAXLEN_UTNAME];          /* Nick name dell'utente                */
int livello;                       /* Livello di accesso dell'utente       */
bool ospite;                       /* L'utente e' un ospite?               */
uint8_t uflags[SIZE_FLAGS];        /* Flags modificabili dall'utente       */
uint8_t sflags[SIZE_FLAGS];        /* Flags di sistem (non modif.)         */
int sesso;                         /* Sesso dell'utente                    */

bool cestino;                      /* TRUE se l'utente e` in Cestino       */
bool find_vroom;                   /* TRUE se l'utente e` in Trova         */
bool blog_vroom;                    /* TRUE se l'utente e` in Blog          */
char canale_chat;                  /* n. canale, o 0 se utente non in chat */

char last_profile[MAXLEN_UTNAME] = "";
char last_mail[MAXLEN_UTNAME] = "";
char last_x[MAXLEN_UTNAME] = "";
char last_xfu[MAXLEN_UTNAME] = ""; /* Autore ultimo X ricevuto       */

/* room and floor data */
long rflags;                       /* Flags della room corrente            */
long utrflags;                     /* Flags UTR per la room corrente       */
char current_floor[FLOORNAMELEN];  /* Nome del floor corrente              */
/* char current_room[ROOMNAMELEN]; */   /* Nome room corrente              */
char current_room[LBUF];           /* Nome room corrente                   */
char room_type[10];

/* others */
char oa[] = {'o', 'a'};            /* oa[sesso] e' la lettera finale */

/* we don't use this anymore
jmp_buf ciclo_principale;
*/

/* prototipi delle funzioni in questo file */
static void info_sul_server(void);
static void ident_client(void);
static char ciclo_client(void);
static void get_environment(void);
static void get_homedir(void);
static void pianta_bbs(void);
static void wrapper_urna(void);

/****************************************************************************
****************************************************************************/
/*
 * main() :
 */
int main(int argc, char **argv)
{
        char buf[LBUF], last_host[LBUF], *rcfile;
        long num_utente, ultimo_login, mail;
        int ut_conn, num_chiamata, login_status;
	bool no_rc;

#ifdef TESTS
        test_string_utils();
#endif

        /* Client locale o client remoto? */
#ifdef LOCAL
        local_client = true;
#else
        local_client = false;
#endif

        /* Inizializzazione variabili */
	srand(time(NULL));
	login_eseguito = false;
        nome[0] = 0;
        ospite = false;
        canale_chat = 0;
	cestino = false;
	find_vroom = false;
	comandi.partenza = NULL;
	comandi.termine = NULL;

        /* settaggio del terminale e segnali  (vedere) */
        term_save();
        term_mode();
	atexit(reset_term);
	atexit(ansi_reset);
        setup_segnali();

	/* Inizializza schermo */
	cti_init();
	cti_term_init();
	cti_term_exit();
	/* Editor_Win = 0; */

	/* Parse command line options */
        parse_opt(argc, argv, &rcfile, &no_rc);

	/* Trova la home directory dell'utente */
	get_homedir();

	/* Leggi file di configurazione */
	cfg_read(rcfile, no_rc);
	if (rcfile != NULL)
		Free(rcfile);

	/* Inizializza il file di autosave per l'editor interno */
#ifndef LOCAL
        // TODO
#endif

	/* Inizializza il diario dei messaggi. */
	xlog_init();

        /* Inizializza il post reference */
        postref_room[0] = 0;
        postref_locnum = 0;

	/* Se non e` stato specificato un editor nel file di configurazione,
	 * prende quello indicato dalla variabile di ambiente $(EDITOR).
	 * Stessa casa per $(SHELL)                                          */
	get_environment();

	/* Inizializza colori ansi */
	ansi_init();

	if (USE_COLORS)                            /* Override default */
		uflags[1] |= UT_ANSI;
	update_cmode();

        /* Connessione al server */
        crea_connessione(HOST, PORT);

        /* Vediamo se il server accetta la connessione
           (limite di connessioni raggiunto o bansite o chessoio... */
        serv_gets(buf);
        printf("%s\n", buf+4);

        /* Identificazione client/server */
        info_sul_server();
        ident_client();
        if (CLIENT_VERSION_CODE > serverinfo.protocol_vcode) {
                printf("Legacy server: entering compatibility mode.\n");
                serverinfo.legacy = true;
        }

        /* Saluta e procedi con il login */
	login_banner();
        login_status = login();

	/* Inserisci doing */
	if (DOING && (DOING[0] != '\0')) {
		serv_putf("EDNG 1|%s", DOING);
		serv_gets(buf);
	}

        /* Carica la configurazione dell'utente: flags, friends, etc... */
        if (!ospite) {
		carica_userconfig(0);
                carica_amici();
	} else {
		uflags[0] = UTDEF_0;
		uflags[1] = UTDEF_1;
		uflags[2] = UTDEF_2;
		uflags[3] = UTDEF_3;
		uflags[4] = UTDEF_4;
		uflags[5] = UTDEF_5;
		uflags[6] = UTDEF_6;
		uflags[7] = UTDEF_7;
	}
	if (USE_COLORS)                            /* Override default */
		uflags[1] |= UT_ANSI;
	update_cmode();

	/* Chiede la configurazione di default dell'ospite. */
	user_config(login_status);

	/* disclaimer */
        leggi_file(STDMSG_MESSAGGI, STDMSGID_DISCLAIMER);
        putchar('\n');
        hit_any_key();

	/* news: se sono state modificate, chiede di premere un tasto. */
        leggi_news();
	if (sflags[0] & SUT_NEWS)
		hit_any_key();

	/* upgrade: se nuovo client, riconfigura. */
	setcolor(C_NEWS);
	if ((sflags[0] & SUT_UPGRADE) &&
	    (CLIENT_VERSION_CODE == serverinfo.newclient_vcode)) {
		upgrade_client(CLIENT_VERSION_CODE);
	}

        /* Controllo posta, validazione utenti etc... (CHECK) */
	setcolor(CYAN);
        if (!ospite) {
                putchar('\n');

                serv_puts("CHEK");
                serv_gets(buf);
                if (buf[0] != '2') {
                        printf("Fatal error: CHEK %s\n", buf);
                        pulisci_ed_esci(NO_EXIT_BANNER);
                }
                ut_conn = extract_int(buf+4, 0);
                num_utente = extract_long(buf+4, 1);
                livello = extract_int(buf+4, 2);
                num_chiamata = extract_int(buf+4, 3);
                ultimo_login = extract_long(buf+4, 4);
                extract(last_host, buf+4, 5);
                mail = extract_long(buf+4, 6);

		switch(ut_conn) {
		 case 1:
                        printf( _("Nessun altro utente connesso.\n"));
			break;
		 case 2:
                        cml_print( _("&Egrave; connesso un altro utente.\n"));
			break;
		 default:
                        printf( _("Sono connessi altri %d utenti.\n"),
				ut_conn-1);
			break;
		}
                if (num_chiamata > 1) {
                        cml_printf( _(
"\nChiamata n.<b>%d</b> per l'utente n.<b>%ld</b> di livello <b>%d</b>\n"
                                      ), num_chiamata, num_utente, livello);

                        cml_printf( _("Ultimo collegamento <b>"));
                        stampa_data_smb(ultimo_login);
                        cml_printf( _("</b> da <b>%s</b>\n"), last_host);

			if (mail == 0) {
				printf(_("Nessun nuovo mail.\n"));
			} else if (mail == 1) {
				printf(_("Hai un nuovo mail.\n"));
			} else {
				cml_printf(_("Hai <b>%ld</b> nuovi mail.\n"),
					   mail);
                        }
                } else {
                        cml_printf(_(
"\nPrima chiamata per l'utente n.%ld di livello <b>%d</b>\n"
                                     ), num_utente, livello);
                }
		IFNEHAK;

        	if (livello >= MINLVL_URNA) {
                        urna_check();
                }
        }

	setcolor(C_NORMAL);
	if (comandi.partenza != NULL) {
		printf(sesso ?
		       _("\nQuesti messaggi sono stati tenuti da "
			 "parte mentre eri impegnata col login.\n\n") :
		       _("\nQuesti messaggi sono stati tenuti da "
			 "parte mentre eri impegnato col login.\n\n"));
		esegue_comandi(0);
	}

	/* Inizializza i keep alive */
	setup_keep_alive();

        /* Creazione files temporanei, settaggio dimens. finestra */
        login_eseguito = true;

	/* Va in Lobby) */
	putchar('\n');
	room_goto(4, true, NULL);

        ciclo_client();     /* Ciclo principale... */

        /* Chiusura della connessione */
        pulisci_ed_esci(SHOW_EXIT_BANNER);
        return 0;
}

/* Trova la home directory dell'utente */
static void get_homedir(void)
{
	char *tmp;
	struct passwd *pw;

	if ( (tmp = getenv("HOME")))
		home_dir = citta_strdup(tmp);
	else if ( (pw = getpwuid(getuid())))
		home_dir = citta_strdup(pw->pw_dir);
	else
		home_dir = NULL;
}

/* Carica le variabili di ambiente non trovate nel file di configurazione */
static void get_environment(void)
{
#ifdef LOCAL
        if (*EDITOR == 0) {
	        Free(EDITOR);
	        EDITOR = getenv("EDITOR");
	}
        if (*SHELL == 0) {
	        Free(SHELL);
	        SHELL = getenv("SHELL");
	}
#endif
}

char * interpreta_tilde_dir(const char *buf)
{
	char tmp[LBUF];

	if (buf == NULL)
		return citta_strdup("");
	if (buf[0] != '~')
		return citta_strdup(buf);
	if (buf[1] != '/')
		return citta_strdup("");
	sprintf(tmp, "%s%s", home_dir, buf+1);
	return citta_strdup(tmp);
}

/*
 * Ciclo principale del client
 */
static char ciclo_client(void)
{
        int cmd;
        char cmdstr[LBUF];

        /* setjmp(ciclo_principale); */

	esegue_cmd_old();
        while ( (cmd = getcmd(cmdstr)) != 0) {
                switch(cmd) {
		 case 1:
                        list_host();
                        break;
		 case 2:
                        broadcast();
                        break;
		 case 3:
                        express(NULL);
                        break;
		 case 4:
                        profile(NULL);
                        break;
		 case 5:
                        bbs_shutdown(1,1);
                        break;
		 case 6:
                        lista_utenti();
                        break;
		 case 7:
                        registrazione(false);
                        break;
		 case 8:
                        leggi_sysconfig();
                        break;
		 case 9:
                        edit_sysconfig();
                        break;
		 case 10:
                        caccia_utente();
                        break;
		 case 11:
                        elimina_utente();
                        break;
		 case 12:
                        edit_user();
                        break;
		 case 13:
                        ora_data();
                        break;
		 case 14: /* lista dei comandi */
			help();
                        break;
		 case 15:
                        Help();
                        break;
		 case 16:
                        Read_Syslog();
                        break;
		 case 17:
                        ascolta_chat();
                        break;
		 case 18:
                        lascia_chat();
                        break;
		 case 19:
                        chat_who();
                        break;
		 case 20: /* <z> invia messaggio in chat */
                        chat_msg();
                        break;
		 case 21:
                        lock_term();
                        break;
		 case 22:
                        chpwd();
                        break;
		 case 23:
                        bug_report();
                        break;
		 case 24:
                        chupwd();
                        break;
		 case 25:
                        enter_profile();
                        break;
		 case 26:
                        edit_userconfig();
                        break;
		 case 27:
                        vedi_userconfig();
                        break;
		 case 28:
			 vedi_amici(friend_list);
			 vedi_amici(enemy_list);
			 break;
		 case 29:
			 edit_amici(friend_list);
			 break;
#ifdef LOCAL
		 case 30:
                        subshell();
                        break;
		 case 31:
                        finger();
                        break;
#endif
		 case 32:
                        statistiche_server();
                        break;
		 case 33:
			room_list();
			break;
		 case 34: /* goto next room */
                         room_goto(0, true, NULL);
			break;
		 case 35:
			enter_message(0, false, 0);
			break;
		 case 36: /* jump to */
                         room_goto(0, false, NULL);
			break;
		 case 37:
			fm_headers();
			break;
		 case 38:
			fm_create();
			break;
		 case 39:
			fm_remove();
			break;
		 case 40:
			fm_read();
			break;
		 case 41:
			fm_info();
			break;
		 case 42:
			room_create();
			break;
		 case 43:
			room_edit(1);
			break;
		 case 44:
			room_delete();
			break;
		 case 45:
			fm_msgdel();
			break;
		 case 46: /* Forward */
                         read_msg(0, 0);
			break;
		 case 47: /* New     */
                         read_msg(1, 0);
			break;
		 case 48: /* Reverse */
                         read_msg(2, 0);
			break;
		 case 49: /* Brief   */
                         read_msg(3, 0);
			break;
		 case 50: /* Skip    */
                         room_goto(1, true, NULL);
			break;
		 case 51:
                         room_info(false, true);
			break;
		 case 52:
			room_edit_info();
			break;
		 case 53: /* all */
			room_list_known(0);
			break;
		 case 54: /* new */
			room_list_known(1);
			break;
		 case 55: /* old */
			room_list_known(2);
			break;
		 case 56: /* zapped */
			room_list_known(3);
			break;
		 case 57: /* floor */
			room_list_known(4);
			break;
		 case 58:
			room_zap();
			break;
		 case 59: /* Read last five msg */
                         read_msg(5, 0);
			break;
		 case 60: /* <k> classico */
			room_list_known(5);
			break;
		 case 61: /* with Editor */
			enter_message(1, false, 0);
			break;
		 case 62:
                         room_info(true, true);
			break;
		 case 63:
			room_new_rh();
			break;
		 case 64:
			room_invite();
			break;
		 case 65:
			room_new_ra();
			break;
		 case 66: /* with default Editor */
			enter_message(-1, false, 0);
			break;
		 case 67: /* Abandon    */
                         room_goto(2, true, NULL);
			break;
		 case 68:
			toggle_uflag(0, UT_ESPERTO);
			break;
		 case 69:
			toggle_uflag(1, UT_BELL);
			break;
		 case 70:
			toggle_uflag(3, UT_LION);
			break;
		 case 71:
			toggle_uflag(1, UT_HYPERTERM);
			update_cmode();
			break;
		 case 72: /* with .killscript */
			bbs_shutdown(1, 0);
			break;
		 case 73: /* now */
			bbs_shutdown(0, 1);
			break;
		 case 74: /* stop */
			bbs_shutdown(-1, 1);
			break;
		 case 75: /* Last N */
                         read_msg(6, 0);
			break;
		 case 76:
			biff();
			break;
		 case 77: /* as RA or RH */
			enter_message(-1, false, 1);
			break;
		 case 78: /* as Aide */
			enter_message(-1, false, 2);
			break;
		 case 79: /* as Sysop */
			enter_message(-1, false, 3);
			break;
		 case 80:
		        toggle_uflag(3, UT_FOLLOWUP);
			break;
/*
                 case 81:
		        validazione();
			break;
 */
                 case 82:
		        urna_completa();
			break;
                 case 83:
			urna_delete();
			break;
                 case 84:
			urna_new();
			break;
                 case 85:
			urna_read(0);
			break;
                 case 86:
			urna_vota();
			break;
		 case 87:
			fm_expand();
			break;
		 case 88:
			urna_list(0);
			break;
		 case 89:
			room_swap();
			break;
		 case 90:
                         room_goto(5, true, NULL);
			break;
		 case 91:
			express(last_xfu);
			break;
		 case 92:
			toggle_uflag(3, UT_X);
			break;
		 case 93:
			room_resize();
			break;
		 case 94:
			banners_modify();
			break;
		 case 95:
			banners_rehash();
			break;
		 case 96:
			room_zap_all();
			break;
		 case 97: /* <.><s>kip  */
                         room_goto(1, false, NULL);
			break;
		 case 98: /* Read msg # */
                         read_msg(7, -1);
			break;
		 case 99:
			room_unzap_all();
			break;
		 case 100:
			toggle_uflag(3, UT_XFRIEND);
			break;
		 case 101:
			room_invite_req();
			break;
		 case 102:
			edit_news();
			break;
		 case 103:
			xlog_browse();
			break;
		 case 104:
			enter_helping_hands();
			break;
		 case 105:
			floor_create();
			break;
		 case 106:
			floor_delete();
			break;
		 case 107:
			floor_list();
			break;
		 case 108:
			floor_move_room();
			break;
		 case 109:
			floor_new_fa();
			break;
		 case 110:
			floor_info(0);
			break;
		 case 111:
			floor_info(1);
			break;
		 case 112:
			floor_edit_info();
			break;
		 case 113: /* Goto next floor */
			floor_goto(0, 1);
			break;
		 case 114:
			floor_known_rooms();
			break;
		 case 115:
			chiedi_valkey();
			break;
		 case 116:
			edit_doing();
			break;
#ifdef LOCAL
		case 117: /* autodump mail */
			AUTO_LOG_MAIL = AUTO_LOG_MAIL ? false : true;
			print_on_off(AUTO_LOG_MAIL);
			break;
		case 118: /* autodump Xmsg */
			AUTO_LOG_X = AUTO_LOG_X ? false : true;
			print_on_off(AUTO_LOG_X);
			break;
		case 119: /* autodump chat */
			AUTO_LOG_CHAT = AUTO_LOG_CHAT ? false : true;
			print_on_off(AUTO_LOG_CHAT);
			break;
#endif
		case 120:
			clear_doing();
			break;
		case 121:
			edit_term();
			break;
		case 122:
			toggle_uflag(1, UT_ANSI);
			update_cmode();
			break;
		case 123:
			server_upgrade_flag();
			break;
		case 124:
			room_kick();
			break;
		case 125:
			room_kick_end();
			break;
		case 126:
			room_kicked_who();
			break;
		case 127:
			room_invited_who();
			break;
		case 128:
			urna_postpone();
			break;
		case 129:
			pianta_bbs();
			break;
		case 130:
			set_all_read();
			break;
		case 131:
			room_goto(6, true, NULL);
			break;
		case 132:
			leggi_news();
			break;
			/*
		case 133:
			send_emote();
			break;
			*/
		case 134:
			urna_list(1);
		        break;
		case 135:
			cti_clear_screen();
			break;
		case 136:
			force_scripts();
			break;
		case 137:
                        edit_amici(enemy_list);
                        break;
		case 138:
                        wrapper_urna();
                        break;
		case 139:
                        cerca();
                        break;
		case 140:
                        blog_goto();
                        break;
		case 141:
                        blog_configure();
                        break;
		case 142:
                        blog_enter_message();
                        break;
		case 143:
                        blog_list();
                        break;
		case 144:
                        toggle_debug_mode();
                        break;
                case 145:
                        room_goto(7, true, NULL);
			break;
                case 146:
                        sysop_reset_consent();
                        break;
                case 147:
                        sysop_unregistered_users();
                        break;
                default:
                        cml_printf("\n<b;fg=1>"
"  *** ERROR cittaclient case %d - please notify the bug!"
                                   "</b;fg=0>\n",
                                   cmd);
                        sleep(1);
		} /* fine switch */

                /* Se qui la coda comandi non e` vuota e` perche' qualcosa */
                /* e` arrivato mentre ero impegnato                        */
		esegue_cmd_old();

                /* Se e` arrivato qualche segnale, trattalo correttamente  */
#ifdef LOCAL
                if (new_signals)
                        esegui_segnali();
#endif
        }
        return 0;
}


/*
 * Manda al server il segnale di chiusura, chiude la connessione,
 * ripristina lo schermo ed esce.
 */
void pulisci_ed_esci(exit_banner show_banner)
{
        char buf[LBUF];
	unsigned long in, out, cmd, online;

	setcolor(C_NORMAL);
        if (show_banner == SHOW_EXIT_BANNER && !client_cfg.no_banner) {
                /* Display logout banner */
                leggi_file(STDMSG_MESSAGGI, STDMSGID_GOODBYE);
        }

        /* Cancellazione file temporanei e robe varie */

        /* Disconnessione */
        printf(_("[Disconnessione...]\n"));
        serv_puts("QUIT");
        serv_gets(buf);
        if (show_banner == SHOW_EXIT_BANNER) {
                cmd = extract_long(buf+4, 0);
                out = extract_long(buf+4, 1);
                in  = extract_long(buf+4, 2);
                online  = extract_long(buf+4, 3);
                printf(
"Hai inviato %ld bytes, ricevuto %ld bytes [compressione %d%%].\n"
"(%ld comandi, %ld sec)\n",
                       out, in, decompress_stat(), cmd, online
                       );
        }
        printf("Bye bye.\n");
        if (close(serv_sock) == -1) {
	    perror("close() socket\n");
	}

        /* Ripristino terminale e segnali */
	/* reset_term(); */

#if DEBUG
        MemstatS();
#endif
        _exit(0);
}

static void info_sul_server(void)
{
        char rbf[LBUF+10], comp[LBUF], *buf;

#ifdef LOCAL
        serv_putf("INFO locale|%s", COMPRESSIONE);
#else
        char rhost[LBUF];

# ifdef LOGINPORT
        serv_putf("INFO remoto|%s|%s", remote_host, remote_key);
# else
        find_remote_host(rhost);
        serv_putf("INFO remoto|%s", rhost);
# endif
#endif
        serv_gets(rbf);

        if (rbf[0] == '2') {
                buf = &rbf[4];
                extractn(serverinfo.software, buf, 0, 50);
                serverinfo.server_vcode = extract_int(buf, 1);
                extractn(serverinfo.nodo, buf, 2, 50);
                extractn(serverinfo.dove, buf, 3, 50);
                serverinfo.protocol_vcode   = extract_int(buf, 4);
                serverinfo.newclient_vcode  = extract_int(buf, 5);
                serverinfo.num_canali_chat  = extract_int(buf, 6);
                serverinfo.maxlineebug      = extract_int(buf, 7);
                serverinfo.maxlineebx       = extract_int(buf, 8);
                serverinfo.maxlineenews     = extract_int(buf, 9);
                serverinfo.maxlineepost     = extract_int(buf, 10);
                serverinfo.maxlineeprfl     = extract_int(buf, 11);
                serverinfo.maxlineeroominfo = extract_int(buf, 12);
                serverinfo.flags            = extract_int(buf, 13);
                extract(comp, buf, 14);

                printf("\nServer: %s version ", serverinfo.software);
		version_print(serverinfo.server_vcode);
                printf("\n        %s (%s)\n", serverinfo.nodo,serverinfo.dove);
                printf("Client: Cittaclient/UX version ");
		version_print(CLIENT_VERSION_CODE);
		putchar('\n');

                if (CLIENT_VERSION_CODE < serverinfo.protocol_vcode) {
                        printf(_("\nATTENZIONE: Questo client e' obsoleto.\n"
                               "Per collegarti alla BBS, scarica il nuovo Cittaclient "));
			version_print(serverinfo.newclient_vcode);
			printf(_("da http://www.cittadellabbs.it/ oppure collegati con un telnet\n"
			       "telnet %s 4001\n\n"), serverinfo.nodo);
			pulisci_ed_esci(NO_EXIT_BANNER);
		}

                if (strncmp(comp, COMPRESSIONE, LBUF)) {
                        Free(COMPRESSIONE);
                        COMPRESSIONE = Strdup(comp);
                }

                if (COMPRESSIONE[0]) {
		        decompress_set(COMPRESSIONE);
                        printf("Compressione trasmissione attivata (%s).\n",
                               COMPRESSIONE);
                } else
                        printf("Connessione non compressa.\n");

        } else {
                pulisci_ed_esci(SHOW_EXIT_BANNER);
        }
}

/* TODO segnala al server l'identita' del client */
static void ident_client(void)
{
}

static void pianta_bbs(void)
{
#ifdef LOCAL
	int i=0;

        signal(SIGHUP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGCONT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

	while (1) {
	        sleep(1);
		printf("\x1b[%dq", (i++) % 4);
		fflush(stdout);
	}
#else
       printf("\n\nConnection closed by foreign host.\n");
       setcolor(C_NORMAL);
       reset_term();
       ansi_reset();
#endif
       exit(0);
}

/**************************************************************************/
/* DA SPOSTARE */
static void wrapper_urna(void)
{
	bool quit;
	int c;

	prompt_curr = P_WRAPPER_URNA;
        setcolor(C_URNA);
	putchar('\n');
	quit = false;
	while (!quit) {
                cml_printf(_(
                       "1. Vedi la lista di tutte le consultazioni in corso\n"
                       "2. Leggi il quesito di un sondaggio o referendum\n"
                       "3. Esprimi il tuo voto per una consultazione\n"
                       "4. Crea un nuovo sondaggio/referendum\n"
                       "5. Cancella una consultazione in corso\n"
                       "6. Posticipa il termine di una consultazione\n"
                       "0. Esci dalla cabina elettorale\n\n"));
                print_prompt();
                setcolor(C_URNA);
		do
			c = inkey_sc(1);
		while (!index("12345670", c));
                del_prompt();
		switch (c) {
		case '1':
			urna_list(0);
                        break;
		case '2':
			urna_read(0);
                        break;
		case '3':
			urna_vota();
                        break;
		case '4':
			urna_new();
                        break;
		case '5':
			urna_delete();
                        break;
		case '6':
			urna_postpone();
                        break;
		case '7':
			urna_list(1);
                        break;
		case '0':	/* Quit */
			quit = true;
			return;
		}
                putchar('\n');
                hit_any_key();
	}
}
