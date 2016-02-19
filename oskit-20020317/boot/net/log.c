/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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
 * Implementation of the OSKit log functions.
 * Allows us to silence overly chatty device drivers.
 */

#include <oskit/dev/dev.h>

#include <stdio.h>
#include <stdarg.h>

/*
 * The priority level below which you will see error messages.
 *
 *	OSENV_LOG_EMERG		0
 *	OSENV_LOG_ALERT		1
 *	OSENV_LOG_CRIT		2
 *	OSENV_LOG_ERR		3
 *	OSENV_LOG_WARNING	4
 *	OSENV_LOG_NOTICE	5
 *	OSENV_LOG_INFO		6
 *	OSENV_LOG_DEBUG		7
 *
 * So setting to zero turns off all logging.
 */
#ifdef DEBUG
int netboot_loglevel = 10000;
#else
int netboot_loglevel = 0;
#endif

void
osenv_vlog(int priority, const char *fmt, void *args)
{
	if (priority >= netboot_loglevel)
		return;

        vprintf(fmt, args);
}

void
osenv_log(int priority, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	osenv_vlog(priority, fmt, args);
	va_end(args);
}
