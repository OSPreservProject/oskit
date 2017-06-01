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

/*
 * Code to set up a basic paged memory environment on the SHARK.
 *
 * The OFW controls this stuff at boot time, so pass this off to
 * the OFW code to duplicate the OFW's page tables into our new table.
 */

#include <oskit/debug.h>
#include <oskit/arm32/base_paging.h>
#include <oskit/arm32/ofw/ofw.h>

void base_paging_init(void)
{
	if (pdir_alloc(&base_pdir_pa))
		panic("Can't allocate kernel page directory");

	ofw_paging_init(base_pdir_pa);
}

