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
        char (*special) (struct sessione *t, char *buf, char c);
} cmd_ptr;

struct comando {
	char token[5]; /* Nome del comando                                  */
	cmd_ptr cmd;   /* Puntatore alla funzione che implementa il comando */
	char argnum;   /* Numero argomenti della funzione                   */
	int  stato;    /* Stato attuale necessario per eseguire il comando  */
	char minlvl;   /* Livello minimo di accesso alla funzione           */
};

static const struct comando cmd_list[] = {
{ "ATXT", { cmd_atxt }, NO_ARG,   CON_MASK_TEXT, ILC_UTENTE },
#ifdef USE_VALIDATION_KEY
{ "AVAL", { cmd_aval }, PASS_ARG, CON_COMANDI, LVL_NON_VALIDATO },
#endif
{ "BEND", { cmd_bend }, NO_ARG,   CON_BROAD,   MINLVL_BROADCAST },
{ "BIFF", { .pass_arg = cmd_biff }, PASS_ARG, CON_COMANDI, MINLVL_BIFF },
#ifdef USE_BLOG
{ "BLGL", { cmd_blgl }, NO_ARG,   CON_COMANDI, MINLVL_BLOG },
#endif
{ "BRDC", { cmd_brdc }, NO_ARG,   CON_COMANDI, MINLVL_BROADCAST },
{ "BREG", { cmd_breg }, NO_ARG,   CON_COMANDI, MINLVL_REGISTRATION },
{ "BUGB", { cmd_bugb }, NO_ARG,   CON_COMANDI, MINLVL_BUGREPORT },
{ "BUGE", { cmd_buge }, NO_ARG,   CON_BUG_REP, MINLVL_BUGREPORT },
{ "CASC", { .pass_arg = cmd_casc }, PASS_ARG, CON_COMANDI, MINLVL_CHAT },
{ "CEND", { cmd_cend }, NO_ARG,   CON_CHAT,    MINLVL_CHAT },
{ "CFGG", { .pass_arg = cmd_cfgg }, PASS_ARG, CON_NOCHECK, MINLVL_USERCFG },
{ "CFGP", { .pass_arg = cmd_cfgp }, PASS_ARG, CON_COMANDI|CON_CONF, MINLVL_USERCFG },
{ "CHEK", { cmd_chek }, NO_ARG,   CON_COMANDI, ILC_NO_LOGIN },
{ "CLAS", { cmd_clas }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "CMSG", { cmd_cmsg }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "CUSR", { .special = cmd_cusr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "CWHO", { cmd_cwho }, NO_ARG,   CON_COMANDI, MINLVL_CHAT },
{ "DEST", { .pass_arg = cmd_dest }, PASS_ARG, CON_XMSG,    MINLVL_XMSG },
{ "ECHO", { .pass_arg = cmd_echo }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "EDNG", { .pass_arg = cmd_edng }, PASS_ARG, CON_LIC|CON_COMANDI, MINLVL_DOING },
{ "ESYS", { .pass_arg = cmd_esys }, PASS_ARG, CON_COMANDI, MINLVL_EDITCONFIG },
{ "EUSR", { .pass_arg = cmd_eusr }, PASS_ARG, CON_EUSR,    MINLVL_EDITUSR },
{ "FBKD", { .pass_arg = cmd_fbkd }, PASS_ARG, CON_UPLOAD,  MINLVL_DOWNLOAD },
{ "FIND", { .pass_arg = cmd_find }, PASS_ARG, CON_COMANDI, MINLVL_FIND },
{ "FMCR", { .pass_arg = cmd_fmcr }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMDP", { .pass_arg = cmd_fmdp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMHD", { .pass_arg = cmd_fmhd }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRI", { .pass_arg = cmd_fmri }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRM", { .pass_arg = cmd_fmrm }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMRP", { .pass_arg = cmd_fmrp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FMXP", { .pass_arg = cmd_fmxp }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },/* log */
{ "FDNL", { .pass_arg = cmd_fdnl }, PASS_ARG, CON_COMANDI, MINLVL_DOWNLOAD },
#ifdef USE_FLOORS
{ "FAID", { .pass_arg = cmd_faid }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "FDEL", { .pass_arg = cmd_fdel }, PASS_ARG, CON_COMANDI, MINLVL_DELFLOOR },
{ "FEDT", { .pass_arg = cmd_fedt }, PASS_ARG, CON_COMANDI|CON_FLOOR_EDIT, ILC_NOCHECK }, /* IS_FH() */
{ "FIEB", { cmd_fieb }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "FIEE", { cmd_fiee }, NO_ARG,   CON_FLOOR_INFO, ILC_NOCHECK }, /* vedere  */
{ "FINF", { cmd_finf }, NO_ARG,   CON_COMANDI, ILC_LOGGED_IN },
{ "FKRA", { .pass_arg = cmd_fkra }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FKRL", { .pass_arg = cmd_fkrl }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FKRM", { .pass_arg = cmd_fkrm }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "FLST", { .pass_arg = cmd_flst }, PASS_ARG, CON_COMANDI, MINLVL_FLOORAIDE },
{ "FMVR", { .pass_arg = cmd_fmvr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_FA*/
{ "FNEW", { .pass_arg = cmd_fnew }, PASS_ARG, CON_COMANDI, MINLVL_NEWFLOOR },
#endif
{ "FRDG", { cmd_frdg }, NO_ARG,   CON_NOCHECK, MINLVL_FRIENDS },
{ "FRDP", { .pass_arg = cmd_frdp }, PASS_ARG, CON_COMANDI, MINLVL_FRIENDS },
{ "FSCT", { cmd_fsct }, NO_ARG,   CON_COMANDI, MINLVL_SHUTDOWN },
{ "FUPB", { .pass_arg = cmd_fupb }, PASS_ARG, CON_POST,    MINLVL_DOWNLOAD },
{ "FUPE", { .pass_arg = cmd_fupe }, PASS_ARG, CON_UPLOAD,  MINLVL_DOWNLOAD },
{ "FUPL", { .pass_arg = cmd_fupl }, PASS_ARG, CON_COMANDI, MINLVL_DOWNLOAD },
{ "GCST", { .pass_arg = cmd_gcst},  PASS_ARG, CON_CONSENT,MINLVL_REGISTRATION},
{ "GMTR", { .pass_arg = cmd_gmtr }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "GOTO", { .pass_arg = cmd_goto }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "GREG", { cmd_greg }, NO_ARG,   CON_REG,     MINLVL_REGISTRATION },
{ "GUSR", { .pass_arg = cmd_gusr }, PASS_ARG, CON_COMANDI, MINLVL_EDITUSR },
{ "GVAL", { cmd_gval }, NO_ARG,   CON_COMANDI, ILC_REGISTRATO },/*log*/
{ "HELP", { cmd_help }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "HWHO", { cmd_hwho }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "INFO", { .pass_arg = cmd_info }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "KUSR", { .pass_arg = cmd_kusr }, PASS_ARG, CON_COMANDI, MINLVL_KILLUSR },/*log*/
{ "LBAN", { .pass_arg = cmd_lban }, PASS_ARG, CON_LIC,     ILC_NOCHECK }, /* !logged_in */
{ "LBEB", { cmd_lbeb }, NO_ARG,   CON_COMANDI, MINLVL_BANNER },
{ "LBEE", { cmd_lbee }, NO_ARG,   CON_BANNER,  MINLVL_BANNER },
{ "LBGT", { cmd_lbgt }, NO_ARG,   CON_COMANDI, MINLVL_BANNER },
{ "LSSH", { .pass_arg = cmd_lssh }, PASS_ARG, CON_COMANDI|CON_SUBSHELL, MINLVL_SUBSHELL },
{ "LTRM", { .pass_arg = cmd_ltrm }, PASS_ARG, CON_COMANDI|CON_LOCK, MINLVL_LOCKTERM },
{ "MCPY", { .pass_arg = cmd_mcpy }, PASS_ARG, CON_COMANDI, ILC_NOCHECK }, /* Controllare */
{ "MDEL", { .pass_arg = cmd_mdel }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "MMOV", { .pass_arg = cmd_mmov }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "MSGI", { .pass_arg = cmd_msgi }, PASS_ARG, CON_NOCHECK, ILC_NOCHECK },
{ "NOOP", { cmd_noop }, CMD_NOOP, CON_NOCHECK, ILC_NOCHECK },
{ "NWSB", { cmd_nwsb }, NO_ARG,   CON_COMANDI, MINLVL_NEWS },
{ "NWSE", { cmd_nwse }, NO_ARG,   CON_NEWS,    ILC_NOCHECK },/*veder*/
{ "PARM", { .pass_arg = cmd_parm }, PASS_ARG, CON_MASK_PARM, ILC_NOCHECK },/*ved*/
{ "PRFB", { cmd_prfb }, NO_ARG,   CON_COMANDI, MINLVL_EDITPRFL },
{ "PRFE", { cmd_prfe }, NO_ARG,   CON_PROFILE, MINLVL_EDITPRFL },
{ "PRFG", { .pass_arg = cmd_prfg }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "PRFL", { .pass_arg = cmd_prfl }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },
{ "PSTB", { .pass_arg = cmd_pstb }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "PSTE", { .pass_arg = cmd_pste }, PASS_ARG, CON_POST,    ILC_NOCHECK },/*veder*/
{ "PWDC", { .pass_arg = cmd_pwdc }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*verif*/
{ "PWDN", { .pass_arg = cmd_pwdn }, PASS_ARG, CON_COMANDI, MINLVL_PWDN },
{ "PWDU", { .pass_arg = cmd_pwdu }, PASS_ARG, CON_COMANDI, MINLVL_PWDU },/* log */
{ "QUIT", { cmd_quit }, NO_ARG,   CON_NOCHECK, ILC_NOCHECK },
{ "RAID", { .pass_arg = cmd_raid }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "RALL", { cmd_rall }, NO_ARG,   CON_COMANDI, LVL_NORMALE },
{ "RCST", { cmd_rcst }, NO_ARG,   CON_COMANDI, LVL_SYSOP   },
{ "RDEL", { .pass_arg = cmd_rdel }, PASS_ARG, CON_COMANDI, MINLVL_DELROOM },
{ "READ", { .pass_arg = cmd_read }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "REDT", { .pass_arg = cmd_redt }, PASS_ARG, CON_COMANDI|CON_ROOM_EDIT, ILC_NOCHECK }, /* IS_RH() */
{ "RGST", { .pass_arg = cmd_rgst }, PASS_ARG, CON_REG, MINLVL_REGISTRATION },
{ "RIEB", { cmd_rieb }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "RIEE", { cmd_riee }, NO_ARG,   CON_ROOM_INFO, ILC_NOCHECK },/*ved*/
{ "RINF", { cmd_rinf }, NO_ARG,   CON_COMANDI, ILC_LOGGED_IN },
{ "RINV", { .pass_arg = cmd_rinv }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RINW", { cmd_rinw }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RIRQ", { .pass_arg = cmd_rirq }, PASS_ARG, CON_COMANDI, MINLVL_AUTOINV },
{ "RKOB", { .pass_arg = cmd_rkob }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKOE", { .pass_arg = cmd_rkoe }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKOW", { cmd_rkow }, NO_ARG,   CON_COMANDI, ILC_UTENTE  },/*IS_RH*/
{ "RKRL", { .pass_arg = cmd_rkrl }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "RLEN", { .pass_arg = cmd_rlen }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RH*/
{ "RLST", { .pass_arg = cmd_rlst }, PASS_ARG, CON_COMANDI, LVL_AIDE    },
{ "RMSG", { .pass_arg = cmd_rmsg }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "RNEW", { .pass_arg = cmd_rnew }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*Control*/
{ "RNRH", { .pass_arg = cmd_rnrh }, PASS_ARG, CON_COMANDI, ILC_NOCHECK },/*IS_RA*/
{ "RSLG", { cmd_rslg }, NO_ARG,   CON_COMANDI, MINLVL_R_SYSLOG },/*log*/
{ "RSST", { cmd_rsst }, NO_ARG,   CON_COMANDI, MINLVL_READ_STATS },
{ "RSWP", { .pass_arg = cmd_rswp }, PASS_ARG, CON_COMANDI, MINLVL_SWAPROOM },
{ "RSYS", { .pass_arg = cmd_rsys }, PASS_ARG, CON_COMANDI, MINLVL_READ_CFG },/*log*/
{ "RUSR", { cmd_rusr }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "RUZP", { cmd_ruzp }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "RZAP", { cmd_rzap }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },/*veder*/
{ "RZPA", { cmd_rzpa }, NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "SDWN", { .pass_arg = cmd_sdwn }, PASS_ARG, CON_COMANDI, MINLVL_SHUTDOWN },
#ifdef USE_REFERENDUM
{ "SCPU", { cmd_scpu },             NO_ARG,   CON_COMANDI, MINLVL_URNA },
#if 0
{ "SCVL", { .pass_arg = cmd_scvl }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
#endif
{ "SDEL", { .pass_arg = cmd_sdel }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "SINF", { .pass_arg = cmd_sinf }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SLST", { .pass_arg = cmd_slst }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SPST", { .pass_arg = cmd_spst }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "SRIS", { .pass_arg = cmd_sris }, PASS_ARG, CON_COMANDI, MINLVL_URNA },
{ "SREN", { .pass_arg = cmd_sren }, PASS_ARG, CON_REF_VOTO, MINLVL_URNA },
{ "SRNB", { .pass_arg = cmd_srnb }, PASS_ARG, CON_COMANDI, MINLVL_REFERENDUM }, /* vedere */
{ "SRNE", { .pass_arg = cmd_srne }, PASS_ARG, CON_REF_PARM, ILC_NOCHECK },
{ "SSTP", { .pass_arg = cmd_sstp }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
#endif
{ "TABC", { .pass_arg = cmd_tabc }, PASS_ARG, CON_MASK_TABC, ILC_LOGGED_IN },
{ "TEXT", { .pass_arg = cmd_text }, PASS_ARG, CON_MASK_TEXT, ILC_NOCHECK },/*ved.*/
{ "TIME", { cmd_time },             NO_ARG,   CON_COMANDI, ILC_NOCHECK },
{ "TMSG", { .pass_arg = cmd_tmsg }, PASS_ARG, CON_COMANDI, ILC_UTENTE  },
{ "UPGR", { cmd_upgr },             NO_ARG,   CON_COMANDI, ILC_NO_LOGIN },
{ "UPGS", { cmd_upgs },             NO_ARG,   CON_COMANDI, LVL_SYSOP   },
{ "UUSR", { .pass_arg = cmd_uusr }, PASS_ARG, CON_COMANDI, LVL_SYSOP   },
{ "USER", { .pass_arg = cmd_user }, PASS_ARG, CON_LIC,     ILC_NOCHECK },/*Control*/
{ "USR1", { .pass_arg = cmd_usr1 }, PASS_ARG, CON_LIC,     ILC_NOCHECK },/*Control*/
{ "XEND", { cmd_xend },             NO_ARG,   CON_XMSG,    MINLVL_XMSG },
{ "XINM", { .pass_arg = cmd_xinm }, PASS_ARG, CON_XMSG,    MINLVL_XMSG },
{ "XMSG", { .pass_arg = cmd_xmsg }, PASS_ARG, CON_COMANDI, MINLVL_XMSG },
{     "", { NULL   },    0    ,      0     ,       0     }
};

/*****************************************************************************
*****************************************************************************/
void cmd_noop(struct sessione *t)
{
        IGNORE_UNUSED_PARAMETER(t);
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
				cprintf(t, "%d %d|%d\n",
                                        ERROR+WRONG_STATE, t->stato,
                                        cmd_list[i].stato);
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
