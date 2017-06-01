/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Support code for Roland's wacky "bus tree walk syntax" to
 * find devices in the oskit bus tree.
 */

#ifndef _OSKIT_UNSUPP_BUS_WALK_H
#define _OSKIT_UNSUPP_BUS_WALK_H 1

#include <oskit/error.h>
struct oskit_device;

/*
 * Parse NAME in the wacky "bus tree walk syntax".
 * See the source (unsupp/bus_walk_lookup.c) for the syntax.
 * If successful, returns the device found in *COM_DEVICEP.
 * If there was any trailing partition string in the specification,
 * *SUBPARTP is set to point to the tail of NAME at the 's', otherwise to null.
 */
oskit_error_t oskit_bus_walk_lookup(const char *name,
				    struct oskit_device **com_devicep,
				    const char **subpartp);

/*
 * Grok a BSD/Linux/Mach-style device name in NAME, and construct
 * a string in bus tree walk syntax to find the device it specifies.
 * If NAME was recognized, returns nonzero and fills in CONSTRUCTED;
 * otherwise returns zero and contents of CONSTRUCTED are undefined.
 */
int bus_walk_from_compat_string(const char *name, char constructed[]);



#endif	/* oskit/unsupp/bus_walk.h */
