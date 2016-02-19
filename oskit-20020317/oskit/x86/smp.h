/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#ifndef _OSKIT_X86_SMP_H_
#define _OSKIT_X86_SMP_H_

#include <oskit/x86/types.h>

#define __SMP__		/* Used by the Linux code */
#define NR_CPUS 32

extern unsigned long apic_addr;
#define apic_reg ((unsigned char *)(apic_addr))

#define APIC_SIZE 4096	/* 4096*256 reserverd in SMP Spec */

/* Used to ack the apic at the end of the IPI */
void smp_apic_ack();

/*
 * IPIs look like hardware Int 13
 * Since this isn't used anymore by the coprocessor, this
 * should be an unused vector on a PC.
 */
#define SMP_IPI_VECTOR	13

#endif /* _OSKIT_X86_SMP_H_ */
