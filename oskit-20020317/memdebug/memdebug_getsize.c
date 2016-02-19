/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Implemement memdebug_getsize()
 * Very similar to memdebug_free only without the free part.
 */
#include <assert.h>
#include <string.h>
#include <oskit/lmm.h>

#include "memdebug.h"

/*
 * memdebug_getsize()
 *
 * returns 0 and fills in the size of the given block if that block is valid
 * otherwise returns an error.
 */
int
memdebug_getsize(void *mem, int *blocksize, char *file, int line)
{
	memdebug_mhead *head;
	memdebug_mtail *tail;
	int rc;

#if ALLOW_MORALLY_QUESTIONABLE_PRACTICE
	/* getsize(NULL) is 0. */
	if (!mem)
	{
		*blocksize = 0;
		return 0;
	}
#else
	if (!mem)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY: Tried to free(NULL) at",
				   file, line);
		return -1;
	}
#endif

	head = (memdebug_mhead*)mem - 1;
	tail = (memdebug_mtail*)((void*)(head + 1) + head->size);

	/*
	 * Do the "already free" check here, as the memdebug_ptrchk() message
	 * isn't appropriate.
	 */
	if (head->magic == FREE_MAGIC)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY: Block has already been free()'d; detected at",
				   file, line);
		memdebug_bogosity(head);
	}

	/*
	 * Do the standard set of sanity checks on the raw memory pointer.
	 */
	rc = memdebug_traced_ptrchk(mem, file, line);
	
	if (rc)

	{
		memdebug_printf("--Detected during free()\n");
#if 0
		/* 
		 * enable this if you want a trace of the stack at the 
		 * point where the free is called
		 */
		dump_stack_trace_bis(memdebug_printf, 20);
#endif
		/* Only bail on really bad errors */
		if (rc == -1)
			return -1;
	}

	*blocksize = head->reqsize;
	return 0;
}
