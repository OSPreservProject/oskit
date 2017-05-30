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

#ifndef _OSKIT_UVM_INTERNAL_H_
#define _OSKIT_UVM_INTERNAL_H_

/* NetBSD headers */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <uvm/uvm.h>

/* OSKit headers */
#include <oskit/c/errno.h>
#include <oskit/threads/pthread.h>
#include <oskit/uvm.h>

#include "oskit_uvm_vmspace.h"

/* is pthread initialized? */
extern int		threads_initialized;

/* is uvm initialized? */
extern int		oskit_uvm_initialized;

struct uvm_object *	oskit_blkio_attach(oskit_iunknown_t *iunknown);
void			oskit_uvm_activate_locked(oskit_vmspace_t vm);
void			oskit_uvm_thread_init(void);
void			oskit_uvm_csw_hook(void);
extern void		oskit_uvm_do_pending_mprotect(void);
void			oskit_uvm_switchvm(pthread_t tid);
oskit_error_t		oskit_uvm_vmspace_alloc(oskit_vmspace_t *out_vm);
void			oskit_uvm_vmspace_free(oskit_vmspace_t vm);
void			oskit_uvm_vmspace_start(void);

/*
 *  Synchronization.  This structure is used for synchronization 
 *  instead of struct proc.
 */
struct lwp {
    struct lwp		*p_forw;	/* Doubly-linked run/sleep queue. */
    struct lwp		*p_back;
    const void		*p_wchan;
    const char		*p_wmesg;
    int			p_stat;
    int			p_sync;
    int			p_priority;
};

extern pthread_mutex_t	oskit_uvm_mutex;

extern void		oskit_uvm_lock(void);
extern void		oskit_uvm_unlock(void);
#define UVM_LOCK	oskit_uvm_lock()
#define UVM_UNLOCK	oskit_uvm_unlock()

extern struct vmspace	vmspace0;		/* kernel vmspace */
extern oskit_vmspace_t	oskit_uvm_curvm;	/* current vmspace pointer */

extern pthread_key_t	oskit_uvm_per_thread_key;

/* machine specific */
#include "oskit_uvm_machdep.h"

/*
 *  Debugging
 */
extern int		oskit_uvm_debug;
#define XPRINTF(mask, args...)	do {		\
    if (mask & oskit_uvm_debug) printf(args);	\
} while (0)
#define OSKIT_DEBUG_FAULT	0x01
#define OSKIT_DEBUG_SWAP	0x02
#define OSKIT_DEBUG_PAGER	0x04
#define OSKIT_DEBUG_SYNC	0x08
#define OSKIT_DEBUG_INIT	0x10

#endif /*_OSKIT_UVM_INTERNAL_H_*/
