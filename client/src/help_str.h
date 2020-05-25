/*
 *  Copyright (C) 1999-2003 by Marco Caldarelli and Riccardo Vianello
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
* File : help_str.h                                                         *
*        definizione delle stringhe di help per comandi.c                   *
****************************************************************************/
#ifndef HELP_STR_H
#define HELP_STR_H

#define HELP_DOT_AIDE      "(Opzioni)\nScegli:\n"  \
                           " new room <a>ide\n"    \
		           " <b>roadcast\n"        \
		           " <e>dit commands\n"    \
		           " new room <h>elper\n"  \
		           " <i>nvite user\n"      \
		           " <k>ill commands\n"    \
		           " create <n>ew room\n"  \
		           " <r>ead commands\n"    \
		           " <s>wap rooms\n"       \
		           " <w>ho invited\n"

#define HELP_DOT_AIDE_EDIT "(Opzioni)\nScegli:\n"            \
			   " <c>onfigurazione del sistema\n" \
			   " room <i>nfo\n"                  \
			   " <m>essage\n"                    \
			   " <n>ews\n"                       \
			   " <p>assword di un utente\n"      \
			   " <r>oom\n"                       \
			   " room <s>ize\n"                  \
			   " <u>ser registration\n"          \
                           " richiedi nuova chiave di <v>alidazione\n"

#define HELP_DOT_AIDE_KILL "(Opzioni)\nScegli:\n"                      \
                           " <c>accia un utente connesso\n"            \
			   " <e>limina definitivamente un utente\n"    \
                           " <k>ick out di un utente da questa room\n" \
                           " end periodo di <K>ick out per un utente\n"\
			   " <r>oom corrente\n"                        \
			   " room kicked <w>ho\n"

#define HELP_DOT_AIDE_READ "(Opzioni)\nScegli:\n"            \
			   " <c>onfigurazione del sistema\n" \
			   " <r>oom list\n"

#define HELP_DOT_BLOG      "(Opzioni)\nScegli:\n"            \
                           " <c>onfigure\n"                  \
                           " <d>escription\n"                  \
			   " <g>oto\n"                       \
			   " <l>ista degli utenti con la casa aperta\n"

#define HELP_DOT_CHAT      "(Opzioni)\nScegli:\n"            \
                           " <a>scolta canale #\n"           \
			   " <e>sci dalla chat\n"            \
			   " <w>ho is in chat\n"

#define HELP_DOT_ENTER     "(Opzioni)\nScegli:\n"                \
                           " message as room <a>ide or helper\n" \
			   " <b>ug report\n"                     \
			   " <c>onfigurazione\n"                 \
			   " <d>oing\n"                          \
			   " message with <e>ditor\n"            \
			   " <i>nvite request\n"                 \
			   " <m>essage\n"                        \
			   " <P>assword\n"                       \
			   " <p>rofile\n"                        \
			   " <r>egistrazione\n"                  \
			   " <t>erminal\n"

#define HELP_DOT_FLOOR     "(Opzioni)\nScegli:\n"            \
		           " new floor <a>ide\n"             \
			   " <c>reate\n"                     \
			   " <d>elete\n"                     \
			   " <E>dit info\n"                  \
			   " <i>nfo\n"                       \
			   " <I>nfo dettagliate\n"           \
			   " <l>ist\n"                       \
			   " <m>ove room\n"                  \
			   " new room <h>elper    ***\n"     \
			   " <s>wap rooms         ***\n"

#define HELP_DOT_FRIEND    "(Opzioni)\nScegli:\n"            \
                           " edit <e>nemy list\n"            \
                           " edit <f>riend list\n"           \
			   " <r>ead friend and enemy lists.\n"

#define HELP_DOT_READ      "(Opzioni)\nScegli:\n"            \
                           " <b>rief\n"                      \
			   " <c>onfiguration\n"              \
			   " <f>orward\n"                    \
			   " <i>nfo dettagliate sulla room\n"\
			   " <n>ew\n"                        \
			   " <N>ews\n"                       \
			   " <p>rofile\n"                    \
			   " <r>everse\n"                    \
			   " <u>ser list\n"                  \
			   " <w>ho is online\n"

