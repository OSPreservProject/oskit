/*
 * Copyright (c) 1995-2001 University of Utah and the Flux Group.
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

#include "fileops.h"
#include "rpc.h"
#include "debug.h"
#include "misc.h"

static struct xport {
	int (*open)(struct in_addr, char *, char *);
	void (*close)(void);
	int (*read)(oskit_addr_t, void *, oskit_size_t, oskit_size_t *);
} xporttab[] = {
	{ nfsopen,	nfsclose,	nfsread },
	{ tftpopen,	tftpclose,	tftpread }
};
static int nxports = sizeof(xporttab) / sizeof(xporttab[0]);

static unsigned currentxport = ~0;

/*
 * Establish a connection to the server identified by ip and prepare it
 * to transfer dirname/filename.
 */
int
fileopen(unsigned xport, struct in_addr ip, char *dirname, char *filename)
{
	/* terminate any open connection */
	fileclose();

	if (xport >= nxports || xporttab[xport].open == NULL)
		return OSKIT_EINVAL;

	currentxport = xport;
	return xporttab[currentxport].open(ip, dirname, filename);
}

/*
 * So we can do multiple loads.
 */
void
fileclose(void)
{
	if (currentxport < nxports)
		xporttab[currentxport].close();
	currentxport = ~0;
}

/*
 * Read a small block of data from the input file.
 */
int
fileread(oskit_addr_t file_ofs, void *buf, oskit_size_t size,
	 oskit_size_t *out_actual)
{
	if (currentxport >= nxports)
		return OSKIT_EINVAL;

	return xporttab[currentxport].read(file_ofs, buf, size, out_actual);
}
