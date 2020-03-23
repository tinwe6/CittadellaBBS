#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __USE_BSD
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#undef RETRIEVE_REMOTE_INFO

#define DEFAULT_LISTEN_PORT 4001
static char default_client_path[] = "./bin/remote_cittaclient";
static const char too_many_conn_msg[] =
    "Too many remote users. Try again later...\n";

/* maximum number of remote connections allowed */
#define MAX_CONNECTIONS 20

/* maximum length for the queue of pending connections. */
#define MAX_PENDING 5

/* Number of times per seconds that the daemon wakes up and reads/sends data */
#define CYCLES_PER_SEC 100

/* We must allow for one socket to listen for new connections, and */
/* two for each connections (telnet socket + master side of pty)   */
#define MAX_SOCKETS (2*MAX_CONNECTIONS + 1)

#define BUFFER_SIZE 256

/* Log level */
typedef enum {
    LL0 = 0, /* normal  */
    LL1,     /* verbose */
    LL2,     /* show transmitted data too */
    LL3,     /* extreme log flood */
    LL_COUNT,
} log_level;

/**************************************************************************/
/* Daemon configuration */

typedef struct {
    int listen_fd;
    int port;
    bool send_host;
    char *auth_key;
    char *logfile; /* if NULL log to stderr */
    log_level loglvl;
    bool do_fork;
    char * const *client_argv;
} daemon_config;

char * const default_client_argv[] = {default_client_path, NULL};


/****************************************************************************/
/* Process command line arguments */

static void print_usage_and_exit(char *arg0, char *arg)
{
    if (arg) {
	fprintf(stderr, "DAEMON unknown option `%s'\n", arg);
    }

    fprintf(stderr,
"Usage: %s [-d|--debug] [-f|--log-file <filename>]\n"
"        [-h|dont-send-host] [-l|--log-lvl <level>] [-p|--port <TCP_port>]"
"        -- <client_program_and_args>\n",
	    arg0);
    exit(EXIT_FAILURE);
}

static daemon_config process_args(int argc, char **argv)
{
    char *s;
    char *prog_name = *argv;

    daemon_config config = {
	.listen_fd = 0,
	.port = DEFAULT_LISTEN_PORT,
	.send_host = true,
	.auth_key = NULL,
	.logfile = NULL,
	.loglvl = LL0,
	.do_fork = true,
	.client_argv = NULL,
    };

    if (argc <= 1) {
	print_usage_and_exit(prog_name, NULL);
    }

    /* skip program name */
    argv++;

    while ( (s = *argv)) {
	argv++;

	if (!strcmp(s, "-d") || !strcmp(s, "--debug")) {
	    /* debug -> do not fork */
	    config.do_fork = false;
	} else if (*argv && (!strcmp(s, "-f") || !strcmp(s, "--log-file"))) {
	    config.logfile = *argv++;
	} else if (*argv && (!strcmp(s, "-h") || !strcmp(s, "--dont-send-host"
							 ))) {
	    config.send_host = false;
	} else if (*argv && (!strcmp(s, "-k") || !strcmp(s, "--auth-key"))) {
	    config.auth_key = *argv++;
	} else if (*argv && (!strcmp(s, "-l") || !strcmp(s, "--log-lvl"))) {
	    if (!isdigit(**argv)) {
		config.loglvl = -1;
		break;
	    }
	    config.loglvl = atol(*argv++);
	} else if (*argv && (!strcmp(s, "-p") || !strcmp(s, "--port"))) {
	    /* port to listen at */
	    config.port = atol(*argv++);
	} else if (!strcmp(s, "--")) {
	    /* end of daemon opts. further args are client program and args */
	    break;
	} else {
	    print_usage_and_exit(prog_name, s);
	}
    }

    if (config.loglvl < 0 || config.loglvl >= LL_COUNT) {
	fprintf(stderr,
		"Invalid log level (must be integer between %d and %d)\n",
		LL0, LL_COUNT - 1);
	print_usage_and_exit(prog_name, NULL);
    }
    if (!*argv) {
	config.client_argv = default_client_argv;
    } else {
	config.client_argv = argv;
    }

    return config;
}

/**************************************************************************/
/* Logging facilities */

static int fd_log = -1;
static FILE *fp_log = NULL;

static log_level ll_current = LL0;

static int init_log(const daemon_config *config)
{
    assert(fp_log == NULL);

    if (config->logfile) {
	/* Log to a file */
	fp_log = fopen(config->logfile, "w");
	if (fp_log == NULL) {
	    fprintf(stderr, "Fatal: could not open log file %s\n%s\n",
		    config->logfile, strerror(errno));
	    exit(EXIT_FAILURE);
	}
	fd_log = fileno(fp_log);
    } else if (!config->do_fork) {
	/* Log to stderr */
	fd_log = dup(STDERR_FILENO);
	fp_log = stderr;
    } else {

    }

    ll_current = config->loglvl;

    dprintf(fd_log, "Cittadaemon log started.\n");
    return fd_log;
}

