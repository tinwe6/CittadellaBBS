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
* File: utente_cmd.h                                                        *
*       comandi del server per la gestione degli utenti.                    *
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
#include "blog.h"
#include "email.h"
#include "extract.h"
#include "fileserver.h"
#include "generic.h"
#include "mail.h"
#include "memstat.h"
#include "socket_citta.h"
#include "utente.h"
#include "utente_cmd.h"
#include "string_utils.h"
#include "ut_rooms.h"
#include "utility.h"

#ifdef USE_POP3
#include "pop3.h"
#endif

/* Prototipi delle funzioni in questo file */
void cmd_user(struct sessione *t, char *nome);
void cmd_usr1(struct sessione *t, char *nome);
void cmd_chek(struct sessione *t);
void cmd_quit(struct sessione *t);
void cmd_prfl(struct sessione *t, char *nome);
void cmd_rusr(struct sessione *t);
void cmd_rgst(struct sessione *t, char *buf);
void cmd_breg(struct sessione *t);
void cmd_greg(struct sessione *t);
void cmd_gcst(struct sessione *t, char *buf);
char cmd_cusr(struct sessione *t, char *nome, char notifica);
void cmd_kusr(struct sessione *t, char *nome);
void cmd_eusr(struct sessione *t, char *buf);
void cmd_gusr(struct sessione *t, char *nome);
#ifdef USE_VALIDATION_KEY
void cmd_aval(struct sessione *t, char *vk);
void cmd_gval(struct sessione *t);
#endif
void cmd_pwdc(struct sessione *t, char *pwd);
void cmd_pwdn(struct sessione *t, char *buf);
void cmd_pwdu(struct sessione *t, char *buf);
void cmd_prfg(struct sessione *t, char *nome);
void cmd_cfgg(struct sessione *t, char *cmd);
void cmd_cfgp(struct sessione *t, char *arg);
void cmd_frdg(struct sessione *t);
void cmd_frdp(struct sessione *t, char *arg);
void cmd_gmtr(struct sessione *t, char *nome);
void notify_logout(struct sessione *t, int tipo);
void stats_new_login(int logged_users);
void stats_new_guest(void);
void stats_new_user(void);
void stats_new_validation(void);

/******************************************************************************
 ******************************************************************************/
/*
 * Procedura di Login, passo 1.
 * Il client comunica al server il nome dell'utente.
 * Sintassi : "USER nome"
 * Risponde : "OK already_conn|is_guest|is_new_usr|is_first_usr|is_validated"
 *   oppure ERROR se nome e' "" o se la sessione ha gia' eseguito un login
 *
 * For compatibility with protocol pre 0.4.5, we send as OK a full code that
 * contains user info (guest, new, not validated, validated). In future
 * releases we'll simply send the OK code '200'.
 */
void cmd_user(struct sessione *t, char *nome)
{
        struct dati_ut *utente;
        bool has_other_connection = false;
        bool is_guest = false;
        bool is_new_user = false;
        bool is_first_user = false;
        bool is_validated = false;
        int code;

        if (t->logged_in || (strlen(nome) == 0)) {
                cprintf(t, "%d\n", ERROR);
                return;
        }

        code = OK;
        if(!strcmp(nome, "Ospite") || !strcmp(nome, "Guest")) {
                is_guest = true;
                code = UT_OSPITE;
        } else {
                /* Ci si mette subito in stato occupato altrimenti
                 * potrebbe e' possibile saltare la registrazione */
                t->occupato = 1;
                /* E' un utente della BBS? */
                utente = trova_utente(nome);
                if (utente == NULL) {
                        /* Abbiamo un nuovo utente! */
                        is_new_user = true;
                        code = NUOVO_UTENTE;
                        is_first_user = (ultimo_utente == NULL);
                } else {
#ifdef USE_VALIDATION_KEY
                        is_validated = (utente->val_key[0] == 0);
                        code = is_validated ? UT_VALIDATO : NON_VALIDATO;
#else
                        utente->val_key[0] = 0;
                        is_validated = true;
                        code = UT_VALIDATO;
#endif
                        /* L'utente e' gia' collegato? */
                        has_other_connection = (collegato(nome) != NULL);
                }
        }
        cprintf(t, "%d %d|%d|%d|%d|%d\n", code, has_other_connection,
                is_guest, is_new_user, is_first_user, is_validated);
}

/*
 * Procedura di Login, Step 2.
 * Verifica la password e associa alla sessione la struct utente.
 * (o la crea se necessario). Viene anche caricata la struttura UTR.
 * Sintassi : "USR1 nome|password"
 * Risponde : "OK messaggio|is_first_user|terms_accepted|is_registered"
 *          o "ERROR+PASSWORD messaggio" se password e' errata
 */
