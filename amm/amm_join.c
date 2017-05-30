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
 * Join adjacent address map entries
 */
#include "amm.h"

/*
 * Called whenever the AMM thinks that two entries can be joined based on
 * comparison of the flags.  Calls the user-set join routine if available.
 * Otherwise performs a simple join here.  Should join them and return zero
 * or return non-zero indicating that they cannot be joined.  A non-zero
 * return value is propogated on to whoever performed the action which
 * triggered the join request.
 *
 * ``pentry'' is a pointer to the next field of the entry previous to the
 * first entry being joined.
 *
 * ``head'' and ``tail'' are the two entries to join.
 *
 * ``new'' is the joined entry.  It may be one of the existing entries
 * or it may be a completely new entry.  Any ``left-over'' entries are
 * freed here.
 */
int
amm_join(struct amm *amm, struct amm_entry **pentry,
	 struct amm_entry *head, struct amm_entry *tail, struct amm_entry **new)
{
	struct amm_entry *entry;
	int rc;

	assert(head->end == tail->start);
	assert(head->flags == tail->flags);

	if (amm->join) {
		rc = (*amm->join)(amm, head, tail, &entry);
		if (rc)
			return rc;
	} else {
		entry = head;
	}
	entry->start = head->start;
	entry->end = tail->end;
	entry->flags = tail->flags;
	entry->next = tail->next;
	*pentry = entry;
	if (head != entry)
		amm_free_entry(amm, head);
	if (tail != entry)
		amm_free_entry(amm, tail);

	*new = entry;
	return 0;
}

