/*
 * Copyright (c) 1995-1998 University of Utah and the Flux Group.
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

#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/pc/pic.h>

/*
 * Program the slave PIC to use a different set of interrupt vectors
 * without disturbing the PIC's current interrupt enable mask.
 * Assumes processor interrupt flag is off.
 */
void pic_set_slave(int base)
{
	unsigned char old_mask;

	/* Save the original interrupt mask.  */
	old_mask = inb(SLAVES_OCW);	PIC_DELAY();

	/* Initialize the slave PIC.  */
	outb(SLAVES_ICW, PICS_ICW1);	PIC_DELAY();
	outb(SLAVES_OCW, base);		PIC_DELAY();
	outb(SLAVES_OCW, PICS_ICW3);	PIC_DELAY();
	outb(SLAVES_OCW, PICS_ICW4);	PIC_DELAY();

	/* Restore the original interrupt mask.  */
	outb(SLAVES_OCW, old_mask);	PIC_DELAY();
}

