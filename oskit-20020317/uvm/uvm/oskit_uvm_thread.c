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

#include "oskit_uvm_internal.h"

pthread_mutex_t		oskit_uvm_mutex;

/* to have struct oskit_uvm_per_thread for each thread */
pthread_key_t		oskit_uvm_per_thread_key;

static void		thread_create_hook(pthread_t tid);
static void		thread_destroy_hook(pthread_t tid);
static void		per_thread_destructor(void *arg);
static void		thread_csw_hook(void);

extern void
oskit_uvm_thread_init()
{
    int rc;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    /* should be recursive */
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&oskit_uvm_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    rc = pthread_key_create(&oskit_uvm_per_thread_key, per_thread_destructor);
    assert(rc == 0);

    /*
     * Setup the thread context switch hook and create thread hook.
     */
    oskit_pthread_aftercsw_hook_set(thread_csw_hook);
    oskit_pthread_create_hook_set(thread_create_hook);
    oskit_pthread_destroy_hook_set(thread_destroy_hook);

    /* Setup per-thread data for myself. */
    thread_create_hook(pthread_self());
}

/*
 * When a thread enters into UVM code, it locks current vmspace and
 * oskit_uvm_mutex.   The reason to lock current vmspace is because when
 * a thread sleeps in the UVM code, it uses the proc structure which is 
 * in a struct oskit_vmspace.
 */

extern void
oskit_uvm_lock()
{
    if (!threads_initialized) {
	return;
    }
    assert(curproc);

#ifdef notused
    /*
     * Since the stuff necessary for synchronization is now allocated by
     * per-thread basis, we don't lock the curvm.
     */

    if (curproc->p_vmspace != &vmspace0) {
	/* Lock the current vmspace that conatains proc structure which might
	 * be used for sycnhronization. */
	pthread_mutex_lock(&oskit_uvm_curvm->vm_lock);
    }
#endif

    pthread_mutex_lock(&oskit_uvm_mutex);

    /* Since NetBSD's lockmgr assumes each threads has unique pid,
     * we set curproc->p_pid every time.  Note that one struct
     * proc might be shared by multiple threads.  See also
     * kern_synch.c */
    curproc->p_pid = (int)pthread_self();
    assert(cpl == 0);
}

extern void
oskit_uvm_unlock()
{
    if (!threads_initialized) {
	return;
    }
    pthread_mutex_unlock(&oskit_uvm_mutex);
#ifdef notused
    if (curproc->p_vmspace != &vmspace0) {
	pthread_mutex_unlock(&oskit_uvm_curvm->vm_lock);
    }
#endif
}


/* ready-to-use queue */
static struct oskit_uvm_per_thread	*pt_head;
/* exited-but-not-destroyed queue */
static struct oskit_uvm_per_thread	*pt_exited;

/*
 *  This function is called every time a thread is created.
 *  Allocate per-thread structure for it.
 */
static void
thread_create_hook(pthread_t tid)
{
    struct oskit_uvm_per_thread *pt;

    UVM_LOCK;
    if (pt_head) {
	pt = pt_head;
	pt_head = pt_head->pt_next;
	UVM_UNLOCK;
    } else {
	UVM_UNLOCK;
	pt = malloc(sizeof(*pt), M_TEMP, M_WAITOK);
	assert(pt);
	bzero(pt, sizeof(*pt));
    }

    pt->pt_tid = tid;
    oskit_pthread_setspecific(tid, oskit_uvm_per_thread_key, (void*)pt);
}

/*
 * Since other destructor functions might exist, the thread has still
 * possibility to cause page faults.  Therefore, we just link it
 * to the queue headed by pt_exited and destroy it in oskit_uvm_destroy_hook().
 */
static void
per_thread_destructor(void *arg)
{
    struct oskit_uvm_per_thread *pt = (struct oskit_uvm_per_thread*)arg;

    assert(pt->pt_tid == pthread_self());
    UVM_LOCK;
    pt->pt_next = pt_exited;
    pt_exited = pt;
    UVM_UNLOCK;

    pthread_setspecific(oskit_uvm_per_thread_key, 0);
}


/*
 * This function is called every time a thread is destroyed.
 */
static void
thread_destroy_hook(pthread_t tid)
{
    struct oskit_uvm_per_thread **pt;
    struct oskit_uvm_per_thread *tmp;
    int found = 0;

    UVM_LOCK;
    for (pt = &pt_exited ; *pt ; pt = &((*pt)->pt_next)) {
	if ((*pt)->pt_tid == tid) {
	    tmp = (*pt)->pt_next;
	    (*pt)->pt_next = pt_head;
	    pt_head = *pt;
 	    *pt = tmp;
	    found = 1;
	    break;
	}
    }
    assert(found);
    UVM_UNLOCK;
}

static void (*uvm_csw_hook)();

/*
 * This function is called from pthread_sched_dispatch() and executed on
 * after every threads context switch.  
 */
static void
thread_csw_hook()
{
    /* Change the vmspace */
    oskit_uvm_activate_locked(oskit_uvm_vmspace_get());

    /*
     * Call the extra hook.  This is used by the simple process library to 
     * keep track of which process is running.
     */
    if (uvm_csw_hook) {
	(*uvm_csw_hook)();
    }
}

extern void
oskit_uvm_csw_hook_set(void (*hook)(void))
{
    uvm_csw_hook = hook;
}
