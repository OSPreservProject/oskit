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

#include <oskit/c/assert.h>
#include <oskit/c/errno.h>
#include <oskit/c/stdio.h>
#include <oskit/uvm.h>
#include <oskit/threads/pthread.h>
#include <oskit/com/wrapper.h>

/*
 * mmap a file, UVM version
 */
extern void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
    oskit_addr_t outaddr = (oskit_addr_t)addr;
    oskit_error_t error;
    oskit_absio_t *absio;

    if (fd_get_absio(fd, &absio) == -1) {
	return (void *)-1;
    }

#if 1
    {
	/* XXX: lock should be per file */
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	oskit_absio_t *intrsafe_absio;
	
	oskit_wrap_absio(absio, (void (*)(void*))pthread_mutex_lock,
			 (void (*)(void*))pthread_mutex_unlock,
			 &mutex, &intrsafe_absio);
	error = oskit_uvm_mmap(oskit_uvm_vmspace_get(), &outaddr, len, prot,
			       flags, (oskit_iunknown_t*)intrsafe_absio, off);
	oskit_absio_release(intrsafe_absio);
    }
#else
    error = oskit_uvm_mmap(oskit_uvm_vmspace_get(), &outaddr, len, prot, flags, 
			   (oskit_iunknown_t*)absio, off);
#endif
    oskit_absio_release(absio);

    if (error) {
	errno = error;
	return MAP_FAILED;
    }

    return (void*)outaddr;
}

extern int 
munmap(void *addr, size_t len)
{
    return oskit_uvm_munmap(oskit_uvm_vmspace_get(), (oskit_addr_t)addr, len);
}

#define NBUF	4
static struct {
    const void	*addr;
    size_t	len;
    int		prot;
} mprotect_buf[NBUF];

static int mprotect_buf_index = 0;

extern int 
mprotect(const void *addr, size_t len, int prot)
{
    extern int oskit_uvm_initialized;

    /*
     * The OSKit threads library calls mprotect during its initialization. 
     * Since UVM depends on the thread library, we cannot process the request
     * immediately.  Remember the request and do it later
     * (in oskit_uvm_do_pending_mprotect()).
     */
    if (!oskit_uvm_initialized) {
	mprotect_buf[mprotect_buf_index].addr = addr;
	mprotect_buf[mprotect_buf_index].len  = len;
	mprotect_buf[mprotect_buf_index].prot = prot;
	mprotect_buf_index++;
	if(mprotect_buf_index == NBUF) {
	    panic(__FUNCTION__": too many call to mprotect\n");
	}
	return 0;
    }

    return oskit_uvm_mprotect(oskit_uvm_vmspace_get(),
			      (oskit_addr_t)addr, len, prot);
}

extern void
oskit_uvm_do_pending_mprotect()
{
    int rc;
    int i;

    for (i = 0 ; i < mprotect_buf_index; i++) {
	rc = mprotect(mprotect_buf[i].addr, mprotect_buf[i].len, 
		      mprotect_buf[i].prot);
	assert(rc == 0);
    }
}


extern int 
madvise(const void *addr, size_t len, int behav)
{
    return oskit_uvm_mprotect(oskit_uvm_vmspace_get(),
			      (oskit_addr_t)addr, len, behav);
}
