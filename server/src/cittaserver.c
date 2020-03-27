/*
 *  Copyright (C) 1999-2005 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello    *
*                                                                             *
* File: cittaserver.c                                                         *
*       gestione dei socket, main(), ciclo base del server                    *
******************************************************************************/
#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef USE_RUSAGE
#include <sys/resource.h>
#endif

#include "cittaserver.h"
#include "banner.h"
#include "blog.h"
#include "cestino.h"
#include "cmd_al_client.h"
#include "coda_testo.h"
#include "compress.h"
#include "email.h"
#include "express.h"
#include "fileserver.h"
#include "interprete.h"
#include "mail.h"
#include "march.h"
#include "memstat.h"
#include "post.h"
#include "remote.h"
#include "signals.h"
#include "socket_citta.h"
#include "string_utils.h"
#include "sysconfig.h"
#include "ut_rooms.h"
#include "utente_cmd.h"
#include "utility.h"
#include "versione.h"
#include "webstr.h"
#include "x.h"

#ifdef USE_FIND
# include "find.h"
#endif
#ifdef USE_FLOORS
# include "floors.h"
# include "floor_ops.h"
#endif
#ifdef USE_CACHE_POST
# include "cache_post.h"
#endif
#ifdef USE_REFERENDUM
# include "urna-servizi.h"
# include "urna-rw.h"
#endif

#ifdef USE_CITTAWEB
# include "cittaweb.h"
#endif

#ifdef USE_POP3
# include "pop3.h"
#endif

#ifdef USE_IMAP4
# include "imap4.h"
#endif

#ifndef MACOSX /* E' necessario avere queste definizioni?!? */
extern FILE *stderr;
extern FILE *stdout;
#endif

/* variabili globali in cittaserver.c */
struct sessione *lista_sessioni;
struct dati_server dati_server;

/* Opzioni di linea del server                                            */
int solo_aide = 0;
int no_nuovi_utenti = 0;
int daemon_server = 1;         /* Se 0 rimane legato al tty e invia i log */
                               /* in stderr, altrimenty nel file syslog.  */
FILE * logfile;                /* File in cui inviare i log.              */

long logged_users = 0;         /* Numero utenti correntemente loggati     */
int citta_shutdown = 0;        /* spegnimento pulito del server           */
                               /* 1: sdwn, 2: daily reboot.               */
int citta_sdwntimer = 0;       /* tempo al shutdown in sec.               */
char chiudi = 0;               /* spegni il server                        */
bool citta_reboot = true;      /* riavvia il server dopo lo shutdown      */
int reboot_day;                /* Giorno del prossimo reboot.             */
int conn_madre = 0;            /* file descriptor della connessione madre */
#ifdef USE_CITTAWEB
int http_socket = 0;           /* file descriptor connessione cittaweb    */
#endif
#ifdef USE_POP3
int pop3_socket = 0;           /* file descriptor connessione pop3        */
#endif
#ifdef USE_IMAP4
int imap4_socket = 0;          /* file descriptor connessione imap4       */
#endif
int maxdesc;                   /* piu' alto file desc utilizzato          */
int max_num_desc = 0;          /* numero massimo di file desc disponibili */
long tics = 0;                 /* contatore cicli per controllo esterno   */
char citta_soft[50];           /* Info sul server                         */
char citta_ver[10];
char citta_nodo[50];
char citta_dove[50];
char citta_ver_client[10];     /* Versione del client piu' recente        */

const char empty_string[] = "";

#undef ISNEWL
#define ISNEWL(ch) ((ch) == '\n')

#ifdef USE_VALIDATION_KEY
char livello_iniziale = LIVELLO_INIZIALE;
#else
char livello_iniziale = LIVELLO_VALIDATO;
#endif

/*
 * prototipi delle funzioni in questo file
 */
void avvio_server(int porta);

void ciclo_server(int s);
void chiusura_server(int s);

void cprintf(struct sessione *t, char *format, ...) CHK_FORMAT_2;
void crash_save_perfavore(void);
void carica_dati_server(void);
void salva_dati_server(void);
void restart_server(void);
void shutdown_server(void);

void session_create(int desc, char *host);
void chiudi_sessione(struct sessione *d);

static void chiusura_sessioni(int s);
static void flush_code_sessione(struct sessione *d);

void serverlock_create(void);
pid_t serverlock_check(void);
int serverlock_delete(void);

void timestamp_write(char * filename);
time_t timestamp_read(char * filename);

static void force_script(void);
static bool force_script_check(void);
static int force_script_delete(void);
void cmd_fsct(struct sessione *t);
void crea_messaggio(char *cmd, char *arg);
int cancella_messaggio(void);
void esegue_messaggio(void);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#define ERR_NOSERVER "Il server Cittadella/UX non gira attualmente!\n"

