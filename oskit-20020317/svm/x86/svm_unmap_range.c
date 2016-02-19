/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

#include <svm/svm_internal.h>

/*
 * Unmap a range of memory. This routine translates from SVM mapping
 * bits and other goo, to the machine dependent bits and the pdir
 * functions in the kernel. In this case (x86) the mapping is easy
 * since we can represent all the bits we want in the PTE, without
 * having to munge them into shape.
 *
 * SVMLOCK should be held.
 */
int
svm_unmap_range(oskit_addr_t va, oskit_size_t size)
{
	pdir_unmap_range(base_pdir_pa, va, size);

	while (size > 0) {
		ftlbentry(va);

		va   += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	return 0;
}
