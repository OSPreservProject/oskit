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
 * This is the printk() routine for the oskit_linux drivers.
 * It calls osenv_log() with OSENV_LOG_INFO priority;
 * if a different priority is desired, osenv_log() should
 * be used directly.
 */

#include <oskit/c/stdarg.h>
#include "osenv.h"
#include "assert.h"

int
printk(const char *fmt, ...)
{
        va_list args;
        int err = 0;

	/*
	 * XXX: This relies on OSENV_LOG_foo corresponding exactly
	 * to Linux's KERN_foo.
	 * Since Linux just prepends the log priority to the string,
	 * we `parse' for it at the beginning.
	 */
	if (fmt[0] == '<' &&
	    fmt[1] >= '0' && fmt[1] <= '7' &&
	    fmt[2] == '>') {
		va_start(args, fmt);
		osenv_vlog(fmt[1] - '0', fmt+3, args);
		va_end(args);
	} else {
		va_start(args, fmt);
		osenv_vlog(OSENV_LOG_INFO, fmt, args);
		va_end(args);
	}

        return err;
}

