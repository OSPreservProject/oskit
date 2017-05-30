/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Stub syslog function.
 */

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <oskit/dev/dev.h>

/* Backwards compatibility with minimal C library. */
oskit_syslogger_t	oskit_libc_syslogger;

void
syslog(int pri, const char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	if (oskit_libc_syslogger)
		oskit_libc_syslogger(pri, fmt, args);
	else
		vprintf(fmt, args);
	va_end(args);
}

/*
 * Do nothing ...
 */
void
openlog(ident, logstat, logfac)
	const char *ident;
	int logstat, logfac;
{
}