int main(int argc, char **argv)
{
	int porta;
	int pos = 1;
	char *i;
	unsigned long int core=CORE;
	struct rlimit rlim;

	pid_t pid;

#ifdef  TESTS
        test_string_utils();
#endif

	if (getuid() == 0) {
		fprintf(stderr, "cittaserver: Si rifiuta codardamente di correre come radice\n");
		exit(1);
	}

	porta = SERVER_PORT;
        logfile = stderr; /* Di default invia i log in stderr */
	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
		case 'a':
			solo_aide = 1;
			citta_log("SYSTEM: Accesso Limitato -- solo aide.");
			break;
		case 'b':
			fprintf(stderr, "Invia un broadcast al server Cittadella/UX.\n");
			pid = serverlock_check();
			if (pid == 0) {
				fprintf(stderr, ERR_NOSERVER);
				exit(-1);
			}
			if (argv[pos+1] == NULL){
				fprintf(stderr, "Mi rifiuto di inviare un broadcast vuoto!\n");
				exit(-1);
			}
			crea_messaggio("B", argv[pos+1]);
			kill(pid, SIGUSR1);
			fprintf(stderr, "Ok.\n");
			exit(0);
 		case 'c':
 			if (argv[pos+1] == NULL){
 				fprintf(stderr, "Manca l'argomento al core!\n");
 				exit(-1);
 			}
 			for(i=argv[pos+1];(*i)!=0; i++){
 					if(*i<'0' || *i>'9'){
 				           fprintf(stderr, "L'argomento di -c  non e` numerico!\n");
 						   exit(-1);
 					}
 			}
 			core=atoi(argv[pos+1]);
 			if(core>MAXCORE){
 					core=MAXCORE;
 			}
 			break;
		case 'd':
			daemon_server = 0;
			citta_log("SYSTEM: Debug, log in stderr.");
			break;
		case 'f':
			fprintf(stderr, "Forza l'esecuzione degli script backup e logrotate al prossimo reboot/shutdown.\nOk.\n");
			force_script();
			exit(0);
		case 'h':
			fprintf(stderr,
				"\n"
				"Cittadella/UX Server %s\n"
				"\n"
				"Uso: %s [-a|-d|-n] [# porta]\n"
				"     %s -b 'messaggio'\n"
				"     %s [-m|-M] msg.txt\n"
				"     %s -s\n"
				"\n"
				" -a         Accesso limitato ai soli aide (DA FARE)\n"
				" -b STRING  Invia un broadcast (max 78 char)\n"
				" -c NUM     Dimensione del core\n"
				" -d         Debug: log in stderr\n"
				" -f         Forza l'esecuzione degli script al shutdown/reboot\n"
				" -M FILE    Invia un email a tutti gli utenti della BBS.\n"
				" -m FILE    Invia un email agli utenti che ricevono la newsletter.\n"
				" -n         Accesso limitato agli utenti validati (DA FARE)\n"
				" -s         Esegui il shutdown del server adesso\n"
				" \n",
				SERVER_RELEASE, argv[0], argv[0], argv[0],
                                argv[0]);
			exit(0);
		case 'm':
			fprintf(stderr, "Invia un email agli utenti che ricevono la newsletter.\n");
			pid = serverlock_check();
			if (pid == 0) {
				fprintf(stderr, ERR_NOSERVER);
				exit(-1);
			}
			if (argv[pos+1] == NULL){
				fprintf(stderr, "Devi dirmi in che file c'e' il testo del mail!\n");
				exit(-1);
			}
			crea_messaggio("m", argv[pos+1]);
			kill(pid, SIGUSR1);
			fprintf(stderr, "Ok.\n");
			exit(0);
		case 'M':
			fprintf(stderr, "Invia un email a tutti gli utenti della BBS.\n");
			pid = serverlock_check();
			if (pid == 0) {
				fprintf(stderr, ERR_NOSERVER);
				exit(-1);
			}
			if (argv[pos+1] == NULL){
				fprintf(stderr, "Devi dirmi in che file c'e' il testo del mail!\n");
				exit(-1);
			}
			crea_messaggio("M", argv[pos+1]);
			kill(pid, SIGUSR1);
			fprintf(stderr, "Ok.\n");
			exit(0);
		case 'n':
			no_nuovi_utenti = 1;
			citta_log("SYSTEM: Accesso Limitato -- niente nuovi utenti.");
			break;
		case 's':
			fprintf(stderr, "Esegui shutdown del server Cittadella/UX adesso.\n");
			pid = serverlock_check();
			if (pid == 0) {
				fprintf(stderr, ERR_NOSERVER);
				exit(-1);
			}
			fprintf(stderr, "Invio segnale SIGHUP al processo %d.\n", pid);
			kill(pid, SIGHUP);
			exit(0);
		default:
			citta_logf("SYSERR: Opzione sconosciuta nella "
			     "linea di comando -%c.", *(argv[pos] +1));
			break;
		}
		pos++;
	}

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			printf("Uso: %s [-a] [-d] [-f] [-n] [-s] [# porta]\n",
			       argv[0]);
			exit(0);
		} else if ((porta = atoi(argv[pos])) <= 1024) {
			printf("Numero di porta non lecito.\n");
			exit(0);
		}
	}

	/* Controlla se Cittadella sta girando */
	if (serverlock_check()) {
		fprintf(stderr, "\nAttenzione! Il server Cittadella/UX sta gia` girando.\n"
			"Cancella il file %s per avviare il server.\n\n",
			SERVER_LOCK);
		exit(-1);
	}

 	if(getrlimit(RLIMIT_CORE, &rlim)) {
                printf("problemi col core");
 	} else {
                if(core > rlim.rlim_max) {
 			printf("non posso aumentare il core");
                } else{
 			rlim.rlim_cur=core;
                        if(setrlimit(RLIMIT_CORE, &rlim))
                                printf("problemi col core");
                }
 	}

	if (daemon_server) {
		/* Si stacca dal tty */
		if ( (pid = fork()) != 0) { /* parent process */
			printf("Server Cittadella/UX %s avviato alla porta %d.\n",
			       SERVER_RELEASE, porta);
			exit(0);
		}

		/* Diventa session leader */
		if (setsid() == -1) {
			fprintf(stderr, "ERROR: setsid() failure - exit server.\n");
			exit(1);
		}

		if ( (pid = fork()) != 0)
			exit(0);

		/* Apre il file per il log di sistema. */
		logfile = fopen(FILE_SYSLOG, "a+");
		if (logfile == NULL) {
			fprintf(stderr, "Apertura file %s fallita.\n",
                                FILE_SYSLOG);
			exit(1);
		}
	}

	serverlock_create();

	citta_logf("SYSTEM: Avvio del server alla porta %d.", porta);

	avvio_server(porta);

	shutdown_server();

	restart_server();

	return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/*
 * Apre i socket, carica tutti i dati, inizializza tutte le strutture e
 * lascia il controllo al ciclo_server().
 */
void avvio_server(int porta)
{
	time_t ora;
	struct tm *tmst;

#ifdef USE_CITTAWEB
	sessioni_http = NULL;
#endif

#ifdef USE_POP3
        sessioni_pop3 = NULL;
#endif

#ifdef USE_IMAP4
        sessioni_imap4 = NULL;
#endif

	lista_sessioni = NULL;

	/* Initialize the random numbers generator */
	random_init();

	citta_log("Cattura segnali.");
	setup_segnali();

	/* Inizializzazione per il prossimo reboot */
	timestamp_write(LAST_REBOOT);   /* Serve ? */
	time(&ora);
	ora += REBOOT_INTERVAL * 24 * 3600;
	tmst = localtime(&ora);
	reboot_day = tmst->tm_mday;

	/* Inizializza l'allocatore di memoria. */
#ifdef USE_MEM_STAT
	citta_log("Inizializzazione Allocatore Memoria.");
	mem_init();
#endif

	/* Legge la configurazione del sistema dal file sysconfig.rc */
	citta_log("Legge configurazione di sistema.");
	legge_configurazione();

	/* Legge i dati degli utenti e fa qualche check */
	citta_log("Lettura dati del server, utenti, file message e room.");
	carica_dati_server();
	fm_load_data();
	room_load_data();
#ifdef USE_FLOORS
	floor_load_data();
#endif
#ifdef USE_CESTINO
	msg_load_dump();
#endif
	carica_utenti();
	utr_check();
	mail_check();
#ifdef USE_REFERENDUM
	rs_load_data();
#endif

	/* Inizializza la cache */
#ifdef USE_CACHE_POST
	citta_log("Inizializzazione cache per i post.");
	cp_init();
#endif

	/* Inizializzazione blog */
#ifdef USE_BLOG
	blog_init();
#endif

        /* Inizializza file server */
        fs_init();
        fs_delete_overwritten();

        /* Inizializza albero dei prefissi per la ricerca nei messaggi */
#ifdef USE_FIND
	citta_log("Inizializzazione albero di ricerca nei messaggi.");
	find_init();
#endif

	/* Prepara il login banner */
	hash_banners();
	banner_load_hash();

        /* TODO togliere: solo per debug */
#if 0
        citta_logf("dati_server.fm_num:    %ld", dati_server.fm_num);
        citta_logf("dati_server.fm_curr:   %ld", dati_server.fm_curr);
        citta_logf("dati_server.fm_basic:  %ld", dati_server.fm_basic);
        citta_logf("dati_server.fm_mail:   %ld", dati_server.fm_mail);
        citta_logf("dati_server.fm_normal: %ld", dati_server.fm_normal);
        citta_logf("dati_server.fm_blog:   %ld", dati_server.fm_blog);
        citta_logf("dati_server.blog_nslot:%ld", dati_server.blog_nslot);
#endif

        /* Inizializza il server BBS */
	citta_log("Apertura connessione madre.");
	conn_madre = iniz_socket(porta);

#ifdef USE_REMOTE_PORT
	/* Generate the secure key for the remote client identification */
	init_remote_key();
	citta_logf("Lancio demone per conn remote a porta %d.", REMOTE_PORT);
	init_remote_daemon();
#endif

#ifdef USE_CITTAWEB
	/* Inizializza il Cittaweb */
	webstr_init();
	citta_logf("Apro porta %d per connessioni HTTP.", HTTP_PORT);
	http_socket = iniz_socket(HTTP_PORT);
#endif

#ifdef USE_POP3
	/* Inizializza il server POP3 */
	citta_logf("Apro porta %d per connessioni POP3.", POP3_PORT);
	pop3_socket = iniz_socket(POP3_PORT);
#endif

#ifdef USE_IMAP4
	/* Inizializza il server IMAP4 */
	citta_logf("Apro porta %d per connessioni IMAP4.", IMAP4_PORT);
	imap4_socket = iniz_socket(IMAP4_PORT);
#endif

	citta_log("Avvio ciclo principale del server.");
	dati_server.server_runs++;

	ciclo_server(conn_madre);
}

