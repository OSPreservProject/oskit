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

#ifndef _OSKIT_ARM32_TRAP_H_
#define _OSKIT_ARM32_TRAP_H_

/*
 * Hardware trap vectors for the StrongARM
 */
#define T_RESET			0
#define T_UNDEF			1		/* undefined instruction */
#define T_SWI			2               /* software interrupt */
#define T_PREFETCH_ABORT	3               /* instruction page fault */
#define T_DATA_ABORT		4               /* data page fault */
#define T_ADDREXC		5		/* manual says undefined? */
#define T_IRQ			6		/* Interrupt Request */
#define T_FIQ			7		/* Fast Interrupt Request */

/*
 * This one is an extra, and it is handy! The assembly trap handler
 * will look for stack overflow, and generate a T_STACK_OVERFLOW trap
 * (using a special overflow stack).  The default behaviour is to turn
 * that into a SIGBUS.
 */
#define T_STACK_OVERFLOW	8

/*
 * Better name! Easier to remember.
 */
#define T_PAGE_FAULT		T_DATA_ABORT

#endif /* _OSKIT_ARM32_TRAP_H_ */
