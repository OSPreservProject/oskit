/*
 * Copyright (c) 1996-1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/smp.h>
#include <oskit/x86/types.h>
#include <oskit/c/stdio.h>

/*
 * This is the default implementation, that doesn't do paging
 * or address translation.
 */

oskit_addr_t smp_map_range(oskit_addr_t start, oskit_size_t size)
{
	printf("default smp_map_range!\n");
	return start;
}

int smp_unmap_range(oskit_addr_t start, oskit_size_t size)
{
	printf("default smp_unmap_range!\n");
	return 0;
}
