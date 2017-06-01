/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#include <oskit/machine/asm.h>
#include <oskit/machine/sproc.h>
#include "syscallno.h"

#define SYSCALL(name, no)			\
GEN_ENTRY(name)					\
	movl $no,%eax		; 		\
	int $0x60		; 		\
	jc cerror		; 		\
	ret

	.globl	EXT(errno)

GEN_ENTRY(cerror)
	movl	%eax,EXT(errno)
	movl	$-1,%eax
	movl	$-1,%edx
	ret	

SYSCALL(syscall_return, SYS_EXIT)
SYSCALL(syscall_putchar, SYS_PUTCHAR)
SYSCALL(syscall_getchar, SYS_GETCHAR)
SYSCALL(syscall_printf, SYS_PRINTF)
SYSCALL(syscall_sleep, SYS_SLEEP)
SYSCALL(syscall_getpid, SYS_GETPID)
SYSCALL(syscall_puts, SYS_PUTS)
SYSCALL(syscall_lock, SYS_LOCK)
SYSCALL(syscall_unlock, SYS_UNLOCK)
SYSCALL(syscall_gettid, SYS_GETTID)
