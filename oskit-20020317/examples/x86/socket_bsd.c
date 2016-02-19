/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * This is a test program for the libc socket implementation based on
 * the TCP/IP stack.
 *
 * It first does two fingers and then waits for incoming date requests.
 *
 * The -s <x> parameter sets the timeout for waiting for date requests
 * to <x>.
 */
#if OSKIT
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/dev/ethernet.h>
#include <oskit/net/freebsd.h>
#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include "bootp.h"

char GATEWAY[20];
char NETMASK[20];

#define IFNUM		0

#endif /* OSKIT */

char IPADDR[256];

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/*
 * this is a simple finger client, programmed to the libbsdnet socket interface
 */
void
fingerclient(char *name, char *host)
{
	int	so;
	int 	namelen, retval, msg_len;
	struct  sockaddr_in addr;
	char    message[256];

	if ((so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(79);

	if ((-1 == connect(so, (struct sockaddr *)&addr, sizeof(addr)))) 
	{
		perror("connect");
		return;
	}

	sprintf(message, "%s\r\n", name);
	retval = write(so, message, msg_len = strlen(message));
	if (retval < 0) {
		perror("write");
		return;
	} else {
		printf("write %d out of %d bytes\n", retval, msg_len);
	}

	do {
		retval = read(so, message, sizeof(message));
		if (retval >= 0) {
			int i;
			for (i = 0; i < retval; i++)
				putchar(message[i]); 
		} else {
			perror("read");
			return;
		}
	} while (retval > 0);

	if ((-1 == close(so))) 
		perror("close");
}

volatile int stop = 0;

/* ARGSUSED */
void timeout(int signal)
{
	stop = 1;
}

/*
 * this is a simple server responding to requests on a given port (daytime)
 */
void
dateserver(short port, int seconds)
{
	int 	so, newso;
	int 	namelen;
	struct  sockaddr_in addr;
	struct  itimerval expire;
	struct  timeval time;

	if ((so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		return;
	}

	/* fill in the sin_addr structure */
	memset(&addr, 0, namelen = sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if ((-1 == bind(so, (struct sockaddr *)&addr, sizeof(addr)))) {
		perror("bind");
		return;
	}

	if ((-1 == listen(so, 5))) {
		perror("listen");
		return;
	}

	signal(SIGALRM, timeout);
	memset(&expire, 0, sizeof expire);
	expire.it_value.tv_sec = seconds;
	setitimer(ITIMER_REAL, &expire, NULL);

	for (;!stop;) {
		char * time_message;
		int  msg_len, written;

		if ((newso = accept(so, (struct sockaddr *)&addr, 
				&namelen)) == -1) 
		{
			if (errno == EINTR)
				break;
			perror("accept");
			return;
		}

		if (gettimeofday(&time, 0) < 0) {
			perror("gettimeofday");
			return;
		}
		time_message = asctime(localtime(&time.tv_sec));	
		msg_len = strlen(time_message);

		written = write(newso, time_message, msg_len);
		if (written < 0)
		{
			perror("write");
			return;
		} else {
			printf("write %d out of %d bytes\n", written, msg_len);
		}

		if ((-1 == close(newso))) {
			perror("close");
			return;
		}
	}
	printf("bailing out of loop...\n");
	close(so);
}

/*
 * the initialization follows the ping_reply example...
 */
int
main(int argc, char **argv)
{
	short port = 3000;
	int dateserverseconds = 60;

#ifndef KNIT
#ifdef OSKIT
#ifdef OSKIT_UNIX
	/*
	 * OSKit/unix needs the threaded_fd support in order for the
	 * timeout (alarm) in dateserver to work.
	 */
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();

	/*
	 * Build using emulated filesystem/sockets, or real drivers.
	 * The real drivers require a second ethernet interface (you
	 * must setenv ETHERIF and ETHERADDR), and a raw disk
	 * partition (either a real one or a pseudo one created with
	 * the vnconfig tools).
	 */
#ifdef REALSTUFF
	start_fs_pthreads("/dev/oskitfs", 0);
	start_network_pthreads();
#else
	start_fs_native_pthreads("/tmp");
	start_network_native_pthreads();
#endif
#else
	oskit_clientos_init();
	start_clock();
	start_fs_bmod();
	start_network();
#endif /* OSKIT_UNIX */
#endif /* OSKIT */
#endif /* !KNIT */

	/*
	 * Accept -s command line parameter for setting
	 * dateserverseconds.
	 */
	if (argc > 1) {
		if (!strcmp(argv[1],"-s")) {
			dateserverseconds = atoi(argv[2]);
		}
	}

	/*
	 * if you want to test pingreply, choose the first piece of code
	 */
#if 0
	fingerclient("oskit", "155.99.212.1"); /* fast.cs.utah.edu */
	fingerclient("ftp", "155.99.212.1"); 	/* fast */
#endif
	printf("responding to date requests for %d seconds\n", dateserverseconds);

	gethostname(IPADDR, sizeof IPADDR);

	printf("type 'telnet %s %d' to test\n", IPADDR, port);
	dateserver(port, dateserverseconds);

	exit(0);
}