/*
 * Reboot del server: esegue gli script di logrotate e backup,
 * poi esegue un nuovo server.
 */
void restart_server(void)
{
	time_t ora, t0;
	bool scripts = false;

	if (!citta_reboot)
		citta_log("SYSTEM: Reboot del server.");
	else
		citta_log("SYSTEM: Normale chiusura del server.");
	fclose(logfile);

	if (force_script_check()) {
		system(SCRIPT_LOGROT);
		timestamp_write(LAST_LOGROT);
		system(SCRIPT_BACKUP);
		timestamp_write(LAST_BACKUP);
		force_script_delete();
		scripts = true;
	}

	if (!citta_reboot)
		return;

	if (!daemon_server)
		return;

	if (!scripts) {
		time(&ora);

		t0 = timestamp_read(LAST_LOGROT);
		if ((ora-t0) > (LOGROT_MIN_INTERVAL * 3600 - 600)) {
			system(SCRIPT_LOGROT);
			timestamp_write(LAST_LOGROT);
		}

		t0 = timestamp_read(LAST_BACKUP);
		if ((ora-t0) > (BACKUP_MIN_INTERVAL * 3600 - 600)) {
			system(SCRIPT_BACKUP);
			timestamp_write(LAST_BACKUP);
		}
	}

	if (execl("./bin/cittaserver", "cittaserver", NULL) == -1)
		fprintf(stderr, "Could not restart the server.\n");
}

/* Chiude tutte le connessioni, salva tutti i dati ed elimina lock. */
void shutdown_server(void)
{
	chiusura_server(conn_madre);

#ifdef USE_CITTAWEB
	http_chiusura_cittaweb(http_socket);
#endif

#ifdef USE_POP3
	pop3_chiusura_server(pop3_socket);
#endif

#ifdef USE_IMAP4
	imap4_chiusura_server(imap4_socket);
#endif

	serverlock_delete();
}

/*****************************************************************************/
/*****************************************************************************/

