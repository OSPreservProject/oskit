/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Boot Loader, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <oskit/x86/asm.h>

	.data
	
	.globl	EXT(pxenv_segment)
LEXT(pxenv_segment)
	.long	0
	
	.globl	EXT(pxenv_offset)
LEXT(pxenv_offset)
	.long	0

	.globl	EXT(pxe_entrypoint)
LEXT(pxe_entrypoint)
	.long	0
	
	.globl	EXT(pxe_routine)
LEXT(pxe_routine)
	.long	0

	.globl	EXT(pxe_databucket)
LEXT(pxe_databucket)
	.long	0

	.text
	.code16
	.globl _start

#define STACK_SIZE 4096

/*
 * This is the entrypoint, as invoked from the PXE boot ROM goop.
 * The calling protocol is as follows. The boot loader must fit
 * in 32k. When invoked, the registers are:
 *
 *	CS:IP contains the value 0:7C00h.
 *	ES:BX contains the address of the PXENV+ structure.
 *	SS:[SP+4] contains the segment:offset address of the !PXE structure.
 *	EDX is no longer used. 
 *	SS:SP contains the address of usuable stack (at least 1.5k free).
 *
 */
_start:
	jmp	_start_two

	/*
	 * I'm gonna drop the MBR right here if we are told to boot from disk.
	 */

	P2ALIGN(9)
_start_two:
	cld

	/*
	 * Setup the segments, which are zero.
	 */
	xorw	%ax, %ax
	movw	%ax, %ds

	/* get the pointer to PXEENV+ structure */
	movw    %es,EXT(pxenv_segment)
	movw	%bx,EXT(pxenv_offset)

	/* and then zero es. */
	movw	%ax, %es

	/* Switch to a stack in the same segment as everything else.  */
	movw	%ax, %ss
	
	/*
	 * 4K is not enough stack space, but we are limited to the 32K
	 * (total) in the top half of the first segment. Well, the freebsd 
	 * code lets the stack grow down into the bottom half of the segment,
	 * in front of the location the NBP is loaded at (0x7c00). Lets
	 * do the same, cause we really need the extra room, and cannot
	 * afford to lose 4K to stack anyway.
	 */
	movw	$_start,%sp
	
	/* Clear our BSS segment.  */
	movl	$EXT(edata),%edi
	movl	$EXT(end),%ecx
	subw	%di,%cx
	xorb	%al,%al
	cld
	rep
	stosb

	/* Make backtraces terminate */
	xorl	%ebp,%ebp

	jmp	EXT(i16_bios_main)



/*
 * Trampoline to the PXE
 */
ENTRY(i16_pxe_invoke)
	pushl	%ebp
	movl	%esp, %ebp

	/*
	 * Push 3 16bit values:
	 *
	 * 1) Our data segment (which is zero),
	 * 2) The address of the data buffer/argument block,
	 * 3) The code for the PXE routine to invoke.
	 */
	xor	%ax, %ax
	push	%ax
	movw	EXT(pxe_databucket),%ax
	push    %ax
	movw	EXT(pxe_routine),%ax
	push	%ax

	/*
	 * Intersegment call.
	 */
	.byte	0xff, 0x1e
	.long   EXT(pxe_entrypoint)

	pop	%dx
	pop	%dx
	pop	%dx

	leave
	ret

/*
 * This diddy just gets the MBR going.
 */
ENTRY(i16_boot_mbr)
	jmp	_start
	ret
