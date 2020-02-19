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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : file_messaggi.c                                                    *
*        Funzioni per creare, cancellare, configurare, gestire i file       *
*        messaggi e per scriverci o leggerci i post.                        *
****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_messaggi.h"
#include "cittaserver.h"
#include "extract.h"
#include "memstat.h"
#include "utility.h"

/*
 * NOTE:
 * (1) Struttura dei file messaggi
 *     I file messaggi sono file circolari, con i messaggi che si susseguono.
 *     Il messaggio e' composto dalla successione:
 *     - byte 255 che indica l'inizio del messaggio
 *     - una stringa (header del messaggio) lunga al piu' 256 bytes
 *     - byte 0 che indica fine header/inizio post
 *     - testo arbitrariamente lungo che si conclude con "000\n"
 *     - byte 0 che indica la fine del testo
 *     - checksum del messaggio (long)
 *     - byte 0 che indica la fine del messaggio.
 *     L'header e' una stringa di argomenti separati dal carattere '|'
 *     il primo argomento e' un long, corrispondente al numero (eternal)
 *     del messaggio sul file messaggi.
 */

/* Prototipi funzioni in file_messaggi.c */
void fm_load_data(void);
void fm_save_data(void);
long fm_create(long fm_size, char *desc);
char fm_delete(long fmnum);
char fm_resize(long fmnum, long newsize);
char fm_expand(long fmnum, long newsize);
void fm_check(void);
void fm_mantain(void);
void fm_rebuild(void);
#ifdef USE_STRING_TEXT
char fm_get(long fmnum, long msgnum, long msgpos, char **txt, size_t *len,
	    char *header);
long fm_put(long fmnum, char *txt, size_t len, char *header, long *msgpos);
#else
char fm_get(long fmnum, long msgnum, long msgpos, struct text *txt,
	    char *header);
long fm_put(long fmnum, struct text *txt, char *header, long *msgpos);
#endif
char fm_delmsg(long fmnum, long msgnum, long msgpos);
char fm_getheader(long fmnum, long msgnum, long msgpos, char *header);
char fm_headers(long fmnum, struct text *txt);
long fm_lowest(long fmnum);
struct file_msg * fm_find(long fmnum);
static void fm_name(char *file_name, long fmnum);
static int fm_getc(FILE *fp, long fmsize);
static void fm_putc(int c, FILE *fp, long fmsize);
static char fm_seek(long fmnum, long msgnum, long msgpos, FILE **fp,
                    struct file_msg **fm);
static long fm_gethdr(FILE *fp, char *header, long size);
static void fm_default(void);

/* Definizione variabili globali */
struct fm_list *lista_fm;
struct fm_list *ultimo_fm;

/****************************************************************************
****************************************************************************/
/*
 * Carica la lista dei dati dei file_messaggi della bbs. Se i dati non
 * esistono, lo notifica nel log.
 */
void fm_load_data(void)
{
        FILE *fp;
        struct fm_list *fm, *punto;
        int hh;

        punto = NULL;
        lista_fm = NULL;
        ultimo_fm = NULL;

        fp = fopen(FILE_MSG_DATA, "r");
        if (!fp) {
        	citta_log("No fmdata: Creo File Messaggi di base.");
		fm_default();
        	return;
        }
        fseek(fp, 0L, 0);

        CREATE(fm, struct fm_list, 1, TYPE_FM_LIST);
        hh = fread((struct file_msg *) &(fm->fm), sizeof(struct file_msg),
                   1, fp);

        if (hh == 1) {
                lista_fm = fm;
                punto = NULL;
        } else {
        	citta_log("fmdata vuoto: Creo File Messaggi di base.");
		fm_default();
	}
	
        while (hh == 1) {
		/* le prossime due righe sono proprio sospette! non servono */
               citta_logf("Aperto FM #%ld [%s] con %ld msg, flags: %ld", fm->fm.num, fm->fm.desc, fm->fm.nmsg, fm->fm.flags);
               if (punto != NULL)
                        punto->next = fm;
                punto = fm;
                punto->next = NULL;
                ultimo_fm = punto;
                CREATE(fm, struct fm_list, 1, TYPE_FM_LIST);
                hh = fread((struct file_msg *) &(fm->fm),
                           sizeof(struct file_msg), 1, fp);
	}

	fclose(fp);
	Free(fm);
}

