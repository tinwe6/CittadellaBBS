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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : config.h                                                           *
*        File di configurazione per il server di Cittadella/UX              *
****************************************************************************/
#ifndef _CONFIG_H
#define _CONFIG_H    1

/* COME USARE QUESTO FILE:

   Utilizzare #define e #undef per definire/non definire le variabili
   di interesse prima di compilare
*/

/* BBS Host                                                                 */
#define BBS_HOST        "localhost"
//#define BBS_HOST        "bbs.cittadellabbs.it"
/* Porta alla quale il server ascolta in attesa di nuove connessioni.       */
#define PORTA_DFLT      4000
/* Porta alla quale si trova il client remoto per le connessioni con telnet.
 * Se e' definita USE_CLIENT_PORT (vedi sotto) e' il client stesso che
 * si occupa di lanciare il client remoto, altrimenti va usata una tecnica
 * diversa (inetd o simile).                                                */
#define PORTA_REMOTA    4001

/* Configurazione del client remoto.
 * Il client remoto permette agli utenti di collegarsi chiamando con
 * il telnet una porta speciale che permette di usare un client che
 * gira sul server stesso. Per funzionare, e' necessario il programma
 * telnetd, che accetti lo switch -L, ad esempio
 * http://packages.debian.org/stable/source/netkit-telnet                   */
/* QUESTA FEATURE E' ANCORA IN SVILUPPO: NON DEFINIRE USE_CLIENT_PORT !!!   */
/* Lancia un demone che ascolta le connessioni al client remoto.            */
#undef USE_CLIENT_PORT
/* Path del telnetd: attenzione, deve avere lo switch -L.                   */
#define TELNETD "/usr/sbin/telnetd"
/* Path del client remoto                                                   */
#define REMOTE_CLIENT "../client/bin/remote_cittaclient"

/* Numero massimo di sessioni contemporanee.                                */
#define MAX_SESSIONI      30

/* Numero massimo di connessioni contemporanee.                             */
#define MAX_CONNESSIONI   40

/* Ora del reboot giornaliero.                                              */
#define DAILY_REBOOT_HOUR  4

/* Numero di giorni tra due reboot automatici.                              *
 * (1 = reboot quotidiano, 0 = non effettuare reboot)                       *
 * Attenzione: il backup automatico viene eseguito solo al momento          *
 *             del reboot                                                   */
#define REBOOT_INTERVAL      1

/* Numero minimo di ore tra due backup successivi.                          */
#define BACKUP_MIN_INTERVAL 24

/* Numero minimo di ore tra due logrotate successivi.                       */
#define LOGROT_MIN_INTERVAL 24

/* Processo di validazione: Se e' definita USE_VALIDATION_KEY, l'utente     *
 * deve fornire una chiave di validazione inviata via Email per accedere    *
 * a livello UTENTE NORMALE. Commentare la linea seguente se non si vuole   *
 * usare le chiavi di validazione.                                          */
//#define USE_VALIDATION_KEY

/* VALKEYLEN contiene la lunghezza della chiave di validazione.             */
#define VALKEYLEN 6

/* Se e' definita NO_DOUBLE_EMAIL durante la fase di registrazione non saranno
 * accettate email che coincidono a quelle di utenti gia' registrati. */
#define NO_DOUBLE_EMAIL

/* Se diverso da 0, permette di usare connessioni compresse.                */
#define ALLOW_COMPRESSION 1

/* Numero massimo di bytes inviati a un singolo utente in un ciclo          */
#define MAX_STRINGA  8*8192   /* Usare come minimo 256 */

/* Livelli iniziali degli utenti:                                           *
 * LIVELLO_INIZIALE: livello dell'utente al primo collegamento, prima della *
 *                   validazione (si consiglia LVL_NON_VALIDATO)            *
 * LIVELLO_VALIDATO: livello che l'utente raggiunge subito dopo la          *
 *                   validazione (si consiglia LVL_NORMALE)                 */
//#define LIVELLO_INIZIALE   LVL_AIDE
//#define LIVELLO_VALIDATO   LVL_AIDE
#define LIVELLO_INIZIALE   LVL_NON_VALIDATO
#define LIVELLO_VALIDATO   LVL_NORMALE

