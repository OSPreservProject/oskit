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
#include "user_syscall.h"

int main()
{
    int pid, tid;
    int i;

    pid = syscall_getpid();
    tid = syscall_gettid();

    for (i = 0 ; i < 10 ; i++) {
	printf("proc 0x8%x, tid %2d: Hello World, iter = %d\n", pid, tid, i);
	syscall_sleep(1);
    }
    printf("tid %d: Bye!\n", tid);

    return 0;
}
