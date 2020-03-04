/*
 *  Copyright (C) 2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX client                                    (C) M. Caldarelli *
*                                                                           *
* File : metadata.c                                                         *
*        operazioni utente sul metadata                                     *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "metadata_ops.h"
#include "ansicol.h"
#include "colors.h"
#include "comunicaz.h"
#include "cittaclient.h"
#include "cittacfg.h"
#include "cml.h"
#include "conn.h"
#include "editor.h"
#include "file_flags.h"
#include "macro.h"
#include "messaggi.h"
#include "room_cmd.h"
#include "room_flags.h"
#include "extract.h"
#include "user_flags.h"
#include "utility.h"
#include "edit.h"

/* Prototipi funzioni statiche */
static bool mdop_user(char *user);
static bool mdop_room(char *room);
static bool mdop_post(char *room, long num);
static bool mdop_blog(char *user, long num);
static bool mdop_link(char *link, char *label);
static bool mdop_file(char *filename, unsigned long filenum,
		      unsigned long size);
static int md_type(char *filename);

/***************************************************************************/
/***************************************************************************/

bool mdop(Metadata_List *mdlist)
{
        Metadata *md;
        int n;
	bool ret = false;

        if ((!mdlist) || (mdlist->num == 0))
                return false;

        if (mdlist->num > 1) {
                mdop_print_list(mdlist);

                do {
                        cleanline();
                        cml_printf(_("<b>Scegli</b> (\\<<<b>Enter</b>> per uscire) : "),
                                   mdlist->num);
                        n = get_int(false);
                } while (n > mdlist->num);

                if (n == 0) {
                        cleanline();
                        return false;
                }
                putchar('\n');
        } else
                n = 1;

        md = mdlist->md[n-1];

        switch (md->type) {
        case MD_USER:
                ret = mdop_user(md->content);
                break;
        case MD_ROOM:
                ret = mdop_room(md->content);
                break;
        case MD_POST:
                ret = mdop_post(md->content, md->locnum);
                break;
        case MD_BLOG:
                ret = mdop_blog(md->content, md->locnum);
                break;
        case MD_LINK:
                mdop_link(md->content, md->str);
                break;
        case MD_FILE:
                mdop_file(md->content, md->filenum, md->size);
                break;
        }
        putchar('\n');

        return ret;

}


void mdop_print_list(Metadata_List *mdlist)
{
        Metadata *md;
        char buf[LBUF];
        int i;

        if (mdlist == NULL)
                return;

        cml_printf(_("*** Lista degli allegati presenti nel post:\n\n"));

        for (i = 0; i < MAXMDXMSG; i++) {
                if ( !mdlist->md[i] )
                        continue;

                md = mdlist->md[i];
                printf("%3d. ", i+1);
                switch (md->type) {
                case MD_USER:
                        printf("Utente ");
                        push_color();
                        setcolor(COLOR_USER);
                        printf("%s", md->content);
                        //print_user(md->content);
                        pull_color();
                        break;
                case MD_ROOM:
                        printf("Abbandona e vai alla room ");
                        push_color();
                        setcolor(COLOR_ROOM);
                        printf("%s>", md->content);
                        pull_color();
                        break;
                case MD_POST:
                        printf("Post ");
                        push_color();
                        setcolor(COLOR_LOCNUM);
                        cml_printf("#<b>%ld</b>", md->locnum);
                        pull_color();
                        cml_printf(_(" nella room "));
                        push_color();
                        setcolor(COLOR_ROOM);
                        printf("%s>", md->content);
                        pull_color();
                        putchar('.');
                        break;
                case MD_BLOG:
                        printf("Messaggio ");
                        push_color();
                        setcolor(COLOR_LOCNUM);
                        cml_printf("#<b>%ld</b>",md->locnum);
                        pull_color();
                        printf(_(" nel blog di "));
                        push_color();
                        setcolor(COLOR_USER);
                        printf("%s", md->content);
                        //print_user(md->content);
                        pull_color();
                        break;
                case MD_LINK:
                        printf("Link ");
                        push_color();
                        setcolor(COLOR_LINK);
                        if (md->str[0])
                                printf("%s", md->str);
                        else if (strlen(md->content) < 68)
                                printf("%s", md->content);
                        else {
                                strcpy(buf, md->content);
                                sprintf(buf+67, "..");
                                printf("%s", buf);
                        }
                        pull_color();
                        break;
                case MD_FILE:
                        cml_printf(_("File "));
                        push_color();
                        setcolor(COLOR_FILE);
                        printf("%s", md->content);
                        pull_color();
                        cml_printf(" (<b>%lu</b> bytes)", md->size);
                        break;
                }
                putchar('\n');
        }
        putchar('\n');
}

