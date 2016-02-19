/*
 * Copyright (c) 1994-1996 Sleepless Software
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Allow the second serial line to be used as a kill switch by setting up
 * an interrupt to call panic. See base_console.c.
 */

#include <oskit/x86/asm.h>

	.text

ENTRY(com_kill_intr)

	/* Push a dummy error code and trap number */
	pushl	$0
	pushl	$-1

	/* Save the rest of the state frame.  */
	pusha
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	/* Load the kernel's segment registers.  */
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es

	/* GCC likes the direction flag cleared.  */
	cld

	pushl	%esp
	call	EXT(base_trap_default_handler)

