/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
#include <oskit/config.h>
#ifdef HAVE_CODE16

#include <oskit/x86/asm.h>

#include "x86_asm.h"

.text

#ifdef HAVE_CODE16GCC
.code16gcc
#else
.code16
#endif


/* void i16_base_gdt_init(void) */
ENTRY(i16_base_gdt_init)
	pushl %ebp
	movl %esp, %ebp

	/* Initialize the base TSS descriptor.  */
	pushl $0					/* sizebits */
	pushl $(ACC_PL_K | ACC_TSS | ACC_P)		/* access flags */
	pushl $(BASE_TSS_SIZE - 1)			/* limit */

	/* Have to kvtolin the address of base_tss. */
ADDR32	movl EXT(linear_base_va), %edx
ADDR32	movl $EXT(base_tss), %eax
	subl %edx, %eax
	pushl %eax					/* size */

ADDR32	pushl $(EXT(base_gdt) + BASE_TSS)		/* address of gdt entry */
	call  EXT(i16_fill_descriptor)
	addl  $20, %esp

	/* Initialize the 32-bit kernel code and data segment descriptors
	   to point to the base of the kernel linear space region.  */
	pushl $SZ_32
	pushl $(ACC_PL_K | ACC_CODE_R)
	pushl $0xffffffff
ADDR32	movl EXT(linear_base_va), %edx			/* kvtolin(0) */
	negl  %edx
	pushl %edx
ADDR32	pushl $(EXT(base_gdt) + KERNEL_CS)
	call EXT(i16_fill_descriptor)
	addl $20, %esp

	pushl $SZ_32
	pushl $(ACC_PL_K | ACC_DATA_W)
	pushl $0xffffffff
ADDR32	movl EXT(linear_base_va), %edx
	negl  %edx
	pushl %edx
ADDR32	pushl $(EXT(base_gdt) + KERNEL_DS)
	call EXT(i16_fill_descriptor)
	addl $20, %esp

	/* Corresponding 16-bit code and data segment descriptors,
	   typically used when entering and leaving real mode.  */
	pushl $SZ_16
	pushl $(ACC_PL_K | ACC_CODE_R)
	pushl $0xffff
ADDR32	movl EXT(linear_base_va), %edx
	negl  %edx
	pushl %edx
ADDR32	pushl $(EXT(base_gdt) + KERNEL_16_CS)
	call EXT(i16_fill_descriptor)
	addl $20, %esp

	pushl $SZ_16
	pushl $(ACC_PL_K | ACC_DATA_W)
	pushl $0xffff
ADDR32	movl EXT(linear_base_va), %edx
	negl  %edx
	pushl %edx
ADDR32	pushl $(EXT(base_gdt) + KERNEL_16_DS)
	call EXT(i16_fill_descriptor)
	addl $20, %esp

	/* Descriptors that direct-map all linear space.  */
	pushl $SZ_32
	pushl $(ACC_PL_K | ACC_CODE_R)
	pushl $0xffffffff
	pushl $0
ADDR32	pushl $(EXT(base_gdt) + LINEAR_CS)
	call EXT(i16_fill_descriptor)
	addl $20, %esp

	pushl $SZ_32
	pushl $(ACC_PL_K | ACC_DATA_W)
	pushl $0xffffffff
	pushl $0
ADDR32	pushl $(EXT(base_gdt) + LINEAR_DS)
	call EXT(i16_fill_descriptor)

	leave
	ret

#endif /* HAVE_CODE16 */
