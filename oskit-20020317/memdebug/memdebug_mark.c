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
 * The mark & check code for checking for memory leaks.
 *
 * Call malloc_mark(), then at a later point when the
 * memory image should be (effectively) unchanged--all
 * malloc'd memory has been free'd--call malloc_check().
 *
 * Hey, its not Purify(TM), but it works.
 */

#include <stdio.h>

#include "memdebug.h"

/*
 * memdebug_mark()
 *
 * Mark all existing blocks in the list.
 */
void 
memdebug_mark(void)
{
	memdebug_mhead *h;

	memdebug_printf("MALLOC_MARK()\n");
	mem_lock();
	for (h = memdebug_all_head.next; h != &memdebug_all_head; h = h->next)
		h->flags |= MARKED_FLAG;
	mem_unlock();
}


/*
 * memdebug_check
 *
 * Any block that isn't marked, was created after the last call to
 * memdebug_mark(), and is an error.
 */
void 
memdebug_check(void)
{
	memdebug_mhead *h;
	int rc;

	memdebug_printf("MALLOC_CHECK():\n");
	mem_lock();
	for (h = memdebug_all_head.next; h != &memdebug_all_head; h = h->next)
	{
		if (!(h->flags & MARKED_FLAG))
		{
			memdebug_printf("MALLOC BOGOSITY: Unfreed block. addr = %p\n", h + 1);
			memdebug_bogosity(h);
		}

		/* Check validity of all recorded ptrs... */
		rc = memdebug_traced_ptrchk(h + 1, 0, 0);
		if (rc == -1)
			break; /* Can't trust the next pointer */
	}
	mem_unlock();
	memdebug_printf("MALLOC_CHECK() done\n");
}

