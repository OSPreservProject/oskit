/*
 * Copyright (c) 1996, 1998, 2000 University of Utah and the Flux Group.
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
 * This routine does a series of sanity checks on a pointer.  Returns
 * zero if no errors.  no-zero if something is wrong.  It also prints
 * a bogosity message on an error.
 */
#include "memdebug.h"

/*
 * Incase the (l)user doesn't #include<memdebug.h>, this will
 * allow them to make calls without the macro, and from
 * within other memdebug functions.
 */
int memdebug_ptrchk(void *ptr)
{
	return memdebug_traced_ptrchk(ptr, 0, 0);
}

/*
 * memdebug_traced_ptrchk()
 *
 * Runs lots of sanity checks on a pointer and its fence posts.
 * Returns -1 if a bad error occured (ie, the data in the fence
 * posts can't be trusted--esp, the "next" ptr.);  1 if there
 * was a problem but its not necessarily fatal; or 0 if there
 * were no problems.
 */
int memdebug_traced_ptrchk(void *ptr, char *file, int line)
{
	memdebug_mhead *head;
	memdebug_mtail *tail;
	int magic_trashed;
	int rc = 0;
	int i;
		
	DPRINTF("checking pointer 0x%p\n", ptr);

	if (ptr == 0)
	{
		memdebug_info_dump("%s: ptr is NULL at", file, line);
		return -1;
	}

	/* Find and check the memory block header.  */
	head = (memdebug_mhead*)ptr - 1;
	if (head->magic == FREE_MAGIC)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY:"
				   " block has been free()'d at",
				   file, line);
		memdebug_bogosity(head);
		rc = 1;
	}
	if (head->magic != HEAD_MAGIC)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY:"
				   " Bogus pointer (head magic wrong)"
				   " at", file, line);
		/* memdebug_bogosity(head); */
		return -1;
	}
	magic_trashed = 0;
	for (i = 0; i < MHEAD_DEADBEEF; i++)
	{
		if (head->deadbeef[i] != 0xdeadbeef)
			magic_trashed++;
	}

	if(magic_trashed)
	{
		memdebug_printf("MEMDEBUG BOGOSITY:"
				" Block underflow"
				" (%d of %d buffer ints trashed)",
				magic_trashed, MHEAD_DEADBEEF);
		memdebug_info_dump(" at", file, line);
		memdebug_bogosity(head);
		rc = 1;
	}
	
	/* Find and check the memory block tailer.  */
	tail = (memdebug_mtail*)((void*)(head + 1) + head->size);

	magic_trashed = 0;
	for (i = 0; i < MTAIL_DEADBEEF; i++)
	{
		if (tail->deadbeef[i] != 0xdeadbeef)
			magic_trashed++;
	}

	if (magic_trashed)
	{
		memdebug_printf("MEMDEBUG BOGOSITY:"
				" Block overflow"
				" (%d of %d buffer ints trashed)",
				magic_trashed, MTAIL_DEADBEEF);
		memdebug_info_dump(" at", file, line);
		memdebug_bogosity(head);
		rc = 1;
	}

	if (tail->magic != TAIL_MAGIC)
	{
		memdebug_info_dump("MEMDEBUG BOGOSITY:"
				   " Trailer magic trashed"
				   " at", file, line);
		memdebug_bogosity(head);
		rc = 1;
	}

	if ((rc == 0) && (tail->size != head->size))
	{
		memdebug_printf("MEMDEBUG BOGOSITY:"
				" Head and tail sizes don't match"
				" at", file, line);
		memdebug_bogosity(head);
		rc = 1;
	}

	return rc;
}

