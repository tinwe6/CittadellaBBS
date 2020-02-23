/*
 *  Copyright (C) 2005 by Marco Caldarelli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/******************************************************************************
* Cittadella/UX server                                      (C) M. Caldarelli *
*                                                                             *
* File: fileserver.c                                                          *
*       server per la gestione dell'upload/download di files.                 *
******************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include "config.h"
#include "fileserver.h"
#include "coda_testo.h"
#include "extract.h"
#include "file_flags.h"
#include "rooms.h"
#include "utente.h"
#include "utility.h"

/* Struttura per */
#define MAGIC_FILE_USERSTATS 1
struct file_userstats {
        unsigned int magic;
        /* Vettori con i dati per ciascun utente                            */
        long *user;                /* Numero di matricola dell'utente       */
        unsigned long *totbytes;   /* Dimensione totale dei file presenti   */
        unsigned long *numfiles;   /* Numero di file presenti               */
        unsigned long *downloads;  /* numero di downloads completati        */
        unsigned long *uploads;    /* numero di uploads completati          */
        long num;                  /* Numero di utenti con dati files       */
        long allocated;            /* Numero di slot allocati               */
        bool dirty;
};


#define MAGIC_FILE_INDEX 1
struct file_index {
        unsigned int magic;
        /* Vettori con i dati per ciascun file                              */
        unsigned long *filenum;   /* Numero di serie del file               */
        unsigned long *size;      /* Dimensione del file in bytes           */
        long *user;               /* Utente che ha effettuato l'upload      */
        long *msgnum;             /* Numero del messaggio a cui e' allegato */
        long *flags;              /* flag associati al file                 */
        unsigned long *downloads; /* Numero di volte che e` stato scaricato */
        unsigned long *copies;    /* Numero di post che linkano il file     */
        char **filename;          /* Nome del file                          */
        /* Dati globali del fileserver                                      */
        unsigned long lastnum;    /* Ultimo numero file assegnato           */
        unsigned long totbytes;   /* Dimensione totale dei file presenti    */
        unsigned long tot_up;     /* Numero totale di uploads completati    */
        unsigned long tot_down;   /* Numero totale di downloads completati  */
        long num;                 /* Numero di files presenti               */
        long allocated;           /* Numero di slot allocati                */
        bool dirty;
};

static struct file_userstats file_ustats;
static struct file_index file_index;
static struct file_booking *book_first;
static struct file_booking *book_last;

/* Numero di slot allocate alla volta */
#define FILE_INDEX_STEP  10
#define FILE_USTATS_STEP 10

/* Prototipi funzioni statiche */
static void fs_alloc_index(long num);
static void fs_alloc_userstats(long num);
static void fs_expand_index(void);
static void fs_expand_userstats(void);
static void fs_save_index(void);
static void fs_save_ustats(void);
static int fs_find_file(unsigned long num);
static int fs_find_user(long user);
static struct file_booking * fs_find_book(unsigned long filenum);
static int fs_adduser(long user);
static int fs_addindex(unsigned long filenum);
static void fs_delfile(int i);
static void fs_cancel_booking(struct file_booking *book, int i);
static void fs_delbook(struct file_booking *book);


