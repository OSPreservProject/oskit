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
 * that implement the oskit_ttydev interface,
 * specifically the FreeBSD PS/2 mouse driver.
 */

#include <stdio.h>
#include <stdlib.h>

#include <oskit/types.h>
#include <oskit/io/mouse.h>
#include <oskit/io/ttystream.h>
#include <oskit/fs/file.h>      /* XXX OSKIT_O_RDWR */
#include <oskit/dev/dev.h>
#include <oskit/dev/tty.h>
#include <oskit/dev/freebsd.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/dev/osenv.h>

#include <oskit/debug.h>

static oskit_ttystream_t *strm;

int
main(int argc, char **argv)
{
	oskit_ttydev_t **ttydev;
	int ndev, rc;
	oskit_mouse_packet_t *p;
	oskit_osenv_t *osenv;

	oskit_clientos_init();
	osenv = start_osenv();

	oskit_dev_init(osenv);
	oskit_freebsd_init_osenv(osenv);
        oskit_freebsd_init_psm();
        oskit_dump_drivers();
        oskit_dev_probe();
        oskit_dump_devices();

	ndev = osenv_device_lookup(&oskit_ttydev_iid, (void***)&ttydev);
        if (ndev <= 0)
                panic("no devices found!");

	getchar();

        /*
         * Open the first TTY device found, which should be the mouse
	 * because that is the only one I initialized.
         */
        rc = oskit_ttydev_open(ttydev[0], OSKIT_O_RDWR, &strm);
        if (rc)
                panic("unable to open psm!: %d", rc);

	if (!oskit_mouse_init(strm, P_PS2, 150))
		panic("couldn't initialize mouse!\n");

	while (1) {
		if ((p = oskit_mouse_get_packet())) {
			printf("got mouse packet (dx = %d, dy = %d, buttons = %d!\n",
			       p->dx, p->dy, p->buttons);
		}
	}

	exit(1);
}