void cmd_usr1(struct sessione *t, char *arg)
{
        char passwd[MAXLEN_PASSWORD], nome[MAXLEN_UTNAME];
        bool wrong_pwd = false;
        bool is_guest = false;
        bool is_new_user = false;
        bool is_first_user = false;
        bool terms_accepted = false;
        struct sessione  *s;
        struct lista_ut *punto = NULL;
        struct dati_ut *utente;

        if (t->logged_in) {
                cprintf(t, "%d Gia' eseguito il login.\n", ERROR);
                return;
        }

        extractn(nome, arg, 0, MAXLEN_UTNAME);

        if (!strcmp(nome, "Ospite") || !strcmp(nome, "Guest")) {
                is_guest = true;
                utente = du_guest();
                t->logged_in = true;
                t->stato = CON_COMANDI;
                t->occupato = 0;
                stats_new_guest();
        } else {
                extractn(passwd, arg, 1, MAXLEN_PASSWORD);
                utente = trova_utente(nome);
                if (utente == NULL) {   /* E' un utente della BBS? */
                        /* Abbiamo un nuovo utente!                */
                        is_new_user = true;
                        citta_logf("Nuovo utente [%s].", nome);
                        /* TODO Controllo Badnick.                 */
                        CREATE(punto, struct lista_ut, 1, TYPE_LISTA_UT);
                        punto->prossimo = NULL;
                        if (ultimo_utente == NULL) {
                                /* E' il primo utente della BBS!!! */
                                is_first_user = true;
                                dati_server.matricola = 0;
                                lista_utenti = punto;
                        } else {
                                ultimo_utente->prossimo = punto;
                        }
                        ultimo_utente = punto;

                        cripta(passwd);

                        utente = du_new_user(nome, passwd, is_first_user);
                        punto->dati = utente;

                        /* Go straight to CON_CONSENT otherwise the user     *
                         * could skip the consent form and the registration. */
                        t->stato = CON_CONSENT;
                        t->occupato = 1;
                        stats_new_user();
                } else if (utente->livello != LVL_OSPITE) {
                        /* Verifica la Password */
                        if (check_password(passwd, utente->password)) {
                                /* Killa l'alter ego se presente */
                                if (( (s = collegato(nome)) != NULL)
                                    && (s != t)) {
                                        s->cancella = true;
                                }
#ifdef USE_POP3
                                /* Elimina la sessione POP3 se presente */
                                pop3_kill_user(nome);
#endif
                        } else {
                                wrong_pwd = true; /* password errata! */
                        }
                }
        }

        if (wrong_pwd) {
                cprintf(t, "%d Password errata.\n", ERROR+PASSWORD);
                citta_logf("SECURE: Pwd errata per [%s] da [%s]", nome,
                           t->host);
                t->stato = CON_LIC;
        } else { /* Password corretta */
                int code = OK;
                /* Linkiamo la struttura dati dell'utente alla sua sessione */
                t->utente = utente;
                utr_load(t);
                mail_load(t);
                (utente->chiamate)++;
                citta_logf("Login di [%s] da [%s].", utente->nome, t->host);

                if (!is_guest) {
                        terms_accepted = has_accepted_terms(utente);
                        if  (terms_accepted && utente->registrato) {
                                /* Login completed */
                                t->stato = CON_COMANDI;
                                t->occupato = 0;
                                assert(!is_new_user);
                        } else if  (terms_accepted) {
                                /* Registration has not been completed yet */
                                t->stato = CON_REG;
                                t->occupato = 1;
                                assert(!is_new_user);
                        } else {
                                /* The user must accept the terms first */
                                t->stato = CON_CONSENT;
                                t->occupato = 1;
                        }
                }

                /* TODO: this is for compatibility only, send simply OK     */
                if (is_first_user) {
                        code = OK + PRIMO_UT;
                }
                cprintf(t, "%d Login eseguito.|%d|%d|%d\n", code,
                        is_first_user, terms_accepted, t->utente->registrato);

                /* Update num connected users and server statistics */
                logged_users++;
                stats_new_login(logged_users);
        }
}


/*
 * Completa la procedura di login: notifica l'entrata e invia dati
 * sull'utente.
 */
void cmd_chek(struct sessione *t)
{
        char buf[LBUF];
        size_t buflen;
        struct sessione *punto;
        struct dati_ut *ut;
        int ut_conn = 0;

        if ((t->utente == NULL) || (t->utente->livello <= LVL_OSPITE)) {
                cprintf(t, "%d\n", ERROR);
                return;
        }
        ut = t->utente;

        /* Notifica a tutti che l'utente e' entrato (oh.. che bello!) */
        sprintf(buf, "%d %c%s\n", LGIN, SESSO(ut), ut->nome);
        buflen = strlen(buf);
        for (punto = lista_sessioni; punto; punto = punto->prossima) {
                ut_conn++;
                if ((punto->utente) && (punto != t) &&
                    (punto->utente->flags[3] & UT_LION)) {
                        metti_in_coda(&punto->output, buf, buflen);
                        punto->bytes_out += buflen;
                }
        }

        /* Cambiare, completare... listing follows, etc... */
        t->logged_in = true;
        t->stato = CON_COMANDI;
        t->occupato = 0;
        cprintf(t, "%d %d|%ld|%d|%d|%ld|%s|%d\n", OK, ut_conn, ut->matricola,
                ut->livello, ut->chiamate, ut->lastcall, ut->lasthost,
                mail_new(t));
        t->utente->lastcall = t->ora_login;
        t->utente->sflags[0] &= ~SUT_NEWS;
}

/*
 * Logout dell'utente
 */
void cmd_quit(struct sessione *t)
{
        time_t time_online;

        notify_logout(t, LGOT);
        t->stato = CON_CHIUSA;
        /* Aggiorna ultima chiamata, ultimo host e tempo online */
        /*        strcpy(t->utente->lasthost, t->host); */
        time_online = (time(0) - t->ora_login);
        cprintf(t, "%d %ld|%ld|%ld|%ld\n", OK, t->num_cmd, t->bytes_in,
                t->bytes_out, time_online);
        if ((t->utente) && (!strcmp(t->utente->nome, "Ospite")
                            || !strcmp(t->utente->nome, "Guest")))
                Free(t->utente);
}