/****************************************************************************/
/*
   Formato file index:

   Prima riga    :     magic num\n
   Seconda riga  :     lastnum tot_up tot_down\n
   Per ogni file :     filenum size msgnum user flags downloads copies\n
                       filename\n
                       
   Formato file userstats:

   Prima riga      :     magic num\n
   Per ogni utente :     user totbytes numfiles downloads uploads\n
*/
/****************************************************************************/
/* Inizializza il file server e carica i dati */
void fs_init(void)
{
	FILE *fp;
        long i, j, k;
        bool corrupted;

	fp = fopen(FILE_FILE_INDEX, "r");
	if (!fp) {
	        citta_logf("FILE: non trovo indice: lo creo.");
                fs_alloc_index(FILE_INDEX_STEP);
	} else {
                fscanf(fp, "%d %ld\n", &file_index.magic, &file_index.num);
                citta_logf("Loading index entries for %ld files", file_index.num);

                fs_alloc_index(file_index.num + FILE_INDEX_STEP);
                fscanf(fp, "%lu %lu %lu\n", &file_index.lastnum,
                       &file_index.tot_up, &file_index.tot_down);
                for (i = 0; i < file_index.num; i++) {
                        fscanf(fp, "%lu %lu %ld %ld %ld %lu %lu\n",
                               file_index.filenum+i, file_index.size+i,
                               file_index.msgnum+i, file_index.user+i,
                               file_index.flags+i, file_index.downloads+i,
                               file_index.copies+i);
                        CREATE(file_index.filename[i], char, MAXLEN_FILENAME+1,
                               TYPE_CHAR);
                        fscanf(fp, "%s\n", file_index.filename[i]);
                }
                fclose(fp);
        }
        file_index.dirty = false;

        file_ustats.dirty = false;
	fp = fopen(FILE_FILE_USTATS, "r");
	if (!fp) {
	        citta_logf("FILE: non trovo userstats: lo creo.");
                fs_alloc_userstats(FILE_USTATS_STEP);
	} else {
                fscanf(fp, "%d %ld\n", &file_ustats.magic, &file_ustats.num);
                citta_logf("Loading file_ustats entries for %ld users",
                      file_ustats.num);
                fs_alloc_userstats(file_ustats.num + FILE_USTATS_STEP);
                for (i = 0, j = 0; j < file_ustats.num; i++, j++) {
                        corrupted = false;
                        fscanf(fp, "%ld %lu %lu %lu %lu\n", file_ustats.user+i,
                               file_ustats.totbytes+i, file_ustats.numfiles+i,
                               file_ustats.downloads+i, file_ustats.uploads+i);
                        citta_logf("%ld %lu %lu %lu %lu", file_ustats.user[i],
                               file_ustats.totbytes[i],file_ustats.numfiles[i],
                             file_ustats.downloads[i],file_ustats.uploads[i]);
                        /* Questi controllo andranno eliminati */
                        if (file_ustats.totbytes[i] > ((unsigned long)-1)/2) {
                                file_ustats.totbytes[i] = 0;
                                file_ustats.dirty = true;
                        }
                        if (file_ustats.numfiles[i] > ((unsigned long)-1)/2) {
                                file_ustats.numfiles[i] = 0;
                                file_ustats.dirty = true;
                        }
                        citta_logf("%ld %lu %lu %lu %lu\n", file_ustats.user[i],
                               file_ustats.totbytes[i],file_ustats.numfiles[i],
                             file_ustats.downloads[i],file_ustats.uploads[i]);
                        for (k = 0 ; k < i; k++) {
                                if (file_ustats.user[k]==file_ustats.user[i]) {
                                        citta_logf("FS_USTATS dati per utente %ld duplicati.",
                                              file_ustats.user[i]);
                                        file_ustats.dirty = true;
                                        i--;
                                        k = i;
                                        corrupted = true;
                                }
                        }
                        if ((!corrupted)
                            && (!trova_utente_n(file_ustats.user[i]))) {
                                citta_logf("FS_USTATS Utente %ld non trovato",
                                      file_ustats.user[i]);
                                file_ustats.dirty = true;
                                i--;
                        }
                }
                file_ustats.num = i;
                citta_logf("FS_USTATS ustats.num = %lu", file_ustats.num);
                fclose(fp);
        }

        /* Inizializza le prenotazioni */
        book_first = NULL;
        book_last = NULL;
}

/* Chiudi il file server */
void fs_close(void)
{
        int i;

        while (book_first) {
                i = fs_find_user(book_first->user);
                file_ustats.totbytes[i] -= book_first->size;
                file_ustats.numfiles[i]--;
                fs_delbook(book_first);
                file_ustats.dirty = true;
        }

        fs_save();

        for (i=0; i<file_index.num; i++)
                Free(file_index.filename[i]);
}


/* Salva i dati del file server */
void fs_save(void)
{
        if (file_index.dirty)
                fs_save_index();

        if (file_ustats.dirty)
                fs_save_ustats();
}

