
#ifndef _REMOTE_H
#define _REMOTE_H 1

#include "config.h"

#ifdef USE_REMOTE_PORT

/* Remote client authentication key */
extern char remote_auth_key[REMOTE_KEY_LEN + 1];

void init_remote_key(void);
void init_remote_daemon(void);
void close_remote_daemon(void);

#endif

#endif /* remote.h */