/*
 * Salva le strutture di dati di ogni file messaggi.
 */
void fm_save_data(void)
{
	FILE *fp;
	struct fm_list *punto, *prossimo;
	int hh;
	char bak[LBUF];

        sprintf(bak, "%s.bak", FILE_MSG_DATA);
	rename(FILE_MSG_DATA, bak);

	fp = fopen(FILE_MSG_DATA, "w");
	if (!fp) {
		citta_log("SYSERR: Non posso aprire in scrittura fmdata.");
		return;
	}
	/* Ciclo di scrittura dati */
	for (punto = lista_fm; punto; punto = prossimo) {
		prossimo = punto->next;
		hh = fwrite((struct file_msg *) &(punto->fm),
			    sizeof(struct file_msg), 1, fp);
		if (hh == 0)
			citta_log("SYSERR: Problemi scrittura struct file_msg.");
	}

	fclose(fp);
}

/*
 * Creazione di un nuovo file_message, lungo fm_size bytes. Viene creato e
 * inizializzato il file dei messaggi, e creata e inizializzata la
 * corrispondente struttura di dati file_msg, che viene successivamente
 * linkata alla lista dei file messaggi.
 * La funzione restituisce il numero di identificazione del nuovo file
 * messaggi se viene creato con successo, altrimenti ritorna zero.
 */
long fm_create(long fm_size, char *desc)
{
	FILE *fp;
	struct fm_list *newfm;
	long i;
	char filename[LBUF];

        /* Si rifiuta di creare file messaggi ridicolmente piccoli */
        if (fm_size < LBUF)
                return 0;

        CREATE(newfm, struct fm_list, 1, TYPE_FM_LIST);
        (dati_server.fm_num)++;
        (dati_server.fm_curr)++;
        newfm->fm.num = dati_server.fm_num;
        newfm->fm.size = fm_size;
        newfm->fm.lowest = 1L;
        newfm->fm.highest = 0L;
        newfm->fm.posizione = 0L;
        newfm->fm.flags = 0;
	newfm->fm.ctime = time(NULL);
        strncpy(newfm->fm.desc, desc, 64);
        newfm->fm.desc[63] = 0;
        newfm->next = NULL;

        fm_name(filename, newfm->fm.num);
	fp = fopen(filename, "wb");
	for (i = 0; i < fm_size; i++)
		putc(0, fp);
	fclose(fp);

	if (lista_fm == NULL)
		lista_fm = newfm;
	else
		ultimo_fm->next = newfm;
	ultimo_fm = newfm;
	citta_logf("SYSTEM: creato file messaggi #%ld, '%s'.", newfm->fm.num, desc);
        return newfm->fm.num;
}

/*
 * Funzione per eliminare un file dei messaggi
 * Cancella il filemessaggio lasciandone una copia con il suffisso .bak
 * per ogni evenienza, elimina la struttura dati del fm dalla lista dei
 * e libera la memoria corrispondente.
 * Ritorna 1 se l'operazione e' avvenuta con successo, altrimenti 0.
 */
char fm_delete(long fmnum)
{
        struct fm_list *punto, *fm, *fm_prev;
        char buf[LBUF], filename[LBUF];

        fm = NULL;

        if (lista_fm == NULL)
                return 0;

        /* Cerca il file messaggi e se lo trova lo elimina dalla lista */
        if (lista_fm->fm.num == fmnum) { /* E' il primo */
                fm = lista_fm;
                lista_fm = fm->next;
                if (lista_fm == NULL)
                        ultimo_fm = NULL;
        } else
                for (punto=lista_fm; punto; fm_prev=punto, punto=punto->next)
                        if (punto->fm.num == fmnum) {
                                fm = punto;
                                fm_prev->next = punto->next;
                                punto->next = NULL;
                        }
        if (fm == NULL)
                return 0;

        fm_name(filename, fmnum);
        strcpy(buf, filename);
        strcat(buf, ".bak");
        rename(filename, buf);

        (dati_server.fm_curr)--;
        citta_logf("SYSTEM: File message [%ld] '%s' deleted.", fmnum, fm->fm.desc);
        Free(fm);
        return 1;
}

