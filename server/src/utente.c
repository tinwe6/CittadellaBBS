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
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File: utente.c                                                            *
*       routines che riguardano operazioni sui dati dell'utente             *
****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#ifdef CRYPT
#include <crypt.h>
#endif

#include "cittaserver.h"
#include "generic.h"
#include "memstat.h"
#include "utente.h"
#include "utility.h"

/* Prototipi delle funzioni in questo file */
void carica_utenti(void);
void salva_utenti(void);
void free_lista_utenti(void);
struct dati_ut * trova_utente(const char *nome);
struct dati_ut * trova_utente_n(long matr);
const char * nome_utente_n(long matr);
struct sessione *collegato(char *ut);
struct sessione * collegato_n(long num);
char check_password(char *pwd, char *real_pwd); /* verifica la password */
void cripta(char *pwd);
bool is_friend(struct dati_ut *amico, struct dati_ut *utente);
bool is_enemy(struct dati_ut *nemico, struct dati_ut *utente);
void sut_set_all(int num, char flag);

#ifdef USE_VALIDATION_KEY
void invia_val_key(char *valkey, char *addr);
void purge_invalid(void);
#endif

struct lista_ut *lista_utenti, *ultimo_utente;

/******************************************************************************
******************************************************************************/
/*
 * carica_utenti() legge la lista delle strutture di utenti dal file
 * file_utenti e fa puntare lista_utenti al primo utente della lista
 */
void carica_utenti(void)
{
        FILE  *fp;
        struct lista_ut *utente, *punto;
        int hh;

        lista_utenti = NULL;
        ultimo_utente = NULL;

        /* Apre il file 'file_utenti' */
        fp = fopen(FILE_UTENTI, "r");
        if (!fp) {
                citta_log("SYSERR: Non posso aprire in lettura il file utenti.");
                return;
        }
        fseek(fp, 0L, 0);

        /* Ciclo di lettura dati */
        CREATE(utente, struct lista_ut, 1, TYPE_LISTA_UT);
        CREATE(utente->dati, struct dati_ut, 1, TYPE_DATI_UT);
        hh = fread((struct dati_ut *)utente->dati, sizeof(struct dati_ut),
		   1, fp);

        if (hh == 1) {
                lista_utenti = utente;
                punto = NULL;
        } else
                citta_log("SYSTEM: File_utenti vuoto, creato al primo salvataggio.");

        while (hh == 1) {
                if (punto != NULL)
                        punto->prossimo = utente;
                punto = utente;
                punto->prossimo = NULL;
                ultimo_utente = punto;
		CREATE(utente, struct lista_ut, 1, TYPE_LISTA_UT);
		CREATE(utente->dati, struct dati_ut, 1, TYPE_DATI_UT);
                hh = fread((struct dati_ut *)utente->dati,
			   sizeof(struct dati_ut), 1, fp);
        }
        Free(utente->dati);
        Free(utente);
        fclose(fp);
        /* Questo serve se si resetta la bbs mantenendo i vecchi utenti */
        if (ultimo_utente &&
            (ultimo_utente->dati->matricola >= dati_server.matricola))
                dati_server.matricola = ultimo_utente->dati->matricola + 1;
}

/*
 * salva_utenti salva la lista di strutture di utenti nel file FILE_UTENTI
 */
void salva_utenti(void)
{
        FILE *fp;
        struct lista_ut *punto;
        int hh;
        char bak[LBUF];

        sprintf(bak, "%s.bak", FILE_UTENTI);
        rename(FILE_UTENTI, bak);

        /* Apre il file 'file_utenti' */
        fp = fopen(FILE_UTENTI, "w");
        if (!fp) {
                citta_log("SYSERR: Non posso aprire in scrittura il file utenti.");
                return;
        }

        /* Ciclo di scrittura dati */
        for (punto = lista_utenti; punto; punto = punto->prossimo) {
                hh = fwrite((struct dati_ut *)punto->dati,
			    sizeof(struct dati_ut), 1, fp);
                if (hh == 0)
                        citta_log("SYSERR: Problemi scrittura struct dati_ut");
        }
        fclose(fp);
}