/*
 * Spedisce al client il profile dell'utente richiesto
 */
void cmd_prfl(struct sessione *t, char *nome)
{
        struct dati_ut *utente;
        struct sessione *sess;
        struct room *r;
        char buf[LBUF], buf1[LBUF];
        char connesso = 0;

        utente = trova_utente(nome);
        if (utente == NULL) {
                cprintf(t, "%d %s\n", ERROR+NO_SUCH_USER, nome);
                return;
        }

        /* Verifica se l'utente e' connesso */
        sess = collegato(nome);
        if (sess != NULL) connesso = 1;

        cprintf(t, "%d\n", SEGUE_LISTA);
        cprintf(t, "%d %s|%d|%d\n", OK, utente->nome,utente->livello,connesso);

        /* 1 - dati privati */
        if ((t->utente == utente) || ((utente->flags[2] & UT_VFRIEND) &&
                                      is_friend(t->utente, utente)))
                sprintf(buf, "%d 1|%s|%s|%s|%s|%s|%s|%s|%s|%c\n", OK,
                        utente->nome_reale, utente->via, utente->cap,
                        utente->citta, utente->stato, utente->tel,
                        utente->email, utente->url,
                        (utente->sflags[0] & SUT_SEX) ? 'F' : 'M');
        else {
                sprintf(buf, "%d 1|", OK);
                if (utente->flags[2] & UT_VNOME)
                        strcat(buf, utente->nome_reale);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VADDR)
                        strcat(buf, utente->via);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VADDR)
                        strcat(buf, utente->cap);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VADDR)
                        strcat(buf, utente->citta);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VADDR)
                        strcat(buf, utente->stato);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VTEL)
                        strcat(buf, utente->tel);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VEMAIL)
                        strcat(buf, utente->email);
                strcat(buf, "|");
                if (utente->flags[2] & UT_VURL)
                        strcat(buf, utente->url);
                if (utente->flags[2] & UT_VSEX)
                        sprintf(buf1, "|%c\n",
                                (utente->sflags[0] & SUT_SEX) ? 'F' : 'M');
                else
                        sprintf(buf1, "|X\n");
                strcat(buf, buf1);
        }
        cprintf(t, "%s", buf);

        /* 2 - dati pubblici */
        cprintf(t, "%d 2|%ld|%d|%d|%d|%ld|%ld|%ld|%s|%ld|%d|%d|%d\n", OK,
                utente->matricola, utente->chiamate, utente->post,
                utente->x_msg, utente->firstcall, utente->time_online,
                utente->lastcall, utente->lasthost, (sess ? sess->ora_login:0),
                utente->mail, utente->chat, (utente->flags[7] & UT_BLOG_ON)
                ? 1 : 0);

        /* 6 - Dati upload/download file */
        fs_send_ustats(t, utente->matricola);

        /* 3 - Room Aide in...   */
        cprintf(t, "%d 3\n", SEGUE_LISTA);
        for (r = lista_room.first; r; r = r->next)
                /* Non e' efficiente: cambiare!!! */
                if (utr_getf(utente->matricola, r->pos) & UTR_ROOMAIDE)
                        cprintf(t, "%d %s\n", OK, r->data->name);
        cprintf(t, "000\n");

        /* 4 - Room Helper in... */
        cprintf(t, "%d 4\n", SEGUE_LISTA);
        for (r = lista_room.first; r; r = r->next)
                if (utr_getf(utente->matricola, r->pos) & UTR_ROOMHELPER)
                        cprintf(t, "%d %s\n", OK, r->data->name);
        cprintf(t, "000\n");

        /* 5 - Dati connessione attuale */
        if (connesso)
                cprintf(t, "%d 5|%s|%ld|%d|%d|%d|%d|%d\n", OK, sess->host,
                        sess->ora_login, sess->occupato, sess->idle.cicli,
                        sess->idle.min, sess->idle.ore,
                        (sess->stato == CON_LOCK) ? 1 : 0);

        cprintf(t, "000\n");
}

/*
 * Lista degli utenti della BBS
 */
void cmd_rusr(struct sessione *t)
{
        struct lista_ut *punto;

        cprintf(t, "%d Lista utenti (RUSR)\n", SEGUE_LISTA);
        for (punto = lista_utenti; punto; punto = punto->prossimo)
                cprintf(t, "%d %s|%d|%d|%ld|%d|%d|%d|%d\n", OK,
                        punto->dati->nome, punto->dati->chiamate,
                        punto->dati->post, punto->dati->lastcall,
                        punto->dati->livello, punto->dati->x_msg,
                        punto->dati->mail, punto->dati->chat);
        cprintf(t, "000\n");
}


