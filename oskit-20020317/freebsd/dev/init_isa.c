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

#include <oskit/dev/dev.h>
#include <oskit/dev/freebsd.h>
#include "osenv.h"

#define INIT(name, desc)						\
	({	oskit_error_t rc = oskit_freebsd_init_##name();		\
		if (rc) osenv_log(OSENV_LOG_ERR,				\
			"error initializing "desc" driver: %d", rc);	\
	})

void oskit_freebsd_init_isa(void)
{
#define driver(name, major, count, description, vendor, author) \
	INIT(name, description);
#define instance(drivername, port, irq, drq, maddr, msize)
#include <oskit/dev/freebsd_isa.h>
#undef driver
#undef instance
}

