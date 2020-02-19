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
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : comandi.c                                                          *
*        Interprete comandi per il client.                                  *
*        In questo file si aggiungera' la possibilita' di ridefinire i      *
*        tasti e caricare da cittaclient.rc la configurazione personale.    *
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ansicol.h"
#include "cittaclient.h"
#include "cml.h"
#include "comandi.h"
#include "help_str.h"
#include "inkey.h"
#include "messaggi.h"
#include "prompt.h"
#include "room_flags.h"
#include "server_flags.h"
#include "user_flags.h"
#include "utility.h"

char barflag = 1;

/* prototipi delle funzioni in questo file */
int getcmd(char *c);
int get_msgcmd(char *str, int metadata);
static int dot_command(void);
static int aide_command(void);
static int aide_edit_command(void);
static int aide_kill_command(void);
static int aide_read_command(void);
static int blog_command(void);
static int chat_command(void);
static int enter_command(void);
static int floor_command(void);
static int friend_command(void);
static int known_command(void);
static int read_command(void);
static int room_command(void);
static int sysop_command(void);
static int sysop_banners_command(void);
static int sysop_fm_command(void);
static int sysop_read_command(void);
static int sysop_shutdown_command(void);
static int toggle_command(void);
static int urna_command(void);

/****************************************************************************
****************************************************************************/
/*
 * Attende un comando da stdin eseguendo gli eventuali comandi provenienti
 * dal server e restituisce il codice del comando del client corrispondente 
 */
