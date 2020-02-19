/****************************************************************************
* Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : errore.h                                                           *
*        Funzioni per stampare i messaggi di errore dal server.             *
****************************************************************************/
#ifndef _ERRORE_H
#define _ERRORE_H   1

/* Errori X */
#define X_ACC_PROIBITO           1
#define X_NO_SUCH_USER           2
#define X_EMPTY                  3
#define X_TXT_NON_INIT           4
#define X_UTENTE_NON_COLLEGATO   5
#define X_SELF                  10
#define X_RIFIUTA               11
#define X_SCOLLEGATO            12
#define X_OCCUPATO              13
#define X_UTENTE_CANCELLATO     14

/* Prototipi delle funzioni in errore.c */
bool print_err_x(char *str);
void print_err_destx(char *str);
void print_err_fm(char *str);
void print_err_edit_room_info(const char *buf);
void print_err_edit_profile(const char *buf);
void print_err_edit_news(const char *buf);
void print_err_test_msg(const char *buf, const char *room);

/****************************************************************************/

#endif /* errore.h */
