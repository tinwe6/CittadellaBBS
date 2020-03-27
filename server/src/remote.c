/*
 * Server functions to handle telnet connections to the remote client.
 *
 * The server generates a SecureKey, that allow the remote cittaclients
 * to identify as such, then forks a cittad daemon. The daemon listens to the
 * PORT_REMOTE port for incoming connections and starts the remote_cittaclient
 * with the '-k SecureKey' option for each new telnet connection.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "utility.h"

#ifdef USE_REMOTE_PORT

char remote_auth_key[REMOTE_KEY_LEN + 1];

static void save_remote_key(const char *key)
{
	assert(strlen(key) == REMOTE_KEY_LEN);
	FILE *fp = fopen(REMOTE_KEY_PATH, "w");
	if (!fp) {
		citta_logf("Remote key: could not open file %s",
			   strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (fputs(key, fp) == EOF) {
		citta_logf("Remote key save error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	fclose(fp);
}

void init_remote_key(void)
{
	generate_random_string(remote_auth_key, REMOTE_KEY_LEN);
	save_remote_key(remote_auth_key);
	citta_logf("Remote key: %s", remote_auth_key);
}

void init_remote_daemon(void)
{

}

void close_remote_daemon(void)
{

}

#endif
