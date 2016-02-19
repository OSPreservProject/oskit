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

#include <oskit/machine/types.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdio.h>
#include <oskit/threads/pthread.h>
#include <oskit/c/errno.h>
#include <oskit/sproc.h>

extern void sys_exit(struct oskit_sproc_thread *sth,
		     void *arg, int *rval) OSKIT_NORETURN;

void
sys_exit(struct oskit_sproc_thread *sth, void *arg, int *rval)
{
    int exit_code = *(int*)arg;
    OSKIT_SPROC_RETURN(sth, exit_code);
}

int
sys_putchar(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    int c = *(int*)arg;
    
    *rval = putchar(c);
    return 0;
}

int
sys_getchar(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    *rval = getchar();
    return 0;
}

int
sys_printf(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    const char *fmt = (const char*)(((char**)arg)[0]);
    oskit_va_list va;
    printf("%s: fmt = %p\n", __FUNCTION__, fmt);
    va = (oskit_va_list)(arg + 4);
    printf("%s: va = %p\n", __FUNCTION__, va);
    *rval = vprintf(fmt, va);
    return 0;
}

int
sys_sleep(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    int t = *(int*)arg;
    *rval = sleep(t);
    return 0;
}

int
sys_getpid(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    *rval = (int)proc;
    return 0;
}

int
sys_puts(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    struct puts_arg {
	const char *str;
    } *ap;
    char buf[256];
    oskit_size_t sz;
    oskit_error_t error;
    ap = (struct puts_arg*)arg;

    error = copyinstr(ap->str, buf, 256, &sz);
    if (error) {
	return error;
    }
    puts(buf);

    return 0;
}

/*
 * Poor locking.
 */
pthread_mutex_t syslock;
int
sys_lock(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    pthread_mutex_lock(&syslock);
    return 0;
}

int
sys_unlock(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    pthread_mutex_unlock(&syslock);
    return 0;
}

int
sys_gettid(struct oskit_sproc_thread *proc, void *arg, int *rval)
{
    *rval = (int)pthread_self();
    return 0;
}

struct oskit_sproc_sysent my_syscall_tab[] = {
    { (oskit_sproc_syscall_func)sys_exit, 1 },
    { sys_putchar, 1 },
    { sys_getchar, 1 },
    { sys_printf, 8 },		/* XXX */
    { sys_sleep, 1 },
    { sys_getpid, 1 },
    { sys_puts, 1 },
    { sys_lock, 0 },
    { sys_unlock, 0 },
    { sys_gettid, 0 },
};


void
syscall_init()
{
    pthread_mutex_init(&syslock, 0);
}
