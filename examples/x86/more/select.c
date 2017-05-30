/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Program demonstrating select().
 *
 * It waits on port 2000 and sends data from one client to all the others,
 * sort of like a cheap chat thing.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#ifdef OSKIT
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#include <oskit/dev/freebsd.h>
#include <oskit/net/freebsd.h>
#include <oskit/startup.h>
#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <oskit/clientos.h>

/* XXX */
oskit_syslogger_t	oskit_libc_syslogger = osenv_log;

#endif

/* //////////////////////////////////////////////////////////////////////////
////    hacks
/////////////////////////////////////////////////////////////////////////  */

static long getaddrbyname(char *name)
{
    struct hostent *he = gethostbyname(name);
    if(!he)
        perror(name);
    return he ? *(long*)*he->h_addr_list : 0;
}

static char *getnamebyaddr(long addr)   /* in network order !!! */
{
    struct in_addr inadd;
    inadd.s_addr = addr;
#if OSKIT
    /* don't do DNS for now! */
    return inet_ntoa(inadd);
#else
    struct hostent *he;
    he = gethostbyaddr((char*)&inadd, sizeof(inadd), AF_INET);
    return he ? (char *)he->h_name : inet_ntoa(inadd);
#endif
}

#define CHECK(x)        { if (-1 == (x)) { perror(#x); \
        printf(__FILE__":%d in "__FUNCTION__"\n", __LINE__); exit(-1); } }

#define N 32

int
main(int ac, char *av[])
{
    struct  sockaddr_in addr;
    fd_set	in;
    int 	namelen, i, n, s, fd[N];
    int 	on = 1;

#ifdef OSKIT
    oskit_clientos_init();
    start_fs_bmod();
    start_clock();
    start_network();		/* does start_devices: probe, etc */
#endif

    memset(fd, 0, sizeof fd);
    CHECK(s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
    fd[s] = 1;

    /* fill in the sin_addr structure */
    memset(&addr, 0, namelen = sizeof(addr));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2000);

    CHECK(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)))
    CHECK(bind(s, (struct sockaddr *)&addr, sizeof(addr)))
    CHECK(listen(s, 10))

    for (;;) {
	struct timeval to = { 10, 0 };	/* timeout in 20 seconds */
	FD_ZERO(&in);
	for (n = 0; n < N; n++)
	    if (fd[n])
		FD_SET(n, &in);
	n = select(N, &in, 0, 0, &to);
	if (n == 0) {
		char *msg = "Is there anybody out there?\n";
		printf("select timed out...\n");
		for (i = 0; i < N; i++)
		    if (i != s && fd[i])
			write(i, msg, strlen(msg)+1);
		continue;
	}
	for (n = 0; n < N; n++) {
	    if (FD_ISSET(n, &in)) {
		if (n != s) {
		    char buf[256];
		    int r;
		    r = read(n, buf, sizeof buf);
		    if (r <= 0) {
			close(n);
			fd[n] = 0;
			continue;
		    }
		    if (!strncasecmp(buf, "SHUTDOWN", 8)) {
			for (i = 0; i < N; i++)
			    if (fd[i])
				fd[i] = 0, close(i);
			exit(0);
		    }
		    for (i = 0; i < N; i++)
			if (fd[i])
			    write(i, buf, r);
		} else {
		    int ns;
		    printf("accepting client\n");
		    CHECK(ns = accept(s, (struct sockaddr *)&addr, &namelen))
		    printf("accepted client from %s\n",
				getnamebyaddr(addr.sin_addr.s_addr));
		    fd[ns] = 1;
		}
	    }
	}
    }
}
