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
 * Modify a range of address space
 */
#include <errno.h>
#include "amm.h"

/*
 * The big cheese.
 * Modifies an address range to be associated with a new entry and new flags.
 * Any existing entries wholly within the range are deleted, any that partly
 * overlap the range are split as necessary.  After adding the new entry,
 * the amm may attempt to join it with adjacent already-existing entries if
 * it appears possible (i.e., if the flags words are the same).  The standard
 * amm_join function is called in this case.
 *
 * Function returns zero if successful, non-zero if a split failed (e.g.,
 * due to lack of memory).
 * 
 * Parameters addr and size define the range to be modified.
 *
 * nflags is the new flags for the range.
 *
 * Nentry is the new entry to use.  nentry may be zero, in which case the amm
 * allocates a new standard entry itself.  If nentry is non-zero, the caller
 * must have already allocated any extra attribate data in the entry.  In
 * either case, the amm will initialize the private part of the new amm_entry,
 * including setting its address range type flags to the nflags parameter.
 */
int
amm_modify(struct amm *amm, oskit_addr_t addr, oskit_size_t size,
	   int nflags, struct amm_entry *nentry)
{
	struct amm_entry **pentry, *entry, *hentry, *tentry;
	oskit_addr_t eaddr;
	int rc;

	eaddr = addr + size;
	if (eaddr < addr)
		return EINVAL;

	entry = amm_find_addr(amm, addr);
	pentry = amm->hint;

	/*
	 * If no explicit entry was specified and the indicated range
	 * already has the desired attributes, there is nothing to do.
	 */
	if (nentry == 0 && addr >= entry->start && eaddr <= entry->end &&
	    nflags == entry->flags)
		return 0;

	/*
	 * Split off anything before the region of interest.
	 */
	if (entry->start < addr) {
		rc = amm_split(amm, pentry, entry, addr, &hentry, &tentry);
		if (rc)
			return rc;
		pentry = &hentry->next;
		entry = tentry;
	}
	assert(entry->start == addr);

	/*
	 * Now traverse entries that are completely covered by the new
	 * one and delete them.
	 */
	while (entry->end < eaddr) {
		*pentry = entry->next;
		amm_free_entry(amm, entry);
		entry = *pentry;
	}

	/*
	 * Split off anything after the region of interest
	 */
	if (entry->end > eaddr) {
		rc = amm_split(amm, pentry, entry, eaddr, &hentry, &tentry);
		if (rc)
			return rc;
		entry = hentry;
	}
	assert(entry->end == eaddr);

	/*
	 * Final entry corresponding to end of range.
	 * If no explicit new entry was specified or the current entry
	 * was specified we just modify the existing entry.
	 */
	if (nentry == entry) {
		entry->start = addr;
		entry->flags = nflags;
		nentry = entry;
	} else {
		if (nentry == 0) {
			nentry = amm_alloc_entry(amm, addr, eaddr-addr, nflags);
			if (nentry == 0)
				return ENOMEM;
		}
		*pentry = nentry;
		nentry->next = entry->next;
		nentry->start = addr;
		nentry->end = eaddr;
		nentry->flags = nflags;
		amm_free_entry(amm, entry);
	}

	/*
	 * Finally, try to merge with previous entry...
	 */
	if (pentry != &amm->nodes) {
		hentry = (struct amm_entry *)pentry;
		tentry = nentry;
		assert(hentry->next == tentry);
		if (hentry->flags == tentry->flags) {
			for (pentry = &amm->nodes; *pentry;
			     pentry = &(*pentry)->next)
				if (*pentry == hentry)
					break;
			assert(*pentry);
			rc = amm_join(amm, pentry, hentry, tentry, &nentry);
			/* XXX return an error on rc != 0? */
			if (rc)
				pentry = &hentry->next;
		}
	}
	/*
	 * ...and next entry.
	 */
	if (nentry->next) {
		hentry = nentry;
		tentry = nentry->next;
		if (hentry->flags == tentry->flags) {
			rc = amm_join(amm, pentry, hentry, tentry, &nentry);
			/* XXX return an error on rc != 0? */
		}

	}

	amm->hint = pentry;
	return 0;
}

