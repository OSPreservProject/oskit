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
 * Define a stub that copies the stashed netboot kernel clone
 * down to its load address and then jumps in.
 */

#include <oskit/machine/asm.h>
#include "reboot.h"

	.text

ENTRY(reboot)
	/*
	 * Load %eax with the address of the reboot_info struct.
	 * The REBOOT_INFO_MAGIC will be replaced with an appropriate
	 * address after we are stashed in high memory.
	 */
	movl	$REBOOT_INFO_MAGIC,%eax

	/* 
	 * Store the REBOOT_INFO_MAGIC back in ourselves before copying down
	 * so the startup code can again replace it with the appropriate
	 * address
	 */
	movl	12(%eax),%ebx
	movl	$REBOOT_INFO_MAGIC,(%ebx)

	/* Save the arg into %ebx. */
	movl	8(%eax),%ebx			/* arg */

	/* Copy the kernel down. */
	movl	0(%eax),%esi			/* src */
	lea	EXT(_start),%edi		/* dst */
	movl	4(%eax),%ecx			/* size */
	rep
	movsb

	/*
	 * Zero out the BSS.
	 * Note this trashes %eax, which was pointing to the reboot_info.
	 */
	lea	EXT(edata),%edi
	lea	EXT(end),%ecx
	subl	%edi,%ecx
	xorl	%eax,%eax
	rep
	stosb

	/* Now load the arg into %ebx and jump in. */
	lea	EXT(_start),%eax		/* entry */
	jmp	*%eax
