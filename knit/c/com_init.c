/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#include <oskit/x86/pc/com_cons.h>
#include <oskit/c/termios.h>
#include <oskit/tty.h>

#include <oskit/error.h>

#define port 1
#define speed B115200;

/*
 * We use dynamic initialization here so that the console can be used
 * very, very early in the boot process (before the first initializer
 * has been called).
 */

static int inited = 0;

static void init(void)
{
        base_cooked_termios.c_ispeed = speed;
        base_cooked_termios.c_ospeed = speed;
        com_cons_init(port, &base_cooked_termios);
        inited = 1;
}

oskit_error_t fini(void)
{
        com_cons_flush(port);
        return 0;
}

int putchar(int c)
{
        if (!inited) init();
	com_cons_putchar(port, c);
        return 0;
}

int
putbytes(const char *s, int len)
{
	int i;
        if (!inited) init();
	for(i=0; i<len; ++i) {
		com_cons_putchar(port, s[i]);
	}
	return 0;
}

int getchar(void)
{
        if (!inited) init();
	return com_cons_getchar(port);
}
