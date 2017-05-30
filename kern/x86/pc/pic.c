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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
Copyright (c) 1988,1989 Prime Computer, Inc.  Natick, MA 01760
All Rights Reserved.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and
without fee is hereby granted, provided that the above
copyright notice appears in all copies and that both the
copyright notice and this permission notice appear in
supporting documentation, and that the name of Prime
Computer, Inc. not be used in advertising or publicity
pertaining to distribution of the software without
specific, written prior permission.

THIS SOFTWARE IS PROVIDED "AS IS", AND PRIME COMPUTER,
INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
NO EVENT SHALL PRIME COMPUTER, INC.  BE LIABLE FOR ANY
SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN ACTION OF CONTRACT, NEGLIGENCE, OR
OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/*
 * Routines for manipulating the 8259A Programmable Interrupt
 * Controller (PIC)
 */

#include <sys/types.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/pic.h>
#include <assert.h>

/*
 * Initialize the PIC for PCs.
 * Note that we don't disable all intrs here, you can call pic_disable_all.
 *
 * XXX the original Mach version put delays after the outb's in this
 * function, so we do the same thing (FreeBSD doesn't.)  Don't know if
 * this is still required, the enable/disable functions below work
 * like champs without it.
 */
void
pic_init(unsigned char master_base, unsigned char slave_base)
{
	/* Initialize the master. */
	outb_p(MASTER_ICW, PICM_ICW1);
	outb_p(MASTER_OCW, master_base);
	outb_p(MASTER_OCW, PICM_ICW3);
	outb_p(MASTER_OCW, PICM_ICW4);

	/* Initialize the slave. */
	outb_p(SLAVES_ICW, PICS_ICW1);
	outb_p(SLAVES_OCW, slave_base);
	outb_p(SLAVES_OCW, PICS_ICW3);
	outb_p(SLAVES_OCW, PICS_ICW4);

	/* Ack any bogus intrs by setting the End Of Interrupt bit. */
	outb_p(MASTER_ICW, NON_SPEC_EOI);
	outb_p(SLAVES_ICW, NON_SPEC_EOI);
}

/*
 * Enable the irq by clearing the appropriate bit in the PIC.
 */
void
pic_enable_irq(unsigned char irq)
{
	unsigned int flags;

	assert(irq < 16);
	flags = get_eflags();
	cli();

	if (irq < 8)
		outb(MASTER_OCW, inb(MASTER_OCW) & ~(1 << irq));
	else {
		/* Enable the cascade line on the master. */
		outb(MASTER_OCW, inb(MASTER_OCW) & ~(1 << 2));

		irq -= 8;
		outb(SLAVES_OCW, inb(SLAVES_OCW) & ~(1 << irq));
	}

	set_eflags(flags);
}

/*
 * Disable the irq by setting the appropriate bit in the PIC.
 */
void
pic_disable_irq(unsigned char irq)
{
	unsigned int flags;

	assert(irq < 16);
	flags = get_eflags();
	cli();

	if (irq < 8)
		outb(MASTER_OCW, inb(MASTER_OCW) | (1 << irq));
	else {
		irq -= 8;
		outb(SLAVES_OCW, inb(SLAVES_OCW) | (1 << irq));
	}

	set_eflags(flags);
}

/*
 * Check to see if the specified irq is currently pending.
 */
int
pic_test_irq(unsigned char irq)
{
	assert(irq < 16);

	if (irq < 8)
		return inb(MASTER_ICW) & (1 << irq);
	else {
		irq -= 8;
		return inb(SLAVES_ICW) & (1 << irq);
	}
}

int
pic_get_irqmask(void)
{
	return (inb(SLAVES_OCW) << 8) | inb(MASTER_OCW);
}

void
pic_set_irqmask(int mask)
{
	unsigned int flags, m, s;

	m = mask & 0xff;
	s = (mask >> 8) & 0xff;

	/* ensure that slave PIC is enabled if necessary */
	if (s != PICS_MASK)
		m &= ~(1 << 2);

	flags = get_eflags();
	cli();

	outb(MASTER_OCW, m);
	outb(SLAVES_OCW, s);

	set_eflags(flags);
}
