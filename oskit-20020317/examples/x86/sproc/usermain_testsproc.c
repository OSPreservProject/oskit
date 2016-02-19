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

/* Sample user program */

#include <oskit/c/stdio.h>
#include <oskit/c/malloc.h>
#include "user_syscall.h"

void
undefined_insn()
{
    asm("ud2");
}

extern int errno;

void
stack_overflow()
{
    char buf[2048];
    printf("Stack %p\n", &buf);
    stack_overflow();
}

int
main()
{
    int pid;
    int rc;

    pid = syscall_getpid();
    printf("Start process (pid %d)\n", pid);

    rc = syscall_puts("THIS IS PRINTED BY PUTS\n");
    if (rc == -1) {
	printf("syscall_puts failed errno %x\n", errno);
    }

#if 0
    /* Test illegal memory access */
    printf("pid %d access 0x1000\n", pid);
    *(int*)0x1000 = 0;

    return 0;
#endif

#if 0
    /* Test undefined instruction */
    undefined_insn();
#endif

#if 0
    /* Test stack overflow */
    stack_overflow();
#endif

#if 1
    /* Test userspace malloc */
    {
	int *base1, *base2;
	syscall_lock();
	base1 = (int*)malloc(10);
	syscall_unlock();
	printf("pid %d: malloc returns %p\n", pid, base1);
	syscall_lock();
	base2 = (int*)malloc(4096);
	syscall_unlock();
	printf("pid %d: malloc returns %p\n", pid, base2);
#if 0
	/free(base2);
	free(base1);
#endif
    }
#endif
    
    return 0;
}
