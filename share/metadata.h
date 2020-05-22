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
* File : metadata.h                                                         *
*        gestione del metadata nei post.                                    *
****************************************************************************/
#ifndef METADATA_H
#define METADATA_H

#include <unistd.h>

/* Maximum number of metadata objects in a single message. */
#define MAXMDXMSG 64
/* Max length of metadata strings (including ending 0) */
#define MAXMDSTR  256
/* Max length of links. Must be smaller than MAXMDSTR. */
/* NOTE: the server accepts input commands of max 1024 chars and the full
         TEXT command with the line containing the link must fit in it.   */
#define MAXLEN_LINK (MAXMDSTR - 1)

/* Metadata type */
#define MD_USER   1
#define MD_ROOM   2
#define MD_POST   3
#define MD_BLOG   4
#define MD_LINK   5
#define MD_FILE   6

typedef struct Metadata_t {
        int num;
        int type;
        int file_type;
        char content[MAXMDSTR];
        char str[MAXMDSTR];
        long locnum;
        long msgnum;
        unsigned long flags;
        unsigned long size;
        unsigned long filenum;
} Metadata;

typedef struct Metadata_List_t {
        Metadata *md[MAXMDXMSG];
        int num;
        int uploads;
} Metadata_List;


/* Prototipi funzioni definite in metadata.c */
void md_init(Metadata_List *mdlist);
void md_free(Metadata_List *mdlist);
unsigned long md_delete(Metadata_List *mdlist, int mdnum);
int md_insert_link(Metadata_List *mdlist, char *link, char *name);
int md_insert_room(Metadata_List *mdlist, char *room);
int md_insert_post(Metadata_List *mdlist, char *room, long locnum);
int md_insert_blogpost(Metadata_List *mdlist, char *room, long locnum);
int md_insert_user(Metadata_List *mdlist, char *user);
int md_insert_file(Metadata_List *mdlist, char *filename, char *path, unsigned long filenum, unsigned long size, unsigned long flags);
size_t md_convert2cml(Metadata_List *mdlist, int md_id, char *str);
size_t md_convert2html(Metadata_List *mdlist, int md_id, char *str);

#endif /* metadata.h */