/*
 * Libera la memoria occupata dalla lista degli utenti, da eseguire prima
 * dello shutdown.
 */
void free_lista_utenti(void)
{
        struct lista_ut *prossimo;

        for ( ; lista_utenti; lista_utenti = prossimo) {
                prossimo = lista_utenti->prossimo;
                Free(lista_utenti->dati);
                Free(lista_utenti);
        }
}

/*
 * Cerca l'utente di nome *nome e se esiste restituisce il puntatore alla
 * sua struttura di dati, altrimenti NULL.
 */
struct dati_ut * trova_utente(const char *nome)
{
        struct lista_ut *punto;

        for (punto = lista_utenti; punto; punto = punto->prossimo) {
                if (!(strcasecmp(punto->dati->nome, nome)))
                        return punto->dati;
        }
        return NULL;
}

/*
 * Cerca l'utente dal # matricola e se esiste restituisce il puntatore
 * alla sua struttura di dati, altrimenti NULL
 */
struct dati_ut * trova_utente_n(long matr)
{
        struct lista_ut *punto;

        for (punto = lista_utenti; punto; punto = punto->prossimo) {
                if ((punto->dati->matricola) == matr)
                        return(punto->dati);
        }
        return(NULL);
}

/*
 * Restituisce il puntatore al nome dell'utente #matr.
 */
const char * nome_utente_n(long matr)
{
	return (trova_utente_n(matr))->nome;
}

/*
 * collegato(nick) restitutisce il puntatore alla struttura utente
 *                 dell'utente con nome nick se esiste, altrimenti NULL.
 */
struct sessione * collegato(char *ut)
{
        struct sessione *punto, *coll;

        coll = NULL;
        for (punto = lista_sessioni; !coll && punto; punto = punto->prossima)
                if ((punto->utente) && (!strcasecmp(ut, punto->utente->nome)))
                                coll = punto;
        return coll;
}

/*
 * collegato(num) restitutisce il puntatore alla struttura utente #num.
 */
struct sessione * collegato_n(long num)
{
        struct sessione *punto, *coll;

        coll = NULL;
        for (punto = lista_sessioni; !coll && punto; punto = punto->prossima)
                if ((punto->utente) && (punto->utente->matricola == num))
			coll = punto;
        return coll;
}

#ifdef USE_VALIDATION_KEY

/*
 * Invia la chiave di validazione per posta elettronica
 */
void invia_val_key(char *valkey, char *addr)
{
        char buf[LBUF];
        char *tmpf, *tmpf2;
#ifdef MKSTEMP
	int fd1, fd2;
#endif
        FILE *tf;

#ifdef MKSTEMP
	tmpf = Strdup(TEMP_VK_TEMPLATE);
	tmpf2 = Strdup(TEMP_VK_TEMPLATE);

	fd1 = mkstemp(tmpf);
	fd2 = mkstemp(tmpf2);
	close(fd1);
	close(fd2);
#else
	tmpf = tempnam(TMPDIR, PFX_VK);
	tmpf2 = tempnam(TMPDIR, PFX_VK);
#endif
        sprintf(buf, "cat ./lib/messaggi/valmail1 > %s", tmpf);
        system(buf);
	tf = fopen(tmpf, "a");
        fprintf(tf, "%s\n", valkey);
        fclose(tf);
        sprintf(buf, "cat %s ./lib/messaggi/valmail2 > %s", tmpf, tmpf2);
        system(buf);
        sprintf(buf, "mail -s 'Cittadella Validation Key' %s < %s\n",
                addr, tmpf2);
        if (system(buf) == -1) {
                sprintf(buf, "SYSERR: Valkey for %s not sent.", addr);
                citta_log(buf);
        }
        unlink(tmpf);
        unlink(tmpf2);
        Free(tmpf);
        Free(tmpf2);
}   

/*
 * Elimina gli utenti di livello 0 e gli utenti che non si sono
 * validati entro il tempo concesso per la validazione.
 */