static void stop_log(void)
{
    if (fp_log) {
	fflush(fp_log);
	fclose(fp_log);
	fp_log = NULL;
    } else if (fd_log > 2) {
	close(fd_log);
    }
    fd_log = -1;
}

static int log_timestamp(void)
{
    time_t now;
    char *timestamp;

    now = time(NULL);
    timestamp = ctime(&now);
    *(timestamp + strlen(timestamp) - 1) = '\0';
    return dprintf(fd_log, "%-19.19s :: ", timestamp);
}

static int log_printf(log_level ll, const char *format, ...)
{
    va_list ap;
    int size;

    if (ll > ll_current) {
	return 0;
    }

    size = log_timestamp();

    va_start(ap, format);
    size += vdprintf(fd_log, format, ap);
    va_end(ap);

    return size;
}

static int log_perror(log_level ll, const char *cmd)
{
    return log_printf(ll, "%s failed: %s\n", cmd, strerror(errno));
}

/* sends program name and cli options to the log */
static void log_argv(log_level ll, const char *pre, char * const * argv)
{
    if (ll > ll_current) {
	return;
    }

    dprintf(fd_log, "%s ", pre);
    while (*argv) {
	dprintf(fd_log, "%s ", *argv++);
    }
    dprintf(fd_log, "\n");
}

static int log_addr(log_level ll, const char *str, unsigned long addr)
{
    int len = log_printf(ll, "%s [%lu.%lu.%lu.%lu]\n", str,
		      (addr >>  0) & 0xff, (addr >>  8) & 0xff,
		      (addr >> 16) & 0xff, (addr >> 24) & 0xff
		      );
    return len;
}

static void log_revents(log_level ll, short int revents)
{
    if (ll > ll_current) {
	return;
    }

    if (revents & POLLIN) {
	dprintf(fd_log, "POLLIN ");
    }
    if (revents & POLLPRI) {
	dprintf(fd_log, "POLLPRI ");
    }
    if (revents & POLLOUT) {
	dprintf(fd_log, "POLLOUT ");
    }
    if (revents & POLLERR) {
	dprintf(fd_log, "POLLERR ");
    }
    if (revents & POLLHUP) {
	dprintf(fd_log, "POLLHUP ");
    }
    if (revents & POLLNVAL) {
	dprintf(fd_log, "POLLNVAL ");
    }
    dprintf(fd_log, "\n");
}


static int bytes_to_str(const char *str, size_t len, char *buf, size_t bufsize)
{
    int written = 0;
    const char hexdigit[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			     'a', 'b', 'c', 'd', 'e', 'f'};
    for (size_t i = 0; i != len; ++i) {
	if (isprint(str[i])) {
	    if ((size_t)written + 1 >= bufsize) {
		break;
	    }
	    buf[written++] = str[i];
	} else {
	    if ((size_t)written + 6 >= bufsize) {
		break;
	    }
	    buf[written++] = '{';
	    buf[written++] = '0';
	    buf[written++] = 'x';
	    buf[written++] = hexdigit[((unsigned char)str[i] >> 4) & 0x0f];
	    buf[written++] = hexdigit[(unsigned char)str[i] & 0x0f];
	    buf[written++] = '}';
	}
    }
    assert((size_t)written < bufsize);
    buf[written++] = 0;

    return written;
}

typedef enum {
    LOG_R, LOG_W, LOG_RW,
} log_tran_type;
static int log_transmission(log_level ll, const char *data, ssize_t len,
			    int from, int to, log_tran_type type)
{
    char pretty[1024];
    int n;

    if (ll > ll_current) {
	return 0;
    }
    bytes_to_str(data, len, pretty, sizeof(pretty));
    char t;
    switch (type) {
    case LOG_R: t = 'R'; break;
    case LOG_W: t = 'W'; break;
    case LOG_RW: t = '|'; break;
    }
    n = log_printf(ll, "%c(%d->%d): '%s' - %zd bytes\n", t, from , to, pretty,
		   len);
    return n;
}

/*************************************************************/

#define MAX_ARGV_SIZE 32
#define MAX_HOST_LEN 256

typedef enum {
    SESS_LISTEN,
    SESS_PIPE,
} session_type;

typedef struct session_data_t {
    session_type type;
    int fd;
    int fd_out;
    bool close_conn;
    bool is_socket;
    bool process_CR;
    char *host;
    char outbuf[BUFFER_SIZE];
    ssize_t outlen;
    struct session_data_t *other_end;
} session_data;

typedef struct {
    struct pollfd fds[MAX_SOCKETS];
    session_data * sessions[MAX_SOCKETS];
    int nfds;
    int connections_count;
    bool needs_cleanup;
    bool stop_server;
} daemon_data;

static void register_fd(daemon_data *data, int fd, short events, int index)
{
    data->fds[index] = (struct pollfd) {
	.fd = fd,
	.events = events,
	.revents = 0
    };
}

