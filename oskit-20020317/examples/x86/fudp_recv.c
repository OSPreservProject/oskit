/*
 * Copyright (c) 1998 The University of Utah and the Flux Group.
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
 * Part of the `fudp' example, which see.
 * This is a Unix program.
 * Waits on a port, specified on the command line,
 * for UDP packets from `PORT' and prints out the contents until
 * a NUL char is seen.
 *
 * Be sure to configure the first part of this file appropriately.
 * Also, the port you pass on the command line must match what is in
 * fudp.c.
 */

/*
 * Configure appropriately.
 */
#define PORT 10000			/* must match fudp.c */

/*======================================================================*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

static	char buf[BUFSIZ];
static	int on = 1;

int
main(int argc, char *argv[])
{
	int	i, done;
	int	sd;
	struct	sockaddr_in sin;
	struct	sockaddr_in rsin;
	int	rsinlen;
	int	port;
	int	rport;
	int	bufsize;

	setbuf(stdout, 0);

	/* Get port. */
	if (argc < 2)
		fprintf(stderr, "Usage: %s port\n", argv[0]), exit(1);
	port = atoi(argv[1]);

	/* Create the socket. */
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Fatal: socket");
		exit(1);
	}
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(int)) != 0)
		perror("setsockopt SO_REUSEADDR");

	/* Bind the socket to port. */
	memset(&sin, 0, sizeof(sin));
	sin.sin_len	    = htons(sizeof(sin));
	sin.sin_family	    = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port	    = htons(port);
	if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("Fatal: bind");
		exit(1);
	}

	/* Print out what we get until we get a NUL char. */
	done = 0;
	while (!done) {
		rsinlen = sizeof rsin;
		bufsize = recvfrom(sd, buf, BUFSIZ, 0,
				   (struct sockaddr *)&rsin,
				   &rsinlen);
		if (bufsize < 0) {
			perror("recvfrom");
			continue;
		}

		rport = ntohs(rsin.sin_port);
		if (rport != PORT) {
			printf("badport %d\n", rport);
			continue;
		}

		for (i = 0; i < bufsize; i++) {
			if (buf[i] == '\0') {
				putchar('\n');
				done = 1;
			}
			else
				putchar(buf[i]);
		}
	}
	return 0;
}
