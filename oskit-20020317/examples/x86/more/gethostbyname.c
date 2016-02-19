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
 * Resolves some hostnames.
 */

#ifdef OSKIT
#include <oskit/dev/dev.h>
#include <oskit/dev/linux.h>
#endif /* OSKIT */
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <fcntl.h>
#include <oskit/startup.h>
#include <oskit/fs/memfs.h>
#include <oskit/c/fs.h>
#include <oskit/clientos.h>

int
resolve(char *b)
{
	struct hostent *he;
	printf("resolving %s...", b);
	fflush(stdout);
	he = gethostbyname(b);
	if (he) {
		printf("done.\n");
		printf("%s -> %s\n", b, inet_ntoa(*((long *)he->h_addr)));
		return 0;
	} else {
		printf("failed.\n");
		return -1;
	}
}

int
cat(char *fname)
{
    char buf[256];
    int r, fd = open(fname, O_RDONLY);
    if (fd == -1)
	return -1;

    #define W(f, s)	write(f, s, strlen(s))
    W(1, "catting ");
    W(1, fname);
    W(1, "\n");
    while ((r = read(fd, buf, sizeof buf)) > 0)
	    write(1, buf, r);
    close(fd);
    return 0;
}

int
main()
{
#ifdef OSKIT
	oskit_clientos_init();
	start_fs_bmod();
	start_clock();
	start_network();
#endif
#if 0
	/* check that unsupported/bootp.c wrote the correct files */
	cat("/etc/host.conf");
	cat("/etc/resolv.conf");
	cat("/etc/hosts");
#endif
	resolve("marker");
	resolve("fast.cs.utah.edu");
	resolve("jackal");
	resolve("localhost");
	resolve("155.99.214.165");
	return 0;
}

#ifdef OSKIT
static void my_syslog(int pri, const char *fmt, ...)
{
        va_list args;

        va_start(args, fmt);
        osenv_log(pri, fmt, args);
        va_end(args);
}

oskit_syslogger_t oskit_libc_syslogger = my_syslog;
#endif