static session_data * session_add(daemon_data *data, session_type type, int fd,
				  int fd_out, char *host, bool is_socket)
{
    session_data *session = (session_data *)malloc(sizeof(session_data));
    session->type = type;
    session->fd = fd;
    session->fd_out = fd_out;
    session->close_conn = false;
    session->is_socket = is_socket;
    session->outbuf[0] = 0;
    session->outlen = 0;
    session->host = host;
    session->other_end = NULL;
    session->process_CR = false;

    short events = (type == SESS_LISTEN) ? POLLIN : (POLLIN|POLLOUT);
    register_fd(data, fd, events, data->nfds);
    data->sessions[data->nfds] = session;
    data->nfds++;

    return session;
}

/* Creates a pipe between a socket and the master fd of the pty */
static void make_pipe(daemon_data *data, int socket_fd, int pty_fd, char *host)
{
    session_data *s1 = session_add(data, SESS_PIPE, socket_fd, pty_fd, host,
				   true);
    session_data *s2 = session_add(data, SESS_PIPE, pty_fd, socket_fd, host,
				   false);
    s1->other_end = s2;
    s2->other_end = s1;
    s1->process_CR = true;
}

/* See https://stackoverflow.com/a/12730776 for how to close a socket */
static int get_and_clear_socket_error(int fd)
{
    int err = 1;
    socklen_t len = sizeof err;
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len)) {
	log_perror(LL0, "Fatal: getSO_ERROR");
	exit(EXIT_FAILURE);
    }
    if (err) {
	errno = err;
    }
    return err;
}

void close_fd(int fd)
{
    if (close(fd) < 0) {
	log_perror(LL0, "close()");
    }
}

void close_socket(int fd)
{
    assert(fd >= 0);
    get_and_clear_socket_error(fd);
    /* terminate the 'reliable' delivery */
    if (shutdown(fd, SHUT_RDWR) < 0) {
	if (errno != ENOTCONN && errno != EINVAL) {
	    /* SGI causes EINVAL */
	    log_perror(LL0, "shutdown()");
	}
    }
    close_fd(fd);
}

static void session_close_fd(daemon_data *data, int index)
{
    assert(data->fds[index].fd >= 0);
    if (data->sessions[index]->is_socket) {
	close_socket(data->fds[index].fd);
    } else {
	close_fd(data->fds[index].fd);
    }
    data->fds[index].fd = -1;
}

static void session_close(daemon_data *data, int index)
{
    assert(data->sessions[index]->close_conn);

    if (data->fds[index].fd == -1) {
	assert(data->sessions[index]->type == SESS_PIPE);
	assert(data->needs_cleanup);
	return;
    }
    if (data->sessions[index]->type == SESS_PIPE) {
	assert(data->sessions[index]->other_end);
	for (int j = 0; j != data->nfds; ++j) {
	    if (data->fds[j].fd == data->sessions[index]->fd_out) {
		assert(data->sessions[j]->fd_out == data->sessions[index]->fd);
		data->sessions[j]->close_conn = true;
		session_close_fd(data, j);
		log_printf(LL1, "Closed session index %d fd %d (pipe)\n",
			   j, data->sessions[j]->fd);
	    }
	}
	data->connections_count--;
    }
    session_close_fd(data, index);
    log_printf(LL1, "Closed session index %d fd %d\n", index,
	       data->sessions[index]->fd);
    data->needs_cleanup = true;
}

static void log_active_sessions(log_level ll, daemon_data *data)
{
    if (ll > ll_current) {
	return;
    }
    log_printf(ll, "active connections:\n");
    for (int i = 0; i < data->nfds; ++i) {
	log_printf(ll, "session %d  fd %d  out %d:\n", i, data->fds[i].fd,
		   data->sessions[i]->fd_out);
    }
}

/*
 * Eliminate the closed sessions from the fds and sessions arrays,
 * and compress the arrays so that no holes are left.
 * NB: does not preserve the order of the sessions.
 */
static void cleanup_sessions(daemon_data *data)
{
    if (!data->needs_cleanup) {
	return;
    }
    log_printf(LL1, "Cleaning up the sessions and fds arrays.\n");

    for (int i = data->nfds - 1; i >= 0; --i) {
	if (data->fds[i].fd == -1) {
	    assert(data->sessions[i]->close_conn);
	    if (data->sessions[i]->host) {
		free(data->sessions[i]->host);
		if (data->sessions[i]->other_end) {
		    data->sessions[i]->other_end->host = NULL;
		}
	    }

	    free(data->sessions[i]);

	    data->fds[i].fd = data->fds[data->nfds - 1].fd;
	    data->fds[i].events = data->fds[data->nfds - 1].events;
	    data->fds[i].revents = data->fds[data->nfds - 1].revents;
	    data->sessions[i] = data->sessions[data->nfds - 1];
	    data->nfds--;

	    data->fds[data->nfds].fd = -1;
	    data->sessions[data->nfds] = NULL;
	}
    }

    for (int i = 0; i < data->nfds; i++) {
	assert(data->fds[i].fd == data->sessions[i]->fd);
    }

    data->needs_cleanup = false;
    log_active_sessions(LL1, data);
}