int getcmd(char *str)
{
        int a, cmd;

        strcpy(str, "");
  
        /* Chiamata alla funzione che disegna il room prompt */
	prompt_curr = P_ROOM;
        print_prompt();

	setcolor(C_COMANDI);
        while (1) {
                /* prende il carattere e lavora per il server */
                a = inkey_sc(1);
		switch(a) {
		case 'A':
                        printf(_("Lascia la chat.\n"));
                        return(18);
		case 'B':
                        printf(_("Broadcast.\n"));
                        return(2);
                        /*
                        printf(_("Bug Report.\n"));
                        return(23);
                        */
		case 'C':
                        printf(_("Ascolta canale chat.\n"));
                        return(17);
#ifdef LOCAL
		case 'D':
                        printf(_("Effettua un finger a un utente.\n"));
                        return(31);
#endif
		case 'E':
                        printf(_("Toggle Expert Mode.\n"));
                        return(68);
		case 'F':
                        printf(_("Modifica la lista degli amici.\n"));
                        return(29);
		case 'H':
                        printf(_("Leggi un file di Help."));
                        return(15);
		case 'I':
                        printf(_("Info dettagliate sulla room.\n"));
                        return(62);
		case 'K':
                        printf(_("All Known Rooms.\n"));
			return(53);
			/*
			  if ((SERVER_FLOORS) && (uflags[4] & UT_FLOOR))
			  return(114);
			  else
			  return(60);
			*/
		case 'L':
                        printf(_("Abbandona e vai in Lobby).\n"));
                        return(131);
		case 'M':
                        printf(_("Abbandona e vai in Mail).\n"));
                        return(90);
		case 'O':
                        printf(_("Cambia la password di un utente.\n"));
                        return(24);
		case 'P':
                        printf(_("Modifica la Password.\n"));
                        return(22);
		case 'Q':
                        printf(_("Quit.\n"));
                        return(0);
		case 'R':
                        return(7);
		case 'S':
                        printf(_("Segna tutti i nuovi messaggi come letti.\n"));
                        return(130);
		case 'U':
                        printf(_("Unzap all rooms.\n"));
                        return(99);
			/*
			  case 'V':
			  printf(_("Validazione automatica.\n"));
			  return(81);
			*/
		case 'V':
                        printf(_("Entra nella cabina elettorale.\n"));
                        return(138);
		case 'W':
                        printf(_("Who in Chat.\n"));
                        return(19);
		case 'X':
                        printf(_("Toggle X-msg reception : "));
                        return(92);
		case 'Y':
                        printf(_("Visualizza il file di log del sistema.\n"));
                        return(16);
		case 'Z':
                        printf(_("Zap all rooms.\n"));
                        return(96);
		case 'a':
                        printf(_("Abbandona la room.\n"));
                        return(67);
		case 'b':
                        printf(_("Blog: lista degli utenti con la casa aperta.\n"));
                        return 143;
		case 'c': /* Messaggio in Chat */
                        return(20);
		case 'd':
			printf(_("enter Doing.\n"));
			return(116);
		case 'e':
                        printf(_("Enter message with default editor.\n"));
                        return(66);
		case 'f':
                        printf(_("Forward Read.\n"));
                        return(46);
		case 'g':
                        printf(_("Goto next room.\n"));
                        return(34);
		case 'h':
                        printf(_("Lista dei comandi disponibili.\n"));
                        return(14);
		case 'i':
                        printf(_("Info.\n"));
                        return(51);
		case 'j':
                        printf(_("Jump to : "));
                        return(36);
		case 'k':
                        printf(_("Known Rooms.\n"));
			if ((SERVER_FLOORS) && (uflags[4] & UT_FLOOR))
				return(57);
			else
				return(60);
		case 'l':
                        printf(_("Logout.\n"));
			printf(sesso ? _("Sei sicura (s/n)? ") : _("Sei sicuro (s/n)? "));
			if (si_no() == 's')
				return(0);
			else
				return(9999);
		case 'n':
                        printf(_("Nuovi messaggi.\n"));
                        return(47);
		case 'p':
                        printf(_("Profile.\n"));
                        return(4);
		case 'r':
                        printf(_("Reverse.\n"));
                        return(48);
		case 's':
                        printf(_("Skip.\n"));
                        return(50);
		case 't':
                        printf(_("Ora e data.\n"));
                        return(13);
		case 'u':
                        printf(_("Lista degli utenti.\n"));
                        return(6);
		case 'v':
                        printf(_("Rispondi all'ultimo X ricevuto."));
			return 91;
		case 'w':
                        cml_print(_("Who: Chi &egrave; in linea.\n"));
                        return(1);
		case 'x':
                        printf(_("eXpress message. [Abort: Ctrl-X  Help: F1]\n"));
                        return(3);
		case 'z':
                        printf(_("Zap.\n"));
                        return(58);
		case ' ': /* Spacescan */
			switch (barflag) {
			case 0:
				printf(_("Goto next room.\n"));
				return(34);
			case 1:
				printf(_("Nuovi messaggi.\n"));
				return(47);
			}
                        return(21);
		case '5':
                        printf(_("Ultimi 5 messaggi.\n"));
                        return(59);
		case '@':
			cml_print(_("C'&egrave; posta nuova?\n"));
                        return(76);
		case '#': /* Legge msg # */
                        return(98);
#ifdef LOCAL
		case '$':
                        cml_printf(_("Entra nella subshell. Digita \\<<b>exit</b>\\> per tornare in Cittadella.\n"));
                        return(30);
#endif
		case '%':
			printf(_("Lock terminale.\n"));
			return(21);
		case '&':
                        printf(_("Diario eXpress-messages.\n"));
			return(103);
		case '-':
			printf(_("Legge gli ultimi messaggi...\n"));
                        return(75);
		case '/':
		        if (SERVER_FIND)
                                return(139);
			break;
                        /*	
                case ';':
                        printf(_("Emote."));
                        return(133);
                        */
                case ':':
                        printf(_("Abbandona e vai a casa (blog).\n"));
                        return(145);
		case Ctrl('L'):
			printf(_("Cancella lo schermo.\n"));
			return 135;
		case '?':
                        printf(_("Lista dei comandi disponibili.\n"));
                        return(14);
		case '.':
                        if ((cmd = dot_command()))
                                return cmd;
                        break;
                }
        }   /* fine while  */
}     /* fine getcmd */

/*
 * Attende un comando da stdin eseguendo gli eventuali comandi provenienti
 * dal server e restituisce il codice del comando del client corrispondente 
 */
