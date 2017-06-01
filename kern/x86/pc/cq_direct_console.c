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
 * asyncio/stream device built around the minimal direct console.
 */

#include <oskit/com/stream.h>
#include <oskit/dev/dev.h>
#include <oskit/com/charqueue.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/soa.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/direct_console.h>
#include <oskit/x86/pc/keyboard.h>
#include <oskit/x86/pio.h>
#include <assert.h>


#define KBD_DELAY 	60
#define KBD_RETRIES	10

#define KBD_IRQ	1
#define CONS_BUFSIZE 256


static void
kbd_wait_while_busy(void)
{
    int i = 1000;

    while (i--) {
        if ((inb(K_STATUS) & K_IBUF_FUL) == 0)
            break;
        osenv_timer_spin(KBD_DELAY);
    }
}


static void
kbd_write_cmd_byte(int cb)
{
	kbd_wait_while_busy();
	outb(K_CMD, KC_CMD_WRITE);
	kbd_wait_while_busy();
	outb(K_RDWR, cb);
}



static void
direct_console_irq(void *data)
{
	oskit_stream_t *inq = data;
	int ch;
	while ((ch = direct_cons_trygetchar()) != -1) {
		char cc = (unsigned char) ch;
		oskit_size_t n;
		oskit_error_t rc;
		rc = oskit_stream_write(inq, &cc, 1, &n);
		if (rc)
			direct_cons_bell();
	}
}


oskit_error_t
cq_direct_console_init(oskit_osenv_irq_t *irq,
		       oskit_osenv_intr_t *intr,
		       oskit_osenv_sleep_t *sleep,
		       oskit_stream_t **out_stream)
{
	oskit_error_t rc;
	oskit_stream_t *outstream;
	oskit_stream_t *inq;
	oskit_stream_t *cons;
	int c, i, retries = KBD_RETRIES;

	rc = direct_console_init(&outstream);
	if (rc)
		return rc;

	rc = oskit_charqueue_intr_stream_create(CONS_BUFSIZE, outstream,
						intr, sleep, &cons, &inq);
	oskit_stream_release(outstream);
	if (rc)
		return rc;

	/* Write the command byte to make sure that IRQ generation is off. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF));

	/* empty keyboard buffer. */
	while (inb(K_STATUS) & K_OBUF_FUL) {
		osenv_timer_spin(KBD_DELAY);
		(void) inb(K_RDWR);
	}

	/* reset keyboard */
	while (retries--) {
		kbd_wait_while_busy();
		outb(K_RDWR, KC_CMD_PULSE);
		i = 10000;
		while (!(inb(K_STATUS) & K_OBUF_FUL) && (--i > 0))
			osenv_timer_spin(KBD_DELAY);
		if ((c = inb(K_RDWR)) == K_RET_ACK)
			break;
	}

	if (retries > 0) {
		retries = KBD_RETRIES;

		while (retries--) {
			/* Need to wait a long time... */
			i = 10000;
			while (!(inb(K_STATUS) & K_OBUF_FUL) && (--i > 0))
				osenv_timer_spin(KBD_DELAY * 10);
			if ((c = inb(K_RDWR)) == K_RET_RESET_DONE)
				break;
		}
	}

	if (retries == 0) {
		/* We can't reset the keyboard!  */
		oskit_stream_release(inq);
		oskit_stream_release(cons);
		return OSKIT_E_DEV_IOERR;
	}

	/* Now turn on IRQ generation. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF|K_CB_ENBLIRQ));

	rc = oskit_osenv_irq_alloc(irq, KBD_IRQ, &direct_console_irq, inq, 0);
	if (rc) {
		oskit_stream_release(cons);
		oskit_stream_release(inq);
		return rc;
	}

	*out_stream = cons;
	return 0;
}
