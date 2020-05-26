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
* Cittadella/UX share                                     (C) M. Caldarelli *
*                                                                           *
* File : metadata.c                                                         *
*        gestione del metadata nei post.                                    *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "metadata.h"
#include "macro.h"
#include "room_flags.h"
#include "user_flags.h"

#define LBUF 256

/* TODO questo va eliminato dopo aver creato le commonutils */
extern char *space2under(char *stringa);


long prompt_attach; /* Numero allegati presenti nel post corrente  */

/* Prototipi funzioni statiche */
static Metadata * md_create(Metadata_List *mdlist);

/***************************************************************************/
/***************************************************************************/
void md_init(Metadata_List *mdlist)
{
        int i;

        if (mdlist == NULL)
                return;

        for (i = 0; i< MAXMDXMSG; i++)
                mdlist->md[i] = NULL;

        mdlist->num = 0;
        mdlist->uploads = 0;
        prompt_attach = 0;
}

void md_free(Metadata_List *mdlist)
{
        int i;

        if (mdlist == NULL)
                return;

        for (i = 0; i < MAXMDXMSG; i++) {
                if (mdlist->md[i]) {
                        Free(mdlist->md[i]);
                        mdlist->md[i] = NULL;
                }
        }
        mdlist->num = 0;
        mdlist->uploads = 0;
        prompt_attach = 0;
}

/* Restituisce il numero della prenotazione se era un upload, 0 altrimenti */
unsigned long md_delete(Metadata_List *mdlist, int mdnum)
{
        Metadata *md;
        int upload = 0;

        mdnum--;
        if ((mdnum >= MAXMDXMSG) || ( (md = mdlist->md[mdnum]) == NULL))
                return 0;

        if ( (md->type == MD_FILE) && (md->str[0])) {
                mdlist->uploads--;
                upload = md->filenum;
        }

        Free(md);
        mdlist->md[mdnum] = NULL;

        mdlist->num--;
        prompt_attach--;

        return upload;
}

int md_insert_link(Metadata_List *mdlist, char *link, char *name)
{
        Metadata *md;

        md = md_create(mdlist);
        if (md == NULL)
                return -1;
        md->type = MD_LINK;
        strncpy(md->content, link, MAXMDSTR);
        strncpy(md->str, name, MAXMDSTR);

        return md->num;
}

int md_insert_room(Metadata_List *mdlist, char *room)
{
        Metadata *md;
        int offset = 0;

        md = md_create(mdlist);
        if (!md)
                return -1;

	md->type = MD_ROOM;

        strncpy(md->content, room + offset, ROOMNAMELEN);

        return md->num;
}

int md_insert_post(Metadata_List *mdlist, char *room, long locnum)
{
        Metadata *md;
        int offset = 0;

        md = md_create(mdlist);
        if (!md)
                return -1; /* Non c'e' piu' posto */

        if (room[0] == ':') {
                md->type = MD_BLOG;
                offset = 1;
        } else
                md->type = MD_POST;

        strncpy(md->content, room + offset, ROOMNAMELEN);
        md->locnum = locnum;

        return md->num;
}

int md_insert_blogpost(Metadata_List *mdlist, char *room, long locnum)
{
        Metadata *md;

        md = md_create(mdlist);
        if (!md)
                return -1; /* Non c'e' piu' posto */

        md->type = MD_BLOG;

        strncpy(md->content, room, ROOMNAMELEN);
        md->locnum = locnum;

        return md->num;
}

int md_insert_user(Metadata_List *mdlist, char *user)
{
        Metadata *md;

        md = md_create(mdlist);
        if (!md)
                return -1;

        md->type = MD_USER;
        strncpy(md->content, user, MAXLEN_UTNAME);

        return md->num;
}

int md_insert_file(Metadata_List *mdlist, char *filename, char *path,
                   unsigned long filenum, unsigned long size,
                   unsigned long flags)
{
        Metadata *md;

        md = md_create(mdlist);
        if (!md)
                return -1;

        md->type = MD_FILE;
        strncpy(md->content, filename, MAXMDSTR);
        if (path) {
                strncpy(md->str, path, MAXMDSTR);
                mdlist->uploads++;
        }
        md->filenum = filenum;
        md->size = size;
        md->flags = flags;

        return md->num;
}