int get_msgcmd(char *str, int metadata)
{
	char prompt[LBUF];
        int a;
	
        strcpy(str, "");
	if (uflags[1] & UT_NOPROMPT)
		return 1;

        /* Chiamata alla funzione che disegna il message prompt */
	prompt_curr = P_MSG;
        msg_prompt(prompt);
	cml_printf("%s", prompt);
	
        while (1) {
                /* prende il carattere e lavora per il server */
                a = inkey_sc(1);
                switch(a) {
		case 'a':
                        printf(_("Again.\r"));
			return 3;
		case 'A':
                        if (metadata) {
                                printf(_("esamina Allegati.\r"));
                                return 12;
                        }
                        break;
		case 'b':
                        printf(_("Back.\r"));
			return 2;
		case 'c':
                        printf(_("Copy.\r"));
			return 4;
		case 'C':
                        printf(_("CML source.\r"));
			return 14;
		case 'D':
                        printf(_("Delete.\r"));
			return 5;
#ifdef LOCAL
		case 'd':
			printf(_("Dump to file.\r"));
			return 11;
#endif
		case 'e':
                        printf(_("Examine Quote..\r"));
			return 10;
		case 'f':
                        printf(_("Free Quote.\r"));
			return 9;
		case 'm':
                        printf(_("Move.\r"));
			return 6;
		case ' ':
		case 'n':
                        printf(_("Next.\r"));
			return 1;
		case 'P':
                        printf(_("memorizza Post reference.\r"));
			return 13;
		case 'p':
                        printf(_("Profile.\r"));
			return 100;
		case 'q':
                        printf(_("Quote.\r"));
			return 8;
		case 'r':
			printf(_("Reply.\r"));
			return 7;
		case 's':
                        printf(_("Stop.\n"));
			return 0;
		case 't':
                        printf(_("Ora e data.\r"));
			return 101;
		case 'v':
                        printf(_("Rispondi all'ultimo X ricevuto.\r"));
			return 111;
		case 'w':
                        printf(_("Who.\r"));
			return 102;
		case 'x':
                        printf(_("X-msg.\r"));
			return 110;
		case '%':
                        printf(_("Lock.\r"));
			return 103;
		case '?': /* Visualizza la lista dei comandi */
		case Key_F(1):
                        printf(_("Lista comandi.\r"));
                        return 199;
                } /* fine switch */
        }   /* fine while  */
}     /* fine get_msgcmd */

/****************************************************************************/
/*
 * Parte dell'interprete che si occupa di processare i dot commands.
 */
static int dot_command(void)
{
        int a, cmd;

        printf(".");
        while (1) {
                cmd = 0;
                /* prende il carattere e lavora per il server */
                a = inkey_sc(1);
		if ((a == 'S') && (livello < 10))
			a = 0;
                switch(a) {
		case Key_BS:
                        delchar();
                        return 0;
		case 'a':
                        cmd = aide_command();
                        break;
		case 'b':
		        if (SERVER_BLOG)
			        cmd = blog_command();
                        break;
		case 'c':
                        cmd = chat_command();
                        break;
		case 'e':
                        cmd = enter_command();
                        break;
		case 'f':
                        cmd = friend_command();
                        break;
		case 'h':
			printf(_("Help."));
                        return 15;
		case 'k':
                        cmd = known_command();
                        break;
		case 'p':
			printf(_("Pianta BBS."));
			return 129;
		case 'r':
                        cmd = read_command();
                        break;
		case 's':
			printf(_("Skip to: "));
			return 97;
		case 't':
                        cmd = toggle_command();
                        break;
		case 'u':
			if (SERVER_REFERENDUM)
				cmd = urna_command();
                        break;
		case 'F':
			if (SERVER_FLOORS)
				cmd = floor_command();
                        break;
		case 'R':
                        cmd = room_command();
                        break;
		case 'S':
                        cmd = sysop_command();
                        break;
		case '?': /* HELP */
		case Key_F(1):
			printf(_("(Opzioni)\nScegli:\n"
				 " <a>ide commands\n"));
			if (SERVER_BLOG)
			        printf(_(" <b>log commands\n"));
			printf(_(" <c>hat commands\n"
				 " <e>nter commands\n"
				 " <f>riend commands\n"
				 " <h>elp\n"
				 " <k>nown rooms commands\n"
				 " <s>kip to specific room\n"
				 " <t>oggle commands\n"));
			if (SERVER_REFERENDUM)
			        printf(_(" <u>rna commands\n"));
			if (SERVER_FLOORS)
				printf(_(" <F>loor commands\n"));
                        printf(_(" <R>oom commands\n"
				 " <S>ysop commands\n"));
                        print_prompt();
                        putchar('.');
                        break;
                }
                if (cmd)
                        return cmd;
        }
}

