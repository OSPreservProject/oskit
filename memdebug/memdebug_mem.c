/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
#include <oskit/c/environment.h>

/*
 * Just what we need, another level of indirection...
 *
 * These are the routines that memdebug calls to actually get memory.
 * They can be overridden as necessary.
 */

void *
memdebug_untraced_alloc(oskit_u32_t size,
			oskit_u32_t align_bits, oskit_u32_t align_ofs)
{
	if (!check_memory_object()) return NULL;

	return oskit_mem_alloc_gen(libc_memory_object, size, 0,
				   align_bits, align_ofs);
}

void
memdebug_untraced_free(void *ptr, oskit_u32_t size)
{
	oskit_mem_free(libc_memory_object, ptr, size, 0);
}
