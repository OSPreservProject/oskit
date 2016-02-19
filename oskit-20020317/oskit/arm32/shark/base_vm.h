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
 * Definitions for the base VM environment provided by the OS toolkit:
 * how to change pointers between physical, virtual, and linear addresses, etc.
 */
#ifndef	_OSKIT_ARM32_SHARK_BASE_VM_H_
#define _OSKIT_ARM32_SHARK_BASE_VM_H_

#include <oskit/arm32/types.h>

/*
 * Simple first cut! 
 */
#define KERNEL_VIRTUAL_REGION		0x10000000

/* Calculate a kernel virtual address from a physical address.  */
#define phystokv(pa)	((oskit_addr_t)(pa) | KERNEL_VIRTUAL_REGION)

/* Same, but in reverse. */
#define kvtophys(va)	((oskit_addr_t)(va) & ~KERNEL_VIRTUAL_REGION)

#endif /* _OSKIT_ARM32_SHARK_BASE_VM_H_ */