/* Carica il file #filenum, nome, per il download.
 * Restituisce la dimensione in bytes del file, oppure 0 se ci sono problemi,
 * nel qual caso la variabile globale errno assume i seguenti valori:
 * ENOENT: No such file or directory
 *
 *
*/
unsigned long fs_download(unsigned long filenum, char *nome, char **data)
{
        char filename[LBUF];
        unsigned long size;
        FILE *fp;
        struct stat filestat;
        size_t len;
        int i;

        i = fs_find_file(filenum);
        if (i == -1) {
                errno = EFSNOTFOUND;
                return 0;
        }
        size = file_index.size[i];

        if (strncmp(nome, file_index.filename[i], MAXLEN_FILENAME)) {
                errno = EFSWRONGNAME;
                return 0;
        }
	citta_logf("FILESERVER: richiesta file %s", nome);
        snprintf(filename, LBUF, "%s/%ld", FILES_DIR, filenum);
        fp = fopen(filename, "r");
        if (fp == NULL) {
                errno = EFSNOENT;
                return 0;
	}
        fstat(fileno(fp), &filestat);
        len = filestat.st_size;
        if (size != len) {
                errno = EFSCORRUPTED;
                return 0;
        }

        CREATE(*data, char, len, TYPE_CHAR);
        fread(*data, filestat.st_size, 1, fp);
        fclose(fp);

        /* Aggiorna le statistiche file */
        file_index.downloads[i]++;
        file_index.tot_down++;
        file_index.dirty = true;

        return size;
}

void fs_delete_file(long msgnum)
{
        int i;

        for (i = 0; i < file_index.num; i++)
                if (file_index.msgnum[i] == msgnum) {
                        if ((--(file_index.copies[i])) == 0)
                                fs_delfile(i);
                }
}

/* Cancella allegati a file che sono stati sovrascritti */
void fs_delete_overwritten(void)
{
        long lowest;
        int i;

        lowest = fm_lowest(dati_server.fm_basic);
        citta_logf("LOWEST %ld FM %ld", lowest, dati_server.fm_basic);
        for (i = file_index.num-1; i >= 0; i--)
                if (file_index.msgnum[i] < lowest) {
                        citta_logf("del msgnum %ld", file_index.msgnum[i]);
                        fs_delfile(i);
                }
}

void fs_addcopy(long msgnum)
{
        int i;

        for (i = 0; i < file_index.num; i++)
                if (file_index.msgnum[i] == msgnum) {
                        file_index.copies[i]++;
                        return;
                }
}

void fs_send_ustats(struct sessione *t, long user)
{
        int i;

        i = fs_find_user(user);
        if (i == -1) {
                cprintf(t, "%d 6\n", ERROR);
                return;
        }
        cprintf(t, "%d 6|%lu|%lu|%lu|%lu\n", OK, file_ustats.totbytes[i],
                file_ustats.numfiles[i], file_ustats.downloads[i],
                file_ustats.uploads[i]);
}

/******************************************************************************/
/* Richiede il download di un file
 * FDNL filenum|size|filename                           */
void cmd_fdnl(struct sessione *t, char *arg)
{
        char nome[LBUF];
        unsigned long filenum, size;
        char *data;
        int i;

        filenum = extract_ulong(arg, 0);
        extract(nome, arg, 2);
        size = fs_download(filenum, nome, &data);

        if (size == 0) {
                if (errno == EFSNOTFOUND)
                        cprintf(t, "%d file not indexed\n", ERROR);
                else if (errno == EFSWRONGNAME)
                        cprintf(t, "%d wrong name\n", ERROR);
                else if (errno == EFSNOENT)
                        cprintf(t, "%d file not found\n", ERROR);
                else if (errno == EFSCORRUPTED)
                        cprintf(t, "%d file corrupted\n", ERROR);
                return;
        }

        if (size != extract_ulong(arg, 1)) {
                cprintf(t, "%d wrong size\n", ERROR);
                Free(data);
                return;
        }

        cprintf(t, "%d %lu|%s\n", SEGUE_BINARIO, size, nome);
        cprintf(t, "%d\n", SET_BINARY);
        metti_in_coda_bin(&t->output, data, size);
        cprintf(t, "%d %ld\n", END_BINARY, 0L);/*TODO? Invia Checksum anzi md5*/

        Free(data);

        /* Aggiorna le statistiche utente */
        if ( (i = fs_find_user(t->utente->matricola)) < 0)
                i = fs_adduser(t->utente->matricola);
        file_ustats.downloads[i]++;
        file_ustats.dirty = true;
}