static int aide_command(void)
{
        int a, cmd;
	
        printf(_("Aide "));
        while (1) {
                cmd = 0;
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
			delnchar(5);
			return 0;
		case 'a':
                        printf(_("New Room Aide.\n"));
                        return 65;
		case 'b':
                        printf(_("Broadcast.\n"));
                        return 2;
		case 'e':
                        cmd = aide_edit_command();
                        break;
		case 'h':
			printf(_("New Room Helper.\n"));
			return 63;
		case 'i':
			printf(_("Invite User.\n"));
			return 64;
		case 'k':
			cmd = aide_kill_command();
			break;
		case 'n':
			printf(_("Create New Room.\n"));
			return 42;
		case 'r':
			cmd = aide_read_command();
			break;
		case 's':
			printf(_("Swap rooms.\n"));
			return 89;
		case 'w':
			printf(_("Who invited.\n"));
			return 127;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_AIDE);
			print_prompt();
			printf(_(".Aide "));
			break;
                }
                if (cmd)
                        return cmd;
        }
}

static int aide_edit_command(void)
{
        int a;
	
        printf(_("Edit "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
			/*
			  case 'a':
			  printf(_("Room Aide.\n"));
			  return 0;
			*/
		case 'c':
                        printf(_("System Configuration.\n"));
                        return 9;
		case 'h':
                        printf(_("Helping Hands.\n"));
                        return 104;
		case 'i':
                        printf(_("room Info.\n"));
                        return 52;
		case 'm':
                        printf(_("Message.\n"));
                        return 78;
		case 'n':
                        printf(_("News.\n"));
                        return 102;
		case 'p':
                        printf(_("Password di un utente.\n"));
                        return 24;
		case 'r':
                        printf(_("Room.\n"));
                        return 43;
		case 's':
                        printf(_("room Size.\n"));
                        return 93;
		case 'u':
                        printf(_("User Registration.\n"));
                        return 12;
		case 'v':
                        printf(_("Richiedi nuova chiave di validazione.\n"));
                        return(115);
		case '?': /* HELP */
                        printf(HELP_DOT_AIDE_EDIT);
                        print_prompt();
                        printf(_(".Aide Edit "));
                        break;
                }
        }
}

static int aide_kill_command(void)
{
        int a;
	
        printf(_("Kill "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'c':
                        printf(_("Caccia un utente connesso.\n"));
                        return 10;
		case 'e':
                        printf(_("Elimina definitivamente un utente.\n"));
                        return 11;
		case 'k':
                        printf(_("Kick Out user.\n"));
                        return 124;
		case 'K':
                        printf(_("End kick out period.\n"));
                        return 125;
		case 'r':
                        printf(_("Room corrente.\n"));
			return 44;
		case 'w':
                        printf(_("Room kicked Who.\n"));
			return 126;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_AIDE_KILL);
                        print_prompt();
                        printf(_(".Aide Kill "));
                        break;
                }
        }
}

static int aide_read_command(void)
{
        int a;

        printf(_("Read "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'c':
                        printf(_("Configuration of Server.\n"));
                        return 8;
		case 'r':
			printf(_("Room List.\n"));
			return 33;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_AIDE_READ);
                        print_prompt();
                        printf(_(".Aide Read "));
                        break;
                }
        }
}

