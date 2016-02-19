/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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
 * Define a stub that copies the kernel into its final resting place,
 * and then "calls" it.
 */

#include <oskit/machine/asm.h>
#include <oskit/x86/multiboot.h>

	.text

/*
 *	%eax	entry point
 *	%esi	where kernel is
 *	%edi	where kernel wants to be
 *	%ecx	how big kernel is
 *	%ebx	addr of multiboot_info struct
 *	%edx	segment we should switch to
 */
ENTRY(do_boot)
	/* Set all the data segs to point to %dx. */
	movw	%dx,%ds
	movw	%dx,%es
	movw	%dx,%fs
	movw	%dx,%gs
	movw	%dx,%ss

	rep
	movsb

	/* copy the entry point to %ecx and put multiboot magic into %eax */
	movl	%eax,%ecx
	movl	$MULTIBOOT_VALID,%eax
	jmp	*%ecx
ENTRY(do_boot_end)

