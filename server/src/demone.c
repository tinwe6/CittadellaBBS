/*
 *  Copyright (C) 1999-2004 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : demone.c                                                           *
*        Ascolta su una porta e lancia un client remoto per Cittadella.     *
*        In realta' questa e' una versione minimale di inetd/xinetd...      *
****************************************************************************/
#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DEFAULT_LISTEN_PORT 4001
#define MAX_CLOSE_FD 256

/* listen queue length */
#define LISTENQ 10

static void client_exited(int signum) {
    /* collect children exit status to avoid zombies */
    while (wait4((pid_t)-1, NULL, WNOHANG, NULL) >= 0)
	;
}


static void start_client(int fd, char ** argv)
{
    switch (fork()) {
    case 0: /* child */
	
	/* duplicate socket to descriptors 0, 1, 2; close socket */
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	if (fd > 2)
	    close(fd);
	
	execvp(argv[0], argv);

	perror("DAEMON exec(client) failed");
	exit(1);
	break;
	
    case -1: /* error */
	perror("DAEMON fork() failed.\n");
	break;
	
    default: /* parent */
	break;
    }
}


static void usage(char * arg0, char * arg)
{
    if (arg)
	fprintf(stderr, "DAEMON unknown option `%s'\n", arg);
    
    fprintf(stderr, "Usage: %s [-d|--debug] [-p|--port <TCP_port>] -- <client_program_and_args>\n", arg0);
    exit(0);
}


static int daemon_setup(char ** argv, char *** client_argv) {
    struct sockaddr_in servaddr;
    /* skip program name */
    char * arg0 = *argv++, * s;
    
    int listen_fd, do_fork = 1, port = DEFAULT_LISTEN_PORT, i;
    
    while ((s = *argv)) {
	argv++;
	
	if (!strcmp(s, "-d") || !strcmp(s, "--debug"))
	    /* debug -> do not fork */
	    do_fork = 0;
	else if (*argv && (!strcmp(s, "-p") || !strcmp(s, "--port")))
	    /* port to listen at */
	    port = atol(*argv++);
	else if (!strcmp(s, "--"))
	    /* end of daemon options. further args are client program and args */
	    break;
	else
	    usage(arg0, s);
    }
 
    if (!*(*client_argv = argv))
	usage(arg0, NULL);
    
    
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("DAEMON socket() failed");
	return -1;
    }

    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    fprintf(stdout, "DAEMON will listen on port %d\n", port);
    
    if (bind(listen_fd, &servaddr, sizeof(servaddr)) < 0) {
	perror("DAEMON bind() error");
	return -1;
    }
    if (listen(listen_fd, LISTENQ) < 0) {
	perror("DAEMON listen() error");
	return -1;
    }

    /* fork in background */
    if (do_fork && fork() != 0) {
	exit(0);
    }

    /* make listen_fd close-on-exec, i.e. spawned processes will not inherit it */
    fcntl(listen_fd, F_SETFD, FD_CLOEXEC);

    /* become session leader */
    setsid();
    
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, client_exited);
    
    chdir("/");

    for (i = 0; i < MAX_CLOSE_FD; i++)
	if (i != listen_fd)
	    close(i);
    
    return listen_fd;
}


static void daemon_loop(int listen_fd, char ** client_argv)
{
    int ret, client_fd;
    fd_set rset;
    
    
    FD_ZERO(&rset);

    for (;;) {
	FD_SET(listen_fd, &rset);

	while ((ret = select(listen_fd+1, &rset, NULL, NULL, NULL)) < 0 && errno == EINTR)
	    ;
	
	if (ret < 0) {
	    perror("DAEMON select() error");
	    return;
	}
	
	if (FD_ISSET(listen_fd, &rset)) {
	    if ((client_fd = accept(listen_fd, (struct sockaddr_in *) NULL, NULL)) >= 0) {
		start_client(client_fd, client_argv);
		close(client_fd);
	    } else
		perror("DAEMON accept() error");
	}
    }
}



int main(int argc, char ** argv)
{
        char ** client_argv;
        int listen_fd;
    
        if ((listen_fd = daemon_setup(argv, &client_argv)) >= 0)
                daemon_loop(listen_fd, client_argv);
    
        return 0;
}