/*
 * Funzione per ridimensionare la lunghezza di un file dei messaggi.
 * Ridimensiona il file messaggi # msgnum e lo porta alla lunghezza
 * newsize, preservando i post non cancellati. Tuttavia se si riduce
 * la dimensione del file messaggi, i messaggi piu' vecchi potrebbero
 * venire sovrascritti e percio' persi.
 * NOTA: Quando verranno implementate le room, andranno aggiornati anche
 *       i fullroom durante l'operazione.
 */
char fm_resize(long fmnum, long newsize)
{
        FILE *fp, *fp1, *fp2;
        struct file_msg *fm, *fm1;
        char filename[LBUF], h[LBUF], chkstr[LBUF], buf[LBUF];
        long newfmnum, num, pos, chksum, i, j;
        int c;

        fm = fm_find(fmnum);
        if (!fm)
                return 2;
        fm_name(filename, fmnum);
        fp = fopen(filename, "r");
        if (fm == NULL) {
                citta_logf("SYSERR: Can't open message file #%ld", fmnum);
                return FMERR_OPEN;
        }
        fseek(fp, fm->posizione, SEEK_SET);

        /* Creo il nuovo fm */
        if (!(newfmnum = fm_create(newsize, fm->desc))) {
                fclose(fp);
                return 1;
        }
        fm1 = fm_find(newfmnum);
        fm1->lowest = fm->lowest;

        fm_name(filename, newfmnum);
        fp1 = fopen(filename, "r+");
        fseek(fp1, 0L, SEEK_SET);
        fp2 = fopen(filename, "r");
        fseek(fp2, 0L, SEEK_SET);

        i = 0;
        do {
                c = fm_getc(fp, fm->size);
                i++;
                if (c == 255) {
                        pos = ftell(fp1);
                        if (fm_getc(fp2, newsize) == 255)
                                (fm1->lowest)++;
                        fm_putc(255, fp1, newsize);
                        chksum = 0L;
                        j = 0L;
                        do {
                                c = fm_getc(fp, fm->size);
                                if (fm_getc(fp2, newsize) == 255)
                                        (fm1->lowest)++;
                                fm_putc(c, fp1, newsize);
                                h[j++] = c;
                                chksum += c;
                                i++;
                                if (j == LBUF-1) {
                                        h[LBUF-1] = 0;
                                        while (c != 0)
                                                c = fm_getc(fp, fm->size);
                                }
                        } while (c != 0);
                        num = extract_long(h, 0);
                        do {
                                c = fm_getc(fp, fm->size);
                                if (fm_getc(fp2, newsize) == 255)
                                        (fm1->lowest)++;
                                fm_putc(c, fp1, newsize);
                                i++;
                                chksum += c;
                        } while (c != 0);
                        j = 0L;
                        do {
                                c = fm_getc(fp, fm->size);
                                if (fm_getc(fp2, newsize) == 255)
                                        (fm1->lowest)++;
                                fm_putc(c, fp1, newsize);
                                i++;
                                chkstr[j++] = c;
                                if (j == LBUF-1)
                                        chkstr[LBUF-1] = 0;
                        } while (c != 0);
                        if (chksum != atol(chkstr))
                                fseek(fp1, pos, SEEK_SET);
                        else
			  /* fm1->highest = MAX(fm1->highest, num); */
                                fm1->highest = (fm1->highest > num) ? 
				  fm1->highest : num;
                }
        } while (i < fm->size);

        fclose(fp2);
        fclose(fp1);
        fclose(fp);
        fm->size = newsize;
        fm->lowest = fm1->lowest;
        fm->highest = fm1->highest;
        fm->posizione = fm1->posizione;

        fm_name(filename, fmnum);
        strcpy(buf, filename);
        strcat(buf, ".bak");
        rename(filename, buf);
        fm_name(buf, newfmnum);
        rename(buf, filename);

        fm_delete(newfmnum);
        (dati_server.fm_num)--;
        return FMERR_OK;
}

/*
 * Espande il file messaggi #fmnum e lo porta a newsize bytes.
 */
