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
 * Part of the hpfq_udp example, which see.
 * This is a Unix program.
 * Waits on port, specified on the command line,
 * for UDP packets from ports in the range [RPORTBASE, RPORTBASE+NPORTS]
 * and keeps track of how many packets are received.
 * It dumps out stats each second showing how many packets were received
 * from each port.
 * The reported stats should look something like the following for the
 * default tree in hpfq_udp.c:
 *
 *	1: 282 (37%)
 *	2: 95 (12%)
 *	3: 376 (49%)
 *	--
 *	1: 285 (37%)
 *	2: 94 (12%)
 *	3: 380 (50%)
 *	--
 *	...
 *
 * This program runs until interrupted.
 *
 * Note: if the percentages reported are all equal,
 * but the tree used by hpfq_udp is not like that,
 * then it is likely that the network sending overhead is smaller than the
 * link scheduling overhead.
 * This can happen with small frame sizes on FastEthernet, but shouldn't
 * for the maximum Ethernet frame size.
 * The H-PFQ library is not heavily tuned.
 *
 * Be sure to configure the first part of this file appropriately.
 * Also, the port specified on the command line must agree with what
 * hpfq_udp.c sends to.
 */

/*
 * Configure appropriately.
 * These values must agree with hpfq_udp.c.
 */
#define RPORTBASE 10000
#define NPORTS 10

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
static	unsigned stats[NPORTS];
static	unsigned packets_received = 0;

static void
dumpstats()
{
	int i, gotsome;
	static unsigned lonely_nights;
	unsigned total;

	gotsome = 0;
	total = 0;
	for (i = 0; i < NPORTS; i++)
		total += stats[i];
	for (i = 0; i < NPORTS; i++)
		if (stats[i]) {
			printf("%d: %d (%d%%)\n",
			       i, stats[i], (int)100.0*stats[i]/total);
			gotsome = 1;
		}
	if (gotsome) {
		printf("--\n");
		lonely_nights = 0;
	}
	else
		lonely_nights++;
	if (lonely_nights == 2) {
		printf("%d packets received\n", packets_received);
		packets_received = 0;
	}
	memset(stats, 0, sizeof stats);
}


/*
 * Try to set the socket receive buffer to the maximum size.
 */
#include <sys/sysctl.h>

static void
setup_recvbuf(int sd)
{
	int max;
	size_t len, len_actual;
	int mib[3];

#if defined(KERN_MAXSOCKBUF)
	mib[0] = CTL_KERN;
	mib[1] = KERN_MAXSOCKBUF;
	len = 2;
#elif defined(KIPC_MAXSOCKBUF)
	mib[0] = CTL_KERN;
	mib[1] = KERN_IPC;
	mib[2] = KIPC_MAXSOCKBUF;
	len = 3;
	/* XXX the above is incorrect somehow, fake it. */
	max = 8192;
	len_actual = sizeof(max);
	goto doit;
#else
	perror("cannot determine max sockbuf");
	return;
#endif
	if (sysctl(mib, len, &max, &len_actual, NULL, 0) != 0) {
		perror("sysctl MAXSOCKBUF");
		return;
	}
 doit:
	if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (void *)&max, len_actual) != 0)
		perror("setsockopt SO_RCVBUF");
}


int
main(int argc, char *argv[])
{
	struct	itimerval itval;
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
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(int));
	setup_recvbuf(sd);

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

	/* Set up dumpstats to run every sec. */
	signal(SIGALRM, dumpstats);
	memset(&itval, 0, sizeof itval);
	itval.it_interval.tv_sec = itval.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &itval, NULL);

	for (;;) {
		rsinlen = sizeof rsin;
		bufsize = recvfrom(sd, buf, BUFSIZ, 0,
				   (struct sockaddr *)&rsin,
				   &rsinlen);
		if (bufsize < 0) {
			perror("recvfrom");
			continue;
		}

		rport = ntohs(rsin.sin_port);
		if (rport < RPORTBASE || rport - RPORTBASE >= NPORTS) {
			printf("badport %d\n", rport);
			continue;
		}
		stats[rport - RPORTBASE]++;
		packets_received++;
	}
	return 0;
}
