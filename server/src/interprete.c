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
* File: interprete.c                                                          *
*       interpreta ed esegue i comandi in arrivo dal client.                  *
******************************************************************************/

#include "config.h"
#include <string.h>
#include "cittaserver.h"
#include "banner.h"
#include "blog.h"
#include "cmd_al_client.h"
#include "express.h"
#include "fileserver.h"
#include "find.h"
#include "fm_cmd.h"
#include "generic.h"
#include "interprete.h"
#include "mail.h"
#include "march.h"
#include "messaggi.h"
#include "parm.h"
#include "post.h"
#include "room_ops.h"
#include "sysconfig.h"
#include "utente_cmd.h"
#include "utility.h"
#include "x.h"

#ifdef USE_FLOORS
# include "floor_ops.h"
#endif
#ifdef USE_REFERENDUM
# include "urna-crea.h"
# include "urna-comm.h"
#endif

#define LTOKEN     4              /* Lunghezza dei token */
#define ARGSTART  (LTOKEN + 1)    /* Inizio dell'argomento del comando */

#define NO_ARG   0
#define PASS_ARG 1
#define CMD_NOOP 2                /* Solo per NOOP: non azzera l'idle. */

#define ILC_NOCHECK      -1
#define ILC_NO_LOGIN     -2
#define ILC_LOGGED_IN    -3
#define ILC_UTENTE       -4
#define ILC_REGISTRATO   -5

/* prototipi delle funzioni in questo file */

void interprete_comandi(struct sessione *t, char *com);
void cmd_noop(struct sessione *t);
void cmd_echo(struct sessione *t, char *buf);
static inline void clear_idle(struct sessione *t);

/* Lista dei comandi e strutture necessarie per la sua memorizzazione */

typedef union {
	void (*no_arg) (struct sessione *t);
	void (*pass_arg) (struct sessione *t, char *buf);
} cmd_ptr;

struct comando {
	char token[5]; /* Nome del comando                                  */
	cmd_ptr cmd;   /* Puntatore alla funzione che implementa il comando */
	char argnum;   /* Numero argomenti della funzione                   */
	int  stato;    /* Stato attuale necessario per eseguire il comando  */
	char minlvl;   /* Livello minimo di accesso alla funzione           */
};

