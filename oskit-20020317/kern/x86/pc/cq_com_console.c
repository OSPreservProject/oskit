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
 * A fully-functional, interrupt-driven, and properly blocking
 * asyncio/stream device built around the serial console.
 */

#include <oskit/com/stream.h>
#include <oskit/dev/dev.h>
#include <oskit/com/charqueue.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/soa.h>
#include <oskit/x86/pc/com_cons.h>
#include <oskit/x86/pc/com_console.h>
#include <termios.h>
#include <assert.h>
#include <oskit/tty.h>

static int com_cons_port;

static void
com_console_irq(void *data)
{
	oskit_stream_t *inq = data;
	int ch;
	while ((ch = com_cons_trygetchar(com_cons_port)) != -1) {
		char cc = (unsigned char) ch;
		oskit_size_t n;
		oskit_error_t rc;
		rc = oskit_stream_write(inq, &cc, 1, &n);
		if (rc)
			com_cons_putchar(com_cons_port, '\a');
	}
}

#define CONS_BUFSIZE 256

oskit_error_t
cq_com_console_init(int com_port, struct termios *com_params,
		    oskit_osenv_irq_t *irq,
		    oskit_osenv_intr_t *intr,
		    oskit_osenv_sleep_t *sleep,
		    oskit_stream_t **out_stream)
{
	oskit_error_t rc;
	oskit_stream_t *outstream;
	oskit_stream_t *inq;
	oskit_stream_t *cons;

	rc = com_console_init(com_port, com_params ?: &base_cooked_termios,
			      &outstream);
	if (rc)
		return rc;
	rc = oskit_charqueue_intr_stream_create(CONS_BUFSIZE, outstream,
						intr, sleep, &cons, &inq);
	oskit_stream_release(outstream);
	if (rc)
		return rc;

	com_cons_port = com_port;
	rc = oskit_osenv_irq_alloc(irq, (com_cons_port & 1) ? 4 : 3,
				   &com_console_irq, inq, 0);
	if (rc) {
		oskit_stream_release(cons);
		oskit_stream_release(inq);
		return rc;
	}
	com_cons_enable_receive_interrupt(com_cons_port);

	*out_stream = cons;
	return 0;
}
