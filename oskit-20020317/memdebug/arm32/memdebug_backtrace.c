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
 * Stores a stack backtrace in the provided buffer.
 *
 * XXX This should probably be made more generic and moved into
 *     stack_trace.c in libc.
 */

#include "memdebug.h"

void
memdebug_store_backtrace(unsigned *backtrace, int max_len)
{
        unsigned *fp;
        unsigned i;

        asm volatile("mov %0,%%fp" : "=r" (fp));

	if (fp)
		fp = (int *)(*(fp - 3));  /* Skip frame for this function */
	
	for (i = 0; i < max_len; i++)
	{
		if (fp)
		{
			backtrace[i] = fp[-1];
			fp = (unsigned*)fp[-3];
		}
		else
			backtrace[i] = 0;
	}

}