static int blog_command(void)
{
        int a;

        printf(_("Blog "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'c':
                        printf(_("Configure.\n"));
                        return 141;
                case 'd': /* TODO deve funzionare solo se sono a casa */
                        printf(_("Description.\n"));
                        return 52;
		case 'g':
                        printf(_("Goto.\n"));
                        return 140;
		case 'l':
                        printf(_("Lista degli utenti con la casa aperta.\n"));
                        return 143;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_BLOG);
                        print_prompt();
                        printf(_(".Blog "));
                        break;
                }
        }
}

static int chat_command(void)
{
        int a;

        printf(_("Chat "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'a': 
			/* printf(_("Ascolta Canale # \n"));  */
                        break;
		case 'e':
                        printf(_("Esci.\n"));
                        return 18;
		case 'w':
                        printf(_("Who.\n"));
                        return 19;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_CHAT);
                        print_prompt();
                        printf(_(".Chat "));
                        break;
                }
        }
}

static int enter_command(void)
{
        int a;

        printf(_("Enter "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
			delnchar(6);
                        return 0;
		case 'a':
                        printf(_("message as Room Aide or Helper.\n"));
                        return(77);
		case 'b':
                        printf(_("Bug Report.\n"));
                        return(23);
		case 'c':
                        printf(_("Configurazione.\n"));
                        return(26);
		case 'D':
                        printf(_("clear Doing.\n"));
                        return(120);
		case 'd':
			printf(_("Doing.\n"));
			return(116);
		case 'e':
			printf(_("message with External editor.\n"));
			return(61);
		case 'i':
                        printf(_("Invite request.\n"));
                        return(101);
		case 'm':
                        printf(_("Message.\n"));
                        return(35);
		case 'P':
                        printf(_("Password.\n"));
                        return 22;
		case 'p':
                        printf(_("Profile.\n"));
                        return(25);
		case 'r':
			printf(_("Registrazione.\n"));
			return(7);
		case 't':
			printf(_("Terminal.\n"));
			return(121);
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_ENTER);
			print_prompt();
                        printf(_(".Enter "));
                        break;
                }
        }
}

static int floor_command(void)
{
        int a, cmd;

        printf(_("Floor "));
        while (1) {
                cmd = 0;
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(6);
                        return 0;
		case 'a':
                        printf(_("New Floor Aide.\n"));
                        return 109;
		case 'c':
                        printf(_("Create.\n"));
                        return 105;
		case 'd':
                        printf(_("Delete.\n"));
                        return 106;
		case 'g':
                        printf(_("Goto.\n"));
                        return 113;
		case 'E':
                        printf(_("Edit Info.\n"));
                        return 112;
		case 'i':
                        printf(_("Info.\n"));
                        return 110;
		case 'I':
                        printf(_("Info Dettagliate.\n"));
                        return 111;
		case 'l':
			printf(_("List.\n"));
			return 107;
		case 'm':
			printf(_("Move Room.\n"));
			return 108;
		case '?':
		case Key_F(1):
                        printf(HELP_DOT_FLOOR);
                        print_prompt();
                        printf(_(".Floor "));
                        break;
                }
                if (cmd)
                        return cmd;
        }
}

static int friend_command(void)
{
        int a;

        printf(_("Friends "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(8);
                        return 0;
		case 'e':
                        printf(_("Modifica la lista dei nemici.\n"));
                        return(137);
                        break;
		case 'f':
                        printf(_("Modifica la lista degli amici.\n"));
                        return(29);
                        break;
		case 'r':
                        printf(_("Visualizza le liste degli amici e dei nemici.\n"));
                        return(28);
                        break;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_FRIEND);
                        print_prompt();
                        printf(_(".Friends "));
                        break;
                }
        }
}