/*********************************************************************/
/* Telnet negotiation and string processing */

#define NUL 0
#define LF  10
#define CR  13
#define TN_BINARY_TRANSMISSION 0x00
#define TN_ECHO                0x01 /* RFC 857 */
#define TN_SUPPRESS_GO_AHEAD   0x03 /* RFC 858 */
#define TN_WILL 0xfb
#define TN_WONT 0xfc
#define TN_DO   0xfd
#define TN_DONT 0xfe
#define TN_IAC  0xff /* telnet 'Interpret As Command' escape character */

/*
 * Negotiate the telnet connection. We take control of echoing and
 * suppress transmission of the telnet go-ahead character in order to
 * have data transmitted one character at a time (and not by lines).
 * (see RFC 854 for telnet protocol specification)
 */
void telnet_negotiation(int fd)
{

    static uint8_t will_echo[3] = {TN_IAC, TN_WILL, TN_ECHO};
    static uint8_t char_mode[3] = {TN_IAC, TN_WILL, TN_SUPPRESS_GO_AHEAD};
    struct pollfd pfd[1];
    uint8_t cmd[3];
    const int tn_timeout_ms = 200;

    /* Negotiate character at a time echoing mode for the telnet connection */
    write(fd, char_mode, 3);
    write(fd, will_echo, 3);

    pfd[0] = (struct pollfd){.fd = fd, .events = POLLIN, .revents = 0};

    for (;;) {
	if (poll(pfd, 1, tn_timeout_ms) == 0) {
		/* poll timed out, we're done */
		assert(!pfd[0].revents);
		break;
	}

	read(fd, &cmd, 3);

	/* Ignore the answers to our requests */
	if (cmd[2] == TN_ECHO || cmd[2] == TN_SUPPRESS_GO_AHEAD) {
	    continue;
	}

	/* Reject all other options by answering DON'T */
	/* to WILL requests and WON'T to DO requests.  */
	if (cmd[1] == TN_WILL) {
	    cmd[1] = TN_DONT;
	} else if (cmd[1] == TN_DO) {
	    cmd[1] = TN_WONT;
	}
	write(fd, cmd, 3);
    }
}

/* Copies the string 'in' to 'out' transforming CR LF in LF and CR NUL in CR,
 * in case the telnet application used to connect to the daemon is protocol
 * compliant and send CR LF when the user presses Enter.                     */
static ssize_t process_data(char *in, ssize_t inlen, char *out)
{
    ssize_t written = 0;

    for (ssize_t i = 0; i != inlen; i++) {
	if ((in[i] == CR) && (i + 1 != inlen)) {
	    if (in[i + 1] == LF) {
		/* eat the CR: CR LF -> LF */
		continue;
	    } else if (in[i + 1] == NUL) {
		/* CR NUL -> CR */
		out[written++] = CR;
		i++; /* eat the NUL */
	    }
	} else if (((unsigned char)in[i] == TN_IAC) && (i + 1 != inlen)) {
	    if ((unsigned char)in[i + 1] == TN_IAC) {
		/* IAC IAC is escaped IAC */
		out[written++] = (char)TN_IAC;
		i++;
	    } else {
		/* ignore the telnet command */
		i += 2;
	    }
	} else {
	    out[written++] = in[i];
	}
    }
    return written;
}

/* Copies the string 'in' to 'out' escaping the IAC characters if any */
static ssize_t escape_iac(char *in, ssize_t inlen, char *out)
{
    ssize_t written = 0;

    for (ssize_t i = 0; i != inlen; i++) {
	if ((unsigned int)in[i] == TN_IAC) {
	    out[written++] = (char)TN_IAC;
	}
	out[written++] = in[i];
    }
    return written;
}

#undef TN_IAC
#undef TN_DONT
#undef TN_DO
#undef TN_WONT
#undef TN_WILL
#undef TN_SUPPRESS_GO_AHEAD
#undef TN_ECHO
#undef TN_BINARY_TRANSMISSION
#undef CR
#undef LF
#undef NUL

/****************************************************************************/
/* Sockets */

/*
 * Sets file descriptor 'fd' to be blocking / nonblocking (for 'bocking' true
 * or false respectively).
 * Returns -1 and sets errno if there was an error.
 */
int enable_socket_blocking(int fd, bool blocking)
{
    if (fd < 0) {
	return false;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
	return -1;
    }
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(fd, F_SETFL, flags);
}

/*
 * The child sets up the slave side of the PTY and executes the
 * remote program. Returns only if there was an exec error.
 */