/*
 * Receives the registrarion data from the client, validates it and stores it
 * in the user data structure. The connection must be in the CON_REG state,
 * and is set to CON_COMANDI unless the user is not successfully registered
 * at the end of the process, in which case it stays in state CON_REG.
 * Syntax: "RGST mode|real_name|street|town|country|cap|tel|email|url|sex"
 *         mode = 0 : do not preserve the change, 1 : perform the changes
 * Mode 0 is necessary for registered users that edit their registration and
 * do not want to keep the changes.
 * Returns: "OK" if the operation is successfull,
 *          "ERROR msg|name|email|taken|reg" where 'name' / 'email' is true
 *          if the name / email is not valid, 'taken' is true if the email is
 *          already in use by another user (to avoid double accounts), 'reg'
 *          is true if the user is currently registered.
 * NOTE: the condition that the connection is in state CON_REG is not really
 * necessary, and by removing it we could eliminate the need for command BREG.
 */
void cmd_rgst(struct sessione *t, char *buf)
{
        bool email_taken = false;

        if (t->stato != CON_REG) {
                cprintf(t, "%d wrong state||||\n", ERROR+WRONG_STATE);
                return;
        }

        bool save_data = extract_bool(buf, 0);
        if (!save_data) {
                if (!t->utente->registrato) {
                        /* stay in CON_REG state, the user must register */
                        cprintf(t, "%d data expected||||0\n", ERROR);
                } else {
                        t->occupato = 0;
                        t->stato = CON_COMANDI;
                        cprintf(t, "%d\n", OK);
                }
                return;
        }

        char real_name[MAXLEN_RNAME + 1];
        char email[MAXLEN_EMAIL + 1];
        extractn(real_name, buf, 1, MAXLEN_RNAME);
        extractn(email, buf, 7, MAXLEN_EMAIL);
        bool valid_name = is_valid_full_name(real_name);
        bool valid_email = is_valid_email(email);

        if (!strcmp(email, t->utente->email)) {
#ifdef USE_VALIDATION_KEY
                /* TODO The email chaged: generate new validation key. */
#endif

#ifdef NO_DOUBLE_EMAIL
                /*
                  TODO eliminate old email from the list of used emails or
                  else changing from email A to B prevents to go back to A

                  TODO the check should be perfored always, not just for new
                  users.
                */
                if (!t->utente->registrato && !check_double_email(t->utente->email)) {
                        citta_logf("SECURE user [%s] is reusing email [%s]",
                                   t->utente->nome, t->utente->email);
                        valid_email = false;
                        email_taken = true;
                }
#endif /*NO_DOUBLE_EMAIL*/
        }

        if (valid_name && valid_email) {
                strncpy(t->utente->nome_reale, real_name, MAXLEN_RNAME);
                strncpy(t->utente->email, email, MAXLEN_EMAIL);
                extractn(t->utente->via, buf, 2, MAXLEN_VIA);
                extractn(t->utente->citta, buf, 3, MAXLEN_CITTA);
                extractn(t->utente->stato, buf, 4, MAXLEN_STATO);
                extractn(t->utente->cap, buf, 5, MAXLEN_CAP);
                extractn(t->utente->tel, buf, 6, MAXLEN_TEL);
                extractn(t->utente->url, buf, 8, MAXLEN_URL);
                bool sex = extract_bool(buf, 9);
                if (sex) {
                        t->utente->sflags[0] |= SUT_SEX;
                } else {
                        t->utente->sflags[0] &= ~SUT_SEX;
                }

                t->utente->registrato = true;
                t->occupato = 0;
                t->stato = CON_COMANDI;
                cprintf(t, "%d\n", OK);
        } else {
                if (t->utente->registrato) {
                        t->occupato = 0;
                        t->stato = CON_COMANDI;
                }
                cprintf(t, "%d invalid data|%d|%d|%d|%d\n", ERROR, !valid_name,
                        !valid_email, email_taken, t->utente->registrato);
        }
}


/*
 * Mette l'utente nello stato di registrazione
 */
void cmd_breg(struct sessione *t)
{
        t->stato = CON_REG;
        t->occupato = 1;
        cprintf(t, "%d\n", OK);
}

/*
 * Get registration: spedisce la registrazione al client
 */
void cmd_greg(struct sessione *t)
{
        struct dati_ut *ut;

        ut = t->utente;
        cprintf(t, "%d %s|%s|%s|%s|%s|%s|%s|%s|%d\n", OK, ut->nome_reale,
                ut->via, ut->citta, ut->cap, ut->stato, ut->tel, ut->email,
                ut->url, (ut->sflags[0]) & SUT_SEX);
}

/*
 * Give ConSenT: Gives (or revokes) the user consent for personal data
 *               processing.
 * The user must be in CON_REG or CON_COMANDI state.
 * Syntax: "GCST 0" revokes consent, "GCST 1" gives consent.
 * Return: "OK need_reg" where need_reg is 1 if the user must do the
 *         registration (and thus the connection placed in CON_REG state)
 */
