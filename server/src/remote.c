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
#include <termios.h>
#include <unistd.h>
#include <util.h>
#include <sys/ioctl.h>

#include "config.h"
#include "remote.h"
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


#define CITTAD_PATH "./bin/cittad"
#define _TO_STR(x) #x
#define TO_STR(x) _TO_STR(x)
char * const cittad_argv[] = {
	CITTAD_PATH,
	"-p", TO_STR(REMOTE_PORT),
	"-f", FILE_CITTADLOG,
	"-t", "-d",
	"-l", "1",
	NULL
};
#undef TO_STR
#undef _TO_STR

pid_t init_remote_daemon(void)
{
	int master;
	pid_t pid = forkpty(&master, NULL, NULL, NULL);    // opentty +
	if (pid == -1) {
		/* forkpty failed */
		citta_logf("Remote daemon forkpty() failed: %s",
			   strerror(errno));
	} else if (pid == 0) { /* child */
		execvp(cittad_argv[0], cittad_argv);
		exit(EXIT_FAILURE);
	} else { /* parent */
		citta_logf("cittad daemon pid %d", pid);
	}
	return pid;
}

void close_remote_daemon(pid_t pid)
{
	if (pid != -1) {
		kill(pid, SIGHUP);
		citta_logf("Remote port daemon shut down.");
	}
}

#endif
