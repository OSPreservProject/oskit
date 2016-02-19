/*
 * Copyright (c) 1994-1997 The University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University.
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	oskit/page.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Machine independent virtual memory parameters.
 *	In this file, the term "page" refers to the _minimum_ page size
 *	supported by the hardware architecture, and is always a power of two.
 *	Some architectures and OS environments support optional
 *	larger page sizes as well, and/or unaligned "block mappings".
 */
#ifndef	_OSKIT_PAGE_H_
#define _OSKIT_PAGE_H_

#include <oskit/machine/page.h>

#ifndef PAGE_SHIFT
#error oskit/machine/page.h needs to define PAGE_SHIFT.
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE (1 << PAGE_SHIFT)
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (PAGE_SIZE-1)
#endif

/*
 *	Convert addresses to pages and vice versa.
 *	No rounding is used.
 */

#define atop(x)		(((oskit_size_t)(x)) >> PAGE_SHIFT)
#define ptoa(x)		((oskit_addr_t)((x) << PAGE_SHIFT))

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define round_page(x)	((oskit_addr_t)((((oskit_addr_t)(x)) + PAGE_MASK) & ~PAGE_MASK))
#define trunc_page(x)	((oskit_addr_t)(((oskit_addr_t)(x)) & ~PAGE_MASK))

/*
 *	Determine whether an address is page-aligned, or a count is
 *	an exact page multiple.
 */

#define	page_aligned(x)	((((oskit_addr_t) (x)) & PAGE_MASK) == 0)

#endif	/* _OSKIT_PAGE_H_ */