size_t md_convert2cml(Metadata_List *mdlist, int md_id, char *str, size_t size)
{
        Metadata *md;
        size_t len;

        if (mdlist == NULL)
                return 0;

        md_id--;
        if ((md_id >= MAXMDXMSG) || (md_id < 0) || (!mdlist->md[md_id])) {
                str[0] = 0;
                return 0;
        }

        md = mdlist->md[md_id];

        switch (md->type) {
        case MD_USER:
                len = snprintf(str, size, "<user=\"%s\">", md->content);
                break;
        case MD_ROOM:
		len = snprintf(str, size, "<room=\"%s\">", md->content);
                break;
        case MD_POST:
                len = snprintf(str, size, "<post room=\"%s\" num=%ld>",
                               md->content, md->locnum);
                break;
        case MD_BLOG:
                len = snprintf(str, size, "<blog user=\"%s\" num=%ld>",
                               md->content, md->locnum);
                break;
        case MD_LINK:
                if (md->str[0] != '\0')
                        len = snprintf(str, size, "<link=\"%s\" label=\"%s\">",
                                       md->content, md->str);
                else
                        len = snprintf(str, size, "<link=\"%s\">", md->content);
                break;
        case MD_FILE:
                len = snprintf(str, size,
                               "<file name=\"%s\" num=%ld size=%ld flags=%ld>",
                               md->content, md->filenum, md->size, md->flags);
                break;
        default:
                assert(false);
        }
        str[size - 1] = 0;

        return len;
}

size_t md_convert2html(Metadata_List *mdlist, int md_id, char *str, size_t size)
{
        Metadata *md;
        size_t len;
        char buf[LBUF];

        if (mdlist == NULL)
                return 0;

        if (md_id > mdlist->num)
                return 0;

        md = mdlist->md[md_id-1];

        switch (md->type) {
        case MD_USER:
                strncpy(buf, md->content, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                len = snprintf(str, size,
                  "<A href=\"/profile/%s\"><SPAN class=\"user\">%s</SPAN></A>",
                               space2under(buf), md->content);
                break;
        case MD_ROOM:
                strncpy(buf, md->content, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                len = snprintf(str, size,
                  "<A href=\"/%s\"><SPAN class=\"room\">%s</SPAN></A>",
                               space2under(buf), md->content);
                break;
        case MD_POST:
                strncpy(buf, md->content, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                len = snprintf(str, size, "<A href=\"/%s#%ld\"><SPAN class=\"room\">%s&gt;</SPAN> <SPAN class=\"postnum\">#%ld</SPAN></A>",
                               space2under(buf), md->locnum,
                               md->content, md->locnum);
                break;
        case MD_BLOG:
                strncpy(buf, md->content, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                len = snprintf(str, size, "<A href=\"/blog/%s#%ld\"><SPAN class=\"room\">blog di %s&gt;</SPAN> <SPAN class=\"postnum\">#%ld</SPAN></A>",
                               space2under(buf), md->locnum, md->content, md->locnum);
                break;
        case MD_LINK:
	        if (md->str[0] != '\0')
                        len = snprintf(str, size, "<A href=\"%s\"><SPAN class=\"link\">%s</SPAN></A>",
                                       md->content, md->str);
                else
                        len = snprintf(str, size, "<A href=\"%s\"><SPAN class=\"link\">%s</SPAN></A>",
                                       md->content, md->content);
                break;
        case MD_FILE:
                len = snprintf(str, size, "<A href=\"/file/%ld/%s\"><SPAN class=\"file\">%s</SPAN> (%ldkb)</A>",
                               md->filenum, md->content, md->content,
                               (long)md->size / 1024);
                break;
        }
        str[size - 1] = 0;

        return len;
}

/******************************************************************************/
static Metadata * md_create(Metadata_List *mdlist)
{
        Metadata *md;
        int num;

        if (mdlist == NULL)
                return NULL;

        /* Trova slot libera */
        num = 0;
        while ((num < MAXMDXMSG) && (mdlist->md[num]))
                num++;

        if (num >= MAXMDXMSG)
                return NULL; /* Non c'e' piu' posto */

        CREATE(md, Metadata, 1, 0);
        md->num = num+1;
        mdlist->md[num] = md;
        mdlist->num++;
        prompt_attach++;

        return md;
}