/* Tempo, in secondi, a disposizione dell'utente per immettere la           *
 * chiave di validazione. Scaduto il tempo, l'account viene cancellato.     */
#define TEMPO_VALIDAZIONE 7*24*3600             /* una settimana            */

/* Dimensioni di default del file dei messaggi creato inizialmente. Per     *
 * iniziare 100kb puo' andare bene, con 3Mb una bbs di dimensioni medie     *
 * gira tranquillamente. In ogni caso, in qualunque momento puoi espandere  *
 * la dimensione del file messaggi (mentre il tool per restringerlo non e'  *
 * attualmente pronto) percio' non temere di fare un file messaggi troppo   *
 * piccolo!                                                                 */
#define DFLT_FM_SIZE  3000000

/* Numero massimo di destinatari degli eXpress messages multipli            */
#define MAX_RCPT          10

/* Numero di canali chat disponibili agli utenti. Il canale # NCANALICHAT   *
 * e' la Sysop Chat.                                                        */
#define NCANALICHAT       3
/* Chat timeout: tempo in minuti dopo il quale l'utente viene cacciato      *
 * dalla chat se non invia messaggi.                                        */
#define CHAT_TIMEOUT      10

/* Definendo USE_FLOORS si attivano i floor per organizzare le room.        *
 * (implementazione in corso: attualmente non completa ma funzionante.)     */
/* #define USE_FLOORS   1 */

/* Definendo la variabile USE_REFERENDUM si attivano gli strumenti di voto  *
 * per sondaggi e referendum                                                */
#define USE_REFERENDUM   1

/* Se definito, l'utente riceve in mail una notifica automatica quando un   *
 * suo post viene cancellato (_DEL), spostato (_MOVE) o reimmeso (_UNDEL).  *
 * (l'attivazione di queste opzioni rende piu' trasparente la gestione      *
 * della BBS ed e` consigliata).                                            */
#define USE_NOTIFICA_DEL     1
#define USE_NOTIFICA_MOVE    1
#define USE_NOTIFICA_UNDEL   1

/* Attiva il cestino per cancellazione/undelete dei post.                   *
 * Se attivato, il <d>elete dei post sposta il messaggio nella dump_room,   *
 * senza cancellarlo definitivamente, lasciando la possibilita` al Sysop    *
 * di controllare l'operato dei Room Aide ed eventualmente effettuare un    *
 * undelete del post.                                                       */
#define USE_CESTINO   1

/* Attiva i Blog per la BBS                                                 */
#define USE_BLOG   1
/* Dimensione iniziale file messaggi per i blog                             */
#define BLOG_FM_SIZE 100000

/* Tempo a disposizione dell'utente per scrivere un post nelle room con     *
 * lock dei post e timeout, espresso in secondi.                            */
#define POSTLOCK_TIMEOUT 60

/* Attiva la possibilita' di effettuare ricerche di parole nei file nei     *
 * messaggi della BBS (disattivare nei sistemi con poca RAM)                */
/* Attenzione: bisogna settare anche USE_CACHE_POST piu' sotto.             */
#define USE_FIND   1
/* lunghezza minima parola memorizzata per il find.                         */
#define MINLEN_WORD 4

/*                                                                          *
 * Upload/Download dei files dalle directory rooms                          *
 *                                                                          */
/* Definire la variabile seguente per permettere l'upload/download dei file */
#define USE_FILESERVER
/* Dimensione massima, in bytes, del singolo file uploadato                 */
#define FILE_MAXSIZE 4*1024*1024
/* Spazio massimo disponibile per upload da un singolo utente               */
#define FILE_MAXUSER 20*1024*1024
/* Numero massimo di file contemporanei da un singolo utente                */
#define FILE_MAXNUM  100
/* Spazio totale disponibile per l'upload dei file                          */
#define FILE_DIRSIZE 100*1024*1024
/* Se definito, permette di scaricare da cittaweb i file                    */
#define FILE_ALLOW_WEB_DOWNLOAD


/* Definire USE_CITTAWEB per attivare la porta http del server, per la      *
 * pubblicazione in rete delle room                                         */
