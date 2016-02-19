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

#include <string.h>

/*
 * One-time initialization for AMM
 */
#include "amm.h"

/*
 * Initialize the given amm structure, and associates the specified
 * flags and entry with the entire supported address range 0 through
 * (oskit_addr_t)-1.
 *
 * The flags parameter is the address range flags word to associate
 * initially with the entire address space.
 *
 * The entry parameter allows the client to provide the initial amm_entry
 * node which will initially be associated with the entire address space;
 * this way the client can allocate a structure larger than the basic
 * amm_entry and store additional attribute data in the extended structure.
 * If the client supplies the init_entry, it must have initialized any
 * client-private data in that entry, but the amm will initialize the
 * amm-private part (the actual struct amm_entry).  If entry is NULL,
 * the amm will call amm_alloc_entry to allocate a struct amm_entry and
 * initialize that.
 *
 * If the client wants to perform allocation or deallocation (or whatever)
 * over only a subset of the full possible (32- or 64-bit) address range,
 * then it can set up the init_entry to represent "invalid"
 * addresses rather than "free" addresses, and then later call
 * amm_modify_range() to create appropriate "free" region(s).
 */
void
amm_init_gen(struct amm *amm, int flags, struct amm_entry *entry,
	     struct amm_entry *(*af)(struct amm *, oskit_addr_t, oskit_size_t, int),
	     void (*ff)(struct amm *, struct amm_entry *),
	     int (*sf)(struct amm *, struct amm_entry *, oskit_addr_t,
		       struct amm_entry **, struct amm_entry **),
	     int (*jf)(struct amm *, struct amm_entry *,
		       struct amm_entry *, struct amm_entry **))
{
	amm->alloc = af;
	amm->free = ff;
	amm->split = sf;
	amm->join = jf;

	if (entry == 0) {
		entry = amm_alloc_entry(amm, AMM_MINADDR,
					AMM_MAXADDR - AMM_MINADDR, flags);
		assert(entry);	/* XXX */
	}
	entry->start = AMM_MINADDR;
	entry->end = AMM_MAXADDR;
	entry->flags = flags;
	entry->next = 0;

	amm->nodes = entry;
	amm->hint = &amm->nodes;
#ifdef STATS
	amm->stats.lookups = amm->stats.hits = amm->stats.entriesscanned = 0;
#endif
}

#if 0
/*
 * XXX these could just be parameters to the init routine rather than seperate
 * calls but we want to keep the amm_init interface simple.
 *
 * XXX these (at least the alloc function) must be part of the init call
 * since init wants to allocate a node.
 */
void
amm_set_alloc_func(struct amm *amm,

{
	amm->alloc = af;
}

void
amm_set_free_func(struct amm *amm,
		  void (*ff)(struct amm *, struct amm_entry *))
{
	amm->free = ff;
}

void
amm_set_split_func(struct amm *amm,
		   int (*sf)(struct amm *, struct amm_entry *, oskit_addr_t,
			     struct amm_entry **, struct amm_entry **))
{
	amm->split = sf;
}

void
amm_set_join_func(struct amm *amm,
		  int (*jf)(struct amm *, struct amm_entry *,
			    struct amm_entry *, struct amm_entry **))
{
	amm->join = jf;
}
#endif
