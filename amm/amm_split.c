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
 * Split an address map entry into two pieces
 */
#include <errno.h>
#include "amm.h"

/*
 * Called whenever the AMM needs to split an entry due to conflicting flags.
 * Calls the user-set split routine if available.  Otherwise performs a
 * simple split here.  Either should split the entries and return zero or
 * return non-zero if the split could not be done.  A non-zero return value
 * is propogated on to whoever performed the action which triggered the split
 * request.
 *
 * ``pentry'' is a pointer to the next field of the entry previous to the
 * one being split.
 *
 * ``entry'' is the entry to be split.
 *
 * ``addr'' is the address at which the split should be made.  Addr is
 * guarenteed to fall in the range of the entry.
 *
 * ``headp'' and ``tailp'' should be set to everything before addr and
 * everything from addr on respectively.  One or both of *headp and *tailp
 * will be a new entry allocated and initialized here or in the user split
 * routine.  If both are new entries, we deallocate the original entry.
 *
 * This routine links the new entries into the map list.
 */
int
amm_split(struct amm *amm, struct amm_entry **pentry, struct amm_entry *entry,
	  oskit_addr_t addr, struct amm_entry **headp, struct amm_entry **tailp)
{
	struct amm_entry *head, *tail;
	int rc;

	assert(addr >= entry->start && addr < entry->end);

	if (amm->split) {
		rc = (*amm->split)(amm, entry, addr, &head, &tail);
		if (rc)
			return rc;
	} else {
		tail = amm_alloc_entry(amm, addr, entry->end - addr,
				       entry->flags);
		if (tail == 0)
			return ENOMEM;
		head = entry;
	}
	head->start = entry->start;
	tail->end = entry->end;
	tail->start = addr;
	head->end = addr;
	head->flags = entry->flags;
	tail->flags = entry->flags;
	tail->next = entry->next;
	head->next = tail;
	*pentry = head;
	if (head != entry && tail != entry)
		amm_free_entry(amm, entry);

	*headp = head;
	*tailp = tail;
	return 0;
}

