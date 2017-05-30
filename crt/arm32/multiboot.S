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
 * Startup code to create MultiBoot-compliant kernel images.
 * You must link this at the beginning of the kernel image.
 * Furthermore, you must link crtn.o at the end of the kernel image,
 * after all of your .o's _and_ after all libraries you link with.
 * (Actually, crtn.o is only strictly required under ELF,
 * but its use is recommended in any case for consistency.)
 *
 * The MultiBoot header defined here assumes none of the specific
 * feature flag bits need to be set
 * (other than the a.out-kludge bit, in the case of a.out format).
 * If you need to use additional features,
 * you will need to define your own MultiBoot header -
 * e.g., just copy this file and modify it as appropriate.
 */

#include <oskit/machine/asm.h>

	.text

	/* 
	 * We are only entered this way when the boot loader doesn't
	 * have the a.out kludge info and assumes the lowest text addr
	 * is the entry point.
	 */
ENTRY(_start)
	b	boot_entry

	/* MultiBoot header - see multiboot.h.  */
	P2ALIGN(2)
boot_hdr:
	.long	0x1BADB002		/* magic */
#ifdef __ELF__
	.long	0x00000000		/* feature flags */
	.long	0-0x1BADB002-0x00000000	/* checksum */
#else /* a.out kludge */
	.long	0x00010000		/* feature flags */
	.long	0-0x1BADB002-0x00010000	/* checksum */
	.long	boot_hdr		/* header_addr */
	.long	EXT(_start)		/* load_addr */
	.long	EXT(edata)		/* load_end_addr */
	.long	EXT(end)		/* bss_end_addr */
	.long	boot_entry		/* entry */
#endif

boot_entry:

	/* Switch to our own stack.  */
	ldr	sp,Lstack

	/* Backtrace back starts here */	
	mov	fp, #0x00000000	

	/* Jump into C code.  */
	bl	EXT(multiboot_main)
	/*
	 * Indirect for stack. Something to do with object file format.
	 */
Lstack:		.word EXT(base_stack_end)

	/*
	 * Drag this symbol in so that the version is always included
	 * in the linked image.
	 */
Lver:		.word EXT(oskit_version_string)

	/*
	 * Stub out the __oskit_init and __oskit_fini stuff till later.
	 */
GLEXT(__oskit_init)
	mov	pc, lr

GLEXT(__oskit_fini)
	mov	pc, lr

	/*
	 * The ARM/ELF compiler sticks this in. Silly.
	 */
GLEXT(__gccmain)
	mov	pc, lr

	/*
	 * Create and align an initial data segment.
	 */
#include <oskit/machine/page.h>

	.data
	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(start_of_data)
LEXT(start_of_data)
	.long	0

	/*
	 * This symbol matches the one in unix/crt0.c. Since usermode
	 * simulation is "important," this symbol is allowed to be
	 * here so it can be referenced in oskit libraries. Jeez. 
	 */
	.globl	EXT(oskit_usermode_simulation)
LEXT(oskit_usermode_simulation)
	.long	0