void ciclo_server(int s)
{
	fd_set input_set, output_set, exc_set;
	struct sessione *punto, *prossimo, *punto1;
#ifdef USE_CITTAWEB
	fd_set http_input_set, http_output_set, http_exc_set;
	struct http_sess *http_p, *http_next;
#endif
#ifdef USE_POP3
	fd_set pop3_input_set, pop3_output_set, pop3_exc_set;
	struct pop3_sess *pop3_p, *pop3_next;
#endif
#ifdef USE_IMAP4
	fd_set imap4_input_set, imap4_output_set, imap4_exc_set;
	struct imap4_sess *imap4_p, *imap4_next;
#endif
	struct timeval t_prec, ora, trascorso, timeout, t_nullo;
	static struct timeval tictac;
	char comm[MAX_STRINGA];
	int cicli = 0, tot_idle;
	int min_da_crashsave = 0;
	int mask;
	int socket_connessi, socket_attivi;
	char buf[LBUF];
        size_t buflen;
	struct tm *tmst;
        /*	int contatore = 0;   */

	maxdesc = s;
#ifdef USE_CITTAWEB
	http_maxdesc = http_socket;
#endif
#ifdef USE_POP3
	pop3_maxdesc = pop3_socket;
#endif
#ifdef USE_IMAP4
	imap4_maxdesc = imap4_socket;
#endif

#if defined (OPEN_MAX)
	max_num_desc = OPEN_MAX - 20; /* rivedere in base a quanti files usa */
#else
	max_num_desc = MAX_DESC_DISP;
#endif

	max_num_desc = MIN(max_num_desc, MAX_CONNESSIONI);

	mask = sigmask(SIGUSR1) | sigmask(SIGUSR2) | sigmask(SIGINT) |
		sigmask(SIGPIPE) | sigmask(SIGALRM) | sigmask(SIGTERM) |
		sigmask(SIGURG) | sigmask(SIGXCPU) | sigmask(SIGHUP) |
		sigmask(SIGSEGV) | sigmask(SIGBUS);

	t_nullo.tv_sec = 0;
	t_nullo.tv_usec = 0;

        TIC_TAC = 1000000 / FREQUENZA;/* microsecondi tra i cicli del server */

	tictac.tv_sec = 0;
	tictac.tv_usec = TIC_TAC; /* microseconds between two cycles */
	gettimeofday(&t_prec, (struct timezone *) 0);

	/* Ciclo Principale */

	while (!chiudi) {
		/* Controlla il tempo trascorso dalla iteraz precedente */
		gettimeofday(&ora, (struct timezone *) NULL);

		/*
		contatore++;
		if (contatore == FREQUENZA) {
			citta_logf("t_prec    %lds. %ldus, timeout %lds. %ldus",
			     t_prec.tv_sec, t_prec.tv_usec,
			     timeout.tv_sec, timeout.tv_usec);
		}
		*/

		/* trascorso = ora - t_prec                               */
		/* timeout   = tictac - trascorso = tictac + t_prec - ora */
		/* t_prec = ora + timeout = t_prec + tictac               */
		if (timeval_subtract(&trascorso, &ora, &t_prec)) {
			/* citta_log("TIME: trascorso negativo!"); */
			trascorso.tv_sec = 0;
			trascorso.tv_usec = 0;
		}
		if (timeval_subtract(&timeout, &tictac, &trascorso)) {
			/* citta_log("TIME: timeout negativo!"); */
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
		}
		if (timeval_add(&t_prec, &ora, &timeout)) {
			/* citta_log("TIME: t_prec negativo!"); */
                        /* TODO aggiungere tictac finche diventa positivo */
			t_prec.tv_sec = 0;
			t_prec.tv_usec = 0;
		}
		/*
		t_prec.tv_sec = ora.tv_sec + timeout.tv_sec;
		t_prec.tv_usec = ora.tv_usec + timeout.tv_usec;
		if (t_prec.tv_usec >= 1000000) {
			t_prec.tv_usec -= 1000000;
			t_prec.tv_sec++;
		}
		*/

                /*
		if (contatore == FREQUENZA) {
			citta_logf("trascorso %lds. %ldus, timeout %lds. %ldus",
			     trascorso.tv_sec, trascorso.tv_usec,
			     timeout.tv_sec, timeout.tv_usec);
			contatore = 0;
		}
                */

		/* Controlla cosa accade al mondo */
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(s, &input_set);

		for (punto = lista_sessioni; punto; punto = punto->prossima) {
			FD_SET(punto->socket_descr, &input_set);
			FD_SET(punto->socket_descr, &exc_set);
			FD_SET(punto->socket_descr, &output_set);
		}

		sigsetmask(mask); /* TODO deprecated: use sigpromask instead */

		/* Aspetto la fine del ciclo. */
		if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0,
			   &timeout) < 0) {
			if (errno != EINTR) {
				Perror("Select sleep");
				exit(1);
			}
		}

		if (select(maxdesc + 1, &input_set, &output_set,
			   &exc_set, &t_nullo) < 0) {
			if (errno != EINTR) {
				Perror("Select poll");
				return;
			}
		}

		sigsetmask(0); /* TODO deprecated: use sigpromask instead */

		/* Risponde a quello che accade al mondo */

		/* Nuova Connessione ? */
		if (FD_ISSET(s, &input_set)) {
			char host[LBUF];
			int desc;

			desc = nuovo_descr(s, host);
			if (desc < 0)
				Perror ("Nuova connessione");
			else if (desc == 0) {
				dati_server.connessioni++;
				citta_log("Connessione rifiutata: troppe conn.");
			} else {
				dati_server.connessioni++;
				session_create(desc, host);
			}
		}

		/* Caccia un po' di gente */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			/* NOTA: E' giusto cacciare quando si verificano */
                        /*       le EXCEPRIONAL CONDITIONS ? */
			if (FD_ISSET(punto->socket_descr, &exc_set)) {
				FD_CLR(punto->socket_descr, &input_set);
				FD_CLR(punto->socket_descr, &output_set);
				notify_logout(punto, DROP);
				chiudi_sessione(punto);
			}
		}

		/* Incrementa l'idle */
		for (punto = lista_sessioni; punto; punto = punto->prossima) {
			punto->idle.cicli++;
			if (punto->idle.cicli >= (60 * FREQUENZA)) {
				punto->idle.min++;
				punto->idle.cicli = 0;
			}
			if (punto->idle.min >= 60) {
				punto->idle.ore++;
				punto->idle.min = 0;
			}
			/* idle per la chat */
			punto->chat_timeout++;
		}

		/* Controlla il timeout nelle room con lock */
		for (punto = lista_sessioni; punto; punto = punto->prossima) {
			if (punto->rlock_timeout && (punto->stato==CON_POST)){
				punto->rlock_timeout--;
				if (punto->rlock_timeout == 0)
					post_timeout(punto);
			}
		}

		/* Legge un po' di input */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			if (FD_ISSET(punto->socket_descr, &input_set)) {
				if (elabora_input(punto) < 0) {
					notify_logout(punto, DROP);
					chiudi_sessione(punto);
				}
			}
		}

		/* Esamina un po' di comandi */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			/* 'attesa' e` proprio inutile cosi'... */
			if (--(punto->attesa) <= 0 ) {
				buflen = prendi_da_coda(&punto->input, comm,
                                                        MAX_STRINGA);
				punto->bytes_in += buflen; /* statistiche.. */
				if (buflen) {
					punto->attesa = 1;
					interprete_comandi(punto, comm);
				}
			}
		}
                for (punto = lista_sessioni; punto; punto = prossimo) {
                        prossimo = punto->prossima;
                        if(punto->cancella)
                                chiudi_sessione(punto);
                }

		/* Spedisce un po' di output */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			/* Trasferiamo un po' di output da coda a buffer */
			//while ( (punto->output.partenza != NULL) )
                        while ( (punto->output.partenza != NULL) &&
				//(MAX_STRINGA-iobuf_olen(&punto->iobuf)>punto->output.partenza->len+1))
				(MAX_STRINGA-1 > iobuf_olen(&punto->iobuf)))
				punto->bytes_out +=
                                        prendi_da_coda_o(&punto->output,
                                                         &punto->iobuf);
			/* Invia l'output */
			if (FD_ISSET(punto->socket_descr, &output_set) &&
			    (iobuf_olen(&punto->iobuf) > 0 )) {
				if (scrivi_a_client_iobuf(punto, &punto->iobuf)
				    == -1) {
                                        chiudi_sessione(punto);
				}
                        }
		}

		/* Cacciamo chi e' idle */
		if (butta_fuori_se_idle) {
			for (punto = lista_sessioni; punto; punto = prossimo) {
				prossimo = punto->prossima;
				if ((punto->stato >= CON_LIC)
				    && (punto->stato != CON_LOCK)) {
					tot_idle=punto->idle.min
						+ 60 * punto->idle.ore;
					if ((tot_idle >= max_min_idle) &&
					    (punto->idle.warning == 2)) {
						punto->stato = CON_CHIUSA;
						sprintf(comm,
							"%d Idle logout.\n",
							ITOU);
						scrivi_a_client(punto,comm);
						notify_logout(punto, LOID);
					} else if ((tot_idle>=min_idle_logout)
						   &&(punto->idle.warning==1)){
						punto->idle.warning = 2;
						idle_cmd(punto);
					} else if ((tot_idle>=min_idle_warning)
						   &&(punto->idle.warning==0)){
						punto->idle.warning = 1;
						idle_cmd(punto);
					}
				}
			}
		}

		/* Caccio dalla Chat gli utenti idle */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			if (punto->canale_chat
			    && (punto->chat_timeout >
				(unsigned long)CHAT_TIMEOUT*60*FREQUENZA))
				chat_timeout(punto);
		}

		/* Cacciamo chi dorme al login */
		sprintf(comm, "%d Login Timeout.\n", TOUT);
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			if ((punto->stato == CON_LIC)
			    && (punto->idle.min >= login_timeout_min)) {
				citta_logf("Login timeout da [%s].", punto->host);
				punto->stato = CON_CHIUSA;
				if (scrivi_a_client(punto, comm) < 0)
					chiudi_sessione(punto);
			}
		}

		/* Cacciamo un altro po' di gente */
		for (punto = lista_sessioni; punto; punto = prossimo) {
			prossimo = punto->prossima;
			if (punto->stato == CON_CHIUSA)
				chiudi_sessione(punto);
		}

		/* (Bene, ora non dovrebbe essere rimasto piu' nessuno ;) */