static int known_command(void)
{
        int a;

        printf(_("Known Rooms "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(12);
                        return 0;
		case 'a':
                        printf(_("All.\n"));
                        return(53);
		case 'f':
			if (SERVER_FLOORS) {
				printf(_("in Floor.\n"));
				return(57);
			}
			break;
		case 'n':
                        printf(_("with New messages.\n"));
                        return(54);
		case 'o':
                        printf(_("with Old messages only.\n"));
                        return(55);
		case 'z':
                        printf(_("Zapped.\n"));
                        return(56);
		case '?': /* HELP */
		case Key_F(1):
                        printf(_("(Opzioni)\nScegli:\n"
			       " <a>ll\n"));
			if (SERVER_FLOORS)
				printf(_(" in <f>loor\n"));
                        printf(_(" with <n>ew messages\n"
			       " with <o>ld messages only\n"
			       " <z>apped\n"));
                        print_prompt();
                        printf(_(".Known Rooms "));
                        break;
                }
        }
}

static int read_command(void)
{
	int a, cmd;

        printf(_("Read "));
        while (1) {
                cmd = 0;
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'b':
                        printf(_("Brief.\n"));
                        return 49;
		case 'c':
                        printf(_("Configuration.\n"));
                        return 27;
		case 'f':
                        printf(_("Forward.\n"));
                        return 46;
		case 'i':
                        printf(_("Info dettagliate sulla room.\n"));
                        return 62;
		case 'n':
                        printf(_("New.\n"));
                        return 47;
		case 'N':
                        printf(_("News.\n"));
                        return 132;
		case 'p':
			printf(_("Profile.\n"));
			return 4;
		case 'r':
			printf(_("Reverse.\n"));
			return 48;
		case 'u':
			printf(_("User List.\n"));
			return 6;
		case 'w':
			printf(_("Who is Online.\n"));
                        return 1;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_READ);
                        print_prompt();
                        printf(_(".Read "));
                        break;
                }
                if (cmd)
                        return cmd;
        }
}

static int room_command(void)
{
        int a, cmd;

        printf(_("Room "));
        while (1) {
                cmd = 0;
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'a':
                        printf(_("New Room Aide.\n"));
                        return 65;
		case 'c':
                        printf(_("Create.\n"));
                        return 42;
		case 'd':
                        printf(_("Delete.\n"));
                        return 44;
		case 'e':
                        printf(_("Edit.\n"));
                        return 43;
		case 'h':
			printf(_("Helper.\n"));
			return 63;
		case 'i':
                        printf(_("Info.\n"));
                        return 52;
		case 's':
			printf(_("Size.\n"));
			return 93;
		case 'S':
			printf(_("Swap.\n"));
			return 89;
		case 'u':
			printf(_("User Invite.\n"));
			return 64;
		case '?':
		case Key_F(1):
			printf(HELP_DOT_ROOM);
			print_prompt();
			printf(_(".Room "));
                        break;
                }
                if (cmd)
                        return cmd;
        }
}

static int sysop_command(void)
{
	int a, cmd;
	
        printf(_("Sysop "));
        while (1) {
                cmd = 0;
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(6);
                        return 0;
		case 'b':
			cmd = sysop_banners_command();
                        break;
		case 'e':
			printf(_("Enter Message."));
                        return 79;
		case 'f':
			cmd = sysop_fm_command();
                        break;
		case 'r':
			cmd = sysop_read_command();
                        break;
		case 's': 
			cmd = sysop_shutdown_command();
                        break;
		case 'u':
			printf(_("Set server <u>pgrade flag."));
			return 123;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_SYSOP);
                        print_prompt();
                        printf(_(".Sysop "));
                        break;
                }
                if (cmd)
                        return cmd;
        }
}

static int sysop_banners_command(void)
{
        int a;

	printf(_("Banners "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(8);
                        return 0;
		case 'm':
                        printf(_("Modify.\n"));
			return 94;
		case 'r':
                        printf(_("Rehash.\n"));
			return 95;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_SYSOP_BANNERS);
                        print_prompt();
                        printf(_(".Sysop Banners "));
                        break;
                }
        }
}

