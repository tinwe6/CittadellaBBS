#ifndef _SOCKET_COMPRESS_H
#define _SOCKET_COMPRESS_H   1

#define MAXLEN_COMPRESS  16 /* Lunghezza massima nome dell'algoritmo di *
                             * compressione                             */

/* forward declaration */
struct sessione;

void compress_check(struct sessione *t, const char *tipo);

void compress_set(struct sessione *t);

int compress_scrivi_a_client(struct sessione *t, char *testo, unsigned int len);

void compress_free(struct sessione *t);


#endif /* _SOCKET_COMPRESS_H */