char fm_expand(long fmnum, long newsize)
{
        FILE *fp, *fp1;
        struct file_msg *fm;
        char filename[LBUF];
        long i;
        int c;

        fm = fm_find(fmnum);
        if (!fm)
                return 2;
        fm_name(filename, fmnum);
        fp = fopen(filename, "r+");
        if (fp == NULL) {
                citta_logf("SYSERR: Can't open message file #%ld", fmnum);
                return 4;
        }
        fseek(fp, 0, SEEK_END);
	for (i = fm->size; i < newsize; i++)
		putc(0, fp);

        fp1 = fopen(filename, "r");
        fseek(fp1, 0L, SEEK_SET);
        fseek(fp, fm->size, SEEK_SET);

	i = 0;
	while(c = fm_getc(fp1, fm->size), ((c!=255) && (i<fm->size))) {
		fm_putc(c, fp, newsize);
		i++;
	}
        fclose(fp1);
        fclose(fp);
        fm->size = newsize;
        return 0;
}
/*
 * Check dei file messaggi, da eseguire quando lo shutdown non e' avvenuto
 * correttamente. Ricostruisce le strutture file_msg leggendo il contenuto
 * dei file_messaggi.
 */
void fm_check(void)
{
        /* Verifica se la lunghezze del file_messaggi corrisponde */
        /* Verifica i checksum dei messaggi                       */
}

/*
 * Funzione di mantenimento del file messaggi, controlla che non sia
 * corrotto e elimina i messaggi scrollati dalla room e messaggi cancellati
 * per liberare del posto. Viene eseguito allo shutdown giornaliero.
 */
void fm_mantain(void)
{

}

/*
 * Legge un carattere tenendo conto della circolarita' dei file messaggi.
 */
static int fm_getc(FILE *fp, long fmsize)
{
        if (ftell(fp) >= fmsize)
                fseek(fp, 0L, SEEK_SET);
        return getc(fp);
}

/*
 * Scrive un carattere tenendo conto della circolarita' dei file messaggi.
 */
static void fm_putc(int c, FILE *fp, long fmsize)
{
        if (ftell(fp) >= fmsize)
                fseek(fp, 0L, SEEK_SET);
        putc(c, fp);
}

/*
 * Trova i dati corrispondenti al file messaggi fmnum, apre il file e si
 * posiziona all'inizio del messaggio (msgnum,msgpos). Se l'operazione ha 
 * ha successo ritorna 0, altrimenti un intero positivo corrispondente
 * all'errore riscontrato, come descritto per la funzione fm_get().
 */
static char fm_seek(long fmnum, long msgnum, long msgpos, FILE **fp,
                    struct file_msg **fm)
{
        char filename[LBUF];
        int c;

        *fm = fm_find(fmnum);
        if (*fm == NULL)
                return FMERR_NOFM;
        if (msgnum < (*fm)->lowest)
                return FMERR_ERASED_MSG;
        if ((msgnum > (*fm)->highest) || (msgpos < 0))
                return FMERR_NO_SUCH_MSG;

        fm_name(filename, fmnum);
        *fp = fopen(filename, "r+");
        if (*fp == NULL) {
                citta_logf("SYSERR: Can't open message file #%ld", fmnum);
                return FMERR_OPEN;
        }

        fseek(*fp, msgpos, SEEK_SET);
        c = fm_getc(*fp, (*fm)->size);
        if (c < FM_MSGINIT) {
                fclose(*fp);
                return FMERR_NO_SUCH_MSG;
        }
        if (c == 254) {
                fclose(*fp);
                return FMERR_DELETED_MSG;
        }
        return FMERR_OK;
}

/*
 * Legge un messaggio dal file messaggi.
 * Legge il messaggio numero msgnum dal file dei messaggi # msgnum,
 * restituisce l'header nella stringa puntata da header e copia il
 * messaggio nel file temporaneo tmpfile.
 * Restituisce 0 se il messaggio e' stato letto con successo, altrimenti
 * un intero corrispondente al tipo di errore riscontrato:
 * - 1 - file messaggi inesistente.
 * - 2 - il messaggio e' stato sovrascritto.
 * - 3 - il messaggio e' inesistente.
 * - 4 - non riesce ad aprire il file dei messaggi.
 * - 5 - il messaggio e' stato cancellato.
 * - 6 - messaggio corrotto (chksum errato).
 * - 7 - il msgnum fornito non corrisponde con quello del messaggio.
 * Nota: *header deve essere allocato almeno con LBUF char per evitare
 *       sorprese.
 */