#ifdef USE_CITTAWEB
		/* Controlla cosa accade nel web */
		FD_ZERO(&http_input_set);
		FD_ZERO(&http_output_set);
		FD_ZERO(&http_exc_set);
		FD_SET(http_socket, &http_input_set);

		for (http_p = sessioni_http; http_p ; http_p = http_p->next) {
			FD_SET(http_p->desc, &http_input_set);
			FD_SET(http_p->desc, &http_exc_set);
			FD_SET(http_p->desc, &http_output_set);
		}

		sigsetmask(mask); /* TODO deprecated: use sigpromask instead */

		if (select(http_maxdesc + 1, &http_input_set, &http_output_set,
			   &http_exc_set, &t_nullo) < 0) {
			citta_log("HTTP select");
			if (errno != EINTR) {
				Perror("HTTP: Select poll");
				return;
			}
		}

		sigsetmask(0); /* TODO deprecated: use sigpromask instead */

		/* Risponde a quello che accade al mondo */

		/* gestione richiesta http */
		if (FD_ISSET(http_socket, &http_input_set)) {
			citta_log("Nuova connessione http in arrivo.");
			if (http_nuovo_descr(http_socket) < 0)
				Perror ("HTTP: Errore su nuova connessione");
			dati_server.http_conn++;
		}

		/* Gestisce le eccezioni delle connessioni HTTP */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			if (FD_ISSET(http_p->desc, &http_exc_set)) {
				citta_log("Eccezione HTTP");
				FD_CLR(http_p->desc, &http_input_set);
				FD_CLR(http_p->desc, &http_output_set);
				http_chiudi_socket(http_p);
			}
		}

		/* Incrementa l'idle sessioni HTTP */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			http_p->idle.cicli++;
			if (http_p->idle.cicli >= (60 * FREQUENZA)) {
				http_p->idle.min++;
				http_p->idle.cicli = 0;
			}
			if (http_p->idle.min >= 60) {
				http_p->idle.ore++;
				http_p->idle.min = 0;
			}
		}

		/* Esamina le eventuali richieste HTTP */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			if (FD_ISSET(http_p->desc, &http_input_set)) {
				if (http_elabora_input(http_p) < 0)
					http_chiudi_socket(http_p);
			}
		}

		/* Esamina un po' di comandi */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			if (prendi_da_coda(&http_p->input, comm, MAX_STRINGA))
				http_interprete_comandi(http_p, comm);
		}

		/* Invia la risposta HTTP */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			/* Trasferiamo un po' di output da coda a buffer */
                        while ((http_p->output.partenza != NULL) &&
                               (MAX_STRINGA-1>iobuf_olen(&http_p->iobuf)))
                                prendi_da_coda_o(&http_p->output,
                                                 &http_p->iobuf);
			/* Invia l'output */
			if (FD_ISSET(http_p->desc, &http_output_set) &&
			    (iobuf_olen(&http_p->iobuf) > 0 ))
				if (scrivi_a_desc_iobuf(http_p->desc,
                                                        &http_p->iobuf) < 0)
					http_chiudi_socket(http_p);
		}

		/* ora chiudiamo i socket chiudibili ? */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
                        if (http_p->finished && (iobuf_olen(&http_p->iobuf)==0)
                            && (http_p->output.partenza == NULL))
                                http_chiudi_socket(http_p);
		}

		/* Caccio sessioni HTTP idle */
		for (http_p = sessioni_http; http_p; http_p = http_next) {
			http_next = http_p->next;
			http_p->idle.cicli++;
                        tot_idle=http_p->idle.min + 60 * http_p->idle.ore;
                        if (tot_idle > HTTP_IDLE_TIMEOUT)
                                http_p->finished = true;
		}

#endif /* USE_CITTAWEB */

#ifdef USE_POP3
		/* Controlla cosa accade nel pop3 */
		FD_ZERO(&pop3_input_set);
		FD_ZERO(&pop3_output_set);
		FD_ZERO(&pop3_exc_set);
		FD_SET(pop3_socket, &pop3_input_set);

		for (pop3_p = sessioni_pop3; pop3_p ; pop3_p = pop3_p->next) {
			FD_SET(pop3_p->desc, &pop3_input_set);
			FD_SET(pop3_p->desc, &pop3_exc_set);
			FD_SET(pop3_p->desc, &pop3_output_set);
		}

		sigsetmask(mask);

		if (select(pop3_maxdesc + 1, &pop3_input_set, &pop3_output_set,
			   &pop3_exc_set, &t_nullo) < 0) {
			citta_log("POP3 select");
			if (errno != EINTR) {
				Perror("POP3: Select poll");
				return;
			}
		}

		sigsetmask(0);

		/* Risponde a quello che accade al mondo */

		/* gestione richiesta pop3 */
		if (FD_ISSET(pop3_socket, &pop3_input_set)) {
			citta_log("Nuova connessione POP3 in arrivo.");
			if (pop3_nuovo_descr(pop3_socket) <0)
				Perror ("POP3: Errore su nuova connessione");
			else
				citta_logf("POP3: Connessione avvenuta.");
			/* dati_server.pop3_conn++; */
		}

		/* Gestisce le eccezioni delle connessioni POP3 */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			if (FD_ISSET(pop3_p->desc, &pop3_exc_set)) {
				citta_log("Eccezione POP3");
				FD_CLR(pop3_p->desc, &pop3_input_set);
				FD_CLR(pop3_p->desc, &pop3_output_set);
				pop3_chiudi_socket(pop3_p);
			}
		}

		/* Incrementa l'idle sessioni POP3 */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			pop3_p->idle.cicli++;
			if (pop3_p->idle.cicli >= (60 * FREQUENZA)) {
				pop3_p->idle.min++;
				pop3_p->idle.cicli = 0;
			}
			if (pop3_p->idle.min >= 60) {
				pop3_p->idle.ore++;
				pop3_p->idle.min = 0;
			}
		}

		/* Esamina le eventuali richieste POP3 */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			if (FD_ISSET(pop3_p->desc, &pop3_input_set)) {
				if (pop3_elabora_input(pop3_p) < 0)
					pop3_chiudi_socket(pop3_p);
			}
		}

		/* Esamina un po' di comandi */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;

			if (prendi_da_coda(&pop3_p->input, comm, MAX_STRINGA))
				pop3_interprete_comandi(pop3_p, comm);
		}

		/* Invia la risposta POP3 */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			/* Trasferiamo un po' di output da coda a buffer */
			while ( (pop3_p->output.partenza != NULL) &&
				((MAX_STRINGA - strlen(pop3_p->out_buf)) >
				 (strlen(pop3_p->output.partenza->testo)+1)) ) {
				prendi_da_coda(&(pop3_p->output), comm,
                                               MAX_STRINGA);
				strcat(pop3_p->out_buf, comm);
			}
			/* Invia l'output */
			if (FD_ISSET(pop3_p->desc, &pop3_output_set) &&
			    (strlen(pop3_p->out_buf) > 0 ))
				if (scrivi_a_desc(pop3_p->desc,pop3_p->out_buf)
				    < 0)
					pop3_chiudi_socket(pop3_p);
		}

		/* ora chiudiamo i socket chiudibili ? */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			if(pop3_p->finished && *(pop3_p->out_buf) == 0 &&
			   pop3_p->output.partenza == NULL)
				pop3_chiudi_socket(pop3_p);
		}

		/* Caccio sessioni POP3 idle */
		for (pop3_p = sessioni_pop3; pop3_p; pop3_p = pop3_next) {
			pop3_next = pop3_p->next;
			pop3_p->idle.cicli++;
                        tot_idle=pop3_p->idle.min + 60 * pop3_p->idle.ore;
                        if (tot_idle > POP3_IDLE_TIMEOUT)
                                pop3_p->finished = true;
		}

#endif /* USE_POP3 */

