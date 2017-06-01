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
 * Iterate over all entries of an AMM, calling a function for each entry.
 */
#include <oskit/amm.h>

/*
 * Call a user-provided function for every entry of an AMM.
 *
 * ifunc is the function to call, arg is an opaque argument which is
 * passed to the function (along with the AMM and entry) for every entry.
 *
 * If any call returns a non-zero value, the iteration is stopped and that
 * value is returned.  Otherwise zero is returned.
 */
int
amm_iterate(struct amm *amm,
	    int (*ifunc)(struct amm *, struct amm_entry *, void *), void *arg)
{
	int rv = 0;
	struct amm_entry *entry;
	oskit_addr_t addr;

	for (entry = amm->nodes; entry; entry = entry->next) {
		addr = entry->end - 1;
		rv = (*ifunc)(amm, entry, arg);
		if (rv)
			break;
		/*
		 * Iteration function may have caused the current
		 * entry to be deleted, split or joined with another.
		 * So we re-lookup the current entry to get the correct
		 * next pointer.
		 */
		entry = amm_find_addr(amm, addr);
	}

	return rv;
}