void mdop_delete(Metadata_List *mdlist, int mdnum)
{
        unsigned long filenum;
        char buf[LBUF];

        filenum = md_delete(mdlist, mdnum);
        if (filenum == 0)
                return;

        /* Cancella la prenotazione */
        serv_putf("FBKD %lu", filenum);
        serv_gets(buf);
}

void mdop_upload_files(Metadata_List *mdlist)
{
        Metadata *md;
        FILE *fp;
        struct stat sb;
        char *data;
        char buf[LBUF];
        int i;

        if ((mdlist == NULL) || (mdlist->uploads == 0))
                return;

        putchar('\n');

        if (mdlist->uploads == 1)
                cml_printf(_("Procedo con l'upload del file.\n"));
        else
                cml_printf(_("Procedo con l'upload dei %ld file.\n"),
                           mdlist->uploads);

        for (i = 0; i < MAXMDXMSG; i++) {
                if ( !mdlist->md[i] )
                        continue;
                md = mdlist->md[i];
                if (md->type == MD_FILE) {
                        if (stat(md->str, &sb) == -1) {
                                cml_printf(_("*** Non trovo pi&uacute; il file <b>%s</b>!\n"), md->str);
                                /* Elimino la prenotazione */
                                serv_putf("FBKD %lu", md->filenum);
                                serv_gets(buf);
                        } else if ((size_t)sb.st_size != md->size) {
                                cml_printf(_("*** La dimensione del file &egrave; cambiata nel frattempo...\n"));
                                /* Elimino la prenotazione */
                                serv_putf("FBKD %lu", md->filenum);
                                serv_gets(buf);
                        } else { /* Invia il file */
                                cml_printf(_(" Invio il file <b>%s</b> (<b>%lu</b> bytes) "), md->content, md->size);
                                serv_putf("FUPL %lu|%lu|%lu|%s", md->size,
                                          md->filenum, md->flags, md->content);
                                serv_gets(buf);
                                if (buf[0] != '2') {
                                        switch (buf[1]) {
                                        case '0':
                                                cml_printf(_("*** Errore: segnala il problema al sysop.\n"));
                                                break;
                                        case '1':
                                                cml_printf(_("*** Errore: l'upload va prima preonotato.\n"));
                                                break;
                                        case '2':
                                                cml_printf(_("*** Errore: il file non corrisponde alla prenotazione.\n"));
                                                break;
                                        }
                                } else {
                                        printf("[          ]");
                                        fflush(stdout);
                                        CREATE(data, char, md->size, 1);
                                        fp = fopen(md->str, "r");
                                        fread(data, md->size, 1, fp);
                                        fclose(fp);

                                        serv_write(data, md->size, true);
                                        putchar(' ');

                                        serv_puts("FUPE");
                                        serv_gets(buf);
                                        if (buf[0] != '2') {
                                         // TODO tratta messaggio di errore
                                         // Attualmente non puo' avvenire..
                                        } else
                                                print_ok();
                                }
                        }
                        md->str[0] = 0;
                        mdlist->uploads--;
                }
        }
        putchar('\n');
}

/******************************************************************************/

