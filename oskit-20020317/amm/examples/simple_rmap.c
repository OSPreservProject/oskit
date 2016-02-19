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

/*
 * Simple(st) resource manager implementation.
 */
#include <oskit/amm.h>
#include <errno.h>
#include <assert.h>

void
simple_rminit(struct simple_rmap *smap, oskit_addr_t saddr, oskit_addr_t eaddr)
{
	assert(saddr != 0);
	amm_init(&smap->amm, saddr, eaddr);
}

oskit_addr_t
simple_rmalloc(struct simple_rmap *smap, oskit_size_t size)
{
	oskit_addr_t addr = 0;
	int rc;

	rc = amm_allocate(&smap->amm, &addr, size, 0);
	return rc ? 0 : addr;	/* return 0 on failure */
}

void
simple_rmfree(struct simple_rmap *smap, oskit_addr_t addr, oskit_size_t size)
{
	int rc;

	rc = amm_deallocate(&smap->amm, addr, size);
	assert(rc == 0);
}
