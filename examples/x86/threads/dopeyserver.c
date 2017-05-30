/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * This program is the server side of the disknet.c test program. Its not
 * an OSKIT program, but is intended to run on some regular BSD machine.
 * It could probably be an OSKIT program if it were changed to use pthreads
 * instead of fork.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <time.h>

int		main_socket;
int		create_listener_socket(int port);
void		dowork(int sock);

void
main()
{
	int		sock, length, pid;
	struct sockaddr client;

	main_socket = create_listener_socket(6969);
	srand(26812);

	while (1) {
		sock = accept(main_socket, &client, &length);

		if (sock < 0) {
			perror("accept");
			exit(1);
		}

		printf("New connection on sock: %d\n", sock);

		if ((pid = fork()) < 0) {
			perror("fork");
			close(sock);
		}
		if (pid == 0) {
			dowork(sock);
			exit(0);
		}
		close(sock);
	}
}

void
dowork(int sock)
{
	int		data, count, len, c;
	char		*buf, *bp;
	struct timeval	timeout;
	
	data = 1024 * 32;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &data, sizeof(data)) < 0) {
		perror("setsockopt SO_SNDBUF");
		exit(1);
	}

	/*
	 * The first word of data is the amount.
	 */
	if ((c = read(sock, &len, sizeof(len))) <= 0) {
		if (c < 0) {
			perror("dowork: read1");
			exit(1);
		}
	}
	len = ntohl(len);

	printf("len = %d bytes\n", len);
	
	/*
	 * Create a buffer.
	 */
	if ((buf = calloc(1, len)) == NULL) {
		printf("Out of memory\n");
		exit(1);
	}

	while (1) {
		/*
		 * Wait for data to come.
		 */
		bp    = buf;
		count = len;
		while (count) {
			c = (count < 0x2000) ? count : 0x2000;

			if ((c = read(sock, bp, c)) <= 0) {
				if (c < 0)
					perror("dowork: read 2");
				goto done;
			}
			count -= c;
			bp    += c;
		}

		/*
		 * Send it back
		 */
		bp    = buf;
		count = len;
		while (count > 0) {
			c = (count < 0x2000) ? count : 0x2000;
			
			if ((c = write(sock, bp, c)) <= 0) {
				if (c < 0)
					perror("dowork: write");
				goto done;
			}
			count -= c;
			bp   += c;
			timeout.tv_sec  = 0;
			timeout.tv_usec = 10000;
			select(0, 0, 0, 0, &timeout);
		}
	}
   done:
	printf("exiting sock: %d\n", sock);
	free(buf);
	close(sock);
}

/*
 * This routine creates a datagram socket, binds a name to it, then reads
 * from the socket.
 */
int
create_listener_socket(int port)
{
	int sock, length, data;
	struct sockaddr_in name;

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	
	data = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &data, sizeof(data))
	    < 0) {
		perror("setsockopt");
		exit(1);
	}

	/* Create name with wildcards. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		perror("binding datagram socket");
		exit(1);
	}
	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		perror("getting socket name");
		exit(1);
	}
	printf("Socket has port #%d\n", ntohs(name.sin_port));

	if (listen(sock, 20) < 0) {
		perror("listen on socket");
		exit(1);
	}

	return sock;
}