static bool mdop_user(char *user)
{
        char room[LBUF];
        int c = 0;

        cml_printf("--- \\<<b>g</b>>oto blog   \\<<b>p</b>>rofile   e\\<<b>x</b>>press message --- <b>Scegli:</b> ");
        while ((c==0) || ((c!=0) && (index("bpx",c) == NULL)))
                c = getchar();
        putchar(c);
        putchar('\n');
        switch (c) {
        case 'b':
                putchar('\n');
                clear_last_msg(true);
                snprintf(room, 50, ":%s", user); //TODO
                room_goto(2, false, room);
                return true;
        case 'p':
                profile(user);
                putchar('\n');
                break;
        case 'x':
                express(user);
                break;
        }
        return false;
}

static bool mdop_room(char *room)
{
        char *destroom;

        cml_printf(_("*** Room <b>%s</b>>\n"), room);
        cml_printf(_("    Vuoi abbandonare questa room e andarci? "));
        if (si_no() == 'n')
                return false;

        putchar('\n');
        destroom = Strdup(room);
        clear_last_msg(true);
        room_goto(2, false, destroom);
        Free(destroom);
        return true;
}

static bool mdop_post(char *room, long num)
{
        int c = 0;

        cml_printf(_("*** Messaggio #<b>%ld</b> in <b>%s</b>>\n"), num, room);
        cml_printf(_("    \\<<b>r</b>>ead post   \\<<b>g</b>>oto post   --- <b>Scegli:</b> "));
        while ((c==0) || ((c!=0) && (index("gr",c) == NULL)))
                c = getchar();
        putchar(c);
        putchar('\n');
        switch (c) {
        case 'g':
                return goto_msg(room, num);
        case 'r':
                show_msg(room, num);
                break;
        }
        return false;
}

static bool mdop_blog(char *user, long num)
{
        char room[LBUF];
        long destnum = num;
        int c = 0;

        sprintf(room, ":%s", user);
        cml_printf(_("*** Messaggio #<b>%ld</b> nella casa di <b>%s</b>:\n"), num, user);
        cml_printf(_("--- \\<<b>r</b>>ead message   \\<<b>g</b>>oto message   \\<<b>p</b>>rofile user   \\<<b>x</b>>-msg --- <b>Scegli:</b> "));
        while ((c==0) || ((c!=0) && (index("gr",c) == NULL)))
                c = getchar();
        putchar(c);
        putchar('\n');
        switch (c) {
        case 'g':
                putchar('\n');
                clear_last_msg(true);
                room_goto(2, false, room);
                read_msg(7, destnum);
                return true;
        case 'p':
                profile(user);
                putchar('\n');
                break;
        case 'r':
                show_msg(room, num);
                break;
        case 'x':
                express(user);
                break;
        }
        return false;
}

/* TODO returns always false.. why? */
static bool mdop_link(char *link, char *label)
{
        char cmd[LBUF]; /* TODO check lunghezza url */
        bool flag = true;

        if (label[0]) {
                cml_printf(_("\nLink \""));
                push_color();
                setcolor(COLOR_LINK);
                printf("%s", label);
                pull_color();
                printf("\"\n");
        } else if (strlen(link) < 73) {
                cml_printf(_("\nLink "));
                push_color();
                setcolor(COLOR_LINK);
                printf("%s", link);
                pull_color();
                flag = false;
        }
        putchar('\n');

        if (flag) {
                cml_printf(_("L'URL completo &egrave;:\n"));
                push_color();
                setcolor(COLOR_LINK);
                printf("%s", link);
                pull_color();
                putchar('\n');
        }

        if (local_client) {
                cml_printf(_("\n<b>Vuoi aprire il link nel tuo browser?</b> "));
                if (si_no() == 's') {
                        sprintf(cmd, "%s '%s'", BROWSER, link);
                        system(cmd);
                }
        } else
                cml_printf(_("\nUsando il client locale potrai aprire direttamente il link.\nPer ora, devi fare un copia e incolla del link nel tuo browser per aprirlo.\n"));

        return false;
}

