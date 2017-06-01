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
 * Example program to test the function of device drivers
 * that implement the oskit_ttydev interface.
 * Specifically the FreeBSD syscons device.
 *
 * It prints out some stuff to the system console and then echos
 * characters back until it sees 'X'.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/file.h>	/* XXX OSKIT_O_RDWR */
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/debug.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/dev/osenv.h>

static char msg[] =
"Hello there!\r\n"
"This is the Flux OS Toolkit's TTY test program.\r\n"
"Press X to exit.\r\n";

int
main(int argc, char **argv)
{
	oskit_ttydev_t **ttydev;
	int ndev;
	oskit_ttystream_t *ttystrm;
	oskit_u32_t actual;
	int rc;
	oskit_osenv_t *osenv;

	oskit_clientos_init();
	osenv = start_osenv();

	printf("Initializing device drivers...\n");

	oskit_dev_init(osenv);
	oskit_freebsd_init_osenv(osenv);
	oskit_freebsd_init_sc();
	oskit_dump_drivers();

	printf("Probing devices...\n");
	oskit_dev_probe();
	oskit_dump_devices();

	/*
	 * Find all TTY device nodes.
	 */
	ndev = osenv_device_lookup(&oskit_ttydev_iid, (void***)&ttydev);
	if (ndev <= 0)
		panic("no TTY devices found!");

	/*
	 * Open the first TTY device found, which should be the display.
	 * because that is the only one I initialized.
	 */
	rc = oskit_ttydev_open(ttydev[0], OSKIT_O_RDWR, &ttystrm);
	if (rc)
		panic("unable to open tty0: %d", rc);

	/*
	 * Display a welcome message.
	 */
	rc = oskit_ttystream_write(ttystrm, msg, strlen(msg), &actual);
	if (rc)
		panic("error writing to tty device: %d", rc);

	/* Read and echo characters until we see an 'X' or EOF. */
	while (1) {
		char c;

		rc = oskit_ttystream_read(ttystrm, &c, 1, &actual);
		if (rc)
			panic("error reading from tty device: %d", rc);
		if (actual == 0)
			break;
		if (c == 'X')
			break;

		rc = oskit_ttystream_write(ttystrm, &c, 1, &actual);
		if (rc)
			panic("error writing to tty device: %d", rc);
	}

	return 0;
}
