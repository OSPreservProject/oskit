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
 * A simple utility function to print out file and line info in a
 * consistent way.
 */

#include "memdebug.h"

/*
 * memdebug_info_dump()
 *
 * Consistently print out the file info, if we have any.
 */
void 
memdebug_info_dump(const char *str, const char *file, int line)
{
	const char *chk;

	/*
	 * Since this package is used for debugging hairy
	 * memory problems, it shouldn't puke when the
	 * file name info is clobbered by some unruly process.
	 * So, we do some ad hoc sanity checks on the filename.
	 */
	chk = file;
	while (chk && *chk)
	{
		if (((*chk) < 32) /* signed, so < 32 includes > 127 */
		    || ((chk - file) > 200)) {
			file = "<pointer STOMPED>";
			break;
		}
		chk++;
	}
	
	memdebug_printf("%s ", str);
	if (file)
		memdebug_printf("(%s::%d)\n", file, line);
	else
		memdebug_printf("(unknown file)\n");
#if DUMP_STACK_WITH_INFO
	dump_stack_trace_bis(memdebug_printf, 16);
#endif
}
