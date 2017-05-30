/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#ifndef _PROC_H_
#define _PROC_H_

#include <oskit/uvm.h>

/* The start address of heap in a user process */
/* 0x50000000 */
#define HEAP_START_ADDR	(OSKIT_UVM_MINUSER_ADDRESS + 0x10000000)

/* just 64kb */
#define HEAP_SIZE	0x10000

/* 0x40000000 - 0x60000000 */
#define PROCESS_SIZE	0x20000000

#endif /*_PROC_H_*/