int exec_client(int fds, char * const *argv)
{
    struct termios slave_orig_term_settings; // Saved term settings
    struct termios new_term_settings; // Current terminal settings
    int rc;

    // Save the default parameters of the slave side of the PTY
    rc = tcgetattr(fds, &slave_orig_term_settings);

    // Set raw mode on the slave side of the PTY
    new_term_settings = slave_orig_term_settings;
    cfmakeraw(&new_term_settings);
    new_term_settings.c_lflag |= ECHO;
    new_term_settings.c_oflag |= (OPOST|ONLCR);
    new_term_settings.c_iflag |= ICRNL;
    new_term_settings.c_iflag &= ~IXOFF;
    new_term_settings.c_cc[VMIN] = 1;
    new_term_settings.c_cc[VTIME] = 0;
    tcsetattr(fds, TCSAFLUSH, &new_term_settings);

    /* The slave side of the PTY becomes the standard input */
    /* and outputs of the child process.                    */
    close(0); /* Close standard input (current terminal)    */
    close(1); /* Close standard output (current terminal)   */
    close(2); /* Close standard error (current terminal)    */

    dup(fds); /* PTY becomes standard input (0)  */
    dup(fds); /* PTY becomes standard output (1) */
    dup(fds); /* PTY becomes standard error (2)  */

    /* Now the original file descriptor is useless */
    close(fds);

    /* Make the current process a new session leader */
    if (setsid() == -1) {
	log_perror(LL0, "setsid()");
	perror("setsid()");
	assert(false);
    }

    /* Set the controlling terminal to be the slave side of the PTY */
    if (ioctl(0, TIOCSCTTY, 1) == -1) {
	log_perror(LL0, "ioctl(stty)");
	perror("ioctl(stty)");
	close(fds);
	return -1;
    }

    log_argv(LL1, "Executing:", argv);

    // Execution of the program. It should not return, unless an error occurs
    // in which case it returns -1 and sets errno to indicate the error.
    rc = execvp(argv[0], argv);

    perror("Daemon: exec failed:");
    close(fds);
    return -1;
}

/*
 * Returns file descriptor of master side of pty, or -1 if something went
 * wrong.
 */
int start_client(int fd_client, char * const argv[])
{
    int fdm, fds, rc;

    fdm = posix_openpt(O_RDWR);
    if (fdm < 0) {
	log_perror(LL0, "posix_openpt()");
	return -1;
    }

    rc = grantpt(fdm);
    if (rc != 0) {
	log_perror(LL0, "grantpt()");
	return -1;
    }

    rc = unlockpt(fdm);
    if (rc != 0) {
	log_perror(LL0, "unlockpt()");
	return -1;
    }

    fds = open(ptsname(fdm), O_RDWR);

    if (enable_socket_blocking(fdm, false) == -1) {
	log_perror(LL0,"Making socket non-blocking");
	#if 0
	close(fds);
	close(fdm);
	return -1;
	#endif
    }

    log_printf(LL1, "PTY Master fd = %d, Slave fd = %d opened for socket %d\n",
	       fdm, fds, fd_client);

    if (fork()) { /* parent */
	close(fds); /* close the slave side of the pty for the child */
    } else { /* child */
	close(fdm); /* close the master side of the pty for the child */
	if (exec_client(fds, argv) == -1) {  /* returns only with an error */
	    close(fds);
	    exit(EXIT_FAILURE);
	}
    }

    return fdm;
}

#ifdef RETRIEVE_REMOTE_INFO
static int get_remote_info(int fd, char *host, size_t host_size)
{
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);
    struct hostent *origin;
    int len;

    if (getpeername(fd, (struct sockaddr *)&address, &address_len) < 0) {
	perror("getpeername");
	host[0] = 0;
	len = -1;
    } else if (!(origin = gethostbyaddr(&address.sin_addr,
					sizeof(address.sin_addr),
					 AF_INET))) {
	len = snprintf(host, host_size, "%d.%d.%d.%d",
		 (address.sin_addr.s_addr >>  0) & 0xff,
		 (address.sin_addr.s_addr >>  8) & 0xff,
		 (address.sin_addr.s_addr >> 16) & 0xff,
		 (address.sin_addr.s_addr >> 24) & 0xff
		 );
    } else {
	strncpy(host, origin->h_name, host_size);
	host[host_size - 1] = 0;
	len = strlen(host);
    }
    return len;
}
#endif

static char * make_host(unsigned long addr)
{
    char * host = (char *)malloc(MAX_HOST_LEN);

    if (addr) {
	snprintf(host, MAX_HOST_LEN - 1, "%lu.%lu.%lu.%lu",
		 (addr >>  0) & 0xff, (addr >>  8) & 0xff,
		 (addr >> 16) & 0xff, (addr >> 24) & 0xff
		 );
    } else {
	host[0] = 0;
    }
    return host;
}

