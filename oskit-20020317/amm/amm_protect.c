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
 * Simple address map protection routine, similar to what POSIX mprotect or
 * Mach vm_protect would require.
 *
 * XXX actually it looks like POSIX says that an mprotect should fail if
 * some part of the specified range is not allocated, so this could not
 * be used for that.  Should we implement that?
 */
#include <errno.h>
#include "amm.h"

/*
 * amm_protect modifies the flags associated with an AMM_ALLOCATED range.
 * AMM_RESERVED/FREE regions are skipped.
 *
 * Addr and size define the range.  Flags is the ``protection'' to set.
 *
 * Returns zero on success, error number on failure.
 */
int
amm_protect(struct amm *amm, oskit_addr_t addr, oskit_size_t size, int prot)
{
	oskit_addr_t saddr, eaddr;
	amm_entry_t *entry;
	int flags, rc;

	saddr = addr;
	eaddr = saddr + size;
	prot = (prot & ~AMM_RESERVED) | AMM_ALLOCATED;
	while (saddr < eaddr) {
		/*
		 * Find the next allocated entry and modify its protection
		 * if necessary.
		 */
		entry = amm_find_addr(amm, saddr);
		flags = amm_entry_flags(entry);
		if ((flags & AMM_ALLOCATED) == AMM_ALLOCATED) {
			if (eaddr < amm_entry_end(entry))
				size = eaddr - saddr;
			else
				size = amm_entry_end(entry) - saddr;
			if (flags != prot) {
				rc = amm_modify(amm, saddr, size, prot, 0);
				if (rc)
					return rc;
			}
		}
		saddr = amm_entry_end(entry);
	}
	return 0;
}
