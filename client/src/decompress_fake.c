
#define ALLOW_COMPRESSION 0

#include <stdio.h>
#include <stdlib.h>

#include "cittacfg.h"
#include "decompress.h"
#include "conn.h"

void decompress_set(const char *tipo)
{
    if (tipo[0]) {
	perror("compressione non supportata!");
	exit(1);
    }
}

int decompress_serv_getc(struct serv_buffer * buf)
{
    return serv_getc_r(buf);
}


/* restituisce la % di compressione dei dati */
int decompress_stat()
{
    return 0;
}