/* File UPload Begin : vede se e' possibile eseguire un upload e prenota
 * un numero di file.
 * FUPB filename|size                                                    */
void cmd_fupb(struct sessione *t, char *arg)
{
        struct file_booking *book;
        unsigned long size;
        int i;

        if (!(t->room->data->flags & RF_DIRECTORY)) {
                cprintf(t, "%d Non e` una directory room\n", ERROR+FS_NOT_HERE);
                return;
        }

        if ( (i = fs_find_user(t->utente->matricola)) < 0)
                i = fs_adduser(t->utente->matricola);

        size = extract_ulong(arg, 1);
        if (size > FILE_MAXSIZE) {
                cprintf(t, "%d file troppo grande\n", ERROR+FS_TOO_BIG);
                return;
        }
        if (file_ustats.totbytes[i]+size > FILE_MAXUSER) {
                cprintf(t, "%d limite spazio utente raggiunto\n",
                        ERROR+FS_USERSPACE);
                return;
        }
        if (file_ustats.numfiles[i] == FILE_MAXNUM) {
                cprintf(t, "%d troppi files\n", ERROR+FS_TOO_MANY_FILES);
                return;
        }
        if (file_index.totbytes + size > FILE_DIRSIZE) {
                cprintf(t, "%d file server pieno\n", ERROR+FS_FULL);
                return;
        }

        file_index.lastnum++;
        file_index.totbytes += size;
        file_index.dirty = true;

        file_ustats.totbytes[i] += size;
        file_ustats.numfiles[i]++;
        file_ustats.dirty = true;

        /* Crea la prenotazione */
        CREATE(book, struct file_booking, 1, TYPE_FILE_BOOKING);
        book->user = t->utente->matricola;
        book->filenum = file_index.lastnum;
        extractn(book->filename, arg, 0, MAXLEN_FILENAME);
        book->size = size;
        book->room = t->room;
        book->next = 0;
        if (book_last)
                book_last->next = book;
        else
                book_first = book;
        book_last = book;
        t->upload_bookings++;

        cprintf(t, "%d %lu\n", OK, file_index.lastnum);
}

// TODO attenzione al carattere "|" nel filename!!!
/* Upload di un file gia' prenotato
 * FUPL size|filenum|flags|filename         */
void cmd_fupl(struct sessione *t, char *arg)
{
        struct file_booking *book;
        char filename[LBUF], filename2[LBUF];
        long user = t->utente->matricola;
        unsigned long filenum = extract_ulong(arg, 1);
        unsigned long size = extract_ulong(arg, 0);
        int i, index;

        if ( (book = fs_find_book(filenum)) == NULL) {
                cprintf(t, "%d no booking\n", ERROR+FS_NO_BOOK);
                return;
        }
        extractn(filename, arg, 3, MAXLEN_FILENAME);
        i = fs_find_user(user);
        if ((book->user != user) || (book->size != size)
            || (strncmp(book->filename, filename, MAXLEN_FILENAME))
            || (book->room != t->room)) {
                if (book->user == user)
                        fs_cancel_booking(book, i);
                cprintf(t, "%d bad booking\n", ERROR+FS_WRONG_BOOK);
                return;
        }

        sprintf(filename2, "%s/%lu", FILES_DIR, filenum);
        t->fp = fopen(filename2, "w"); // TODO chiuderlo se cade la sessione!
        if (t->fp == NULL) {
                cprintf(t, "%d\n", ERROR);
                fs_cancel_booking(book, i);
                return;
        }
        t->binsize = size;

        index = fs_addindex(filenum);
        file_index.size[index] = size;
        file_index.user[index] = user;
        file_index.msgnum[index] = t->last_msgnum;
        citta_logf("Upload file associato a msgnum %ld, index %d", t->last_msgnum,
              index);
        file_index.flags[index] = extract_ulong(arg, 2);
        file_index.downloads[index] = 0;
        file_index.copies[index] = 1;
        strncpy(file_index.filename[index], filename, MAXLEN_FILENAME);

        file_index.tot_up++;
        file_index.dirty = true;

        file_ustats.uploads[i]++;
        file_ustats.dirty = true;

        fs_delbook(book);

        t->stato = CON_UPLOAD;
        cprintf(t, "%d %lu\n", INVIA_BINARIO, size);
}

