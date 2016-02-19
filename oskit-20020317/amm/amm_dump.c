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
 * Dump an address map
 */
#include <stdio.h>
#include <oskit/amm.h>

void
amm_dump(struct amm *amm)
{
	struct amm_entry *entry;

	printf("AMM: %p\n", amm);
#ifdef STATS
	printf("\t%d lookups, %d hits, %d scans\n",
	       amm->stats.lookups, amm->stats.hits, amm->stats.entriesscanned);
#endif
	for (entry = amm->nodes; entry; entry = entry->next)
		printf("\t%p: %c[0x%x - 0x%x]: flags=0x%x\n",
		       entry, *amm->hint == entry ? '*' : ' ',
		       entry->start, entry->end, entry->flags);
}