#ifdef USE_STRING_TEXT
char fm_get(long fmnum, long msgnum, long msgpos, char **txt, size_t *len,
	    char *header)
#else
char fm_get(long fmnum, long msgnum, long msgpos, struct text *txt,
            char *header)
#endif
{
        struct file_msg *fm;
        FILE *fp;
        char err, chkstr[LBUF];
        long num, chksum;
        register int c;
	int i;
#ifdef USE_STRING_TEXT
	register char *ptr, *endptr;
	size_t size;
#endif

        if ((err = fm_seek(fmnum, msgnum, msgpos, &fp, &fm)))
                return err;

        chksum = fm_gethdr(fp, header, fm->size);

        num = extract_long(header, 0);
        if (num != msgnum)
                return FMERR_MSGNUM;

#ifdef USE_STRING_TEXT
# define STR_LEN 64
	size = STR_LEN;
	CREATE(*txt, char, STR_LEN, TYPE_CHAR);
	ptr = *txt;
	endptr = *txt + size;
	do {
		if (ptr == endptr) {
			size <<= 2; /* Raddoppia il blocco */
			RECREATE(*txt, char, size);
			endptr = *txt + size;
			ptr = *txt + (size >> 2);
		}
                c = fm_getc(fp, fm->size);
                chksum += c;
		if (c == '\n')
			*ptr = '\0';
		else
			*ptr = c;
		ptr++;
	} while(c != 0);
	*len = (size_t)(ptr - *txt);
	RECREATE(*txt, char, *len);
#else
        do {
                c = fm_getc(fp, fm->size);
                chksum += c;
                txt_putc(txt, c);
        } while (c != 0);
#endif

        /* Verifica Checksum */
        i = 0;
        do {
                c = fm_getc(fp, fm->size);
                chkstr[i++] = c;
                if (i == LBUF-1) {
                        chkstr[LBUF-1] = 0;
			c = 0;
		}
        } while (c != 0);
	chkstr[i] = '\0';
        fclose(fp);
        if (chksum != atol(chkstr))
                return FMERR_CHKSUM;
        return FMERR_OK;
}

/*
 * Scrive un messaggio sul file messaggi
 * Scrive nel file messaggi numero fmnum il messaggio nel file msgfile con
 * header puntato da header. Restituisce il numero del messaggio se il
 * post e' stato salvato con successo, altrimenti 0.
 */
