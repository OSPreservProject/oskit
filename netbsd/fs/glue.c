/*
 * Copyright (c) 1997-1999, 2002 University of Utah and the Flux Group.
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

#include "fs_glue.h"

#include <sys/domain.h>
#include <oskit/c/stdarg.h>
#include <machine/intr.h>
#include <oskit/dev/dev.h>

volatile int cpl = 0;
int securelevel = 1;
struct vnode *rootvp = 0;
const char *panicstr = 0;
int imask[7] = {1, 2, 4, 8, 16, 32, 64};
struct proclist allproc;
struct proc *curproc = 0;
volatile struct timeval time;
volatile struct timeval mono_time; 
struct domain *domains = 0;

int copyinstr(void *udaddr, void *kaddr, size_t len, size_t *done)
{
    return copystr(udaddr,kaddr,len,done);
}


int copyoutstr(void *kaddr, void *udaddr, size_t len, size_t *done)
{
    return copystr(udaddr,kaddr,len,done);
}


int copyin(void *udaddr, void *kaddr, size_t len)
{
    bcopy(udaddr,kaddr,len);
    return 0;
}


int copyout(void *kaddr, void *udaddr, size_t len)
{
    bcopy(kaddr,udaddr,len);
    return 0;        
}


void delay(int n)
{
    struct proc *p;
    int d;
    
    p = curproc;
    while (n > 0) {
	    if (n > 100)
		    d = 100;
	    else
		    d = n;
	    
	    osenv_timer_spin(d * 1000000); /* Nanoseconds as a long */
	    n -= d;
    }
    curproc = p;
    return;            
}


void printf(const char *fmt, ...)
{
    va_list ap;
    struct proc *p;

    va_start(ap, fmt);
    p = curproc;
    osenv_vlog(OSENV_LOG_INFO, fmt, ap);
    curproc = p;
    va_end(ap);
}


void uprintf(const char *fmt, ...)
{
    va_list ap;
    struct proc *p;

    va_start(ap, fmt);
    p = curproc;    
    osenv_vlog(OSENV_LOG_INFO, fmt, ap);
    curproc = p;
    va_end(ap);
}


void log(int level, const char *fmt, ...)
{
    va_list ap;
    struct proc *p;

    va_start(ap, fmt);
    p = curproc;
    osenv_vlog(OSENV_LOG_INFO, fmt, ap);
    curproc = p;
    va_end(ap);
}


void panic(const char *fmt, ...)
{
    va_list ap;
    struct proc *p;

    va_start(ap, fmt);
    p = curproc;    
    osenv_vpanic(fmt, ap);
    curproc = p;
    va_end(ap);
}


void *malloc(unsigned long size, int type, int flags)
{
    struct proc *p;
    int osenv_flags = OSENV_AUTO_SIZE;
    void *ptr;

    if (flags & M_NOWAIT)
	osenv_flags |= OSENV_NONBLOCKING;
    
    p = curproc;
    ptr = osenv_mem_alloc(size, osenv_flags, 0);
    curproc = p;
    if (!ptr && (flags & M_WAITOK))
	    panic("netbsd glue malloc: out of memory");
    return ptr; 
}

    
void free (void *addr, int type)
{
    struct proc *p;
    p = curproc;
    osenv_mem_free(addr, OSENV_AUTO_SIZE, 0);
    curproc = p;
}


void inittodr(time_t base)
{
    time.tv_sec = base;
    time.tv_usec = 0;
}

void microtime(struct timeval *tvp)
{
    /* Right thing to do? */
    if (gettimeofday(tvp, 0))
	    panic("netbsd glue microtime: gettimeofday failed");

    while (tvp->tv_usec > 1000000)
    {
	tvp->tv_sec++;
	tvp->tv_usec -= 1000000;
    }
}


void tablefull(const char *tab)
{
    printf("%s:  table is full\n", tab);
}


int copystr(void *f, void *t, size_t maxlen, size_t *lencopied)
{
    char *from = (char *) f;
    char *to = (char *) t;
    size_t len;


    len = 0;
    while (len < maxlen)
    {
	len++;
	if ((*to++ = *from++) == '\0')
	{
	    if (lencopied)
		    *lencopied = len;
	    return 0;
	}
    }

    if (lencopied)
	*lencopied = len;
    return ENAMETOOLONG;
}


int soo_stat(struct socket *so, struct stat *sb)
{
    bzero(sb,sizeof(sb));
    sb->st_mode = S_IFSOCK;
    return 0;
}