static bool mdop_file(char *filename, unsigned long filenum,
		      unsigned long size)
{
        char buf[LBUF], fname[LBUF], filepath[LBUF];
	char *application;
        unsigned long len, i, prog_step;
        FILE *fp;
        int c = 0, progress, j, type;
        struct stat sb;

        if (!local_client) {
                cml_printf(_(
			     "Devi usare il client locale per scaricare "
			     ));
#if 0
		/* TODO the client should ask a cittaweb link from which
		   the user can download the file using a web browser.   */
		cml_printf(_(
"Usando il client locale potrai scaricare e aprire gli allegati direttamente\n"
"dal client. Nel frattempo, puoi accedere al file aprendo il link seguente"
			     ));
                /* Richiedi la prenotazione per un download */
		char link[LBUF];
                strcpy(link, "http://localhost:4040/");

                push_color();
                setcolor(COLOR_LINK);
                printf("%s", link);
                pull_color();
                cml_printf(_(
			     "nel tuo browser (link valido per un download).\n"
			     ));
#endif
                return false;
        }
        cml_printf(_("File <b>%s</b> (%lu bytes) --- Vuoi scaricarlo ora? "),
                   filename, size);
        if (si_no() == 'n')
                return false;

        sprintf(filepath, "%s/%s", DOWNLOAD_DIR, filename);
        if (stat(filepath, &sb) == 0) {
                c = 0;
                cml_printf(_("\nIl file <b>%s</b> esiste gi&agrave;.\n"),
                           filepath);
                cml_printf(_("Vuoi \\<<b>a</b>>nnullare \\<<b>r</b>>inominare o \\<<b>s</b>>ovrascrivere ? "));
                while ((c==0) || ((c!=0) && (index("ars",c) == NULL)))
                        c = getchar();
                putchar(c);
                putchar('\n');
                switch (c) {
                case 'a':
                        return false;
                case 'r':
                        if ( (getline_scroll(_("<b>Nuovo nome:</b> "),
                                      getcolor(), fname, LBUF/2, 0, 0) > 0)) {
                                strcpy(fname, filename);
                                sprintf(filepath, "%s/%s", DOWNLOAD_DIR, fname);
                        } else
                                return false;
                case 's':
                        unlink(filepath);
                        break;
                }

        }

        fp = fopen(filepath, "w");
        if (fp == NULL) {
                cml_printf(("*** Non riesco ad aprire in scrittura il file.\n"
                            "    Controlla i permessi in <b>%s</b>.\n"),
                           DOWNLOAD_DIR);
                return false;
        }

        serv_putf("FDNL %lu|%lu|%s", filenum, size, filename);
        serv_gets(buf);
        if (buf[0] != '2') {
                /* TODO sistemare i messaggi di errore */
                cml_printf(_("Il file che cerchi non &egrave; disponibile per il download.\n"));
                fclose(fp);
                return false;
        }

        len = extract_ulong(buf+4, 0);
        /* extract(name, buf+4, 1); NON SERVE */

        cml_printf(_("\nScarico il file <b>%s</b> (%lu bytes)... [>         ]"),
                   filename, len);
        serv_gets(buf);

        fflush(stdout);

        progress = 1;
        prog_step = len / 10;
        for (i = 0; i < len; i++) {
                if (i > prog_step*progress) {
                        delnchar(11);
                        for (j = 0; j < progress-1; j++)
                                putchar('-');
                        putchar('>');
                        for (j++; j < 10; j++)
                                putchar(' ');
                        putchar(']');
                        progress++;
                        fflush(stdout);
                }
                c = serv_getc();
                fputc(c, fp);
        }
        fclose(fp);
        putchar(' ');

        serv_gets(buf);

        print_ok();
        cml_printf(_("File salvato in <b>%s</b>\n\n"), filepath);

        type = md_type(filename);
        switch (type) {
        case FILE_UNKNOWN:
                return false;
        case FILE_IMAGE:
                cml_printf(_("Vuoi vedere l'immagine? "));
                application = PICTURE;
                break;
        case FILE_AUDIO:
                cml_printf(_("Vuoi ascoltare il brano? "));
                application = AUDIO;
                break;
        case FILE_VIDEO:
                cml_printf(_("Vuoi vedere il video? "));
                application = MOVIE;
                break;
        case FILE_HTML:
                cml_printf(_("Vuoi aprire il file nel tuo browser? "));
                application = BROWSER;
                break;
        default:
                cml_printf(_("Vuoi aprire il documento? "));
                switch (type) {
                case FILE_PDF:
                        application = PDFAPP;
                        break;
                case FILE_PS:
                        application = PSAPP;
                        break;
                case FILE_DVI:
                        application = DVIAPP;
                        break;
                case FILE_RTF:
                        application = RTFAPP;
                        break;
                case FILE_DOC:
                        application = DOCAPP;
                        break;
                        /* TODO visualizzare il testo col pager
                case FILE_TEXT:
                        application = TEXTAPP;
                        break;
                        */
                }
        }
        if (si_no() == 's') {
                sprintf(buf, "%s '%s'", application, filepath);
                system(buf);
        }

        return false;
}