static char opt_R[] = "-R";
static char opt_r[] = "-r";
/* Adds to the program name and arguments of the remote client given */
static void make_argv(char *argv[], char *host, daemon_config *config)
{
    int n = 0;
    while (config->client_argv[n]) {
	argv[n] = config->client_argv[n];
	++n;
    }
    if (config->auth_key) {
	argv[n++] = opt_r;
	argv[n++] = config->auth_key;
    }
    if (config->send_host) {
	argv[n++] = opt_R;
	argv[n++] = host;
    }
    argv[n] = NULL;
}

/* Accept all new incoming connections. For each of them, find info  */
/* about the remote host, set up a pty and start the remote program, */
/* and negotiate the telnet connection.                              */
static void new_connections(daemon_config *config, daemon_data *data,
			    int index)
{
    log_printf(LL1, "New connection to listening socket.\n");

    for (;;) {
	struct sockaddr_in address;
	socklen_t address_len = sizeof(address);

	int fd_client = accept(data->fds[index].fd,
			       (struct sockaddr *)&address, &address_len);
	if (fd_client == -1) {
	    /* If accept fails with EWOULDBLOCK, all new */
	    /* connections have been accepted.           */
	    if (errno != EWOULDBLOCK) {
		perror("accept()");
		data->stop_server = true;
	    }
	    break;
	}

	if (data->connections_count == MAX_CONNECTIONS) {
	    write(fd_client, too_many_conn_msg, strlen(too_many_conn_msg));
	    close_socket(fd_client);
	    log_addr(LL0, "Too many users: Refused connection from",
		     address.sin_addr.s_addr);
	    continue;
	}

#ifdef RETRIEVE_REMOTE_INFO
	char hostname[MAX_HOST_LEN];
	get_remote_info(fd_client, hostname, sizeof(hostname));
	log_printf(LL0, "New connection from [%s] (fd %d)\n", hostname,
		   fd_client);
#else
	log_addr(LL0, "New connection from", address.sin_addr.s_addr);
#endif

	/* NOTE: the new connection inherits the non blocking */
	/*       status from the listening socket             */
#if 0
	enable_socket_blocking(fd_client, false);
#endif
	char *client_argv[MAX_ARGV_SIZE];
	char *host = make_host(address.sin_addr.s_addr);

	make_argv(client_argv, host, config);

	int fd_master = start_client(fd_client, client_argv);

	if (fd_master == -1) {
	    log_printf(LL0, "Could not setup new connection (fd=%d).",
		       fd_client);
	    close_socket(fd_client);
	    return;
	}
	make_pipe(data, fd_client, fd_master, host);
	data->connections_count++;

	telnet_negotiation(fd_client);
    }
}

/* Receive all incoming data from s->fd and moves it to the output buffer */
static void read_data(session_data *s)
{
    while (s->outlen < (ssize_t)sizeof(s->outbuf)) {
	ssize_t bytes;

	bytes = read(s->fd, s->outbuf + s->outlen,
		     sizeof(s->outbuf) - s->outlen);
	if (bytes == -1) {
	    if (errno != EWOULDBLOCK) {
		log_printf(LL0, "read on socket %d failed: %s\n", s->fd,
			   strerror(errno));
		s->close_conn = true;
	    }
	    break;
	}

	if (bytes == 0) {
	    log_printf(LL0, "Connection closed [%s] (fd %d)\n", s->host,
		       s->fd);
	    s->close_conn = true;
	    break;
	}
	s->outlen += bytes;

	log_transmission(LL2, s->outbuf, bytes, s->fd, s->fd_out, LOG_R);
    }
}

/* Receive all incoming data from s->fd and processes the CR LF and CR NUL
 * sequences before moving it to the output buffer.                         */
static void read_and_process_data(session_data *s)
{
    char buf[BUFFER_SIZE];

    while (s->outlen < (ssize_t)sizeof(s->outbuf)) {
	ssize_t bytes;

	bytes = read(s->fd, buf, sizeof(s->outbuf) - s->outlen);
	if (bytes == -1) {
	    if (errno != EWOULDBLOCK) {
		log_printf(LL0, "read on socket %d failed: %s\n", s->fd,
			   strerror(errno));
		s->close_conn = true;
	    }
	    break;
	}

	if (bytes == 0) {
	    log_printf(LL0, "Connection closed [%s] (fd %d)\n", s->host,
		       s->fd);
	    s->close_conn = true;
	    break;
	}
	s->outlen += process_data(buf, bytes, s->outbuf + s->outlen);

	log_transmission(LL2, s->outbuf, bytes, s->fd, s->fd_out, LOG_R);
    }
}

