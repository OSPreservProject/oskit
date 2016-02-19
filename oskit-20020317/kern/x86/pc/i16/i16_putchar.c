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
 * Default i16_putchar() implementation for programs
 * running directly under the 16-bit PC BIOS.
 */

#include <oskit/x86/i16.h>
#include <oskit/x86/pc/i16_bios.h>

CODE16

int i16_putchar(int ch)
{
	if (ch == '\n')
		i16_bios_putchar('\r');
	i16_bios_putchar(ch);
	return ch;
}

int i16_console_putbytes(const char *s, int len)
{
	while (len) {
		i16_bios_putchar(*s++);
		len--;
	}

	return 0;
}
