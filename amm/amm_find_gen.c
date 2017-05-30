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
 * Find a range of address space meeting certain criteria.
 */
#include <oskit/amm.h>
#include <errno.h>

struct amm_entry *
amm_find_gen(struct amm *amm, oskit_addr_t *addrp, oskit_size_t size,
	     int flags, int flagmask, int align_bits, oskit_addr_t align_off,
	     int find_flags)
{
	struct amm_entry *entry, *sentry, **pentry;
	oskit_addr_t addr, eaddr, mask;

	assert(amm);
	assert(addrp);
	assert(size);
	assert(align_bits < sizeof(oskit_addr_t) * 8);

#if 1
	/*
	 * XXX we heavily assume AMM_FORWARD in the following
	 */
	if (find_flags & (AMM_BACKWARD|AMM_BESTFIT))
		panic("amm_find_gen: AMM_BACKWARD/AMM_BESTFIT not supported");
#endif

	/*
	 * Align hint address
	 */
	mask = (1 << align_bits) - 1;
	addr = ((*addrp + mask) & ~mask) + align_off;
	eaddr = addr + size;
	assert(addr < eaddr);

	/*
	 * Find entry containing the initial address
	 */
	sentry = amm_find_addr(amm, addr);
	pentry = amm->hint;

	for (entry = sentry; entry; pentry = &entry->next, entry = *pentry) {
		assert(eaddr >= entry->start);

		/*
		 * Alignment constraints may cause us to skip a few entries.
		 */
		if (addr >= entry->end)
			continue;

		/*
		 * Flags don't match, either look some more or punt
		 */
		if (((entry->flags) & flagmask) != flags) {
			if (find_flags & AMM_EXACT_ADDR)
				break;
			addr = ((entry->end + mask) & ~mask) + align_off;
			eaddr = addr + size;
			sentry = entry->next;
			continue;
		}

		/*
		 * This entry meets the criteria.  If it contains our
		 * endpoint, then we are all done.  Otherwise move on to
		 * the next entry.
		 */
		if (eaddr <= entry->end) {
			*addrp = addr;
			amm->hint = pentry;
			return sentry;
		}
	}

	return 0;
}