#define USE_CITTAWEB
#ifdef USE_CITTAWEB
/* Numero della porta che riceve le connessioni http                        */
#define HTTP_PORT 4040
/* Virtual Host per le connessioni http                                     */
#define HTTP_HOST   BBS_HOST
/* Numero di post per room da inviare con il cittaweb.                      */
#define CITTAWEB_POSTS 20
/* Tempo massimo di idle in minuti prima di venire cacciati.                */
#define HTTP_IDLE_TIMEOUT 10
#endif

/* Server POP3 per permettere di scaricare i mail ricevuti in BBS con il    *
 * proprio client email.                                                    */
#undef USE_POP3
#ifdef USE_POP3
/* Numero della porta che riceve le connessioni http                        */
#define POP3_PORT 4003
/* Virtual Host per le connessioni pop3                                     */
#define POP3_HOST "bbs.cittadellabbs.it"
/* Tempo massimo di idle in minuti prima di venire cacciati.                */
/* Nota: RFC 1939 richiede che questo tempo sia al minimo 10 minuti.        */
#define POP3_IDLE_TIMEOUT 10
/* Definire per avere il log delle sessioni POP3 in syslog (per debug)      */
#undef POP3_DEBUG
#endif

/* Server IMAP4 per collegarsi alla BBS con il proprio client email.        */
/* ATTENZIONE: L'implementazione non e' terminata!   (SPERIMENTALE)         */
#undef USE_IMAP4
#ifdef USE_IMAP4
/* Numero della porta che riceve le connessioni http                        */
#define IMAP4_PORT 4004
/* Virtual Host per le connessioni IMAP4                                    */
#define IMAP4_HOST "bbs.cittadellabbs.it"
/* Tempo massimo di idle in minuti prima di venire cacciati.                */
#define IMAP4_IDLE_TIMEOUT 10
/* Definire per avere il log delle sessioni IMAP4 in syslog (per debug)     */
#undef IMAP4_DEBUG
#endif

/* Email della bbs (vengono mandati ad esempio i bug report                 */
#define BBS_EMAIL "\"Cittadella BBS\" <bbs@cittadellabbs.it>"

/* Home Page della BBS                                                      */
#define BBS_HOME_PAGE "http://bbs.cittadellabbs.it/"

/* Usa il sistema di assegnazione di distintivi agli utenti. (Non implem.)  */
#undef USE_BADGES

/* Se e` definita ONLY_BANNER, non sono permessi i login, e la connessione  */
/* viene chiusa subito dopo l'invio del banner.                             */
#undef ONLY_BANNER

/* Lunghezza massima permessa per i vari testi, espressa in numero righe    */
#define MAXLINEEBX             6     /* Broadcast e eXpress-messages        */
#define MAXLINEEBUG           20     /* Bug Reports                         */
#define MAXLINEENEWS         200     /* File delle News (al login)          */
#define MAXLINEEPOST         200     /* Lunghezza post                      */
#define MAXLINEEPRFL          24     /* Profile                             */
#define MAXLINEEROOMINFO     100     /* File delle info delle room          */
#define MAXLINEEFLOORINFO    100     /* File delle info dei floors          */
#define MAXLINEEBANNERS   100000     /* Massimo numero di banners diversi   */

/* Definire USE_CACHE_POST se si vuole attivare la cache per i post         */
#define USE_CACHE_POST
/* Dimensioni della cache dei post                                          */
#define POST_CACHE_SIZE        300
#define POST_CACHE_MAXMEM   500000

/* Definire USE_MEM_STAT se si vogliono tenere statistiche sull'uso della   *
 * memoria. Utile principalmente per il debugging; per un uso normale di    *
 * Cittadella commentare la riga: spreca solo risorse.                      */
#define USE_MEM_STAT

/* Nuovo sistema di memorizzazione dei testi per l'uso della cache dei      *
 * post.                                                                    */
#define USE_STRING_TEXT

/* Log delle resource usage. Scommentare la seguente riga per attivarlo.    *
 * Credo abbia bisogno di un kernel con 'BSD ACCOUNTING' per funzionare.    */
#define USE_RUSAGE