static void read_and_escape_iac(session_data *s)
{
    char buf[BUFFER_SIZE];

    while (s->outlen < (ssize_t)sizeof(s->outbuf)) {
	ssize_t bytes;

	bytes = read(s->fd, buf, sizeof(s->outbuf) - s->outlen);
	if (bytes == -1) {
	    if (errno != EWOULDBLOCK) {
		log_printf(LL0, "read on socket %d failed: %s\n", s->fd,
			   strerror(errno));
		s->close_conn = true;
	    }
	    break;
	}

	if (bytes == 0) {
	    log_printf(LL0, "Connection closed [%s] (fd %d)\n", s->host,
		       s->fd);
	    s->close_conn = true;
	    break;
	}
	s->outlen += escape_iac(buf, bytes, s->outbuf + s->outlen);

	log_transmission(LL2, s->outbuf, bytes, s->fd, s->fd_out, LOG_R);
    }
}

/* Write all data in the output buffer to the destination socket */
void write_data(session_data *s)
{
    while(s->outlen) {
	ssize_t sent = write(s->fd_out, s->outbuf, s->outlen);
	if (sent == -1) {
	    log_printf(LL0, "write to socket %d failed: %s\n", s->fd_out,
		       strerror(errno));
	    s->close_conn = true;
	    break;
	}
	log_transmission(LL2, s->outbuf, sent, s->fd, s->fd_out, LOG_W);
	if (sent < s->outlen) {
	    memmove(s->outbuf, s->outbuf + sent, s->outlen - sent);
	    s->outlen -= sent;
	} else {
	    assert(sent == s->outlen);
	    s->outlen = 0;
	}
    }
}

/**************************************************************************/
/* Signals handling */

static bool got_sighup = false;
static bool got_sigint = false;

static void handle_signals(int sig)
{
    sigset_t pending;

    switch (sig) {
    case SIGHUP: got_sighup = true; break;
    case SIGINT: got_sigint = true; break;
    default: assert(false); break;
    }

    sigpending(&pending);
    if (sigismember(&pending, SIGHUP)) {
	got_sighup = true;
    }
    if (sigismember(&pending, SIGINT)) {
	got_sigint = true;
    }
}

static void setup_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_signals;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    got_sighup = false;
    got_sigint = false;

#if 1
    signal(SIGCHLD, SIG_IGN);
    //    signal(SIGCHLD, client_exited);
#endif
}

void cleanup_and_exit(daemon_data *data)
{
    log_printf(LL0, "Cittadaemon stopped - graceful exit.\n");
    stop_log();

    /* Clean up all of the sockets that are open */
    for (int i = 0; i < data->nfds; i++) {
	if (data->fds[i].fd >= 0) {
	    session_close_fd(data, i);
	}
    }
}

/***************************************************************************/
/* Daemon */

static int daemon_init(daemon_config *config)
{
    int port = config->port;
    int fd = -1, on = 1;

    log_printf(LL0, "Cittadaemon - Socket will listen on port %d\n", port);

    /* Get a socket descriptor representing an endpoint */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
	log_perror(LL0, "socket()");
	return -1;
    }
    /* Allows to restart a closed/killed process on the same local address
     * before the required wait time ( https://stackoverflow.com/a/577905 ). */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	log_perror(LL0, "setsockopt()");
	return -1;
    }
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    enable_socket_blocking(fd, false);

    // Get a unique name for the socket
    struct sockaddr_in servaddr = {
	    .sin_family = AF_INET,
	    .sin_port = htons(port),
	    .sin_addr = { htonl(INADDR_ANY) },
    };

    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	perror("bind() error");
	return -1;
    }

    // Allow the server to accept incoming connections
    if (listen(fd, MAX_PENDING) < 0) {
	perror("DAEMON listen() error");
	return -1;
    }

    /* fork in background */
    if (config->do_fork && fork() != 0) {
	/* parent */
	fprintf(stdout, "Cittadaemon forked.\n");
	exit(EXIT_SUCCESS);
    }

    /* Set the close-on-exec flag for listen_fd, so that it gets closed
     * when the process or any children call an exec*() function.       */
    fcntl(config->listen_fd, F_SETFD, FD_CLOEXEC);

    /* become session leader */
    setsid();

#if 0
#define MAX_CLOSE_FD 256
    for (int i = 0; i < MAX_CLOSE_FD; i++) {
	if (i != fd && i != fd_log) {
	    close(i);
	}
    }
#endif
    config->listen_fd = fd;

    return config->listen_fd;
}

