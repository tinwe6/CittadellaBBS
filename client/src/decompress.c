

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cittacfg.h"
#include "decompress.h"
#include "conn.h"

#define ALLOW_COMPRESSION 1

#if ALLOW_COMPRESSION

#include <zlib.h>


static z_stream Z;
#define z (&Z)



/***************************************************************************/
/* Gestione della decompressione                                             */
/***************************************************************************/

/* Controlla che l'algoritmo richiesto sia disponibile e setta il flag di
 * decompressione.                                                           */
void decompress_set(const char *tipo)
{
    if (!strcmp(tipo, "z")) {
	client_cfg.compressing = 1;
	if (inflateInit(z) == Z_OK)
	    return;
	perror("inflateInit");
    } else
	perror("compressione non supportata!");
}

static char zdata[4096];

static struct serv_buffer Zbuf = { zdata, 0, 0, sizeof(zdata) };
#define zbuf (&Zbuf)


static int decompress_some(struct serv_buffer * buf)
{
    /* decomprimi il testo gia' ricevuto */
    if (zbuf->start < zbuf->end) {
	
        z->next_in = (Bytef *)zbuf->data + zbuf->start;
	z->avail_in = zbuf->end - zbuf->start;
	
	z->next_out = (Bytef *)buf->data + buf->end;
	z->avail_out = buf->size - buf->end;
	    
	if (inflate(z, Z_SYNC_FLUSH) == Z_OK) {

	    /* aggiorna i dati non ancora decompressi */
	    if (z->avail_in < zbuf->end - zbuf->start)
		if ((zbuf->start = zbuf->end - z->avail_in) >= zbuf->end)
		    zbuf->start = zbuf->end = 0;
	    
	    /* aggiorna i dati decompressi */
	    if (z->avail_out < buf->size - buf->end) {
		buf->end = buf->size - z->avail_out;
		return(1);
	    } else
		return(0);
	} else
	    return(-1);
    }
    return(0);
}

int decompress_serv_getc(struct serv_buffer * buf)
{
    int ricevuti;
    
    if (buf->start < buf->end)
	return buf->data[buf->start++];

    buf->start = buf->end = 0;
    
    do {
            if ( (ricevuti = decompress_some(buf) > 0)) {
                    return buf->data[buf->start++];
            }
#ifdef EINTR
	while ((ricevuti = read(serv_sock, zbuf->data + zbuf->end, zbuf->size - zbuf->end)) < 0 &&
	       errno == EINTR)
	    ;
#else
	ricevuti = read(serv_sock, zbuf->data + zbuf->end, zbuf->size - zbuf->end);
#endif

	if (ricevuti > 0)
	    zbuf->end += ricevuti;

    } while (ricevuti > 0);

    printf("%s",str_conn_closed);
    exit(1);
}


/* restituisce la % di compressione dei dati */
int decompress_stat()
{
    if (z->total_out)
	return 100 - 100 * z->total_in / z->total_out;
    return 0;
}




#else /* ! ALLOW_COMPRESSION */


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


#endif /* ALLOW_COMPRESSION */