#ifdef USE_IMAP4
		/* Controlla cosa accade nel imap4 */
		FD_ZERO(&imap4_input_set);
		FD_ZERO(&imap4_output_set);
		FD_ZERO(&imap4_exc_set);
		FD_SET(imap4_socket, &imap4_input_set);

		for (imap4_p=sessioni_imap4; imap4_p; imap4_p=imap4_p->next) {
			FD_SET(imap4_p->desc, &imap4_input_set);
			FD_SET(imap4_p->desc, &imap4_exc_set);
			FD_SET(imap4_p->desc, &imap4_output_set);
		}

		sigsetmask(mask);

		if (select(imap4_maxdesc + 1, &imap4_input_set,
			   &imap4_output_set, &imap4_exc_set, &t_nullo) < 0) {
			citta_log("IMAP4 select");
			if (errno != EINTR) {
				Perror("IMAP4: Select poll");
				return;
			}
		}

		sigsetmask(0);

		/* Risponde a quello che accade al mondo */

		/* gestione richiesta imap4 */
		if (FD_ISSET(imap4_socket, &imap4_input_set)) {
			citta_log("Nuova connessione IMAP4 in arrivo.");
			if (imap4_nuovo_descr(imap4_socket) <0)
				Perror ("IMAP4: Errore su nuova connessione");
			else
				citta_logf("IMAP4: Connessione avvenuta.");
			/* dati_server.imap4_conn++; */
		}

		/* Gestisce le eccezioni delle connessioni IMAP4 */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			if (FD_ISSET(imap4_p->desc, &imap4_exc_set)) {
				citta_log("Eccezione IMAP4");
				FD_CLR(imap4_p->desc, &imap4_input_set);
				FD_CLR(imap4_p->desc, &imap4_output_set);
				imap4_chiudi_socket(imap4_p);
			}
		}

		/* Incrementa l'idle sessioni IMAP4 */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			imap4_p->idle.cicli++;
			if (imap4_p->idle.cicli >= (60 * FREQUENZA)) {
				imap4_p->idle.min++;
				imap4_p->idle.cicli = 0;
			}
			if (imap4_p->idle.min >= 60) {
				imap4_p->idle.ore++;
				imap4_p->idle.min = 0;
			}
		}

		/* Esamina le eventuali richieste IMAP4 */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			if (FD_ISSET(imap4_p->desc, &imap4_input_set)) {
				if (imap4_elabora_input(imap4_p) < 0)
					imap4_chiudi_socket(imap4_p);
			}
		}

		/* Esamina un po' di comandi */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;

			if (prendi_da_coda(&imap4_p->input, comm, MAX_STRINGA))
				imap4_interprete_comandi(imap4_p, comm);
		}

		/* Invia la risposta IMAP4 */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			/* Trasferiamo un po' di output da coda a buffer */
			while ( (imap4_p->output.partenza != NULL) &&
				((MAX_STRINGA - strlen(imap4_p->out_buf)) >
				 (strlen(imap4_p->output.partenza->testo)+1))){
				prendi_da_coda(&(imap4_p->output), comm,
                                               MAX_STRINGA);
				strcat(imap4_p->out_buf, comm);
			}
			/* Invia l'output */
			if (FD_ISSET(imap4_p->desc, &imap4_output_set) &&
			    (strlen(imap4_p->out_buf) > 0 ))
				if (scrivi_a_desc(imap4_p->desc,imap4_p->out_buf)
				    < 0)
					imap4_chiudi_socket(imap4_p);
		}

		/* ora chiudiamo i socket chiudibili ? */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			if(imap4_p->finished && *(imap4_p->out_buf) == 0 &&
			   imap4_p->output.partenza == NULL)
				imap4_chiudi_socket(imap4_p);
		}

		/* Caccio sessioni IMAP4 idle */
		for (imap4_p = sessioni_imap4; imap4_p; imap4_p = imap4_next) {
			imap4_next = imap4_p->next;
			imap4_p->idle.cicli++;
                        tot_idle=imap4_p->idle.min + 60 * imap4_p->idle.ore;
                        if (tot_idle > IMAP4_IDLE_TIMEOUT)
                                imap4_p->finished = true;
		}

#endif /* USE_IMAP4 */

		/* E infine un po' di contabilita' */
		cicli++;

		if (auto_save)
			if (!(cicli % (60 * FREQUENZA)))   /* un minuto */
				if(++min_da_crashsave >= tempo_auto_save) {
					min_da_crashsave = 0;
					crash_save_perfavore();
				}

		if (!(cicli % (60 * 5 * FREQUENZA))) {  /* ogni 5 minuti */
			socket_connessi = 0;
			socket_attivi = 0;
			for (punto = lista_sessioni; punto; punto = prossimo) {
				prossimo = punto->prossima;
				socket_connessi++;
				if (punto->logged_in)
					socket_attivi++;
			}
#ifdef USE_MEM_STAT
			citta_logf("carico: sock. %d conn. %d attivi. Mem %ld bytes.",
                              socket_connessi, socket_attivi, mem_tot());
#else
			citta_logf("carico: sock. %d conn. %d attivi",
			     socket_connessi, socket_attivi);
#endif

#ifdef USE_REFERENDUM
			/* Controlla se e' terminato qualche sondaggio */
			rs_expire();
#endif
#ifdef USE_RUSAGE
			{
				struct rusage rusagedata;

				if (getrusage(RUSAGE_SELF, &rusagedata) == 0)
#ifdef MACOSX
					citta_logf("carico: User time: %lds %dms  Sys time: %lds %dms %ld %ld %ld",
						rusagedata.ru_utime.tv_sec,
						rusagedata.ru_utime.tv_usec/1000,
						rusagedata.ru_stime.tv_sec,
						rusagedata.ru_stime.tv_usec/1000,
						rusagedata.ru_minflt,
						rusagedata.ru_majflt,
						rusagedata.ru_nswap);
#else
					citta_logf("carico: User time: %lds %ldms  Sys time: %lds %ldms %ld %ld %ld",
						rusagedata.ru_utime.tv_sec,
						rusagedata.ru_utime.tv_usec/1000,
						rusagedata.ru_stime.tv_sec,
						rusagedata.ru_stime.tv_usec/1000,
						rusagedata.ru_minflt,
						rusagedata.ru_majflt,
						rusagedata.ru_nswap);
#endif
				else
					citta_log("rusage error");
			}
#endif
		}

		if (citta_shutdown) {
			citta_sdwntimer--;
			if (citta_sdwntimer<=0)
				chiudi = 1;
			else
                                /* Notifica lo shutdown ogni 5 minuti */
                                /* ed ogni minuto gli ultimi 5 minuti */
				if ( !(citta_sdwntimer % (60 * FREQUENZA)) &&
				     ((citta_sdwntimer <= (5*60*FREQUENZA)) ||
				      !(citta_sdwntimer%(5*60*FREQUENZA)))) {
					sprintf(buf, "%d %d|%d\n", SDWN,
						(int)citta_sdwntimer
						/(60*FREQUENZA),
						citta_shutdown);
                                        buflen = strlen(buf);
					for (punto1 = lista_sessioni; punto1;
					     punto1 = punto1->prossima) {
						metti_in_coda(&punto1->output,
							      buf, buflen);
						punto1->bytes_out += buflen;
					}
				}
		}

		if (cicli >= (10 * 60 * FREQUENZA)) { /* ogni 10 minuti */
			cicli = 0;
#ifdef USE_VALIDATION_KEY
			/* Elimina utenti non validati dopo 48 ore */
			purge_invalid();
#endif

			/* check_reboot */
			if (REBOOT_INTERVAL) {
				tmst = localtime(&(ora.tv_sec));
				if ((tmst->tm_hour == DAILY_REBOOT_HOUR)
				    && (tmst->tm_mday == reboot_day)
				    && !citta_shutdown) {
					citta_log("Daily reboot in 15 minutes.");
					citta_reboot = true;
					citta_shutdown = 2;
					citta_sdwntimer = 15*60*FREQUENZA+1;
					/* 15 minuti di preavviso */
				}
			}
		}
		if (segnale_messaggio)
			esegue_messaggio();

		tics++; /* cicli dall'ultimo signal di checkpoint */
	}
}


