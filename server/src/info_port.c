/*
 *  Copyright (C) 1999-2000 by Marco Caldarelli and Riccardo Vianello
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
* File : info_port.c                                                        *
*        Gestisce la porta di informazioni di Cittadella/UX                 *
****************************************************************************/
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "cittaserver.h"
#include "info_port.h"

#define LISTENQ 10
#define LBUF 256

pthread_t tid;

/* Prototipi funzioni in questo file */
void info_port(void);
void close_info_port(void);
static void * listen_info_port(void *arg);
static void write_infoport(int c);

/***************************************************************************/
void info_port(void)
{
	pthread_attr_t attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr,
			   (void * (*)(void *)) listen_info_port, NULL)==0)
		log("INFOPORT thread started.");
	else
		log("INFOPORT cannot create thread.");
}

static void * listen_info_port(void *arg)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	struct timeval tv;
	fd_set rset;
	
/*	pthread_detach(pthread_self()); */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	listenfd = iniz_socket(INFOPORT);
	nonblock(listenfd);
	/*
	if ((listenfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log("INFOPORT cannot create socket.");
		return;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(INFOPORT);
	bind(listenfd, (struct sockaddr_in *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	 */
	for ( ; ; ) {
		pthread_testcancel();
/*
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&rset);
		FD_SET(listenfd, &rset);
		select(listenfd+1, &rset, NULL, NULL, &tv);
		if (FD_ISSET(listenfd, &rset)) {
*/
		connfd = -1;
		connfd = accept(listenfd, (struct sockaddr_in *) NULL,
				NULL);
		if (connfd != -1) {
			write_infoport(connfd);
			close(connfd);
		}
/*		} */
	}
}

void close_info_port(void)
{
/*	pthread_kill_other_threads_np(); */

	if (pthread_cancel(tid)==0)
		log("INFOPORT thread cancelled.");
	else
		log("INFOPORT cannot cancel thread.");
/*
	if (pthread_join(tid, NULL)==0)
		log("INFOPORT thread joined.");
	else
		log("INFOPORT cannot join thread.");
*/
	sleep(1);
}

static void write_infoport(int c)
{
	char nm[LBUF];
        struct sessione *punto;
        const char LC[] = "Lista connessioni (HWHO)\n";
	
        write(c, LC, strlen(LC));
        for (punto = lista_sessioni; punto; punto = punto->prossima) {
		if (punto->logged_in)
			sprintf(nm, "%s\n", punto->utente->nome);
                else
                        sprintf(nm, "(login in corso)\n");
		write(c, nm, strlen(nm));
	}
}

/*
int main(void)
{
	info_port();
	while(1);
}
*/