#define MS_TO_NS  1000000L
#define SEC_TO_NS 1000000000L
static void daemon_loop(daemon_config config, daemon_data data)
{
    int timeout_ms = 0; //60*1000;
    int fd_socket = config.listen_fd;
    long ns_per_cycle = SEC_TO_NS / CYCLES_PER_SEC;

    /* set up the initial listening socket */
    session_add(&data, SESS_LISTEN, fd_socket, -1, 0, true);

    clock_t t_start_time = clock();
    clock_t t_last_cycle = t_start_time;
    uint32_t cycle_number = 0;
    do {
	clock_t t_start_cycle = clock();
	//long last_cycle_duration = t_last_cycle - t_start_cycle;
	//clock_gettime();

	/* collect zombie child if there is one */
	waitpid(-1, NULL, WNOHANG);

	log_printf(LL3, "Polling (%d sockets, listen_fd %d)\n", data.nfds,
		   fd_socket);
	int num_ready = poll(data.fds, data.nfds, timeout_ms);
	if (num_ready == 0) { /* poll timed out */
	    goto TIMING;
	} else if (num_ready == -1) {
	    if (errno == EINTR) {
		log_printf(LL1, "poll() incoming signa: stop server.\n");
		data.stop_server = true;
	    } else {
		log_perror(LL0, "Fatal - poll()");
	    }
	    break;
	}
	log_printf(LL3, "%d descriptors are readable.\n", num_ready);

	int num_sockets = data.nfds;
	for (int i = 0; i < num_sockets; ++i) {
	    assert(data.fds[i].fd != -1);
	    if (data.fds[i].revents == 0) {
		goto TIMING;
	    }

	    short events = POLLIN|POLLOUT|POLLHUP;
	    if (data.fds[i].revents & ~events) {
		log_printf(LL0,
			   "Fatal: socket %d unexpected revents = %d\n",
			   data.fds[i].fd, data.fds[i].revents);
		log_revents(LL0, data.fds[i].revents);
		data.stop_server = true;
		break;
	    }

	    if (data.fds[i].revents & POLLHUP) {
		if (data.sessions[i]->close_conn) {
		    goto TIMING;
		}
		log_printf(LL1, "Connection closed [%s] (fd %d) (POLLHUP)\n",
			   data.sessions[i]->host, data.sessions[i]->fd);
		log_revents(LL1, data.fds[i].revents);
		if (data.sessions[i]->outlen) {
		    log_printf(LL1, "%ld bytes yet to be written on %d\n",
			       data.sessions[i]->outlen, data.sessions[i]->fd);
		}
		data.sessions[i]->close_conn = true;
		goto TIMING;
	    }

	}

	/* read available data */
	num_sockets = data.nfds;
	for (int i = 0; i < num_sockets; ++i) {
	    if (data.fds[i].revents & POLLIN) {
		if (data.sessions[i]->type == SESS_LISTEN) {
		    new_connections(&config, &data, i);
		    log_active_sessions(LL1, &data);
		} else {
		    assert(data.sessions[i]->type == SESS_PIPE);
		    /* an existing connection is readable, get the data */
		    log_printf(LL2, "Descriptor %d readable\n",
			       data.fds[i].fd);
		    if (data.sessions[i]->process_CR) {
			read_and_process_data(data.sessions[i]);
		    } else {
			//read_and_escape_iac(data.sessions[i]);
			read_data(data.sessions[i]);
		    }
		}
	    }
	}

	/* write outgoing data */
	num_sockets = data.nfds;
	for (int i = 0; i < num_sockets; ++i) {
	    assert(data.fds[i].fd != -1);
	    if (data.fds[i].revents & POLLOUT) {
		assert(data.sessions[i]->type == SESS_PIPE);
		write_data(data.sessions[i]);
	    }
	}

	/* close connections */
	for (int i = 0; i < num_sockets; ++i) {
	    if (data.sessions[i]->close_conn) {
		session_close(&data, i);
	    } else {
		assert(data.fds[i].fd != -1);
	    }
	}
	cleanup_sessions(&data);
	log_active_sessions(LL3, &data);

	if (got_sighup || got_sigint) {
	    log_printf(LL0, "Got hup/int signal\n");
	    data.stop_server = true;
	}

    TIMING:
	{};
	clock_t t_end_update = clock();
	long ns_elapsed = (t_end_update - t_start_cycle)*SEC_TO_NS
	    / CLOCKS_PER_SEC;
	if (ns_elapsed < ns_per_cycle) {
	    struct timespec delta = {
		.tv_sec = 0,
		.tv_nsec = ns_per_cycle - ns_elapsed,
	    };
	    //log_printf(LL0, "nap %f ms\n", delta.tv_nsec / (float)MS_TO_NS);
	    nanosleep(&delta, NULL);
	} else {
	    log_printf(LL0, "Cycle %ld too long (%f ms)\n", cycle_number,
		       ns_elapsed / (float)MS_TO_NS);
	}
	cycle_number++;
	t_last_cycle = t_start_cycle;
    } while (!data.stop_server);
}

int main(int argc, char *argv[])
{
    daemon_data data = {0};
    daemon_config config = process_args(argc, argv);
    init_log(&config);
    log_argv(LL1, "Remote program:", config.client_argv);

    int listen_fd = daemon_init(&config);
    if (listen_fd == -1) {
	log_printf(LL0, "Fatal: Socket init to listen on port %d failed.\n",
		   config.port);
	exit(EXIT_FAILURE);
    }
    log_printf(LL1, "Listening on port %d with socket fd=%d .\n", config.port,
	       listen_fd);

    setup_signals();

    daemon_loop(config, data);
    cleanup_and_exit(&data);

    return EXIT_SUCCESS;
}