void cmd_fupe(struct sessione *t, char *arg)
{
	IGNORE_UNUSED_PARAMETER(arg);

        t->stato = CON_COMANDI;
        cprintf(t, "%d\n", OK);
}

/* File BooKing Delete
 * FBKD filenum          */
void cmd_fbkd(struct sessione *t, char *arg)
{
        struct file_booking *book;
        unsigned long filenum;
        int i;

        filenum = extract_ulong(arg, 0);
        book = fs_find_book(filenum);
        if (book == NULL) {
                cprintf(t, "%d No such booking.\n", ERROR);
                return;
        } else if (book->user != t->utente->matricola) {
                cprintf(t, "%d Wrong user.\n", ERROR);
                return;
        }

        i = fs_find_user(t->utente->matricola);
        fs_cancel_booking(book, i);
        t->upload_bookings--;

        cprintf(t, "%d\n", OK);
}

/******************************************************************************/

static void fs_alloc_index(long num)
{
        file_index.magic = MAGIC_FILE_INDEX;
        CREATE(file_index.filenum, unsigned long, num, TYPE_LONG);
        CREATE(file_index.size, unsigned long, num, TYPE_LONG);
        CREATE(file_index.user, long, num, TYPE_LONG);
        CREATE(file_index.msgnum, long, num, TYPE_LONG);
        CREATE(file_index.flags, long, num, TYPE_LONG);
        CREATE(file_index.downloads, unsigned long, num, TYPE_LONG);
        CREATE(file_index.copies, unsigned long, num, TYPE_LONG);
        CREATE(file_index.filename, char *, num, TYPE_VOIDSTAR);
        file_index.lastnum = 0;
        file_index.tot_up = 0;
        file_index.tot_down = 0;
        //file_index.num = 0;
        file_index.allocated = num;
        file_index.dirty = false;
}

static void fs_alloc_userstats(long num)
{
        file_ustats.magic = MAGIC_FILE_USERSTATS;        
        CREATE(file_ustats.user, long, num, TYPE_LONG);
        CREATE(file_ustats.totbytes, unsigned long, num, TYPE_LONG);
        CREATE(file_ustats.numfiles, unsigned long, num, TYPE_LONG);
        CREATE(file_ustats.downloads, unsigned long, num, TYPE_LONG);
        CREATE(file_ustats.uploads, unsigned long, num, TYPE_LONG);
        file_ustats.allocated = num;
        file_ustats.dirty = false;
}

static void fs_expand_index(void)
{
        int num;

        file_index.allocated += FILE_INDEX_STEP;
        num = file_index.allocated;
        RECREATE(file_index.filenum, unsigned long, num);
        RECREATE(file_index.size, unsigned long, num);
        RECREATE(file_index.user, long, num);
        RECREATE(file_index.msgnum, long, num);
        RECREATE(file_index.flags, long, num);
        RECREATE(file_index.downloads, unsigned long, num);
        RECREATE(file_index.copies, unsigned long, num);
        RECREATE(file_index.filename, char *, num);
}

static void fs_expand_userstats(void)
{
        int num;

        file_ustats.allocated += FILE_INDEX_STEP;
        num = file_ustats.allocated;
        RECREATE(file_ustats.user, long, num);
        RECREATE(file_ustats.totbytes, unsigned long, num);
        RECREATE(file_ustats.numfiles, unsigned long, num);
        RECREATE(file_ustats.downloads, unsigned long, num);
        RECREATE(file_ustats.uploads, unsigned long, num);
}

/* Salva i dati dell'index */
static void fs_save_index(void)
{
        char backup[LBUF];
	FILE *fp;
        long i;

        sprintf(backup, "%s.bak", FILE_FILE_INDEX);
        rename(FILE_FILE_INDEX, backup);

        fp = fopen(FILE_FILE_INDEX, "w");
        if (!fp) {
                citta_logf("SYSERR: Non posso aprire in scrittura %s",
                      FILE_FILE_INDEX);
                rename(backup, FILE_FILE_INDEX);
        } else {
                fprintf(fp, "%d %ld\n", file_index.magic, file_index.num);
                fprintf(fp, "%lu %lu %lu\n", file_index.lastnum,
                        file_index.tot_up, file_index.tot_down);
                for (i = 0; i < file_index.num; i++) {
                        fprintf(fp, "%lu %lu %ld %ld %ld %lu %lu\n",
                                file_index.filenum[i], file_index.size[i],
                                file_index.msgnum[i], file_index.user[i],
                                file_index.flags[i], file_index.downloads[i],
                                file_index.copies[i]);
                        fprintf(fp, "%s\n", file_index.filename[i]);
                }
                fclose(fp);
                unlink(backup);
                file_index.dirty = false;
        }
}

