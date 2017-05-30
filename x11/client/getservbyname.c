/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#include <string.h>
#include <netdb.h>

/* The appropriate lines from /etc/services are:
 *
 *  xdmcp           177/tcp    #X Display Manager Control Protocol
 *  xdmcp           177/udp    #X Display Manager Control Protocol
 *  #x11            6000-6063/tcp   X Window System
 *  #x11            6000-6063/udp   X Window System
 *
 */

static struct servent services[] = {
	{ "xdmcp", NULL, 177, "tcp"},
	{ "xdmcp", NULL, 177, "udp"},
	{ "x11", NULL, 6000, "tcp"},
	{ "x11", NULL, 6000, "udp"},
	{ 0 }
};


struct servent *
getservbyname(const char *name, const char *proto)
{
	struct servent *entry;

	entry = services;

	while (entry++) {
		if (strcmp(name, entry->s_name) != 0)
			continue;

		if (proto == 0 || strcmp(entry->s_proto, proto) == 0)
			break;
	}

	if (!entry)
		printf("%s: getservbyname wants something else (%s)\n",
		       __FUNCTION__, name);

	return entry;
}
