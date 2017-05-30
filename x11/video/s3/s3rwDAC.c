/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#include "compiler.h"

#include "s3.h"
#include "regs3.h"

extern int s3DAC8Bit;

void
s3WriteDAC(index, r, g, b)
        int index;
        int r, g, b;
{
        int i;

	
        outb(DAC_W_INDEX, index);

	if (s3DAC8Bit) {
		outb(DAC_DATA, r);
		outb(DAC_DATA, g);
		outb(DAC_DATA, b);
	} else {
		outb(DAC_DATA, r >> 2);
		outb(DAC_DATA, g >> 2);
		outb(DAC_DATA, b >> 2);
	}

        i = inb(vgaIOBase + 0x0A);   /* reset flip-flop */
        outb(0x3C0, 0x11 | 0x20);
}

void
s3ReadDAC(index, r, g, b)
        int index;
        int *r, *g, *b;
{
        outb(DAC_R_INDEX, index);
        *r = inb(DAC_DATA);
        *g = inb(DAC_DATA);
        *b = inb(DAC_DATA);

	outb(DAC_W_INDEX, 0);
}
