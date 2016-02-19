/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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

#ifndef _OSKIT_ARM32_UVM_H_
#define _OSKIT_ARM32_UVM_H_

/*
 *  OSKit UVM Virtual Memory Layout
 *
 *  XXX fixme XXX
 */

/* should be same to USERBASE in pmap.h */
#define OSKIT_UVM_MINUSER_ADDRESS		0x0
/* (PDSLOT_PTE << PDSHIFT) - UPAGES*NBPG */
#define OSKIT_UVM_MAXUSER_ADDRESS		0x0
/* (PDSLOT_PTE << PDSHIFT) + (PDSLOT_PTE << PGSHIFT) */
#define OSKIT_UVM_MAX_ADDRESS			0x0
/* PDSLOT_KERN << PDSHIFT */
#define OSKIT_UVM_MIN_KERNEL_ADDRESS		0x0
/* PDSLOT_APTE << PDSHIFT */
#define OSKIT_UVM_MIN_KERNEL_VIRTUAL_AVAIL	0x0
/* PDSLOT_APTE << PDSHIFT */
#define OSKIT_UVM_MAX_KERNEL_ADDRESS		0x0

#ifdef COMPILING_UVM
#define VM_MIN_ADDRESS			((vaddr_t)OSKIT_UVM_MINUSER_ADDRESS)
#define VM_MAXUSER_ADDRESS		((vaddr_t)OSKIT_UVM_MAXUSER_ADDRESS)
#define VM_MAX_ADDRESS			((vaddr_t)OSKIT_UVM_MAX_ADDRESS)
#define VM_MIN_KERNEL_ADDRESS		((vaddr_t)OSKIT_UVM_MIN_KERNEL_ADDRESS)
#define VM_MIN_KERNEL_VIRTUAL_AVAIL	((vaddr_t)OSKIT_UVM_MIN_KERNEL_VIRTUAL_AVAIL)
#define VM_MAX_KERNEL_ADDRESS		((vaddr_t)OSKIT_UVM_MAX_KERNEL_ADDRESS)
#endif /*COMPILING_UVM*/

#endif /*_OSKIT_ARM32_UVM_H_*/
