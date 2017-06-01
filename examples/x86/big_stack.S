/*
 * Copyright (c) 1996-1998, 2000 University of Utah and the Flux Group.
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
 * This overrides the default stack (kern/x86/base_stack.S) if it is
 * larger than I deem necessary.
 *
 * Just link this with your kernel to override the stack.
 */

#include <oskit/config.h>
#include <oskit/x86/asm.h>
#include <oskit/x86/base_stack.h>
#include <oskit/x86/paging.h>

#define MY_STACK_SIZE 0x20000

#ifdef HAVE_WORKING_BSS
        .bss
#else
        .data
#endif

	P2ALIGN(PAGE_SHIFT)
        .globl  EXT(base_stack_start)
LEXT(base_stack_start)

        .space  MY_STACK_SIZE

        .globl  EXT(base_stack_end)
LEXT(base_stack_end)
	
	.data
	.globl	EXT(base_stack_size)
LEXT(base_stack_size)
	.long	MY_STACK_SIZE