void purge_invalid (void)
{
        struct lista_ut *precedente, *punto, *prossimo;
        char nome[128];
        long tempo;

        tempo = time(0);
        /* Se e' il primo utente della lista si tratta a parte */
        /* Questo caso non dovrebbe verificarsi mai */
        if ((lista_utenti->dati->livello <= LVL_DA_BUTTARE) ||
            ((lista_utenti->dati->livello <= LVL_NON_VALIDATO) &&
             (tempo - lista_utenti->dati->firstcall > TEMPO_VALIDAZIONE))) {
                strcpy(nome, lista_utenti->dati->nome);
                /*    cmd_cusr(t, nome, 0);  */
                prossimo = lista_utenti->prossimo;
                Free(lista_utenti->dati);
                Free(lista_utenti);
                lista_utenti = prossimo;
                citta_logf("KILL: [%s] eliminato per mancata validazione.", nome);
        }
        /* vede se e' uno dei successivi */
        precedente = lista_utenti;
        for (punto = lista_utenti->prossimo; punto; punto = prossimo) {
                prossimo = punto->prossimo;
                if ( (punto->dati->livello <= LVL_DA_BUTTARE) ||
                     ( (punto->dati->livello <= LVL_NON_VALIDATO) &&
                       (tempo - punto->dati->firstcall > TEMPO_VALIDAZIONE))) {
                        strcpy(nome, punto->dati->nome);
                        /*      cmd_cusr(t, nome, 0); */
                        if (prossimo == NULL) {
                                /* Se e' l'ultimo utente della lista */
                                precedente->prossimo = NULL;
                                ultimo_utente = precedente;
                        } else {
                                /* Se e' in mezzo alla lista */
                                precedente->prossimo = prossimo;
                                prossimo = NULL;
                        }
			Free(punto->dati);
			Free(punto);
			citta_logf("KILL: [%s] eliminato per mancata validazione.",
			     nome);
                }
                precedente = precedente->prossimo;
        }
}
#endif

/*
 * verifica la password
 */
char check_password(char *pwd, char *real_pwd)
{
#ifdef CRYPT
        char *result;

        /* Read in the user's password and encrypt it,
           passing the expected password in as the salt. */
        result = crypt(pwd, real_pwd);

        if (!strcmp(result, real_pwd))
                return 1;
        else
                return 0;
#else
        if (!strcmp(pwd, real_pwd))
                return 1;
        else
                return 0;
#endif
}

/*
 * Cripta una parola
 */
void cripta(char *pwd)
{
#ifdef CRYPT
        unsigned long seed[2];
        char salt[] = "$1$........";
        const char *const seedchars =
                "./0123456789ABCDEFGHIJKLMNOPQRST"
                "UVWXYZabcdefghijklmnopqrstuvwxyz";
        int i;

        /* Generate a (not very) random seed.
           You should do it better than this... */
        seed[0] = time(NULL);
        seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);

        /* Turn it into printable characters from `seedchars'. */
        for (i = 0; i < 8; i++)
                salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

        /* Encrypt the password */
        pwd = crypt(pwd, salt);
#else
	IGNORE_UNUSED_PARAMETER(pwd);
        return;
#endif
}

/*
 * Restituisce true se *amico e' nella Friendlist di *utente, altrimenti false.
 */
bool is_friend(struct dati_ut *amico, struct dati_ut *utente)
{
        int i;
        long matr;

        matr = amico->matricola;
        for (i = 0; i < NFRIENDS; i++)
                if (utente->friends[i] == matr)
                        return true;
        return false;
}

/*
 * Restituisce true se *nemico e' nella Enemylist di *utente, altrimenti false.
 */
bool is_enemy(struct dati_ut *nemico, struct dati_ut *utente)
{
        int i;
        long matr;

        matr = nemico->matricola;
        for (i = NFRIENDS; i < 2*NFRIENDS; i++)
                if (utente->friends[i] == matr)
                        return true;
        return false;
}

/*
 * Setta il flag 'flag' di sflag[num] per tutti gli utenti.
 */
void sut_set_all(int num, char flag)
{
        struct lista_ut *punto;

        for (punto = lista_utenti; punto; punto = punto->prossimo)
		punto->dati->sflags[num] |= flag;
}

/***************************************************************************/
