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

/*
 * C run time for user mode process.
 * Initialize minimal C library, allocate malloc arena, etc.
 */

#include <oskit/c/stdio.h>
#include <oskit/com/mem.h>
#include <oskit/lmm.h>
#include <oskit/c/malloc.h>
#include <oskit/c/environment.h> /* libc_memory_object */
#include <oskit/c/unistd.h>	/* exit */
#include <oskit/uvm.h>

#include "proc.h"
#include "user_syscall.h"

lmm_t malloc_lmm = LMM_INITIALIZER;
struct lmm_region region;

extern void syscall_return(int code);
extern int main(int argc, char **argv);

extern int
_start(int arg1, int arg2, int arg3)
{
    oskit_mem_t *memi;
    static volatile int initstate = 0;

    /* Poor lock */
    syscall_lock();
    if (initstate == 0) {

	memi = oskit_mem_init();

	printf("Start user process\n");
	/* Print the arguments received from the kernel */
	printf("arg1 = %d, arg2 = %d, arg3 = %d\n", arg1, arg2, arg3);

	/* Initialize LMM for userspace malloc heap */
	lmm_add_region(&malloc_lmm, &region, (void*)HEAP_START_ADDR,
		       HEAP_SIZE, 0, 0);
	lmm_add_free(&malloc_lmm, (void*)HEAP_START_ADDR, HEAP_SIZE);
	
	libc_memory_object = memi;

	/* set exit hook */
	oskit_libc_exit = syscall_return;
	initstate = 1;
    }
    syscall_unlock();

    /* XXX: please someone add argc and argv! */
    exit(main(0, NULL));
}

/*
 * This is more efficient for console output, and allows similar treatment
 * in usermode where character based output is really bad.
 */
int
console_putbytes(const char *s, int len)
{
#if 1
    return syscall_puts(s);
#else
    while (len) {
	syscall_putchar(*s++);
	len--;
    }
    return 0;
#endif
}

/* For libc */
int
console_putchar(int c)
{
    return syscall_putchar(c);
}
