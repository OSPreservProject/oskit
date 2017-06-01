/*
 * Copyright (c) 1997-1998, 2000, 2001 University of Utah and the Flux Group.
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
#include <oskit/console.h>
#include <oskit/dev/dev.h>

#include "native.h"
#include "support.h"

extern int threads_initialized;  /* from threads/pthread_init.c */

/*
 * On normal exit, turn off NBIO/ASYNC mode so that the emacs shell does
 * not go nuts.
 */
void
oskitunix_stdio_exit()
{
	/*
	 * Ignore any error on fixing up fd 0.  We're already
	 * exiting at this point.
	 */
	oskitunix_unset_async_fd(0);
}

/*
 * Why is there a SIGINT catcher? Well, interrupting a process with an
 * NBIO and/or ASYNC stdin/stdout while running inside an emacs shell
 * buffer causes that shell to go nuts. This is almost certainly a feature
 * of FreeBSD emacs or the shell, but I'm not inclined to go find out why ...
 */
void
oskitunix_stdio_interrupt()
{
	oskitunix_stdio_exit();
	_exit(1);
}

/*
 * Put stdin in NBIO/ASYNC mode so that console input does not block
 * in the kernel. We want to use the threaded_fd stuff, but that uses
 * osenv_sleep, which requires that the process lock is taken. Well,
 * the stdio functions are not provided as a COM object that can be
 * wrapped like other device drivers, so the process lock *might not*
 * be taken when we get here. So, take and release the lock as
 * necessary so that we can use the already existing threaded_fd
 * stuff. What a hack!
 *
 * stdout is left in blocking mode since printf is called from interrupt
 * level all over the place, so we cannot allow a context switch to happen.
 */
void
oskitunix_stdio_init()
{
    	struct sigaction sa;
	sigset_t set;

	NATIVEOS(sigemptyset)(&set);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags   = 0;
	sa.sa_mask    = set;

	if (NATIVEOS(sigaction)(SIGIO, &sa, 0) < 0) {
		oskitunix_perror("installing SIGIO handler");
		exit(0);
	}

	/*
	 * Ignore any errors on making fd 0 async.  Such errors merely
	 * indicate that the native OS guarantees that fd 0 will never
	 * block a read (e.g., its been re-directed from /dev/null).
	 */
	oskitunix_set_async_fd(0);

	oskitunix_set_signal_handler(SIGINT,
		(void (*)(int,int,struct sigcontext*))oskitunix_stdio_interrupt);
}

/* Supply console routines normally pulled out of libkern. */

int
console_puts(const char *str)
{
	int	rc;
	int	len = strlen(str);

	if ((rc = console_putbytes(str, len)) == EOF)
		return EOF;
	else
		return console_putchar('\n');
}

int
console_putchar(int c)
{
	unsigned char   foo = (unsigned char) c;

	return console_putbytes(&foo, 1);
}

int
console_getchar()
{
	int		rc;
	unsigned char   foo;

	if (! threads_initialized) {
		rc = NATIVEOS(read)(0, &foo, 1);
	}
	else {
		int locked = osenv_process_locked();

		if (! locked)
			osenv_process_lock();

		rc = oskitunix_threaded_read(0, &foo, 1);

		if (! locked)
			osenv_process_unlock();
	}
	if (rc <= 0)
		return EOF;
	else
		return (int) foo;
}

/*
 * Added to improve console output performance.
 */
int
console_putbytes(const char *str, int len)
{
	int	rc = 0;
	int	enabled;

	enabled = osenv_intr_save_disable();

	while (len) {
		rc = NATIVEOS(write)(1, str, len);

		if (rc < 0) {
			if (errno == EAGAIN)
				continue;
			break;
		}

		len -= rc;
		str += rc;
	}

	if (enabled)
		osenv_intr_enable();

	if (rc < 0)
		return EOF;
	else
		return 0;
}

void
base_console_init(int argc, char **argv)
{
    console_putbytes("base_console_init called!",
		     strlen("base_console_init called!"));
}
