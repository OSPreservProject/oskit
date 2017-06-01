/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
#include <stdlib.h>
#include <oskit/dev/dev.h>

/*
 * Allocate a buffer for use as an skbuff.
 * Returns a pointer to a buffer of at least the indicated size.  Also returns
 * the actual size of the buffer allocated.  Any additional space returned may
 * be used by the caller.  The caller should present the same size when freeing
 * the buffer.  Flags are OSKIT_MEM_ (aka OSENV_) allocation flags used to
 * indicate special memory needs.
 *
 * This routine may be called at hardware interrupt time and should protect
 * itself as necessary.
 */
void *
oskit_skbufio_mem_alloc(oskit_size_t size, int mflags, oskit_size_t *out_size)
{
	void *ptr;
	int intson;

	intson = osenv_intr_save_disable();
	ptr = malloc(size);
	if (intson)
		osenv_intr_enable();

	*out_size = size;
	return ptr;
}

/*
 * Free a buffer used as an skbuff.
 * The pointer and size passed should match those returned by a call to
 * oskit_skbufio_mem_alloc.
 *
 * This routine may be called at hardware interrupt time and should protect
 * itself as necessary.
 */
void
oskit_skbufio_mem_free(void *ptr, unsigned int size)
{
	int intson;

	intson = osenv_intr_save_disable();
	free(ptr);
	if (intson)
		osenv_intr_enable();
}