#define HELP_DOT_ROOM      "(Opzioni)\nScegli:\n"            \
                           " new room <a>ide\n"              \
			   " <c>reate\n"                     \
			   " <d>elete\n"                     \
			   " <E>dit\n"                       \
			   " <h>elper\n"                     \
			   " <i>nfo\n"                       \
			   " <s>ize\n"                       \
			   " <S>wap\n"                       \
			   " invite <u>ser\n"

#define HELP_DOT_SYSOP     "(Opzioni)\nScegli:\n"            \
                           " <b>anner commands\n"            \
                           " reset <d>ata protection consent\n" \
			   " <e>nter message\n"              \
			   " <f>ile message commands\n"      \
			   " <r>ead commands\n"              \
			   " set <u>pdate flag\n"            \
			   " <s>hutdown commands\n"

#define HELP_DOT_SYSOP_BANNERS "(Opzioni)\nScegli:\n"        \
			       " <m>odify\n"                 \
			       " <r>ehash\n"

#define HELP_DOT_SYSOP_FM  "(Opzioni)\nScegli:\n"            \
                           " <c>reate\n"                     \
			   " <d>elete post\n"                \
			   " <e>xpand\n"                     \
			   " <h>eader list\n"                \
			   " <i>nfo\n"                       \
			   " <k>ill\n"                       \
			   " <r>ead post\n"                  \
			   " <s>tatistiche\n"

#define HELP_DOT_SYSOP_READ "(Opzioni)\nScegli:\n"           \
                            " system <l>ogs\n"               \
                            " <s>erver statistics\n"         \
                            " <u>nregistered users\n"

#define HELP_DOT_SYSOP_SHUTDOWN "(Opzioni)\nScegli:\n"       \
			        " <b>egin\n"                 \
                                " <f>orce backup/logrotate scripts execution\n"\
				" with .<k>illscript\n"      \
				" <n>ow\n"                   \
				" <s>top\n"

#ifdef LOCAL
#define HELP_DOT_TOGGLE    "(Opzioni)\nScegli:\n"                  \
			   " <a>nsi colors\n"                      \
			   " <b>ell\n"                             \
			   " trascrizione discussioni in <c>hat\n" \
			   " <d>ebug mode\n"                       \
			   " <e>xpert mode\n"                      \
			   " X-msg reception from <f>riends\n"     \
			   " <g>rassetto\n"                        \
			   " salvataggio automatico <m>ail\n"      \
			   " <n>otifiche login/logout\n"           \
			   " follow <u>p for X-msg\n"              \
			   " salvataggio automatico <X>-msg\n"     \
			   " <x>-msg reception\n"
#else
#define HELP_DOT_TOGGLE    "(Opzioni)\nScegli:\n"                  \
			   " <b>ell\n"                             \
			   " <e>xpert mode\n"                      \
			   " X-msg reception from <f>riends\n"     \
			   " <g>rassetto\n"                        \
			   " <n>otifiche login/logout\n"           \
			   " follow <u>p for X-msg\n"              \
			   " <x>-msg reception\n"
#endif

#define HELP_DOT_URNA      "(Opzioni)\nScegli:\n"              \
			   " <c>ompleta\n"                     \
			   " <d>elete\n"                       \
			   " <l>ista consultazioni in corso\n" \
			   " <n>uova consultazione\n"          \
			   " <p>osticipa una consultazione\n"  \
			   " <r>ead\n"                         \
			   " <v>ota\n"

#define HELP_PAGER_CML "<b>Comandi del Message Pager:</b>\n\n"                 \
        " \\<<b>Space</b>>, \\<<b>n</b>>ext     : Avanza di una pagina.\n"     \
        " \\<<b>Enter</b>>, \\<<b>f</b>>orward  : Avanza di una riga.\n"       \
        " \\<<b>b</b>>ackward          : Torna indietro di una riga\n"         \
        " \\<<b>p</b>>revious          : Torna indietro di una pagina\n"       \
        " \\<<b>r</b>>efresh, \\<<b>Ctrl-L</b>> : Rivisualizza la schermata\n" \
        " \\<<b>s</b>>top, \\<<b>q</b>>uit      : Interrompe la lettura.\n"    \
        " \\<<b>?</b>>                 : questo aiuto.\n"


#endif /* help_str.h */
