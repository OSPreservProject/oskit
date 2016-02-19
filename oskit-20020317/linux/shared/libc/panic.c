/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * This provides a ``panic'' routine for libdev_linux.
 * It calls a libfdev defined function, allowing the host OS
 * to override it as necessary.
 */

#include <linux/kernel.h>
#include <oskit/c/stdarg.h>
#include <oskit/c/stdlib.h>
#include "osenv.h"

void panic(const char *fmt, ...)
{
        va_list args;

        va_start(args, fmt);
        osenv_vpanic(fmt, args);
        va_end(vl);
#ifndef KNIT
        /* This is here to suppress the noreturn warning */
	exit(1);
#endif
}
