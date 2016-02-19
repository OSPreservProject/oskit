/*
 * Copyright (c) 1995-1996, 1998 University of Utah and the Flux Group.
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
#include <oskit/debug.h>

void anno_init(void)
{
	extern struct anno_entry __ANNO_START__[], __ANNO_END__[];
	struct anno_entry *base;
	static int inited;

	/* Simple multiple-initialization protection.  */
	if (inited)
		return;
	inited = 1;

	/* Sort the tables using a slow, simple selection sort;
	   it only needs to be done once.  */
	for (base = __ANNO_START__; base < __ANNO_END__; base++)
	{
		struct anno_entry *cur, *low, tmp;

		/* Select the lowermost remaining entry,
		   and swap it into the base slot.
		   Sort by table first, then by val1, val2, val3.  */
		low = base;
		for (cur = base+1; cur < __ANNO_END__; cur++)
			if ((cur->table < low->table)
			    || ((cur->table == low->table)
			        && ((cur->val1 < low->val1)
				    || ((cur->val1 == low->val1)
				        && ((cur->val2 < low->val2)
					    || ((cur->val2 == low->val2)
					        && (cur->val3 < low->val3)))))))
				low = cur;
		tmp = *base;
		*base = *low;
		*low = tmp;
	}

	/* Initialize each anno_table structure with entries in the array.  */
	for (base = __ANNO_START__; base < __ANNO_END__; )
	{
		struct anno_entry *end;

		for (end = base;
		     (end < __ANNO_END__) && (end->table == base->table);
		     end++);
		base->table->start = base;
		base->table->end = end;

		base = end;
	}

#if 0 /* debugging code */
	anno_dump();
#endif
}