/*****************************************************************************/
void chiusura_server(int s)
{
	chiusura_sessioni(s);

	citta_log("Salva le strutture dati.");
#ifdef USE_BLOG
	blog_close();
#endif
#ifdef USE_FLOORS
	floor_save_data();
#endif
	room_save_data();
	fm_save_data();
#ifdef USE_CESTINO
	msg_save_dump();
#endif
	salva_utenti();
	salva_dati_server();
#ifdef USE_REFERENDUM
	rs_save_data();
	rs_free_data();
#endif
	free_lista_utenti();
#ifdef USE_FLOORS
	floor_free_list();
#endif
	room_free_list();
#ifdef USE_CESTINO
	msg_dump_free();
#endif

        fs_close();

#ifdef USE_CACHE_POST
	cp_destroy();
#endif

#ifdef USE_FIND
        find_free();
#endif

#ifdef USE_MEM_STAT
	mem_log(); /* Verifica memory leaks: non dovrebbe rimanere nulla! */
#endif

#ifdef USE_REMOTE_PORT
	close_remote_daemon();
#endif
}

/*****************************************************************************/

/*
 * Invia al client una stringa formattata
 * (metti_in_coda() formattato)
 */
void cprintf(struct sessione *t, char *format, ...)
{
	va_list ap;
	int size;
#ifdef USE_MEM_STAT
	char *tmp;
#endif

	va_start(ap, format);
#ifdef USE_MEM_STAT
	/* TODO Non e' molto bello... si puo' fare di meglio... */
	size = vasprintf(&tmp, format, ap);
#else
	size=vasprintf(&nuovo->testo, format, ap);
#endif
	va_end(ap);
        metti_in_coda(&t->output, tmp, size);

        //citta_logf("OUT [%s]", tmp);

#ifdef USE_MEM_STAT
        free(tmp);
#endif
}

/*****************************************************************************/
/*
 * CRASH SAVE
 */
void crash_save_perfavore(void)
{
	salva_dati_server();
	salva_utenti();
#ifdef USE_BLOG
	blog_save();
#endif
#ifdef USE_FLOORS
	floor_save_data();
#endif
 	room_save_data();
	fm_save_data();
#ifdef USE_CESTINO
	msg_save_dump();
#endif
#ifdef USE_REFERENDUM
	rs_save_data();
#endif
        fs_save();
}

void carica_dati_server(void)
{
	FILE  *fp;
	int hh = 0;
	int i;

	fp = fopen(FILE_DATI_SERVER, "r");
	if (!fp)
		citta_log("SYSERR: Non posso aprire in lettura il file dati_server.");
	else {
		fseek(fp,0L,0);
		hh=fread((struct dati_server *) &dati_server,
			 sizeof(struct dati_server), 1, fp);
		fclose(fp);
	}
	if (hh != 1) {
		citta_log("SYSTEM: dati_server vuoto, creato al primo salvataggio.");
		dati_server.server_runs = 0;
		dati_server.matricola = 0;
		dati_server.connessioni = 0;
		dati_server.local_cl = 0;
		dati_server.remote_cl = 0;
		dati_server.login = 0;
		dati_server.ospiti = 0;
		dati_server.nuovi_ut = 0;
		dati_server.validazioni = 0;
		dati_server.max_users = 0;
		dati_server.max_users_time = 0;
		dati_server.http_conn = 0;
		dati_server.http_conn_ok = 0;
		dati_server.X = 0;
		dati_server.broadcast = 0;
		dati_server.mails = 0;
		dati_server.posts = 0;
		dati_server.chat = 0;
		dati_server.blog = 0;
		dati_server.referendum = 0;
		dati_server.sondaggi = 0;
		dati_server.fm_num = 0;
		dati_server.fm_curr = 0;
		dati_server.fm_basic = 0;
		dati_server.fm_mail = 0;
		dati_server.fm_normal = 0;
		dati_server.room_num = 0;
		dati_server.room_curr = 0;
		dati_server.utr_nslot = 0;
		dati_server.mail_nslot = MAIL_SLOTNUM;
		dati_server.mailroom = 0;
		dati_server.baseroom = 0;
		dati_server.blog_nslot = BLOG_SLOTNUM;
		dati_server.fm_blog = 0;
		dati_server.floor_num = 0;
		dati_server.floor_curr = 0;
		for (i = 0; i < 24; i++)
			dati_server.stat_ore[i] = 0;
		for (i = 0; i < 7; i++)
			dati_server.stat_giorni[i] = 0;
		for (i = 0; i < 12; i++)
			dati_server.stat_mesi[i] = 0;
		dati_server.ws_start = 0;
		for (i = 0; i < WS_DAYS; i++) {
			dati_server.ws_connessioni[i] = 0;
			dati_server.ws_local_cl[i] = 0;
			dati_server.ws_remote_cl[i] = 0;
			dati_server.ws_login[i] = 0;
			dati_server.ws_ospiti[i] = 0;
			dati_server.ws_nuovi_ut[i] = 0;
			dati_server.ws_validazioni[i] = 0;
			dati_server.ws_X[i] = 0;
			dati_server.ws_broadcast[i] = 0;
			dati_server.ws_mails[i] = 0;
			dati_server.ws_posts[i] = 0;
			dati_server.ws_read[i] = 0;
			dati_server.ws_chat[i] = 0;
		}
	}
}

/*
 * salva_utenti salva la lista di strutture di utenti nel file FILE_UTENTI
 */
void salva_dati_server(void)
{
	FILE *fp;
	char bak[LBUF];

	sprintf(bak, "%s.bak", FILE_DATI_SERVER);
	rename(FILE_DATI_SERVER, bak);

	fp = fopen(FILE_DATI_SERVER, "w");
	if (!fp) {
		citta_log("SYSERR: Non posso aprire in scrittura il file dati_server.");
		return;
	}

	fwrite((struct dati_server *) &dati_server,
		    sizeof(struct dati_server), 1, fp);
        /* TODO check that fwrite successful */
	fclose(fp);
}

void chiusura_sessioni(int s)
{
	citta_log("Chiusura di tutti i sockets.");

	while (lista_sessioni)
		chiudi_sessione(lista_sessioni);

	close(s);
}

void chiudi_sessione(struct sessione *d)
{
	struct sessione *tmp;

	/* Se e` in una room virtuale, prima la elimino */
	kill_virtual_room(d);

	if (d->utente) {
                citta_logf("Chiudo sessione di %s.", d->utente->nome);
		/* Aggiorna ultima chiamata, ultimo host e tempo online */
		strcpy(d->utente->lasthost, d->host);
		d->utente->time_online += (time(0) - d->ora_login);
		/* Se lockava una room, la libera                       */
		if ((d->room->lock) && (d->stato & CON_POST))
			d->room->lock = false;
                /* Elimina le prenotazioni di upload */
                fs_cancel_all_bookings(d->utente->matricola);
		/* Salva i dati */
		utr_save(d);
		utr_free(d);
		mail_save(d);
		mail_free(d->mail);
		logged_users--;
	}

	close(d->socket_descr);
	flush_code_sessione(d); /* VEDERE */
	if (d->socket_descr == maxdesc)
		--maxdesc;

	/* Eliminiamo i riferimenti alla sessione appena chiusa */
	if (d == lista_sessioni)
		lista_sessioni = lista_sessioni->prossima;
	else {
		for (tmp = lista_sessioni; tmp && (tmp->prossima != d);
		     tmp = tmp->prossima)
			;
                if (tmp)
                        tmp->prossima = d->prossima;
	}

        compress_free(d);

	Free(d);
	/*vedere, vedere, vedere...*/
}

static void flush_code_sessione(struct sessione *d)
{
        /* TODO usare iobuf */
	flush_coda(&d->input);
	flush_coda(&d->output);
}