void cmd_gcst(struct sessione *t, char *buf)
{
        struct room *room;
        struct text *txt;
        bool user_has_given_consent = extract_bool(buf, 0);
        bool user_needs_registration = false;

        t->utente->sflags[0] &= ~SUT_NEED_CONSENT;
        if (user_has_given_consent) {
                t->utente->sflags[0] |= SUT_CONSENT;
                citta_logf("Data protection: User [%s] accepted the terms.",
                           t->utente->nome);

                if (has_requested_deletion(t->utente)) {
                        t->utente->sflags[0] &= ~SUT_DELETEME;
                        if ( (room = room_find(sysop_room))) {
                                txt = txt_create();
                                txt_putf(txt,
"L'utente [%s] ha deciso di accettare le condizioni sul"
                                         , t->utente->nome);
                                txt_putf(txt,
"trattamento dei dati e di mantenere il proprio account."
                                         );
                                txt_putf(txt,
"*** <b>NON</b> cancellare l'account di [%s] ***"
                                         , t->utente->nome);
                                citta_post(room,
                                           "Revoca richiesta di cancellazione",
                                           txt);
                                txt_free(&txt);
                        }
                }

                /* we can proceed to the next step */
                if (t->utente->registrato) {
                        t->stato = CON_COMANDI;
                } else {
                        /* it's a new user, still need to register */
                        user_needs_registration = true;
                        t->stato = CON_REG;
                }
        } else if (!has_requested_deletion(t->utente)) {
                t->utente->sflags[0] &= ~SUT_CONSENT;
                t->utente->sflags[0] |= SUT_DELETEME;
                citta_logf("Data protection: user [%s] revoked the terms.",
                           t->utente->nome);

                if ( (room = room_find(sysop_room))) {
                        txt = txt_create();
                        txt_putf(txt,
"L'utente [%s] non ha accettato le condizione sul"
                                 , t->utente->nome);
                        txt_putf(txt,
"trattamento dei dati e percio' il suo account e tutti i suoi dati"
                                 );
                        txt_putf(txt,
"personali vanno cancellati entro 48 ore."
                                 );
                        citta_post(room, "GDPR: Richiesta di cancellazione",
                                   txt);
                        txt_free(&txt);
                }
        } else {
                /* the user was already marked for deletion, *
                 * no need to notify it again...             */
        }

        cprintf(t, "%d %d\n", OK, user_needs_registration);
}

/*
 * Caccia User: Comando per cacciare un utente connesso.
 * Se l'utente non e' connesso restituisce 0, se lo sconnette 1.
 * Se notifica==1 risponde al client, altrimenti no.
 */
char cmd_cusr(struct sessione *t, char *nome, char notifica)
{
        struct sessione *punto, *prossima;
        char buf[128];
        char ok;

        ok = 0;
        if ((t->utente == NULL) || t->utente->livello < MINLVL_KICKUSR) {
                if (notifica) {
                        cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
                        if (t->utente)
                                citta_logf(
"SECURE: CUSR non autorizzato di [%s] da [%s].",
                                           t->utente->nome, t->host);
                }
                return ok;
        }

        for (punto = lista_sessioni; punto; punto = prossima) {
                prossima = punto->prossima;
                if ((punto->utente) && (!strcmp(punto->utente->nome, nome))) {
                        /* Notifica all'utente che e' stato cacciato da */
                        sprintf(buf, "%d %s\n", KOUT, t->utente->nome);
                        scrivi_a_client(punto, buf);
                        /* Caccia l'utente */
                        cmd_quit(punto);
                        punto->utente = NULL;
                        ok = 1;
                }
        }
        if (notifica) {
                cprintf(t, "%d %d\n", OK, ok);
                if (ok)
                        citta_logf("KO: [%s] cacciato da [%s].", nome,
                                   t->utente->nome);
        }
        return(ok);
}

/*
 * Kill User: Comando per eliminare la struttura utente di 'nome'
 * Restituisce 0 se l'utente non esiste e 1 se esiste e viene eliminato.
 * Nota: tutta la lista viene sempre esaminato, nel caso ci fossero
 * dei cloni (ma non dovrebbero esserci, no?)
 */
void cmd_kusr(struct sessione *t, char *nome)
{
        struct lista_ut *precedente, *punto, *prossimo;
        char ok = 0;

        /* Anzitutto se l'utente e' connesso lo caccia */
        cmd_cusr(t, nome, 0);

        /* Elimina l'utente */
        /* Se e' il primo utente della lista si tratta a parte */
        if (!strcmp(lista_utenti->dati->nome, nome)) {
                prossimo = lista_utenti->prossimo;
                Free(lista_utenti->dati);
                Free(lista_utenti);
                lista_utenti = prossimo;
                ok = 1;
        }
        /* vede se e' uno dei successivi */
        precedente = lista_utenti;
        for (punto = lista_utenti->prossimo; punto; punto = prossimo) {
                prossimo = punto->prossimo;
                if (!(strcmp(punto->dati->nome, nome))) {
                        if (prossimo == NULL) {
                                /* Se e' l'ultimo utente della lista */
                                precedente->prossimo = NULL;
                                ultimo_utente = precedente;
                                Free(punto->dati);
                                Free(punto);
                                ok = 1;
                        } else {
                                /* Se e' in mezzo alla lista */
                                precedente->prossimo = prossimo;
                                Free(punto->dati);
                                Free(punto);
                                prossimo = NULL;
                                ok = 1;
                        }
                }
                precedente = precedente->prossimo;
        }
        if (ok)
                citta_logf("KILL: [%s] e' stato eliminato da [%s].", nome,
                           t->utente->nome);

        /* Notifica il risultato al client */
        cprintf(t, "%d %d\n", OK, ok);
}

/*
 * Edit User: modifica i dati di un utente (per aide)
 */