#ifdef USE_STRING_TEXT
long fm_put(long fmnum, char *txt, size_t len, char *header, long *msgpos)
#else
long fm_put(long fmnum, struct text *txt, char *header, long *msgpos)
#endif
{
        struct file_msg *fm;
        char fmname[LBUF], h[LBUF];
        size_t hlen;
        int c, c1;
        long msgnum, i;
        long chksum = 0;
        FILE *fp, *fp2;
#ifdef USE_STRING_TEXT
	size_t pos;
#else
        char *str;
#endif

        fm = fm_find(fmnum);
        if (!fm) {
		citta_logf("FM: non esiste il file messaggi #%ld", fmnum);
		return 0;
	}
	
        fm_name(fmname, fmnum);
        msgnum = ++(fm->highest);
        hlen = sprintf(h, "%ld|", msgnum);
        if (strlen(header)+hlen > (LBUF-1)) {
                citta_log("SYSERR Header too long!!");
                return -1; /* header too long */
        }
        hlen += sprintf(h+hlen, "%s", header);

#ifdef USE_STRING_TEXT
	/* msglen = len; */
#else
        /* msglen = txt_tell(txt); */
        txt_rewind(txt);
#endif
	*msgpos = fm->posizione;
        if ((fp = fopen(fmname, "r+")) == NULL)
		return 0;
        fseek(fp, fm->posizione, SEEK_SET);
        if ((fp2 = fopen(fmname, "r")) == NULL) {
		fclose(fp);
		return 0;
	}
        fseek(fp2, fm->posizione, SEEK_SET);

        /* Inizio messaggio */
        c1 = fm_getc(fp2, fm->size);
        if (c1 >= FM_MSGINIT) {
                (fm->lowest)++;
		(fm->nmsg)--;
		if (c1 == FM_MSG_DELETED)
			(fm->ndel)--;
	}
        fm_putc(255, fp, fm->size);
        /* header */
        i = 0;
        do { /* TODO potrebbe essere un for fino a hlen... */
		c1 = fm_getc(fp2, fm->size);
		if (c1 >= FM_MSGINIT) {
			(fm->lowest)++;
			(fm->nmsg)--;
			if (c1 == FM_MSG_DELETED)
				(fm->ndel)--;
		}
                fm_putc(h[i], fp, fm->size);
                chksum += h[i];
        } while (h[i++] != 0);

#ifdef USE_STRING_TEXT
	for (pos = 0; pos < len; pos++) {
		if (*txt != '\0')
                        c = *txt;
		else
			c = '\n';
		txt++;
/*
		if (c < 0)
			c = 0;
*/
		chksum += c;
		c1 = fm_getc(fp2, fm->size);
		if (c1 >= FM_MSGINIT) {
			(fm->lowest)++;
			(fm->nmsg)--;
			if (c1 == FM_MSG_DELETED)
				(fm->ndel)--;
		}
		fm_putc(c, fp, fm->size);
        }
#else
        while ((str = txt_get(txt))) {
                /* Questa risolve il problema dei 255, ma non rischio di */
                /* buttare caratteri utili???                            */
                while (*str != 0) {
                        c = *str;
                        str++;
                        if (c < 0)
                                c = 0;
                        chksum += c;
			c1 = fm_getc(fp2, fm->size);
			if (c1 >= FM_MSGINIT) {
				(fm->lowest)++;
				(fm->nmsg)--;
				if (c1 == FM_MSG_DELETED)
					(fm->ndel)--;
			}
                        fm_putc(c, fp, fm->size);
                }
        }
#endif
	c1 = fm_getc(fp2, fm->size);
	if (c1 >= FM_MSGINIT) {
		(fm->lowest)++;
		(fm->nmsg)--;
		if (c1 == FM_MSG_DELETED)
			(fm->ndel)--;
	}
        fm_putc(0, fp, fm->size);

        /* Scrivo il cheksum, ma primo vedo se sovrascrivo altri messaggi */
        sprintf(h, "%ld", chksum);
        i = 0L;
        do {
		c1 = fm_getc(fp2, fm->size);
		if (c1 >= FM_MSGINIT) {
			(fm->lowest)++;
			(fm->nmsg)--;
			if (c1 == FM_MSG_DELETED)
				(fm->ndel)--;
		}
               fm_putc(h[i], fp, fm->size);
        } while (h[i++] != 0);
        fm->posizione = ftell(fp);
        fclose(fp);
        fclose(fp2);

        /* Qui si potrebbe mettere un fm_save() per sicurezza, ma
         * probabilmente e' sufficiente farlo al momento del crash_save
         */

	(fm->nmsg)++;
        return msgnum;
}

/* 
 * Setta il flag "cancellato" per un messaggio di un file messaggi. Il
 * messaggio verra' successivamente cancellato fisicamente dalla chiamata
 * alla funzione fm_mantain().
 */
char fm_delmsg(long fmnum, long msgnum, long msgpos)
{
        FILE *fp;
        struct file_msg *fm;
        char err;

        if ((err = fm_seek(fmnum, msgnum, msgpos, &fp, &fm)))
                return err;
        fseek(fp, msgpos, SEEK_SET);
        fm_putc(254, fp, fm->size);
        fclose(fp);
	(fm->ndel)++;
        return 0;
}

/*
 * Salva nel file tmpfile gli headers di tutti i messaggi presenti nel
 * file dei messaggi fmnum con il loro numero e loro posizione.
 * Restituisce 0 se l'operazione ha successo, altrimenti un intero
 * positivo che descrive il tipo di errore riscontrato.
 */
