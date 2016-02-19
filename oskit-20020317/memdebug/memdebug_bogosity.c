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
 * This is the vehicle through which memdebug complains when it finds an error.
 */

#include "memdebug.h"

/*
 * memdebug_bogosity()
 *
 * For error messages generated in this package.  Prints the backtrace
 * stored with each allocation.
 */
void
memdebug_bogosity(memdebug_mhead *head)
{
	if (head)
	{
		unsigned i;

		memdebug_info_dump("Block allocated by", head->file, head->line);
		memdebug_printf("head: %p\n", head);
		memdebug_printf("Block addr= %p, a %d byte malloc.\n",
				head + 1, head->reqsize);
		memdebug_printf("Backtrace:");

		for (i = 0; i < MHEAD_BTLEN; i++)
		{
			if (head->backtrace[i] == 0)
				break;
			/*
			 * This is formated to match the output of
			 * dump_stack_trace()
			 */
			memdebug_printf(" %08x", head->backtrace[i]);
		}
		memdebug_printf("\n");
	}

	/* XXX fflush(stdout); */
}
