/*
 * Copyright (c) 1994-1995, 1998, 1999 University of Utah and the Flux Group.
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

/* This is a special "feature" (read: kludge)
   intended for use only for kernel debugging.
   It enables an extremely simple console output mechanism
   that sends text straight to CGA/EGA/VGA video memory.
   It has the nice property of being functional right from the start,
   so it can be used to debug things that happen very early
   before any devices are initialized.  */

#include <oskit/c/string.h>
#include <oskit/base_critical.h>
#include <oskit/arm32/base_vm.h>
#include <oskit/arm32/pio.h>
#include <oskit/arm32/ofw/ofw.h>

#define CGA_BUF		((unsigned char *) 0xb8000)
#define CGA_PORT	0x3d4

static void
fillw(unsigned short pat, const void *base, oskit_size_t cnt) {
	volatile unsigned short *p;

	p = (unsigned short *)base;

	while (cnt--)
		writew(p++, pat);
}

static void
set_cursor(unsigned int pos)
{
	pos += 80 * 24;

	/* set cursor position high byte */
	outb_p(CGA_PORT, 0xe);
	outb_p(CGA_PORT + 1, (unsigned char)((pos >> 8)) & 0xff);

	/* set cursor position low byte */
        outb_p(CGA_PORT, 0xf);
        outb_p(CGA_PORT + 1, (unsigned char)(pos & 0xff));
}

int
direct_cons_putchar(int c)
{
	int		i;
	unsigned char	*f, *t;
	static int	ofs = -1;

	c = (unsigned char)c;

	base_critical_enter();

	if (ofs < 0) {
		ofw_vga_init();
		ofs = 0;
		direct_cons_putchar('\n');
	}

	switch (c)
	{
	case '\n':
		/*
		 * Move everything up one line.
		 */
		for (i = 0, f = CGA_BUF, t = CGA_BUF + 80*2;
		     i < 80*24*2; i++, f++, t++)
			writeb(f, readb(t));

		/* Clear the last line */
		fillw(0x0f00, CGA_BUF + 80*24*2, 80);

		/* fall through... */
	case '\r':
		ofs = 0;
		break;
	case '\b':
		if (ofs > 0) ofs--;
		break;
	case '\t':
		do {
			direct_cons_putchar(' ');
		} while ((ofs & 7) != 0);
		break;

	default:
		/* Wrap if we reach the end of a line.  */
		if (ofs >= 80) {
			direct_cons_putchar('\n');
		}

		/* Stuff the character into the video buffer. */
		writeb(CGA_BUF + (((80*24) + ofs) * 2),     c);
		writeb(CGA_BUF + (((80*24) + ofs) * 2) + 1, 0x0f);
		ofs++;
		break;
	}
	set_cursor(ofs);

	base_critical_leave();

	return c;
}