/* Se MKSTEMP e' definita, viene utilizzata la funzione mkstemp() per       */
/* generare il nome dei file temporanei, altrimenti usa tempnam().          */
/* Attenzione, se usate le glibc di versione precedente alla 2.0.7,         */
/* mkstemp() setta male, ed e' prudente non definire MKSTEMP in quel caso.  */
#define MKSTEMP

/****************************************************************************
 * ATTENZIONE: i defines che seguono possono essere modificati PRIMA        *
 *             dell'installazione della BBS. Una volta creati tutti i       *
 *             files della BBS, rischiate di corrompere dei dati!           *
 ***************************************************************************/

/* Capienza delle mailbox degli utenti (n. mail che vengono conservati)     */
#define MAIL_SLOTNUM      50

/* Capienza dei blog degli utenti (n. log che vengono conservati)           */
#define BLOG_SLOTNUM      50

/* Numero di slots nella friendlist e nella enemy list.                     */
#define NFRIENDS          10

/* Directory per i files temporanei                                         */
#define TMPDIR       "./tmp"

/* Directory con i files della bbs                                          */
#define LIBDIR       "./lib"

/* Directory con gli eseguibili della bbs                                   */
#define BIN_DIR      "./bin"

/* Script per l'esecuzione del logrotate                                    */
#define SCRIPT_LOGROT     BIN_DIR      "/logrotate"

/* Script per la creazione del backup dei dati della BBS                    */
#define SCRIPT_BACKUP     BIN_DIR      "/backup"


/****************************************************************************
 * Non modificare il resto di questo file!                                  *
 ***************************************************************************/

/* Nome delle file che contiene l'indice dei file disponibili alla lettura */
/* nella stessa directory (essenzialmente per messaggi/ e help/            */
#define STDFILE_INDEX   "indice"

/* Prefissi file temporanei                                                 */
#define PFX_BR    "br"
#define PFX_EMAIL "email"
#define PFX_PWDU  "pwdu"
#define PFX_RCPT  "rcpt"
#define PFX_VK    "vk"

/* Templates per l'uso di mkstemp().                                        */
#ifdef MKSTEMP
#define TEMP_BR_TEMPLATE    TMPDIR "/" PFX_BR    "XXXXXX"
#define TEMP_EMAIL_TEMPLATE TMPDIR "/" PFX_EMAIL "XXXXXX"
#define TEMP_PWDU_TEMPLATE  TMPDIR "/" PFX_PWDU  "XXXXXX"
#define TEMP_RCPT_TEMPLATE  TMPDIR "/" PFX_RCPT  "XXXXXX"
#define TEMP_VK_TEMPLATE    TMPDIR "/" PFX_VK    "XXXXXX"
#endif

/* Path directory e file della bbs.                                         */
#define ROOTDIR           "."
#define FILE_SYSLOG       ROOTDIR      "/syslog"
#define BANNER_DIR        LIBDIR       "/banners"
#define FILES_DIR         LIBDIR       "/files"
#define FILE_MSG_DIR      LIBDIR       "/msgfiles"
#define HELP_DIR          LIBDIR       "/help"
#define IMAGES_DIR        LIBDIR       "/images"
#define MESSAGGI_DIR      LIBDIR       "/messaggi"
#define ROOMS_DIR         LIBDIR       "/rooms"
#define SERVER_DIR        LIBDIR       "/server"
#define UTENTI_DIR        LIBDIR       "/utenti"
#define MAIL_DIR          UTENTI_DIR   "/mail"

#ifdef USE_BLOG
#define BLOG_DIR          UTENTI_DIR   "/blog"
#endif

#ifdef USE_BADGES
#define BADGES_DIR        UTENTI_DIR   "/badges"
#define BADGES_FILE       BADGES_DIR   "/badges"
#endif

#define FILE_FILE_INDEX   FILES_DIR    "/index"
#define FILE_FILE_USTATS  FILES_DIR    "/userstats"

