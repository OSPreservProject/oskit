/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Select (isolate) a range of address space
 */
#include <errno.h>
#include "amm.h"

/*
 * Select (isolate) the specified range by splitting off everything before
 * and after as necessary.  Result is one or more entries exactly covering
 * the range.  Returns a pointer to the first entry for the start of the
 * range or zero if the range could not be isolated.  Successive entries
 * can be located using amm_find_addr.
 */
amm_entry_t *
amm_select(struct amm *amm, oskit_addr_t addr, oskit_size_t size)
{
	struct amm_entry **pentry, *entry, *hentry, *tentry;
	oskit_addr_t eaddr;
	int rc;

	eaddr = addr + size;
	if (eaddr < addr)
		return 0;

	entry = amm_find_addr(amm, addr);
	pentry = amm->hint;

	/*
	 * Split off anything before the region of interest.
	 * Leave hint pointing at the beginning of the region.
	 */
	if (entry->start < addr) {
		rc = amm_split(amm, pentry, entry, addr, &hentry, &tentry);
		if (rc)
			return 0;
		pentry = &hentry->next;
		entry = tentry;
		amm->hint = pentry;
	}
	assert(entry->start == addr);

	/*
	 * Skip over entries that are completely covered by the range.
	 */
	while (entry->end < eaddr) {
		pentry = &entry->next;
		entry = *pentry;
	}

	/*
	 * Split off anything after the region of interest
	 */
	if (entry->end > eaddr) {
		rc = amm_split(amm, pentry, entry, eaddr, &hentry, &tentry);
		if (rc)
			return 0;
		entry = hentry;
	}
	assert(entry->end == eaddr);

	/*
	 * Return the first entry.
	 */
	return *amm->hint;
}

