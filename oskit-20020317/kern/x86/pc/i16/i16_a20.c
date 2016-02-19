/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Routines to enable and disable the A20 address line on a PC or PS/2.
 * The A20 line is generally disabled by default on system startup,
 * for compatibility with really really old 16-bit DOS software;
 * it has to be enabled before touching any memory above 1MB.
 */

#include <oskit/x86/pio.h>
#include <oskit/x86/i16.h>
#include <oskit/x86/pc/a20.h>
#include <oskit/x86/pc/keyboard.h>


CODE16


/*
   This routine ensures that the keyboard command queue is empty
   (after emptying the output buffers)

   No timeout is used - if this hangs there is something wrong with
   the machine, and we probably couldn't proceed anyway.
   XXX should at least die properly
*/
static void i16_empty_8042(void)
{
	int status;

retry:
	i16_iodelay();
	status = i16_inb(K_STATUS);

	if (status & K_OBUF_FUL)
	{
		i16_iodelay();
		i16_inb(K_RDWR);
		goto retry;
	}

	if (status & K_IBUF_FUL)
		goto retry;
}

/* Enable the A20 address line.  */
void i16_enable_a20(void)
{
	int v;

	/* XXX try int 15h function 24h */

	if (i16_test_a20())
		return;

	/* PS/2 */
	v = i16_inb(0x92);
	i16_iodelay();
	i16_outb(0x92,v | 2);

	if (i16_test_a20())
		return;

	/* AT */
	i16_empty_8042();
	i16_outb(K_CMD, KC_CMD_WOUT);
	i16_empty_8042();
	i16_outb(K_RDWR, KB_ENABLE_A20);
	i16_empty_8042();

	/* Wait until the a20 line gets enabled.  */
	while (!i16_test_a20());
}

/* Disable the A20 address line.  */
void i16_disable_a20(void)
{
	int v;

	if (!i16_test_a20())
		return;

	/* PS/2 */
	v = i16_inb(0x92);
	i16_iodelay();
	i16_outb(0x92, v & ~2);

	if (!i16_test_a20())
		return;

	/* AT */
	i16_empty_8042();
	i16_outb(K_CMD, KC_CMD_WOUT);
	i16_empty_8042();
	i16_outb(K_RDWR, KB_DISABLE_A20);
	i16_empty_8042();

	/* Wait until the a20 line gets disabled.  */
	while (i16_test_a20());
}

