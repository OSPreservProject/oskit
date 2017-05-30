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
 * The brute force, check `em all sweep through memory. 
 *
 * Hey, its not Purify(TM), but it works.  Most of the time.
 */

#include <stdio.h>

#include "memdebug.h"

/*
 * memdebug_traced_sweep()
 *
 * Sweep through all pointers and make sure they're fine...
 */
void
memdebug_traced_sweep(char *file, int line)
{
	memdebug_mhead *h;

	DPRINTF("memdebug_sweep()\n");
	
	mem_lock();
	for (h = memdebug_all_head.next; h != &memdebug_all_head; h = h->next)
	{
		int rc;
		rc = memdebug_traced_ptrchk(h + 1, 0, 0);
		if (rc)
			memdebug_printf("NOTE: Detected memdebug_sweep() called at %s:%d\n",
					file, line);
		if (rc == -1) /* Really bad error, don't follow off into la-la land. */
			break;
	}
	mem_unlock();
}

/*
 * memdebug_sweep()
 *
 * Wrapper for memdebug_traced_sweep().  If <oskit/memdebug.h> is included,
 * a macro wrapper will be provided, in case the library is linked without
 * including the header file, this is the entry point that will get
 * linked in.
 */
void
memdebug_sweep(void)
{
	memdebug_traced_sweep("unknown file", 0);
}
