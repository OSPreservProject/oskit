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

#include <oskit/anno.h>

struct anno_entry *anno_find_lower(struct anno_table *tab, oskit_addr_t val1)
{
	struct anno_entry *low = tab->start;
	struct anno_entry *high = tab->end;

	/*
	 * If the annotation table didn't get initialized,
	 * its probably because it has no entries in it,
	 * so the search always fails.
	 */
	if (low == 0)
		return 0;

	/*
	 * Before the first annotation, return zero.
	 */
	if (val1 < low->val1)
		return 0;

	while (1)
	{
		struct anno_entry *mid = low + (unsigned)(high - low) / 2;

		if (mid == low)
			return mid;
		if (val1 < mid->val1)
			high = mid;
		else
			low = mid;
	}
}