/* Salva i dati dell'index */
static void fs_save_ustats(void)
{
        char backup[LBUF];
	FILE *fp;
        long i;

        sprintf(backup, "%s.bak", FILE_FILE_USTATS);
        rename(FILE_FILE_USTATS, backup);

        fp = fopen(FILE_FILE_USTATS, "w");
        if (!fp) {
                citta_logf("SYSERR: Non posso aprire in scrittura %s",
                      FILE_FILE_USTATS);
                rename(backup, FILE_FILE_USTATS);
        } else {
                fprintf(fp, "%d %ld\n", file_ustats.magic, file_ustats.num);
                for (i = 0; i < file_ustats.num; i++) {
                        fprintf(fp,"%ld %lu %lu %lu %lu\n",file_ustats.user[i],
                              file_ustats.totbytes[i], file_ustats.numfiles[i],
                              file_ustats.downloads[i],file_ustats.uploads[i]);
                }
                fclose(fp);
                unlink(backup);
                file_ustats.dirty = false;
        }
}

static int fs_find_file(unsigned long num)
{
        int i;

        for (i = 0; i < file_index.num; i++)
                if (file_index.filenum[i] == num)
                        return i;
        return -1;
}

static int fs_find_user(long user)
{
        int i;

        for (i = 0; i < file_ustats.num; i++)
                if (file_ustats.user[i] == user)
                        return i;
        return -1;
}

static struct file_booking * fs_find_book(unsigned long filenum)
{
        struct file_booking *book;

        book = book_first;
        while (book) {
                if (book->filenum == filenum)
                        return book;
                book = book->next;
        }
        
        return NULL;
}

static int fs_adduser(long user)
{
        int i;

        if (file_ustats.num == file_ustats.allocated)
                fs_expand_userstats();

        for (i = 0; (i < file_ustats.num) && (file_ustats.user[i] < user); i++);

        if (i < file_ustats.num) {
                memmove(file_ustats.user+i+1, file_ustats.user+i,
                        file_ustats.num-i);
                memmove(file_ustats.totbytes+i+1, file_ustats.totbytes+i,
                        file_ustats.num-i);
                memmove(file_ustats.numfiles+i+1, file_ustats.numfiles+i,
                        file_ustats.num-i);
                memmove(file_ustats.downloads+i+1, file_ustats.downloads+i,
                        file_ustats.num-i);
                memmove(file_ustats.uploads+i+1, file_ustats.uploads+i,
                        file_ustats.num-i);
        }

        file_ustats.user[i] = user;
        file_ustats.totbytes[i] = 0;
        file_ustats.numfiles[i] = 0;
        file_ustats.downloads[i] = 0;
        file_ustats.uploads[i] = 0;

        file_ustats.num++;
        file_ustats.dirty = true;

        return i;
}

static int fs_addindex(unsigned long filenum)
{
        int i;

        if (file_index.num == file_index.allocated)
                fs_expand_index();

        for (i = 0; (i < file_index.num) && (file_index.filenum[i] < filenum);
             i++);

        if (i < file_index.num) {
                memmove(file_index.filenum+i+1, file_index.filenum+i,
                        file_index.num-i);
                memmove(file_index.size+i+1, file_index.size+i,
                        file_index.num-i);
                memmove(file_index.user+i+1, file_index.user+i,
                        file_index.num-i);
                memmove(file_index.msgnum+i+1, file_index.msgnum+i,
                        file_index.num-i);
                memmove(file_index.flags+i+1, file_index.flags+i,
                        file_index.num-i);
                memmove(file_index.downloads+i+1, file_index.downloads+i,
                        file_index.num-i);
                memmove(file_index.copies+i+1, file_index.copies+i,
                        file_index.num-i);
                memmove(file_index.filename+i+1, file_index.filename+i,
                        file_index.num-i);
        }
        file_index.filenum[i] = filenum;
        CREATE(file_index.filename[i], char, MAXLEN_FILENAME+1, TYPE_CHAR);
        file_index.num++;

        return i;
}

