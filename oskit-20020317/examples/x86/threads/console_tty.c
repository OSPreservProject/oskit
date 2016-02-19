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
 * Simple program to demonstrate the use of the tty drivers as the
 * console interface. After initializing the serial console device,
 * have the C library reconstitute its notion of the stdio streams.
 *
 * It prints out some stuff to the system console and then echos
 * characters back until it sees ^D (EOF).
 *
 * NOTE: There is a nasty problem with the TTY libraries. They require the
 * process lock, but many of the other device libraries take the process
 * lock and then call printf, which results in a recursive process lock
 * panic. Not sure what to do about that.
 */

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/file.h>	/* XXX OSKIT_O_RDWR */
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/dev/osenv.h>
#include <oskit/c/string.h>

#define DISKNAME "wd1"
#define PARTITION "b"

int
main(int argc, char **argv)
{
	int rc;
	char buf[128];
	fd_set  in;
	oskit_osenv_t *osenv;
	struct timeval timeout;

	/*
	 * Init the OS.
	 */
	oskit_clientos_init_pthreads();
	osenv = start_osenv();
	start_clock();
	start_pthreads();
	oskit_dev_init(osenv);
	/* This is the console TTY device. A serial line. */
	oskit_freebsd_init_osenv(osenv);
	oskit_freebsd_init_sio();
	/* Note that process lock is held around device startup */
	osenv_process_lock();
	start_devices();
	osenv_process_unlock();
	start_fs_pthreads(DISKNAME, PARTITION);
	start_console_pthreads();

	printf("\nOkay, now type stuff and it will echo. Type ^D to exit\n");
	
	while (1) {
		FD_ZERO(&in);
		FD_SET(0, &in);
		timeout.tv_sec  = 5;
		timeout.tv_usec = 0;
		
		if ((rc = select(1, &in, 0, 0, &timeout)) == 0) {
			printf("\nTimed out! Trying again\n");
			continue;
		}

		if (fgets(buf, 128, stdin) == NULL)
			break;
		
		printf("%s", buf);
	}

	return 0;
}
