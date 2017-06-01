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
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/c/termios.h>
extern struct termios base_cooked_termios;

extern int putbytes(const char *s, int len);

#define PORT 1

int putchar(int ch)
{
        direct_cons_putchar(ch); // hack, hack
        com_cons_putchar(PORT,ch);
        return 0;
}

int getchar(void)
{
        return com_cons_getchar(PORT);
}


oskit_error_t
com_init(void)
{
        base_cooked_termios.c_ispeed = B115200;
        base_cooked_termios.c_ospeed = B115200;
        com_cons_init(PORT, &base_cooked_termios);
        return 0;
}        

oskit_error_t
com_fini(void)
{
        com_cons_flush(PORT);
        return 0;
}
