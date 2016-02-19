/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * Simple address map allocation routine, similar to what POSIX mmap or
 * Mach vm_allocate would require.
 */
#include <errno.h>
#include "amm.h"

/*
 * amm_allocate looks for a range with flags AMM_FREE and modifies it to
 * have the attributes AMM_ALLOCATED|flags.
 *
 * On input, *addrp specifies a hint address at which to start searching.
 * On output, *addrp is the address chosen. Size is the amount of the
 * resource desired and flags additional application-specific attributes to
 * associate with the range.
 *
 * Returns zero on success, an error number on failure.
 */
int
amm_allocate(struct amm *amm, oskit_addr_t *addrp, oskit_size_t size, int prot)
{
	struct amm_entry *entry;
	oskit_addr_t addr;
	int rc;

	addr = *addrp;
	if (addr < AMM_MINADDR || addr >= AMM_MAXADDR)
		addr = AMM_MINADDR;
	prot &= ~(AMM_ALLOCATED|AMM_RESERVED);

	entry = amm_find_gen(amm, &addr, size, AMM_FREE, -1, 0, 0, 0);
	if (entry == 0)
		return ENOMEM;

	rc = amm_modify(amm, addr, size, AMM_ALLOCATED|prot, 0);
	if (rc == 0)
		*addrp = addr;

	return rc;
}
