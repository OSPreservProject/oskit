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
 * IStream wrapper for direct console
 */

#include <oskit/com/stream.h>
#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/direct_console.h>

#include <oskit/com/trivial_stream.h>


/*
 * This is exported in case anyone wants to use it before
 * calling direct_console_init.  This might be necessary at
 * boot time.
 *
 * Everyone outside this file should declare it as oskit_stream_t.
 * Efforts to give it that type in this file failed miserably.
 */

struct oskit_trivial_stream boot_direct_console = {
	streami: { &oskit_trivial_stream_ops },
	getchar: direct_cons_getchar,
	putchar: direct_cons_putchar
};

oskit_error_t
direct_console_init(oskit_stream_t **out_stream)
{
	*out_stream = &boot_direct_console.streami;
	return 0;
}
