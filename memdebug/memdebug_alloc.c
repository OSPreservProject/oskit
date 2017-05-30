/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * This file implements the core routines in setting up the fence posts,
 * checking them, and complaining when something goes wrong.
 */

#include <assert.h>
#include <string.h>
#include <oskit/lmm.h>

#include "memdebug.h"

memdebug_mhead memdebug_all_head;

#if NO_MEM_FATAL
#define RETURN_NULL panic("libmemdebug malloc failure")
#else
#define RETURN_NULL return 0
#endif


/*
 * memdebug_malloc()
 *
 * Do a memory allocation, setup the fence posts, and record all the
 * information.
 * This is written for the most restrictive case, an smemalign().
 * All of the other memory allocations can be written in terms of this.
 * (And they are..)
 */
void* 
memdebug_alloc(size_t alignment, size_t bytes, unsigned lmm_flags,
	       unsigned char initval, char caller, char *file, int line)
{
	memdebug_mhead *head = NULL;
	memdebug_mtail *tail = NULL;
	size_t req_size;
	size_t req_align;
	unsigned align_shift;
	unsigned i;

	/* No longer supporting this interface! */
	assert(lmm_flags == 0);

	assert((caller == SMALLOC_CALLER)
	       || (caller == MALLOC_CALLER));

	DPRINTF(" align= %d, size= %d, init= %2.2x\n",
		alignment, bytes, initval);
	DPRINTF(" file = %s\n",	(file ? file : "N/A"));
	DPRINTF(" line = %d\n", (file ? line : 0));

#if ALLOW_MORALLY_QUESTIONABLE_PRACTICE
# if MALLOC_0_RET_NULL
	if (bytes == 0)
	{
		RETURN_NULL; 
	}
# endif
#else /* Do not allow such practices */
	if (bytes == 0)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY: Tried to allocate 0 bytes at", file, line);
# if MALLOC_0_RET_NULL
		RETURN_NULL;
# endif
	}
#endif
	if ((bytes < 0) || (bytes > (1 * 1024 * 1024 * 1024)))
	{
		memdebug_printf("MEMDEBUG BOGOSITY: ");
		memdebug_printf("Tried to allocate chunk of size %d (unsigned: %u)", 
				bytes, bytes);
		memdebug_info_dump(" at", file, line);
		RETURN_NULL;
	}
	if (alignment < 0)
	{
		memdebug_printf("MEMDEBUG BOGOSITY: ");
		memdebug_printf("Tried to allocate chunk with alignment %d",
				alignment);
		memdebug_info_dump(" at", file, line);
		RETURN_NULL;
	}
	
	
	/*
	 * Compute size of allocation with header and tail.  Figure
	 * out the alignment (if its less than the size of the
	 * memdebug_head, and non-zero, we have to align to the 
	 * aligned memdebug_mhead size.)
	 */
	req_size = word_align(bytes)
		 + sizeof(memdebug_mhead) + sizeof(memdebug_mtail);

	/* Double check the value of NP2_SZ_MHEAD */
	assert(((sizeof(memdebug_mhead) << 1) > NP2_SZ_MHEAD)
	       && (sizeof(memdebug_mhead) < NP2_SZ_MHEAD));

	if (alignment && (alignment < NP2_SZ_MHEAD))
		req_align = NP2_SZ_MHEAD;
	else
		req_align = alignment;

	assert((req_align == 0)  
	       || ((int)req_align - (int)sizeof(memdebug_mhead)) > 0);
			
	/* Find the alignment shift in bits.  XXX use proc_ops.h  */
	for (align_shift = 0; (1 << align_shift) < req_align; align_shift++)
		/* nothing */;

	DPRINTF("req_size= %d, req_align= %d\n",
		req_size, req_align);
	DPRINTF("(align= %d; min_align= %d)\n",
		alignment, (int)NP2_SZ_MHEAD);
	DPRINTF("align_shift= %d, hd offset = %d\n",
		align_shift, req_align - sizeof(memdebug_mhead));

	if (!(head = memdebug_untraced_alloc(req_size, align_shift,
					     req_align ? req_align -
					     sizeof(memdebug_mhead) : 0))) {
		memdebug_info_dump("MEMDEBUG BOGOSITY: "
				   "Ran out of memory! @ ", file, line);
		RETURN_NULL;
	}

	/* Initialize the memory header.  */
	head->reqsize = bytes;
	bytes = word_align(bytes);
	head->magic = (unsigned)(HEAD_MAGIC);
	head->size = bytes;
	head->flags = NO_FLAGS;
	for (i = 0; i < MHEAD_DEADBEEF; i++)
		head->deadbeef[i] = 0xdeadbeef;
	head->file = file;
	head->line = line;
	if (caller == SMALLOC_CALLER)
		head->flags |= SMALLOC_CREATED;
	else
		head->flags &= ~SMALLOC_CREATED;


	/*
	 * Create a backtrace of the stack and store it in the header.  
	 */
	memdebug_store_backtrace(head->backtrace, MHEAD_BTLEN); 
	
	/* Initialize the memory tailer.  */
	tail = (memdebug_mtail*)((void*)(head + 1) + bytes);
	for (i = 0; i < MTAIL_DEADBEEF; i++)
		tail->deadbeef[i] = 0xdeadbeef;
	tail->size = bytes;
	tail->magic = (unsigned)(TAIL_MAGIC);

	/* Initialize the memory block itself,
	   either to garbage or to zeros, as appropriate.  */
	memset(head + 1, initval, bytes);

	/*
	 * Add it to the global memory list. Use mem locks to keep it
	 * thread-safe.
	 */
	mem_lock();
	if (memdebug_all_head.next == 0)
	{
		memdebug_all_head.next = memdebug_all_head.prev = &memdebug_all_head;
	}
	head->next = &memdebug_all_head;
	head->prev = memdebug_all_head.prev;
	head->prev->next = head;
	memdebug_all_head.prev = head;
	mem_unlock();
	
	DPRINTF("Done\n");

	return head + 1;
}



