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
 * IStream wrapper for interrupt-driven direct console
 */

#include <oskit/com/stream.h>
#include <oskit/com/trivial_stream.h>

#include <oskit/x86/pc/direct_cons.h>
#include <oskit/x86/pc/intr_cons.h>
#include <oskit/x86/pc/direct_console.h>

static struct oskit_trivial_stream intr_console_impl = {
	streami: { &oskit_trivial_stream_ops },
	getchar: intr_cons_getchar,
	putchar: direct_cons_putchar
};

oskit_error_t
intr_console_init(oskit_stream_t **out_stream)
{
	intr_cons_init();

	*out_stream = &intr_console_impl.streami;
	return 0;
}
