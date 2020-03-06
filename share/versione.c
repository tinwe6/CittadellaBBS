/****************************************************************************
* Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : versione.c                                                         *
*        Funzioni per gestire le versioni del programma.                    *
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "versione.h"

void version_print(int code)
{
	printf("%d.%d.%d", MAJOR(code), MINOR(code), SUBLVL(code));
}

#if 0 /* NOT USED */
char * version_name(int code)
{
	char * name;

	asprintf(&name, "%d.%d.%d", MAJOR(code), MINOR(code), SUBLVL(code));
	return name;
}
#endif
