/*
 * Copyright (c) 1998-2001 The University of Utah and the Flux Group.
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

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <oskit/tmcp.h>
#include "tmcp.h"

#define LOGCMDLEN	4
#define TMCDBUFSIZE	1024
#define LOGBUFLEN	(TMCDBUFSIZE-LOGCMDLEN-1)

static char logcmd[] = "log ";
static char logbuf[LOGBUFLEN];
static char *logbp = logbuf;
static int logbufwrapped = 0;

void
tmcp_log(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	tmcp_vlog(fmt, args);
	va_end(args);
}

void
tmcp_vlog(const char *fmt, va_list args)
{
	char buf[LOGBUFLEN], *bp;
	int blen, n;

	vsnprintf(buf, LOGBUFLEN, fmt, args);
	bp = buf;
	blen = strlen(buf);

	/*
	 * Fill from current buffer offset to end of data or end of buffer
	 */
	n = (logbuf + LOGBUFLEN) - logbp;
	if (n > blen)
		n = blen;
	assert(n > 0);
	memcpy(logbp, bp, n);

	/*
	 * Advance the buffer and check for wrap
	 */
	logbp += n;
	if (logbp == logbuf + LOGBUFLEN) {
		logbp = logbuf;
		logbufwrapped = 1;
	}

	/*
	 * If more to write, fill from beginning of buffer
	 */
	if (blen > n) {
		bp += n;
		blen -= n;
		assert(logbp == logbuf);
		memcpy(logbp, bp, blen);
		logbp += blen;
	}
	assert(logbp < logbuf + LOGBUFLEN);
}

/*
 * Linearize the data from the circular log buffer, null terminate it,
 * and send it off.
 */
int
tmcp_logdump(void)
{
	int len, err;
	char buf[TMCDBUFSIZE], *bp;

	if (logbp == logbuf && !logbufwrapped)
		return 0;

	bp = buf;
	memcpy(bp, logcmd, LOGCMDLEN);
	bp += LOGCMDLEN;

	if (logbufwrapped) {
		len = (logbuf + LOGBUFLEN) - logbp;
		memcpy(bp, logbp, len);
		bp += len;
	}
	len = logbp - logbuf;
	memcpy(bp, logbuf, len);
	bp += len;
	*bp = 0;

	len = 0;
	err = tmcp_sendmsg(buf, &len);
	return err;
}

