/*
 * Copyright (c) 1995-1999 University of Utah and the Flux Group.
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
#include <oskit/c/stdio.h>

void anno_dump(void)
{
	extern struct anno_entry __ANNO_START__[], __ANNO_END__[];
	struct anno_entry *base;
	struct anno_table *t = 0;

	for (base = __ANNO_START__; base < __ANNO_END__; base++) {
		if (t != base->table) {
			t = base->table;
			printf("table %p: [%p-%p), %d entries\n",
			       t, t->start, t->end, t->end - t->start);
			assert(t->start == base);
		}
		printf("  vals %08x %08x %08x\n",
		       base->val1, base->val2, base->val3);
	}
}
