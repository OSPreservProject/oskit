/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * Switch to a real tty driver for the console. If we find a tty driver
 * to use, get it ready to use as a console device. Then ask the C
 * library (well, POSIX library) to replace its notion of the console
 * with the new one.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/file.h>	/* XXX OSKIT_O_RDWR */
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/io/asyncio.h>
#include <oskit/io/posixio.h>
#include <oskit/clientos.h>

#ifdef PTHREADS
#include <oskit/com/wrapper.h>
#include <oskit/threads/pthread.h>

#define	start_console		start_console_pthreads
#endif

/*
 * Default settings for the console TTY.
 */
#define	TTYDEF_IFLAG \
	(OSKIT_BRKINT|OSKIT_ICRNL|OSKIT_IXON|OSKIT_IXANY)
#define TTYDEF_OFLAG \
	(OSKIT_OPOST|OSKIT_ONLCR)
#define TTYDEF_LFLAG \
	(OSKIT_ECHO|\
	 OSKIT_ICANON|OSKIT_ISIG|OSKIT_IEXTEN|OSKIT_ECHOE|OSKIT_ECHOK)
#define	TTYDEF_CFLAG \
	(OSKIT_CREAD|OSKIT_CS8|OSKIT_HUPCL)

void
start_console(void)
{
	oskit_ttydev_t		**ttydev;
	oskit_ttystream_t	*ttystrm;
	oskit_termios_t		termios;
	int			rc, ndev, i, enabled;

	osenv_process_lock();
	/*
	 * Find all TTY device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_ttydev_iid, (void***)&ttydev);
	if (ndev <= 0)
		panic("no TTY devices found!");

	/*
	 * Open the first TTY device found. I am going to say thats
	 * the console device the application wanted to use.
	 */
	rc = oskit_ttydev_open(ttydev[0], OSKIT_O_RDWR, &ttystrm);
	if (rc)
		panic("unable to open tty0: %d", rc);

	/*
	 * Okay, release our references.
	 */
	for (i = 0; i < ndev; i++)
		oskit_ttydev_release(ttydev[i]);

	/*
	 * Set properties for a normal tty.
	 */
	rc = oskit_ttystream_getattr(ttystrm, &termios);
	if (rc)
		panic("unable to getattr: %d", rc);
	
	termios.iflag = TTYDEF_IFLAG;
	termios.oflag = TTYDEF_OFLAG;
	termios.cflag = TTYDEF_CFLAG;
	termios.lflag = TTYDEF_LFLAG;
	termios.cc[3] = 8; /* ^H */
	
	rc = oskit_ttystream_setattr(ttystrm, OSKIT_TCSANOW, &termios);
	if (rc)
		panic("unable to setattr: %d", rc);

#ifdef  PTHREADS
	/*
	 * Need to wrap the stream in a thread-safe wrapper.
	 */
	{
		oskit_ttystream_t	*wrapped_ttystrm;
		
		rc = oskit_wrap_ttystream(ttystrm,
			  (void (*)(void *))osenv_process_lock, 
			  (void (*)(void *))osenv_process_unlock,
			  0, &wrapped_ttystrm);

		if (rc)
			panic("oskit_wrap_ttystream() failed: "
			      "errno 0x%x\n", rc);

		/* Don't need the original anymore, the wrapper has a ref. */
		oskit_ttystream_release(ttystrm);

		ttystrm = wrapped_ttystrm;
	}
#endif
        osenv_process_unlock();

	/*
	 * Okay, now replace the default console stream.
	 */
	oskit_clientos_setconsole(ttystrm);

	/*
	 * Release our reference since setconsole took one.
	 */
	oskit_ttystream_release(ttystrm);

	enabled = osenv_intr_save_disable();

	/*
	 * Okay, tell the C library to switch over!
	 */
	printf("Reconstituting stdio streams to console TTY...\n");
	oskit_console_reinit();

	if (enabled)
		osenv_intr_enable();
}
