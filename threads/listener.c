/*
 * Copyright (c) 1997-2000 University of Utah and the Flux Group.
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
 * This implements an threaded adaptor on an oskit_listener. Standard
 * listeners run the callback as a direct upcall from whatever environment
 * the listener was notified in. The threaded listener arranges to run the
 * callback in a thread context. 
 *
 * Creating a listener requires creating a thread that when run will invoke
 * callback(arg). However, since listeners can be fired more than once, the
 * thread will actually loop, suspending itself after each notification,
 * until the next notification or until the listener is released and the
 * thread is canceled.
 *
 * This implementation is rather doggy. If it becomes useful, we can get
 * serious about making it better.
 */

#include <oskit/com.h>
#include <oskit/com/listener.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/malloc.h>
#include "pthread_internal.h"
#include "pthread_mutex.h"

typedef struct pthread_listener_impl {
	oskit_listener_t	iol;		/* COM interface */
        unsigned                count;		/* reference count */
	oskit_listener_t       *listener;	/* Actual listener */
	oskit_iunknown_t       *notify;		/* Time to notify */
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
} pthread_listener_impl_t;

static OSKIT_COMDECL
listener_query(oskit_listener_t *io,
	       const oskit_iid_t *iid, void **out_ihandle)
{
        pthread_listener_impl_t *li = (pthread_listener_impl_t *)io;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_listener_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &li->iol;
                ++li->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
listener_addref(oskit_listener_t *io)
{
        pthread_listener_impl_t *li = (pthread_listener_impl_t *)io;

        if (li->count == 0)
		return OSKIT_E_INVALIDARG;

        return ++li->count;
}

static OSKIT_COMDECL_U
listener_release(oskit_listener_t *io)
{
        pthread_listener_impl_t *li = (pthread_listener_impl_t *)io;
        unsigned newcount;

        if (li == NULL || li->count == 0)
		return OSKIT_E_INVALIDARG;

        newcount = --li->count;
        if (newcount == 0) {
		/*
		 * Service threads hold references to our listener object
		 * so they should be dead before we reach here and thus it
		 * is safe to release the real listener object and free
		 * our memory.
		 */
		oskit_listener_release(li->listener);
		sfree(li, sizeof(*li));
        }

        return newcount;
}

/* ARGSUSED */
static OSKIT_COMDECL
listener_notify(oskit_listener_t *io, oskit_iunknown_t *obj)
{
        pthread_listener_impl_t *li = (pthread_listener_impl_t *)io;

	/*
	 * Spin lock the mutex and set the notify object. Signal the
	 * the thread to invoke the listener. This could happen in
	 * an interrupt context, hence the spinning mutex lock. The
	 * thread will not keep this lock very long, so its okay.
	 * It will also disable interrupts to prevent interrupt deadlock.
	 */
	fast_mutex_spinlock(&li->mutex);
	li->notify = obj;
	fast_mutex_unlock(&li->mutex);
	pthread_cond_signal(&li->cond);

	return 0;
}

static struct oskit_listener_ops oskit_pthread_listener_ops = {
	listener_query,
	listener_addref,
	listener_release,
	listener_notify
};

static void
listener_canceled(void *arg)
{
	pthread_listener_impl_t	*li = (pthread_listener_impl_t *) arg;

	/* free this thread's reference */
	listener_release(&li->iol);
}

/*
 * Turn the calling thread into a dedicated listener thread.
 * This thread holds a reference to the object, thus all such threads must
 * be destroyed before the object is freed.
 *
 * XXX currently the thread never returns, the thread must be explicitly
 * destroyed by the caller.
 */
void
oskit_run_threaded_listener(oskit_listener_t *io)
{
        pthread_listener_impl_t *li = (pthread_listener_impl_t *) io;
	oskit_iunknown_t	*obj;

	li->count++;
	pthread_cleanup_push(listener_canceled, li);

	/*
	 * Loop handling notifications. The safe version of condwait allows
	 * interrupts to be disabled on entry, but does not check for
	 * cancelation.
	 */
	while (1) {
		assert_interrupts_enabled();
		disable_interrupts();
		
		pthread_mutex_lock(&li->mutex);
		pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock,
				     &li->mutex);
		while (!li->notify) {
			pthread_cond_wait_safe(&li->cond, &li->mutex);
			pthread_testcancel();
		}
		pthread_cleanup_pop(0);
		obj = li->notify;
		li->notify = NULL;
		pthread_mutex_unlock(&li->mutex);

		enable_interrupts();

		oskit_listener_notify(li->listener, obj);
		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
}

oskit_listener_t *
oskit_create_threaded_listener(oskit_listener_callback_t handler, void *arg)
{
	pthread_listener_impl_t	*li;
	
	if ((li = (pthread_listener_impl_t *) smalloc(sizeof(*li))) == NULL)
		return 0;

	li->iol.ops = &oskit_pthread_listener_ops;
	li->count = 1;

	/*
	 * Ensure the mutex is valid before we register the actual handler
	 * in case it triggers an immediate callback.
	 */
	pthread_mutex_init(&li->mutex, NULL);
	pthread_cond_init(&li->cond, NULL);
	pthread_mutex_lock(&li->mutex);

	/*
	 * Create the underlying listener.
	 */
	li->listener = oskit_create_listener(handler, arg);
	pthread_mutex_unlock(&li->mutex);

	return &li->iol;
}