/* TODO spostare in metadata.c - Usare mime type? */
int md_type(char *filename)
{
        char *extension;

        extension = find_extension(filename);
        if (!strcasecmp(extension, "jpg") ||
            !strcasecmp(extension, "jpeg") ||
            !strcasecmp(extension, "png") ||
            !strcasecmp(extension, "tiff") ||
            !strcasecmp(extension, "tif") ||
            !strcasecmp(extension, "gif") ||
            !strcasecmp(extension, "bmp") ||
            !strcasecmp(extension, "ico") ||
            !strcasecmp(extension, "pnm") ||
            !strcasecmp(extension, "pbm") ||
            !strcasecmp(extension, "pgm") ||
            !strcasecmp(extension, "ppm") ||
            !strcasecmp(extension, "qtif") ||
            !strcasecmp(extension, "qti") ||
            !strcasecmp(extension, "xbm") ||
            !strcasecmp(extension, "xpm"))
                return FILE_IMAGE;
        if (!strcasecmp(extension, "mp3") ||
            !strcasecmp(extension, "mpga") ||
            !strcasecmp(extension, "mp2") ||
            !strcasecmp(extension, "wav") ||
            !strcasecmp(extension, "aif") ||
            !strcasecmp(extension, "aiff") ||
            !strcasecmp(extension, "aifc"))
                return FILE_AUDIO;
        if (!strcasecmp(extension, "mov") ||
            !strcasecmp(extension, "mp4") ||
            !strcasecmp(extension, "mpeg") ||
            !strcasecmp(extension, "mpg") ||
            !strcasecmp(extension, "mpe") ||
            !strcasecmp(extension, "qt") ||
            !strcasecmp(extension, "avi") ||
            !strcasecmp(extension, "movie"))
                return FILE_VIDEO;
        if (!strcasecmp(extension, "pdf"))
                return FILE_PDF;
        if (!strcasecmp(extension, "ps") ||
            !strcasecmp(extension, "eps") ||
            !strcasecmp(extension, "ai"))
                return FILE_PS;
        if (!strcasecmp(extension, "dvi"))
                return FILE_DVI;
        if (!strcasecmp(extension, "html") ||
            !strcasecmp(extension, "htm"))
                return FILE_HTML;
        /* TODO quando verra' visualizzato col pager
        if (!strcasecmp(extension, "asc") ||
            !strcasecmp(extension, "txt"))
                return FILE_TEXT;
        */
        if (!strcasecmp(extension, "rtf"))
                return FILE_RTF;
        if (!strcasecmp(extension, "doc"))
                return FILE_DOC;
        return FILE_UNKNOWN;
}
