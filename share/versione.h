/****************************************************************************
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* version_string 0.4.5                                                      *
*                                                                           *
* File : versione.h                                                         *
*        Funzioni per gestire le versioni del programma.                    *
****************************************************************************/
#ifndef _VERSIONE_H
#define _VERSIONE_H   1

#define CLIENT_RELEASE "0.4.5"
#define SERVER_RELEASE "0.4.5"
#define PROTCL_RELEASE "0.4.5"
#define CLIENT_VERSION_CODE VERSION_CODE(0, 4, 5)
#define SERVER_VERSION_CODE VERSION_CODE(0, 4, 5)
#define PROTCL_VERSION_CODE VERSION_CODE(0, 4, 5)
#define PATCH_LEVEL 0

#define MAJOR(ver)  ((ver) >> 16)
#define MINOR(ver)  (((ver) >> 8) & 0xff)
#define SUBLVL(ver) ((ver) & 0xff)
#define VERSION_CODE(major, minor, sublvl) \
         (((major) << 16) | ((minor) << 8) | (sublvl))

/* Prototipi funzioni in versione.c */
void version_print(int code);

/* char * version_name(int code); */

#endif /* versione.h */
