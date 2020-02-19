


#ifndef _CITTA_DECOMPRESS_H
#define _CITTA_DECOMPRESS_H   1

#include "conn.h"

void decompress_set(const char * compress);

int decompress_serv_getc(struct serv_buffer * buf);

/* restituisce la % di compressione dei dati */
int decompress_stat();


#endif /* _CITTA_DECOMPRESS_H */