static int sysop_fm_command(void)
{
        int a;

	printf(_("File message "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(13);
                        return 0;
		case 'c':
                        printf(_("Create.\n"));
			return 38;
		case 'd':
                        printf(_("Delete Post.\n"));
			return 45;
		case 'e':
                        printf(_("Expand.\n"));
			return 87;
		case 'h':
			printf(_("Header list.\n"));
			return 37;
		case 'i':
                        printf(_("Info\n"));
                        return 41;
		case 'k':
                        printf(_("Kill.\n"));
			return 39;
		case 'r':
			printf(_("Read post.\n"));
			return 40;
		case 's':
                        printf(_("Statistiche\n"));
                        return 41;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_SYSOP_FM);
                        print_prompt();
                        printf(_(".Sysop File message "));
                        break;
                }
        }
}

static int sysop_read_command(void)
{
        int a;

	printf(_("Read "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'l':
                        printf(_("System Logs.\n"));
			return 16;
		case 's':
                        printf(_("Server Statistics.\n"));
			return 32;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_SYSOP_READ);
                        print_prompt();
                        printf(_(".Sysop Read "));
                        break;
                }
        }
}

static int sysop_shutdown_command(void)
{
        int a;

	printf(_("Shutdown "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(9);
                        return 0;
		case 'b':
                        printf(_("Begin.\n"));
			return 5;
		case 'f':
			printf(_("Force script execution.\n"));
                        return 136;
		case 'k':
                        printf(_("with .killscript.\n"));
			return 72;
		case 'n':
			printf(_("Now.\n"));
			return 73;
		case 's':
                        printf(_("Stop.\n"));
                        return 74;
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_SYSOP_SHUTDOWN);
                        print_prompt();
                        printf(_(".Sysop Shutdown "));
                        break;
                }
        }
}

static int toggle_command(void)
{
        int a;

        printf(_("Toggle "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(7);
                        return 0;
		case 'a':
			printf(_("ANSI colors : "));
			return 122;
		case 'b':
			printf(_("Bell : "));
			return 69;
#ifdef LOCAL
		case 'c':
			printf(_("Chat automatic transcription : "));
			return 119;
#endif
                case 'd':
                        printf(_("debug mode: "));
                        return 144;
		case 'e': 
			printf(_("Expert mode : "));
                        return 68;
		case 'f':
			printf(_("X-mgs reception from friends : "));
                        return 100;
		case 'g':
			printf(_("Grassetto : "));
			return 71;
#ifdef LOCAL
		case 'm':
			printf(_("Mail automatic dump : "));
			return 117;
#endif
		case 'n':
			printf(_("Notifiche Login/Logout : "));
			return 70;
		case 'x': 
			printf(_("X-mgs reception : "));
                        return 92;
		case 'u': 
			printf(_("Automatic Follow Up for X-msg : "));
                        return 80;
#ifdef LOCAL
		case 'X':
			printf(_("X-msg automatic dump : "));
			return 118;
#endif
		case '?': /* HELP */
		case Key_F(1):
                        printf(HELP_DOT_TOGGLE);
                        print_prompt();
                        printf(_(".Toggle "));
                        break;
                }
        }
}

static int urna_command(void)
{
        int a;

        printf(_("Urna "));
        while (1) {
                a = inkey_sc(1);
                switch(a) {
		case Key_BS:
                        delnchar(5);
                        return 0;
		case 'c':
                        printf(_("Completa una consultazione in corso.\n"));
                        return(82);
                        break;
		case 'd':
                        printf(_("Delete.\n"));
                        return(83);
                        break;
		case 'l':
                        printf(_("Lista consultazioni in corso.\n"));
                        return(88);
                        break;
		case 'L':
                        printf(_("Lista totale consultazioni in corso.\n"));
                        return(134);
                        break;
		case 'n':
                        printf(_("Nuova consultazione.\n"));
                        return(84);
                        break;
		case 'r':
                        printf(_("Read dati consultazione.\n"));
                        return(85);
                        break;
		case 'v':
                        printf(_("Vota per una consultazione.\n"));
                        return(86);
                        break;
		case 'p':
                        printf(_("Posticipa una consultazione.\n"));
                        return(128);
                        break;
		case '?': /* HELP */
		case Key_F(1):
			printf(HELP_DOT_URNA);
                        print_prompt();
                        printf(_(".Urna "));
                        break;
                }
        }
}