/* TODO this function is unused */
#if 0
static int fs_deluser(long user)
{
        int i;

        if (file_ustats.num == file_ustats.allocated)
                fs_expand_userstats();

        for (i = 0; (i < file_ustats.num) && (file_ustats.user[i] < user); i++);

        if (i < file_ustats.num) {
                memmove(file_ustats.user+i+1, file_ustats.user+i,
                        file_ustats.num-i);
                memmove(file_ustats.totbytes+i+1, file_ustats.totbytes+i,
                        file_ustats.num-i);
                memmove(file_ustats.numfiles+i+1, file_ustats.numfiles+i,
                        file_ustats.num-i);
                memmove(file_ustats.downloads+i+1, file_ustats.downloads+i,
                        file_ustats.num-i);
                memmove(file_ustats.uploads+i+1, file_ustats.uploads+i,
                        file_ustats.num-i);
        }
        file_ustats.user[i] = user;
        file_ustats.num++;
        file_ustats.dirty = true;

        return i;
}
#endif

static void fs_delfile(int i)
{
        char filename[LBUF];
        long user;

        citta_logf("FILE elimino file #%lu [%s]", file_index.filenum[i],
              file_index.filename[i]);

        sprintf(filename, "%s/%lu", FILES_DIR, file_index.filenum[i]);
        if (unlink(filename) == -1)
                citta_logf("SYSERR File deletion failed.");

        file_index.totbytes -= file_index.size[i];
        file_index.dirty = true;

        if ( (user = fs_find_user(file_index.user[i])) != -1) {
                file_ustats.totbytes[user] -= file_index.size[i];
                file_ustats.numfiles[user]--;                
        }
        file_ustats.dirty = true;

        Free(file_index.filename[i]);
        memmove(file_index.filenum+i, file_index.filenum+i+1,
                file_index.num-i-1);
        memmove(file_index.size+i, file_index.size+i+1,
                file_index.num-i-1);
        memmove(file_index.user+i, file_index.user+i+1,
                file_index.num-i-1);
        memmove(file_index.msgnum+i, file_index.msgnum+i+1,
                file_index.num-i-1);
        memmove(file_index.flags+i, file_index.flags+i+1,
                file_index.num-i-1);
        memmove(file_index.downloads+i, file_index.downloads+i+1,
                file_index.num-i-1);
        memmove(file_index.copies+i, file_index.copies+i+1,
                file_index.num-i-1);
        memmove(file_index.filename+i, file_index.filename+i+1,
                file_index.num-i-1);
        file_index.num--;
}

/* Elimina tutte le prenotazioni dell'utente #user */
void fs_cancel_all_bookings(long user)
{
        struct file_booking *book, *next;
        int i;

        i = fs_find_user(user);
        if (i < 0)
                return;
        for (book = book_first; book; book = next) {
                next = book->next;
                if (book->user == user)
                        fs_cancel_booking(book, i);
        }
}

/* Elimina la prenotazione, ripristinando i dati di utilizzo dello spazio */
static void fs_cancel_booking(struct file_booking *book, int i)
{
        if (i < 0)
                return;

        if (file_index.lastnum == book->filenum)
                file_index.lastnum--;
        file_index.totbytes -= book->size;
        file_index.dirty = true;

        file_ustats.totbytes[i] -= book->size;
        file_ustats.numfiles[i]--;
        file_ustats.dirty = true;

        fs_delbook(book);
}

/* Elimina i dati sulla prenotazione book */
static void fs_delbook(struct file_booking *book)
{
        struct file_booking *b;

        b = book_first;
        if (b == book) {
                book_first = book->next;
                if (book_last == book)
                        book_last = NULL;
                Free(book);
        } else {
                while (b) {
                        if (b->next == book) {
                                b->next = book->next;
                                if (book_last == book)
                                        book_last = b;
                                Free(book);
                                return;
                        }
                        b = b->next;
                }
        }
}