char fm_headers(long fmnum, struct text *txt)
{
        struct file_msg *fm;
        char filename[LBUF], header[LBUF];
        long num, pos, i;
        int c;
        FILE *fp;

        fm = fm_find(fmnum);
        if (!fm)
                return FMERR_NOFM;

        fm_name(filename, fmnum);
        fp = fopen(filename, "r");
        if (fm == NULL) {
                citta_logf("SYSERR: Can't open message file #%ld", fmnum);
                return FMERR_OPEN;
        }

        fseek(fp, fm->posizione, SEEK_SET);

        i = 0;
        do {
                c = fm_getc(fp, fm->size);
                i++;
                if (c >= FM_MSGINIT) {
                        pos = ftell(fp) - 1;
                        fm_gethdr(fp, header, fm->size);
                        num = extract_long(header, 0);
                        i += strlen(header)+1;
                        switch(c) {
                        case 255: /* messaggio normale */
                                txt_putf(txt, "*** Messaggio normale n.%ld, "
                                        "pos = %ld.\n", num, pos);
                                break;
                        case 254: /* messaggio cancellato */
                                txt_putf(txt,"*** Messaggio cancellato n.%ld, "
                                        "pos = %ld.\n", num, pos);
                                break;
                        }
			txt_putf(txt, "%s\n", header);
                }
        } while (i < fm->size);

        fclose(fp);
        return FMERR_OK;
}

/*
 * Restituisce il puntatore alla struttura di dati del file messaggi numero
 * fmnum. Se tale file messaggi non esiste, restituisce NULL.
 */
struct file_msg * fm_find(long fmnum)
{
        struct fm_list *punto;

        for (punto = lista_fm; punto; punto = punto->next)
                if (punto->fm.num == fmnum)
                        return &(punto->fm);

        return NULL;
}

/*
 * Restituisce il nome del file messaggi # filenum con il suo path 
 * in filename. (Eventualmente trasformare in macro)
 */
static void fm_name(char *file_name, long fmnum)
{
  sprintf(file_name, "%s/file_msg_%ld", FILE_MSG_DIR, fmnum);
}

/*
 * Carica l'header del messaggio puntato da fp e lo salva in header.
 * Restituisce il checksum corrispondente all'header.
 */
static long fm_gethdr(FILE *fp, char *header, long size)
{
        long i;
        long chksum = 0;
        int c;

        i = 0;
        do {
                c = fm_getc(fp, size);
                chksum += c;
                header[i] = c;
                i++;
                if (i == LBUF-1) {
                        header[LBUF-1] = 0;
                        while (c != 0)
                                c = fm_getc(fp, size);
                }
        } while (c != 0);
        return chksum;
}

/*
 * Restituisce il numero del messaggio inferiore nel file messaggi.
 */
long fm_lowest(long fmnum)
{
	struct file_msg *fm;
	
	fm = fm_find(fmnum);
	if (fm)
		return fm->lowest;
	else
		return (-1L);
}


/*
 * Legge l'header di un messaggio dal file messaggi.
 * Legge l'header del messaggio #msgnum dal file dei messaggi #fmnum e
 * restituisce l'header nella stringa puntata da header.
 * Inoltre verifica l'integrita' del messaggio con il checksum:
 * restituisce 0 se il msg e' OK, altrimenti un codice di errore.
 * Nota: *header deve essere allocato almeno con LBUF char per evitare
 *       sorprese.
 */
char fm_getheader(long fmnum, long msgnum, long msgpos, char *header)
{
        struct file_msg *fm;
        FILE *fp;
        char err, chkstr[LBUF];
        long num, chksum;
        int c, i;

        if ((err = fm_seek(fmnum, msgnum, msgpos, &fp, &fm)))
                return err;

        chksum = fm_gethdr(fp, header, fm->size);

        num = extract_long(header, 0);
        if (num != msgnum)
                return FMERR_MSGNUM;

        do {
                c = fm_getc(fp, fm->size);
                chksum += c;
        } while (c != 0);

        /* Verifica Checksum */
        i = 0;
        do {
                c = fm_getc(fp, fm->size);
                chkstr[i++] = c;
                if (i == LBUF-1)
                        chkstr[LBUF-1] = 0;
        } while (c != 0);
        fclose(fp);
        if (chksum != atol(chkstr))
                return FMERR_CHKSUM;
        return FMERR_OK;
}

/*
 * Crea i file messaggi di default al primo run del server.	
 * (Per ora un unico file messaggi per tutte le room)
 */
static void fm_default(void)
{
	dati_server.fm_basic = fm_create(DFLT_FM_SIZE, "Basic");
	dati_server.fm_mail = dati_server.fm_basic;
	dati_server.fm_normal = dati_server.fm_basic;
}
