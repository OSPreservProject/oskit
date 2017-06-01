/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#ifndef	_OSKIT_X86_EFLAGS_H_
#define	_OSKIT_X86_EFLAGS_H_

/*
 *	i386 flags register
 */
#define	EFL_CF		0x00000001		/* carry */
#define	EFL_PF		0x00000004		/* parity of low 8 bits */
#define	EFL_AF		0x00000010		/* carry out of bit 3 */
#define	EFL_ZF		0x00000040		/* zero */
#define	EFL_SF		0x00000080		/* sign */
#define	EFL_TF		0x00000100		/* trace trap */
#define	EFL_IF		0x00000200		/* interrupt enable */
#define	EFL_DF		0x00000400		/* direction */
#define	EFL_OF		0x00000800		/* overflow */
#define	EFL_IOPL	0x00003000		/* IO privilege level: */
#define	EFL_IOPL_KERNEL	0x00000000			/* kernel */
#define	EFL_IOPL_USER	0x00003000			/* user */
#define	EFL_NT		0x00004000		/* nested task */
#define	EFL_RF		0x00010000		/* resume without tracing */
#define	EFL_VM		0x00020000		/* virtual 8086 mode */
#define	EFL_AC		0x00040000		/* alignment check */
#define	EFL_VIF		0x00080000		/* virtual interrupt flag */
#define	EFL_VIP		0x00100000		/* virtual interrupt pending */
#define	EFL_ID		0x00200000		/* CPUID instruction support */

#endif	/* _OSKIT_X86_EFLAGS_H_ */