static const struct comando cmd_list[] = {
{ "ATXT", { (void *)&cmd_atxt }, NO_ARG,   CON_MASK_TEXT, ILC_UTENTE },
#ifdef USE_VALIDATION_KEY
{ "AVAL", { (void *)&cmd_aval }, PASS_ARG, CON_COMANDI, LVL_NON_VALIDATO },
#endif
{ "BEND", { (void *)&cmd_bend }, NO_ARG,   CON_BROAD,   MINLVL_BROADCAST },
{ "BIFF", { (void *)&cmd_biff }, PASS_ARG, CON_COMANDI, MINLVL_BIFF },
#ifdef USE_BLOG
{ "BLGL", { (void *)&cmd_blgl }, NO_ARG,   CON_COMANDI, MINLVL_BLOG },
#endif
{ "BRDC", { (void *)&cmd_brdc }, NO_ARG,   CON_COMANDI, MINLVL_BROADCAST },
{ "BREG", { (void *)&cmd_breg }, NO_ARG,   CON_COMANDI, MINLVL_REGISTRATION },
{ "BUGB", { (void *)&cmd_bugb }, NO_ARG,   CON_COMANDI, MINLVL_BUGREPORT },
{ "BUGE", { (void *)&cmd_buge }, NO_ARG,   CON_BUG_REP, MINLVL_BUGREPORT },
{ "CASC", { (void *)&cmd_casc }, PASS_ARG, CON_COMANDI, MINLVL_CHAT },
{ "CEND", { (void *)&cmd_cend }, NO_ARG,   CON_CHAT,    MINLVL_CHAT },
{ "CFGG", { (void *)&cmd_cfgg }, PASS_ARG, CON_NOCHECK, MINLVL_USERCFG },
{ "CFGP", { (void *)&cmd_cfgp }, PASS_ARG, CON_COMANDI|CON_CONF,    MINLVL_USERCFG },
{ "CHEK", { (void *)&cmd_chek }, NO_ARG,   CON_COMANDI, ILC_NO_LOGIN },
{ "CLAS", { (void *)&cmd_clas }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "CMSG", { (void *)&cmd_cmsg }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "CUSR", { (void *)&cmd_cusr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "CWHO", { (void *)&cmd_cwho }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "DEST", { (void *)&cmd_dest }, PASS_ARG, CON_XMSG,    MINLVL_XMSG },
{ "ECHO", { (void *)&cmd_echo }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "EDNG", { (void *)&cmd_edng }, PASS_ARG, CON_LIC|CON_COMANDI, MINLVL_DOING },
{ "ESYS", { (void *)&cmd_esys }, PASS_ARG, CON_COMANDI, MINLVL_EDITCONFIG },
{ "EUSR", { (void *)&cmd_eusr }, PASS_ARG, CON_EUSR,    MINLVL_EDITUSR },
{ "FBKD", { (void *)&cmd_fbkd }, PASS_ARG, CON_UPLOAD,  MINLVL_DOWNLOAD },
{ "FIND", { (void *)&cmd_find }, PASS_ARG, CON_COMANDI, MINLVL_FIND },
{ "FMCR", { (void *)&cmd_fmcr }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMDP", { (void *)&cmd_fmdp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMHD", { (void *)&cmd_fmhd }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRI", { (void *)&cmd_fmri }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRM", { (void *)&cmd_fmrm }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRP", { (void *)&cmd_fmrp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMXP", { (void *)&cmd_fmxp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FDNL", { (void *)&cmd_fdnl }, PASS_ARG, CON_COMANDI, MINLVL_DOWNLOAD },
#ifdef USE_FLOORS
{ "FAID", { (void *)&cmd_faid }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "FDEL", { (void *)&cmd_fdel }, PASS_ARG, CON_COMANDI, MINLVL_DELFLOOR },
{ "FEDT", { (void *)&cmd_fedt }, PASS_ARG, CON_COMANDI|CON_FLOOR_EDIT, ILC_NOCHECK }, /* IS_FH() */
{ "FIEB", { (void *)&cmd_fieb }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "FIEE", { (void *)&cmd_fiee }, NO_ARG,   CON_FLOOR_INFO, ILC_NOCHECK }, /* vedere  */
{ "FINF", { (void *)&cmd_finf }, NO_ARG,   CON_COMANDI, ILC_LOGGED_IN },
{ "FKRA", { (void *)&cmd_fkra }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FKRL", { (void *)&cmd_fkrl }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FKRM", { (void *)&cmd_fkrm }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FLST", { (void *)&cmd_flst }, PASS_ARG, CON_COMANDI, MINLVL_FLOORAIDE },
{ "FMVR", { (void *)&cmd_fmvr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_FA*/
{ "FNEW", { (void *)&cmd_fnew }, PASS_ARG, CON_COMANDI, MINLVL_NEWFLOOR },
#endif
{ "FRDG", { (void *)&cmd_frdg }, NO_ARG,   CON_NOCHECK, MINLVL_FRIENDS },
{ "FRDP", { (void *)&cmd_frdp }, PASS_ARG, CON_COMANDI, MINLVL_FRIENDS },
{ "FSCT", { (void *)&cmd_fsct }, NO_ARG,   CON_COMANDI, MINLVL_SHUTDOWN },
{ "FUPB", { (void *)&cmd_fupb }, PASS_ARG, CON_POST,    MINLVL_DOWNLOAD },
{ "FUPE", { (void *)&cmd_fupe }, PASS_ARG, CON_UPLOAD,  MINLVL_DOWNLOAD },
{ "FUPL", { (void *)&cmd_fupl }, PASS_ARG, CON_COMANDI, MINLVL_DOWNLOAD },
{ "GMTR", { (void *)&cmd_gmtr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "GOTO", { (void *)&cmd_goto }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "GREG", { (void *)&cmd_greg }, NO_ARG,   CON_REG,     MINLVL_REGISTRATION },
{ "GUSR", { (void *)&cmd_gusr }, PASS_ARG, CON_COMANDI, MINLVL_EDITUSR },
{ "GVAL", { (void *)&cmd_gval }, NO_ARG,   CON_COMANDI, ILC_REGISTRATO },/*log*/
{ "HELP", { (void *)&cmd_help }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "HWHO", { (void *)&cmd_hwho }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "INFO", { (void *)&cmd_info }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "KUSR", { (void *)&cmd_kusr }, PASS_ARG, CON_COMANDI, MINLVL_KILLUSR },/*log*/
{ "LBAN", { (void *)&cmd_lban }, PASS_ARG, CON_LIC,     ILC_NOCHECK }, /* !logged_in */
{ "LBEB", { (void *)&cmd_lbeb }, NO_ARG,   CON_COMANDI, MINLVL_BANNER },
{ "LBEE", { (void *)&cmd_lbee }, NO_ARG,   CON_BANNER,  MINLVL_BANNER },
{ "LBGT", { (void *)&cmd_lbgt }, NO_ARG,   CON_COMANDI, MINLVL_BANNER },
{ "LSSH", { (void *)&cmd_lssh }, PASS_ARG, CON_COMANDI|CON_SUBSHELL, MINLVL_SUBSHELL },
{ "LTRM", { (void *)&cmd_ltrm }, PASS_ARG, CON_COMANDI|CON_LOCK, MINLVL_LOCKTERM },
{ "MCPY", { (void *)&cmd_mcpy }, PASS_ARG, CON_COMANDI, ILC_NOCHECK }, /* Controllare */
{ "MDEL", { (void *)&cmd_mdel }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "MMOV", { (void *)&cmd_mmov }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "MSGI", { (void *)&cmd_msgi }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "NOOP", { (void *)&cmd_noop }, CMD_NOOP, CON_NOCHECK, ILC_NOCHECK },
{ "NWSB", { (void *)&cmd_nwsb }, NO_ARG,   CON_COMANDI, MINLVL_NEWS },
{ "NWSE", { (void *)&cmd_nwse }, NO_ARG,   CON_NEWS,    ILC_NOCHECK },/*veder*/
{ "PARM", { (void *)&cmd_parm }, PASS_ARG, CON_MASK_PARM, ILC_NOCHECK },/*ved*/
{ "PRFB", { (void *)&cmd_prfb }, NO_ARG,   CON_COMANDI, MINLVL_EDITPRFL },
{ "PRFE", { (void *)&cmd_prfe }, NO_ARG,   CON_PROFILE, MINLVL_EDITPRFL },
{ "PRFG", { (void *)&cmd_prfg }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "PRFL", { (void *)&cmd_prfl }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "PSTB", { (void *)&cmd_pstb }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "PSTE", { (void *)&cmd_pste }, PASS_ARG, CON_POST,    ILC_NOCHECK },/*veder*/
{ "PWDC", { (void *)&cmd_pwdc }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*verif*/
{ "PWDN", { (void *)&cmd_pwdn }, PASS_ARG, CON_COMANDI, MINLVL_PWDN },
{ "PWDU", { (void *)&cmd_pwdu }, PASS_ARG, CON_COMANDI, MINLVL_PWDU },/* log */
{ "QUIT", { (void *)&cmd_quit }, NO_ARG,   CON_NOCHECK, ILC_NOCHECK },
{ "RAID", { (void *)&cmd_raid }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "RALL", { (void *)&cmd_rall }, NO_ARG,   CON_COMANDI, LVL_NORMALE },
{ "RDEL", { (void *)&cmd_rdel }, PASS_ARG, CON_COMANDI, MINLVL_DELROOM },
{ "READ", { (void *)&cmd_read }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "REDT", { (void *)&cmd_redt }, PASS_ARG, CON_COMANDI|CON_ROOM_EDIT, ILC_NOCHECK }, /* IS_RH() */
{ "RGST", { (void *)&cmd_rgst }, PASS_ARG, CON_REG,     MINLVL_REGISTRATION },
{ "RIEB", { (void *)&cmd_rieb }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "RIEE", { (void *)&cmd_riee }, NO_ARG,   CON_ROOM_INFO, ILC_NOCHECK },/*ved*/
{ "RINF", { (void *)&cmd_rinf }, NO_ARG,   CON_COMANDI, ILC_LOGGED_IN },
{ "RINV", { (void *)&cmd_rinv }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RINW", { (void *)&cmd_rinw }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RIRQ", { (void *)&cmd_rirq }, PASS_ARG, CON_COMANDI, MINLVL_AUTOINV },
{ "RKOB", { (void *)&cmd_rkob }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKOE", { (void *)&cmd_rkoe }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKOW", { (void *)&cmd_rkow }, NO_ARG,   CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKRL", { (void *)&cmd_rkrl }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "RLEN", { (void *)&cmd_rlen }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RLST", { (void *)&cmd_rlst }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "RMSG", { (void *)&cmd_rmsg }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "RNEW", { (void *)&cmd_rnew }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*Control*/
{ "RNRH", { (void *)&cmd_rnrh }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RA*/
{ "RSLG", { (void *)&cmd_rslg }, NO_ARG,   CON_COMANDI, MINLVL_R_SYSLOG },/*log*/
{ "RSST", { (void *)&cmd_rsst }, NO_ARG,   CON_COMANDI, MINLVL_READ_STATS },
{ "RSWP", { (void *)&cmd_rswp }, PASS_ARG, CON_COMANDI, MINLVL_SWAPROOM },
{ "RSYS", { (void *)&cmd_rsys }, PASS_ARG, CON_COMANDI, MINLVL_READ_CFG },/*log*/
{ "RUSR", { (void *)&cmd_rusr }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "RUZP", { (void *)&cmd_ruzp }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "RZAP", { (void *)&cmd_rzap }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "RZPA", { (void *)&cmd_rzpa }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "SDWN", { (void *)&cmd_sdwn }, PASS_ARG, CON_COMANDI, MINLVL_SHUTDOWN },
#ifdef USE_REFERENDUM
{ "SCPU", { (void *)&cmd_scpu }, NO_ARG,   CON_COMANDI, MINLVL_URNA }, 
#if 0
{ "SCVL", { (void *)&cmd_scvl }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
#endif
{ "SDEL", { (void *)&cmd_sdel }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "SINF", { (void *)&cmd_sinf }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SLST", { (void *)&cmd_slst }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SPST", { (void *)&cmd_spst }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "SRIS", { (void *)&cmd_sris }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SREN", { (void *)&cmd_sren }, PASS_ARG, CON_REF_VOTO, MINLVL_URNA },
{ "SRNB", { (void *)&cmd_srnb }, PASS_ARG, CON_COMANDI, MINLVL_REFERENDUM }, /* vedere */
{ "SRNE", { (void *)&cmd_srne }, PASS_ARG, CON_REF_PARM, ILC_NOCHECK },
{ "SSTP", { (void *)&cmd_sstp }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
#endif
{ "TABC", { (void *)&cmd_tabc }, PASS_ARG, CON_MASK_TABC, ILC_LOGGED_IN },
{ "TEXT", { (void *)&cmd_text }, PASS_ARG, CON_MASK_TEXT, ILC_NOCHECK },/*ved.*/
{ "TIME", { (void *)&cmd_time }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "TMSG", { (void *)&cmd_tmsg }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "UPGR", { (void *)&cmd_upgr }, NO_ARG,   CON_COMANDI, ILC_NO_LOGIN },
{ "UPGS", { (void *)&cmd_upgs }, NO_ARG,   CON_COMANDI, LVL_SYSOP   },
{ "USER", { (void *)&cmd_user }, PASS_ARG, CON_LIC,     ILC_NOCHECK },/*Control*/
{ "USR1", { (void *)&cmd_usr1 }, PASS_ARG, CON_LIC,     ILC_NOCHECK },/*Control*/
{ "XEND", { (void *)&cmd_xend }, NO_ARG,   CON_XMSG,    MINLVL_XMSG },
{ "XINM", { (void *)&cmd_xinm }, PASS_ARG, CON_XMSG,    MINLVL_XMSG },
{ "XMSG", { (void *)&cmd_xmsg }, NO_ARG,   CON_COMANDI, MINLVL_XMSG },
{     "", { (void *)   NULL   },    0    ,      0     ,       0     }
};

/*****************************************************************************
*****************************************************************************/
void cmd_noop(struct sessione *t)
{
}

void cmd_echo(struct sessione *t, char *buf)
{
	if (strlen(buf) >= 5)
		cprintf(t, "%d ok: '%s'\n", OK, &buf[5]);
	else
		cprintf(t, "%d ok: ''\n", OK);
}

void interprete_comandi(struct sessione *t, char *com)
{
	int i = 0;
	int minlvl;

        /* citta_logf("INTERPRETE %s", com); */
	while ( *(cmd_list[i].token) != '\0' ) {
		if (!strncmp(com, cmd_list[i].token, LTOKEN)) {
			/* Controlla che l'utente e` nello stato giusto */
			if ((cmd_list[i].stato & t->stato) == 0) {
				cprintf(t, "%d\n", ERROR+WRONG_STATE);
				return;
			}

			/* Controlla il livello minimo per il comando */
			minlvl = cmd_list[i].minlvl;
			switch (minlvl) {
			case ILC_NOCHECK:
				break;
			case ILC_NO_LOGIN:
				if (t->logged_in) {
					cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
					return;
				}
				break;
			case ILC_LOGGED_IN:
				if (!t->logged_in) {
					cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
					return;
				}
				break;
			case ILC_UTENTE:
				if (t->utente == NULL) {
					cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
					return;
				}
				break;
			case ILC_REGISTRATO:
				if ((t->utente == NULL)
				    || (!t->utente->registrato)) {
					cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
					return;
				}
				break;
			default:
				if ((t->utente == NULL)
				    || (t->utente->livello < minlvl)) {
					cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
					return;
				}
				break;
			}

			/* Esegue il comando */
			switch (cmd_list[i].argnum) {
			case CMD_NOOP:
				/* cmd_noop(t); */
				break;
			case NO_ARG:
				cmd_list[i].cmd.no_arg(t);
				clear_idle(t);
				break;
			case PASS_ARG:
				if ( com[LTOKEN] == '\0')
					com[ARGSTART] = '\0';
				cmd_list[i].cmd.pass_arg(t, com+ARGSTART);
				clear_idle(t);
				break;
			}
			/* Post execution command */
			t->num_cmd++;
			return;
		}
		i++;
	}
	cprintf(t, "%d Comando `%s' non riconosciuto.\n", ERROR, com);
}

/*
 * Cancella l'idle time della sessione t
 */
static inline void clear_idle(struct sessione *t)
{
	t->idle.cicli = 0;
	t->idle.min = 0;
	t->idle.ore = 0;
	t->idle.warning = 0;	
}

/*****************************************************************************/