void cmd_eusr(struct sessione *t, char *buf)
{
        char nome[MAXLEN_UTNAME];
        char newnick[MAXLEN_UTNAME];
        int lvl, spp;
        struct dati_ut *utente;
        struct text *txt;
        struct room *room;
        struct sessione *s;

        extractn(nome, buf, 0, MAXLEN_UTNAME);
        utente = trova_utente(nome);
        if (utente == NULL)
                cprintf(t, "%d No '%s' user.\n", ERROR+NO_SUCH_USER, nome);
        else if (utente->livello > t->utente->livello)
                cprintf(t, "%d\n", ERROR+ACC_PROIBITO);
        else {
                lvl = extract_int(buf, 9);
                if (lvl > t->utente->livello) {
                        cprintf(t, "%d\n", ERROR+ARG_NON_VAL);
                } else {
                        extractn(utente->nome_reale, buf, 1, MAXLEN_RNAME);
                        extractn(utente->via, buf, 2, MAXLEN_VIA);
                        extractn(utente->citta, buf, 3, MAXLEN_CITTA);
                        extractn(utente->stato, buf, 4, MAXLEN_STATO);
                        extractn(utente->cap, buf, 5, MAXLEN_CAP);
                        extractn(utente->tel, buf, 6, MAXLEN_TEL);
                        extractn(utente->email, buf, 7, MAXLEN_EMAIL);
                        extractn(utente->url, buf, 8, MAXLEN_URL);
                        spp = extract_int(buf, 11);
                        utente->sflags[1] = extract_int(buf, 12);
                        if (spp > 255)
                                spp = 255;
                        utente->secondi_per_post = spp;
                        utente->livello = lvl;
                        if (extract_int(buf, 10) && (utente->val_key[0])) {
                                utente->val_key[0] = 0;
                                if (utente->livello <= LIVELLO_INIZIALE)
                                        utente->livello = LIVELLO_VALIDATO;
                                citta_logf("VALIDATE: validazione di [%s].",
                                           utente->nome);
                                /* Notifica all'utente interessato... */
                                stats_new_validation();
                        }
                        extractn(newnick, buf, 13, MAXLEN_UTNAME);
                        if (*newnick && strncmp(nome, newnick, MAXLEN_UTNAME)
                            && (trova_utente(newnick) == NULL)) {
                                citta_logf("Cambiamento nick [%s] --> [%s]",
                                           nome, newnick);
                                if ( (room = room_find(lobby))) {
                                        txt = txt_create();
                                        txt_putf(txt,
"L'utente [%s] ha cambiato nome, e si collegher&agrave;",
                                                 nome);
                                        txt_putf(txt,
"d'ora in avanti col nuovo nome <b>%s</b>.",
                                                 newnick);
                                        citta_post(room,
                                            "Un utente ha cambiato nickname!",
                                                   txt);
                                        txt_free(&txt);
                                }
                                txt = txt_create();
                                txt_putf(txt,
                                         "Il nickname del tuo utente [%s]",
                                         nome);
                                txt_putf(txt, "e` stato cambiato in [%s].",
                                         newnick);
                                if (!send_email(txt, NULL,
"Il tuo nickname e` stato modificato!",
                                                utente->email, EMAIL_SINGLE))
                                        citta_logf(
"Email di notifica %s -> %s non inviato.",
                                                   nome, newnick);
                                txt_free(&txt);
                                blog_newnick(utente, newnick);
                                strncpy(utente->nome, newnick, MAXLEN_UTNAME);
                        }
                        /* Chiede al client di aggiornare i flags */
                        if ( (s = collegato_n(utente->matricola)))
                                cprintf(s, "%d\n", RFLG);

                        cprintf(t, "%d %s\n", OK, nome);
                }
        }
        t->occupato = 0;
        t->stato = CON_COMANDI;
}

/*
 * Get User: spedisce i dati su un utente al client
 * NOTA: Ci saranno molti altri dati da aggiungere
 */
void cmd_gusr(struct sessione *t, char *nome)
{
        struct dati_ut *ut;

        t->occupato = 1;
        t->stato = CON_EUSR;
        ut = trova_utente(nome);
        if (ut == NULL) {
                cprintf(t, "%d l'utente %s non esiste.\n", ERROR+NO_SUCH_USER,
                        nome);
                t->occupato = 0;
                t->stato = CON_COMANDI;
                return;
        }
        cprintf(t, "%d %s|%s|%s|%s|%s|%s|%s|%s|%d|%s|%d|%d\n", OK,
                ut->nome_reale, ut->via, ut->citta, ut->cap, ut->stato,
                ut->tel, ut->email, ut->url, ut->livello, ut->val_key,
                ut->secondi_per_post, ut->sflags[1]);
}

/*
 * Processa la richiesta di autovalidazione da parte di un utente
 */
#ifdef USE_VALIDATION_KEY
void cmd_aval(struct sessione *t, char *vk)
{
        if (!strcmp(t->utente->val_key, vk)) {
                /* Impedisce di entrare a ospite con livello */
                /* superiore modificando il client */
                /* Validation key corretta */
                t->utente->val_key[0] = 0;
                if (t->utente->livello <= LIVELLO_INIZIALE)
                        t->utente->livello = LIVELLO_VALIDATO;
                citta_logf("VALIDATE: Auto-validazione di [%s].",
                           t->utente->nome);
                cprintf(t, "%d\n", OK);
                stats_new_validation();
        } else
                cprintf(t, "%d\n", ERROR);
}
#endif

/*
 * Genera e invia una chiave di validazione per l'utente
 */
