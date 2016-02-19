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
 * Find a range of address space
 */
#include <oskit/amm.h>
#include <errno.h>

struct amm_entry *
amm_find_addr(struct amm *amm, oskit_addr_t addr)
{
	struct amm_entry *entry, **pentry;
#ifdef STATS
	int first = 1;

	amm->stats.lookups++;
#endif

	assert(amm);

	pentry = amm->hint;
	assert(*pentry);
	if (addr < (*pentry)->start)
		pentry = &amm->nodes;

	while (1) {
#ifdef STATS
		amm->stats.entriesscanned++;
#endif
		entry = *pentry;
		if (entry->start <= addr && addr < entry->end)
			break;
#ifdef STATS
		first = 0;
#endif

		if (entry->next)
			pentry = &entry->next;
		else
			pentry = &amm->nodes;

		assert(pentry != amm->hint);
	};

#ifdef STATS
	if (first)
		amm->stats.hits++;
#endif
	amm->hint = pentry;
	return entry;
}