void session_create(int desc, char *host)
{
	struct sessione *nuova_ses;

	/* Poi va creata una nuova struttura sessione per il nuovo arrivato */

  	CREATE(nuova_ses, struct sessione, 1, TYPE_SESSIONE);

	/* Inizializziamo la struttura coi dati della nuova sessione */

	nuova_ses->socket_descr = desc;
	/* TODO SOSTITUIRE '49' CON APPOSITA COSTANTE */
	strncpy(nuova_ses->host, host, 49);
#if ALLOW_COMPRESSION
	nuova_ses->compressione[0] = 0;  /* Di default non comprime */
#endif
	nuova_ses->utente = NULL;
	nuova_ses->logged_in = false;
	nuova_ses->stato = CON_LIC;
	nuova_ses->occupato = 1;
        nuova_ses->cancella = false;
	nuova_ses->ora_login = time(0); /* COMPLETARE */
	nuova_ses->idle.cicli = 0;
	nuova_ses->idle.min = 0;
	nuova_ses->idle.warning = 0;
	nuova_ses->canale_chat = 0;
	nuova_ses->chat_timeout = 0;
	nuova_ses->num_rcpt = 0;
	nuova_ses->doing = NULL;
	nuova_ses->text = NULL;
	nuova_ses->text_max = 0;
	nuova_ses->rdir = 0;
	nuova_ses->rlock_timeout = 0;
	nuova_ses->parm = NULL;
	nuova_ses->parm_com = 0;
	nuova_ses->parm_num = 0;
	nuova_ses->bytes_in = 0;
	nuova_ses->bytes_out = 0;
	nuova_ses->num_cmd = 0;
	nuova_ses->lastread = 0;
	nuova_ses->room = lista_room.first;
        nuova_ses->last_msgnum = -1;
        nuova_ses->upload_bookings = 0;
        nuova_ses->fp = NULL;
        nuova_ses->binsize = 0;
        init_coda(&nuova_ses->input);
        init_coda(&nuova_ses->output);
        iobuf_init(&nuova_ses->iobuf);
#ifdef USE_FLOORS
	nuova_ses->floor = floor_list.first;
#endif
	utr_alloc(nuova_ses);
	nuova_ses->mail = mail_alloc();

	/* Lo appiccichiamo in cima alla lista     */
	nuova_ses->prossima = lista_sessioni;
	lista_sessioni = nuova_ses;

}

/*****************************************************************************/
/* Funzioni per la gestione del file di lock del server.                     */
/*****************************************************************************/
/* Crea il lock file, che contiene il pid del server */
void serverlock_create(void)
{
	FILE * lockfile;
	pid_t pid = getpid();

	lockfile = fopen(SERVER_LOCK, "w");
	if (lockfile == NULL) {
		fprintf(stderr, "Non riesco a creare il lock file %s!\n",
			SERVER_LOCK);
		exit(-1);
	}
	fprintf(lockfile, "%u\n", pid);
	fclose(lockfile);
}

/* Controlla se Cittadella sta girando:
 * se il lockfile e' presente, restituisce il PID del processo
 * che lo ha creato, altrimenti restituisce 0.                 */
pid_t serverlock_check(void)
{
	FILE * lockfile;
	pid_t pid = getpid();

	lockfile = fopen(SERVER_LOCK, "r");
	if (lockfile == NULL)
		return 0;

	fscanf(lockfile, "%u\n", &pid);
	fclose(lockfile);

	return pid;
}

/* Elimina il lock file. Restituisce 0 in caso di successo, -1 altrimenti. */
int serverlock_delete(void)
{
	return unlink(SERVER_LOCK);
}
/*****************************************************************************/
/* Crea timestamp filename e ci salva l'ora attuale. */
void timestamp_write(char * filename)
{
	time_t ora;
	FILE * fp;

	time(&ora);
	fp = fopen(filename, "w");
	if (fp == NULL) {
		citta_logf("Non riesco a creare il timestamp %s.\n", filename);
		return;
	}
	fprintf(fp, "%lu\n", ora);
	fclose(fp);
}

/* Restituisce l'ora del timestamp filename */
time_t timestamp_read(char * filename)
{
	time_t ora;
	FILE * fp;

	time(&ora);
	fp = fopen(filename, "r");
	if (fp == NULL) {
		citta_logf("Non riesco ad aprire il timestamp %s.\n", filename);
		return 0;
	}
	fscanf(fp, "%lu\n", &ora);
	fclose(fp);

	return ora;
}

/*****************************************************************************/
/* Crea il file FORCE_SCRIPTS. */
static void force_script(void)
{
	FILE * fp;

	fp = fopen(FORCE_SCRIPTS, "w");
	if (fp == NULL) {
		citta_logf("Non riesco a creare il file %s.\n", FORCE_SCRIPTS);
		return;
	}
	fclose(fp);
}

/* Controlla se il file FORCE_SCRIPTS e' presente */
static bool force_script_check(void)
{
	FILE * fp;

	fp = fopen(FORCE_SCRIPTS, "r");
	if (fp == NULL)
		return false;
	fclose(fp);

	return true;
}

/* Elimina il lock file. Restituisce 0 in caso di successo, -1 altrimenti. */
static int force_script_delete(void)
{
	return unlink(FORCE_SCRIPTS);
}

/* Comando server per forzare l'esecuzione degli script */
void cmd_fsct(struct sessione *t)
{
	force_script();
	cprintf(t, "%d\n", OK);

}


/*****************************************************************************/
/* Crea un messaggio per il server. */
void crea_messaggio(char *cmd, char *arg)
{
	FILE * fp;

	fp = fopen(EXT_MESSAGE, "w");
	if (fp == NULL) {
		citta_logf("Non riesco a creare il file %s.\n", EXT_MESSAGE);
		return;
	}
	fprintf(fp, "%s\n", cmd);
	fprintf(fp, "%s", arg);
	fclose(fp);
}

/* Elimina il messaggio al server.
 * Restituisce 0 in caso di successo, -1 altrimenti. */
int cancella_messaggio(void)
{
	return unlink(EXT_MESSAGE);
}

/* Restituisce l'ora del timestamp filename */
void esegue_messaggio(void)
{
	char cmd;
	char buf[LBUF];
	FILE * fp;

	segnale_messaggio = false;

	fp = fopen(EXT_MESSAGE, "r");
	if (fp == NULL) {
		citta_logf("Non riesco ad aprire il messaggio %s.\n", EXT_MESSAGE);
		return;
	}
	fscanf(fp, "%c\n", &cmd);
        citta_logf("ADMIN Arrivato messaggio con comando %c.", cmd);
	switch (cmd) {
	case 'B': /* invia un broadcast */
		if (fgets(buf, 79, fp))
			citta_broadcast(buf);
		break;
	case 'm': /* invia un email a tutti gli utenti newsletterati */
		if (fgets(buf, 79, fp)) {
			if (send_email(NULL,buf,"Messaggio di amministrazione",
				       NULL, EMAIL_NEWSLETTER))
                                citta_log("Email di amministrazione inviato.");
			else
				citta_logf("Errore apertura file [%s].", buf);
		}
                break;
	case 'M': /* invia un email a tutti gli utenti */
		if (fgets(buf, 79, fp)) {
			if (send_email(NULL,buf,"Messaggio di amministrazione",
				       NULL, EMAIL_VALID))
                                citta_log("Email di amministrazione inviato.");
			else
				citta_logf("Errore apertura file [%s].", buf);
		}
                break;
	default:
		citta_logf("Messaggio con comando [%c] non riconosciuto.", cmd);
	}
	fclose(fp);
}

/*****************************************************************************/
