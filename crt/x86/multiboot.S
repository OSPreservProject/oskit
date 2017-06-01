/*
 * Copyright (c) 1996, 1998, 1999, 2000 University of Utah and the Flux Group.
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

#include <oskit/x86/asm.h>

	.text

	/*
	 * See multiboot_dyn.S, where this actually means something different
	 * than _start. 
	 */
	.globl	_start_of_kernel
_start_of_kernel:

	/* 
	 * We are only entered this way when the boot loader doesn't
	 * have the a.out kludge info and assumes the lowest text addr
	 * is the entry point.
	 */
ENTRY(_start)
	jmp	boot_entry

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
	movl	$EXT(base_stack_end),%esp

	/*
	 * Reset EFLAGS to a known state.
	 * This will also clear the DF flag for the string-clear below.
	 */
	pushl	$0
	popf

	/* Clear uninitialized data.  */
	/* XXX this shouldn't be necessary, as the multiboot spec now says
	   that the bootloader does this rather than the OS kernel.
	   Remove when we're sure all boot loaders have been updated. */
	lea	EXT(edata),%edi
	lea	EXT(end),%ecx
	subl	%edi,%ecx
	xorl	%eax,%eax
	rep
	stosb

	/* 
	 * Turn any floppies off (the motors).
	 * This is in case we were booted from a floppy.
	 * We don't want the damn thing to spin forever and wear a hole in
	 * our disk.
	 */
	xorb	%al,%al
	movw	$0x3f2,%dx		/* all floppies on primary adapter */
	outb	%al,%dx
#if 0
	movw	$0x372,%dx		/* all floppies on secondary adapter */
	outb	%al,%dx
#endif

	/* Push the boot_info parameter for multiboot_main()
	   before ebx gets trashed by the initialization code in crti.h.  */
	pushl	%ebx

#include "crti.h"

	/* Jump into C code.  */
	call	EXT(multiboot_main)

	/*
	 * Create and align an initial data segment.
	 */
#include <oskit/x86/paging.h>

	.data
	P2ALIGN(PAGE_SHIFT)
	.globl	EXT(start_of_data)
LEXT(start_of_data)
	.long	0

	/*
	 * Drag this symbol in so that the version is always included
	 * in the linked image.
	 */
.L69:
	.long EXT(oskit_version_string)

	/*
	 * This symbol matches the one in unix/crt0.c. Since usermode
	 * simulation is "important," this symbol is allowed to be
	 * here so it can be referenced in oskit libraries. Jeez. 
	 */
	.globl	EXT(oskit_usermode_simulation)
LEXT(oskit_usermode_simulation)
	.long	0