#define BANNER_FILE       BANNER_DIR   "/banners"
#define BANNER_HASHFILE   BANNER_DIR   "/hash"
#define TOD_FILE          BANNER_DIR   "/tip_of_the_day"
#define TOD_HASHFILE      BANNER_DIR   "/tip_hash"
#define FILE_MSG_DATA     FILE_MSG_DIR "/fmdata"
#define FILE_MSGDATA      ROOMS_DIR    "/msgdata/msg"
#define FILE_DUMP_DATA    ROOMS_DIR    "/msgdata/dump"
#define FILE_DATI_SERVER  SERVER_DIR   "/dati_server"
#define FILE_MAIL         MAIL_DIR     "/mailbox"
#define FILE_BLOG         BLOG_DIR     "/blog"
#define FILE_BLOGINFO     BLOG_DIR     "/bloginfo"
#define FILE_BLOGROOM     BLOG_DIR     "/blogroom"
#define FILE_BLOGDFR      BLOG_DIR     "/dfr"
#define FILE_UTENTI       UTENTI_DIR   "/file_utenti"
#define FILE_SYSCONFIG    SERVER_DIR   "/sysconfig.rc"
#define PROFILE_PATH      UTENTI_DIR   "/profile"
#define ROOMS_DATA        ROOMS_DIR    "/roomdata"
#define ROOMS_INFO        ROOMS_DIR    "/room_info"

#define FILE_BBS_CLOSED   MESSAGGI_DIR "/bbs_closed"
#define FILE_LOGO         MESSAGGI_DIR "/logo"
#define FILE_NEWS         MESSAGGI_DIR "/news"
#define FILE_HELP         HELP_DIR     "/lista_comandi"


/* Informazioni per il reboot */
#define SERVER_LOCK       SERVER_DIR   "/lock"
#define LAST_REBOOT       SERVER_DIR   "/last_reboot"
#define LAST_BACKUP       SERVER_DIR   "/last_backup"
#define LAST_LOGROT       SERVER_DIR   "/last_logrot"
#define FORCE_SCRIPTS     SERVER_DIR   "/force_scripts"
#define EXT_MESSAGE       SERVER_DIR   "/messaggio"

#ifdef NO_DOUBLE_EMAIL
#define FILE_DOUBLE_EMAIL SERVER_DIR   "/email.list"
#endif /* NO_DOUBLE_EMAIL */

#ifdef USE_FLOORS
#define FLOOR_DIR         LIBDIR       "/floors"
#define FLOOR_DATA        FLOOR_DIR    "/floordata"
#define FLOOR_ROOMLIST    FLOOR_DIR    "/room_list/floor"
#define FLOOR_INFO        FLOOR_DIR    "/floor_info"
#endif

#ifdef USE_REFERENDUM
#define URNA_DIR          LIBDIR       "/urna"
#define URNA_INFOFILE     URNA_DIR     "/urna_info"
#define URNA_DATAFILE     URNA_DIR     "/urna_data"
#define URNA_USERFILE     URNA_DIR     "/urna_user"
#endif

/* USE_STRING_TEXT e USE_FIND si possono usare solo se si usa anche la cache.*/
#ifndef USE_CACHE_POST
#undef USE_STRING_TEXT
#undef USE_FIND
#endif

/* Setta i flags del server da trasmettere al client.                       */
#include "server_flags.h"
#ifdef USE_VALIDATION_KEY
#define SFVK 1
#else
#define SFVK 0
#endif
#ifdef USE_FLOORS
#define SFFL 1
#else
#define SFFL 0
#endif
#ifdef USE_REFERENDUM
#define SFRE 1
#else
#define SFRE 0
#endif
#ifdef USE_MEM_STAT
#define SFMS 1
#else
#define SFMS 0
#endif
#ifdef USE_FIND
#define SFFI 1
#else
#define SFFI 0
#endif
#ifdef USE_BLOG
#define SFBL 1
#else
#define SFBL 0
#endif

#define SERVER_FLAGS ( (SFVK * SF_VALKEY) + (SFFL * SF_FLOORS) + \
	  (SFRE * SF_REFERENDUM) + (SFMS * SF_MEMSTAT) + (SFFI * SF_FIND) \
		       + (SFBL * SF_BLOG))

/* Lunghezza standard dei buffers di char                                   */
#define LBUF 256

#ifdef _MEMSTAT_H
# error "do not include memstat.h directly, include config.h instead"
#endif
#include "memstat.h"

#endif /* config.h */