void cmd_gval(struct sessione *t)
{
#ifdef USE_VALIDATION_KEY
        char valkey[VALKEYLEN+1];

        if (t->utente->val_key[0] != 0) {
                /* genera chiave validazione */
                gen_rand_string(valkey, VALKEYLEN);
                strcpy(t->utente->val_key, valkey);
                /* questa puo' servire se il mail non arriva: */
                citta_logf("VALIDATE: Valkey [%s] per [%s].",
                           t->utente->val_key, t->utente->nome);
                /* Invia la val_key per posta elettronica */
                invia_val_key(valkey, t->utente->email);
                cprintf(t, "%d\n", OK);
        } else { /* Prima l'utente si deve registrare! */
                cprintf(t, "%d Utente gia' validato.\n", ERROR);
                citta_logf("SECURE: Richiesta valkey da [%s] validato.",
                           t->utente->nome);
        }
#else
        t->utente->val_key[0] = 0;
        cprintf(t, "%d\n", OK);
#endif
}

/*
 * Verifica la password inviata dall'utente
 */
void cmd_pwdc(struct sessione *t, char *pwd)
{
        if ((t->utente == NULL) || (t->utente->livello == LVL_OSPITE)) {
                cprintf(t, "%d\n", ERROR);
                return;
        }
        if (check_password(pwd, t->utente->password))
                cprintf(t, "%d\n", OK);
        else {
                citta_logf("SECURE: PWDC, Pwd errata per [%s] da [%s]",
                           t->utente->nome, t->host);
                cprintf(t, "%d\n", ERROR);
        }
}

/*
 * Modifica la password
 */
void cmd_pwdn(struct sessione *t, char *buf)
{
        char oldpwd[MAXLEN_PASSWORD], newpwd[MAXLEN_PASSWORD];

        extractn(oldpwd, buf, 0, MAXLEN_PASSWORD);
        extractn(newpwd, buf, 1, MAXLEN_PASSWORD);
        if (check_password(oldpwd, t->utente->password)) {
                cripta(newpwd);
                strcpy(t->utente->password, newpwd);
                cprintf(t, "%d\n", OK);
        } else {
                citta_logf("SECURE: [%s] PWDN fallito.", t->utente->nome);
                cprintf(t, "%d\n", ERROR+PASSWORD);
        }
}

/* Modifica la password di un utente */
void cmd_pwdu(struct sessione *t, char *buf)
{
        char pwd[MAXLEN_PASSWORD], newpwd[MAXLEN_PASSWORD],
                nome[MAXLEN_UTNAME], buf1[LBUF];
        struct dati_ut *utente;
        char *tmpf, *tmpf2;
#ifdef MKSTEMP
        int fd1, fd2;
#endif
        FILE *tf;

        extractn(pwd, buf, 0, MAXLEN_PASSWORD);
        extractn(nome, buf, 1, MAXLEN_UTNAME);
        utente = trova_utente(nome);
        if (utente == NULL) {
                cprintf(t, "%d L'utente [%s] non esiste.\n",
                        ERROR+NO_SUCH_USER, nome);
                return;
        }
        if (check_password(pwd, t->utente->password)) {
                /* Genera la nuova password di 8 caratteri */
                gen_rand_string(newpwd, 8);
                /* Invia la nuova password all'utente via Email */
#ifdef MKSTEMP
                tmpf = Strdup(TEMP_PWDU_TEMPLATE);
                tmpf2 = Strdup(TEMP_PWDU_TEMPLATE);

                fd1 = mkstemp(tmpf);
                fd2 = mkstemp(tmpf2);
                close(fd1);
                close(fd2);
#else
                tmpf = tempnam(TMPDIR, PFX_PWDU);
                tmpf2 = tempnam(TMPDIR, PFX_PWDU);
#endif
                sprintf(buf1, "cat ./lib/messaggi/pwdmail > %s", tmpf);
                system(buf1);
                tf = fopen(tmpf, "a");
                fprintf(tf, "%s\n", newpwd);
                fclose(tf);
                sprintf(buf1, "cat %s ./lib/messaggi/endmail > %s",
                        tmpf, tmpf2);
                system(buf1);
                sprintf(buf1,
                        "mail -s 'New Cittadella Password for %s' %s < %s\n",
                        nome, utente->email, tmpf2);
                if (system(buf1) == -1) {
                        citta_logf("SYSERR: New Password for [%s] not sent.",
                                   nome);
                        cprintf(t, "%d Could not send mail.\n",	ERROR);
                } else {
                        cripta(newpwd);
                        strcpy(utente->password, newpwd);
                        citta_logf(
                             "AIDE: New Password for [%s] generated by [%s].",
                                   nome, t->utente->nome);
                        cprintf(t, "%d Pwd changed.\n", OK);
                }
                unlink(tmpf);
                unlink(tmpf2);
                Free(tmpf);
                Free(tmpf2);
        } else {
                citta_logf("SECURE: [%s] PWDU fallito.", t->utente->nome);
                cprintf(t, "%d User pwd wrong.\n", ERROR+PASSWORD);
        }
}

/*
 * PRoFile Get: Invia il profile personalizzato al client
 */
