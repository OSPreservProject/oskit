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

#include <oskit/c/stdio.h>
#include <oskit/c/malloc.h>
#include "user_syscall.h"

int main()
{
    int pid, tid;
    int i;
    int *p;

    pid = syscall_getpid();
    tid = syscall_gettid();

    printf("proc 0x8%x, tid %2d: Malloc Test\n", pid, tid);
    for (i = 0 ; i < 10 ; i++) {

	/* Lock is necessary to protect malloc against other threads */
	syscall_lock();
	p = (int*)malloc(i * 128);
	syscall_unlock();
	if (p == 0) {
	    printf("malloc failed!\n");
	    return 0;
	}
	/* Access it */
	memset(p, i, i * 128);
    }
    printf("tid %d: Bye!\n", tid);

    return 0;
}
