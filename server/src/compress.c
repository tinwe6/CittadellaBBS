

# include "config.h"
# include "compress.h"
# include "utility.h"
# include "strutture.h"


#if ALLOW_COMPRESSION

# include <string.h>
# include <zlib.h>

struct compress_buf {
        unsigned char * data;
        int start, end, size;
};

# define out_ ((struct compress_buf *)t->compress_data)
# define z   ((z_streamp)t->zlib_data)

/***************************************************************************/
/* Gestione della compressione                                             */
/***************************************************************************/
/* Controlla che l'algoritmo richiesto sia disponibile e setta il flag di
 * compressione.                                                           */
void compress_check(struct sessione *t, const char *tipo)
{
    if (!strncmp(tipo, "z", MAXLEN_COMPRESS)) {

	CREATE(t->compress_data, struct compress_buf, 1, TYPE_POINTER);
	
	out_->start = out_->end = 0;
	out_->size = MAX_STRINGA;
	CREATE(out_->data, unsigned char, out_->size, TYPE_CHAR);

	CREATE(t->zlib_data, z_stream, 1, TYPE_POINTER);
	z->zalloc = Z_NULL;
	z->zfree  = Z_NULL;
	z->opaque = NULL;
	
	if (deflateInit(z, Z_BEST_COMPRESSION) == Z_OK) {
	    strncpy(t->compressione, tipo, MAXLEN_COMPRESS);
	    return;
	}
	Free(z);
	Free(out_->data);
	Free(out_);
	t->zlib_data = NULL;
	t->compress_data = NULL;
    }
    strcpy(t->compressione, "");
}


static int sync_sessione(struct sessione *t)
{
        citta_log("Sync sessione");
        while ( (t->output.partenza != NULL) &&
                (MAX_STRINGA-1 > iobuf_olen(&t->iobuf))) {
                t->bytes_out += prendi_da_coda_o(&t->output, &t->iobuf);
                if (scrivi_tutto_a_client(t, &t->iobuf) < 0)
                        return -1;
        }
        return 0;
}


/* Se la sessione ha un algoritmo di compressione impostato, lo attiva */
void compress_set(struct sessione *t)
{
    if (!(t->compressing =
          strcmp(t->compressione, "") &&
	  /* va svuotato _TUTTO_ il buffer prima di attivare la compressione! */
	  sync_sessione(t) >= 0)) {
            compress_free(t);
    }
}

void compress_free(struct sessione *t)
{
    if (z) {
	deflateEnd(z);
	Free(z);
	Free(out_->data);
	Free(out_);
	t->compressing = 0;
	t->zlib_data = NULL;
	t->compress_data = NULL;
    }
}

int compress_scrivi_a_client(struct sessione *t, char * testo, unsigned int len)
{
	struct compress_buf * q = out_;
        int wlen, ret;

        if (len == 0)
                return 0;

        /* TODO
           NOTA Z_BUF_ERROR = -5
              no progress can be made.

  If flush is set to Z_FULL_FLUSH, all output is flushed as with
  Z_SYNC_FLUSH, and the compression state is reset so that decompression can
  restart from this point if previous compressed data has been damaged or if
  random access is desired. Using Z_FULL_FLUSH too often can seriously degrade
  the compression.

  If deflate returns with avail_out == 0, this function must be called again
  with the same value of the flush parameter and more output space (updated
  avail_out), until the flush is complete (deflate returns with non-zero
  avail_out). In the case of a Z_FULL_FLUSH or Z_SYNC_FLUSH, make sure that
  avail_out is greater than six to avoid repeated flush markers due to
  avail_out == 0 on return.

        */
        /* comprimi il testo */
	z->next_in = (unsigned char *)testo;
	z->avail_in = len;

	z->next_out = q->data + q->end;
	z->avail_out = q->size - q->end;

        /* spedisci i dati compressi */
        if (z->avail_out == 0) {
                //citta_log("MY ADDITION...");
                q->end = q->size - z->avail_out;
                if ( (wlen = write(t->socket_descr, q->data + q->start,
                                  q->end - q->start)) > 0) {
                        if ((q->start += wlen) >= q->end)
                                q->start = q->end = 0;
                } else
                        return -1;
                return 0;
        }

        /* TODO se mando file posso togliere Z_SYNC_FLUSH? */
	if ( (ret = deflate(z, Z_SYNC_FLUSH)) == Z_OK) {
                /* aggiorna i dati non ancora compressi */
                if (z->avail_in < len) {
                        /* copia anche il '\0' finale */
                        memmove(testo, testo+len-z->avail_in, z->avail_in + 1);
                }
		    
                /* spedisci i dati compressi */
                if (z->avail_out < q->size - q->end) {
                        q->end = q->size - z->avail_out;
                        if ((wlen = write(t->socket_descr, q->data + q->start,
                                         q->end - q->start)) > 0) {
                                if ((q->start += wlen) >= q->end)
                                        q->start = q->end = 0;
                        } else
                                return -1;
                }
	} else {
                //citta_logf("Deflate returns %d, avail-out %d", ret, z->avail_out);
                return 0;
        }
        //citta_logf("Scritto %d bytes - Inviato %d bytes", wlen, len);

        return len-z->avail_in;
}











#else /* ! ALLOW_COMPRESSION */


# include <string.h>


void compress_check(struct sessione *t, const char *tipo)
{
  strcpy(t->compressione, "");
}

void compress_set(struct sessione *t)
{
}

int compress_scrivi_a_client(struct sessione *t, char * testo)
{
    return scrivi_a_client(t, testo);
}

void compress_free(struct sessione *t)
{
}

#endif /* ALLOW_COMPRESSION */