void cmd_prfg (struct sessione *t, char *nome)
{
        FILE *fp;
        char filename[LBUF], buf[LBUF];
        struct dati_ut *ut;

        /* verifica se esiste il profile */
        ut = trova_utente(nome);
        if (ut != NULL) {
                sprintf(filename, "%s/%ld", PROFILE_PATH, ut->matricola);
                /* Apre file */
                if ((fp = fopen(filename, "r")) == NULL) {
                        cprintf(t, "%d Errore in apertura file.\n",
                                ERROR+NO_SUCH_FILE);
                        return;
                }

                cprintf(t, "%d\n", SEGUE_LISTA);

                /* Legge file e mette in coda */
                while (fgets(buf, LBUF, fp) != NULL)
                        cprintf(t, "%d %s", OK, buf);
                fclose(fp);
                cprintf(t, "000\n");
        } else
                cprintf(t, "%d Utente inesistente.\n", ERROR+NO_SUCH_USER);
}

/*
 * Invia la configurazione dell'utente al client
 */
void cmd_cfgg(struct sessione *t, char *cmd)
{
        if (extract_int(cmd, 0))
                t->stato = CON_CONF;
        cprintf(t, "%d %d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                OK, t->utente->flags[0], t->utente->flags[1],
                t->utente->flags[2], t->utente->flags[3],
                t->utente->flags[4], t->utente->flags[5],
                t->utente->flags[6], t->utente->flags[7],
                t->utente->sflags[0], t->utente->sflags[1],
                t->utente->sflags[2], t->utente->sflags[3],
                t->utente->sflags[4], t->utente->sflags[5],
                t->utente->sflags[6], t->utente->sflags[7]);
}

/*
 * Modifica la configurazione dell'utente
 * Sintassi: "CFGP mode|Flag0|...|Flag7"
 *           mode = 0, torna semplicemente in modo comandi,
 *           mode = 1, salva le modifiche.
 */
void cmd_cfgp(struct sessione *t, char *arg)
{
        int i;

        if (extract_int(arg, 0))
                for (i = 0; i < 8; i++)
                        t->utente->flags[i] = (char) extract_int(arg, i+1);
        cprintf(t, "%d\n", OK);
        t->stato = CON_COMANDI;
}

void cmd_frdg(struct sessione *t)
{
        struct dati_ut *amico;
        int i;
        long matr;

        cprintf(t, "%d\n", SEGUE_LISTA);
        for (i = 0; i < NFRIENDS; i++){
                matr = t->utente->friends[i];
                if (matr != -1) {
                        amico = trova_utente_n(matr);
                        if (amico != NULL)
                                cprintf(t, "%d %d|%ld|%s\n", OK, i, matr,
                                        amico->nome);
                        else
                                /* Utente con quella matricola eliminato */
                                t->utente->friends[i] = -1;
                }
        }
        cprintf(t, "000\n");
        for (i = NFRIENDS; i < 2*NFRIENDS; i++){
                matr = t->utente->friends[i];
                if (matr != -1) {
                        amico = trova_utente_n(matr);
                        if (amico != NULL)
                                cprintf(t, "%d %d|%ld|%s\n", OK, i, matr,
                                        amico->nome);
                        else
                                /* Utente con quella matricola eliminato */
                                t->utente->friends[i] = -1;
                }
        }
        cprintf(t, "000\n");
}

void cmd_frdp(struct sessione *t, char *arg)
{
        int i;

        for (i = 0; i < 2*NFRIENDS; i++)
                t->utente->friends[i] = extract_long(arg, i);
        cprintf(t, "%d\n", OK);
}

void cmd_gmtr(struct sessione *t, char *nome)
{
        struct dati_ut *utente;

        utente = trova_utente(nome);
        if (utente != NULL)
                cprintf(t, "%d %ld\n", OK, utente->matricola);
        else
                cprintf(t, "%d\n", ERROR);
}

/*
 * Notifica l'uscita di un utente.
 * 'tipo' indica il motivo: puo' avere i seguenti valori:
 * LGOT: logout (Quit normale).
 * DROP: dropped link.
 * LOID: superato idle time.
 */
void notify_logout(struct sessione *t, int tipo)
{
        char buf[LBUF];
        size_t buflen;
        struct sessione *punto;

        if ((t->stato > CON_LIC) && (t->utente) &&
            (t->utente->livello > LVL_OSPITE) && (t->utente->registrato)) {
                sprintf(buf, "%d %c%s\n", tipo, SESSO(t->utente),
                        t->utente->nome);
                buflen = strlen(buf);
                for (punto = lista_sessioni; punto; punto = punto->prossima)
                        if ((punto->utente) && (punto != t)
                            && (punto->utente->flags[3] & UT_LION)) {
                                metti_in_coda(&punto->output, buf, buflen);
				punto->bytes_out += buflen;
                        }
        }
}

/* TODO move the stats function in another file */

/* updates server statistics after new successful login */
void stats_new_login(int logged_users)
{
        struct tm *tmst;
        time_t ora;

        dati_server.login++;
        time(&ora);
        tmst = localtime(&ora);
        dati_server.stat_ore[tmst->tm_hour]++;
        dati_server.stat_giorni[tmst->tm_wday]++;
        dati_server.stat_mesi[tmst->tm_mon]++;
        /* Record di connessioni contemporanee? */
        if (dati_server.max_users < logged_users) {
                dati_server.max_users = logged_users;
                time(&(dati_server.max_users_time));
        }
}

/* updates server statistics after a new guest login */
void stats_new_guest(void)
{
        dati_server.ospiti++;
}

/* updates server statistics after a new user login */
void stats_new_user(void)
{
        dati_server.nuovi_ut++;
}

/* updates server statistics after a successful validation */
void stats_new_validation(void)
{
        dati_server.validazioni++;
}
