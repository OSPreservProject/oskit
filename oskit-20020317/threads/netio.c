/*
 * Copyright (c) 1999, 2000 University of Utah and the Flux Group.
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
 * This implements a threaded adaptor on an oskit_netio.  It is basically
 * the threaded listener code (see listener.c).
 */

#include <oskit/com.h>
#include <oskit/io/netio.h>
#include <oskit/c/stdio.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/string.h>
#include <oskit/c/malloc.h>
#include "pthread_internal.h"
#include "pthread_mutex.h"

/*
 * Seems like a good number to me :-)
 */
#define MAXPAX		16

#undef NETIOSTATS

typedef struct pthread_netio_impl {
	oskit_netio_t	 iol;		/* COM interface */
        unsigned	 count;		/* reference count */
	pthread_t	 tid;		/* The thread */
	oskit_netio_t	 *netio;	/* Actual netio */
	pthread_mutex_t	 mutex;
	pthread_cond_t	 cond;

	/*
	 * We maintain a small queue of MAXPAX packets.
	 */
	int		qhead;
	int		qtail;
	int		qmax;
#ifdef NETIOSTATS
	int		qpushes;
	int		qdrops;
#endif
	oskit_bufio_t  *packets[MAXPAX+1];
#define NEXTI(i)	((i != nio->qmax) ? i+1 : 0)
} pthread_netio_impl_t;

static OSKIT_COMDECL
netio_query(oskit_netio_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
        pthread_netio_impl_t *nio = (pthread_netio_impl_t *)io;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_netio_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &nio->iol;
                ++nio->count;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
netio_addref(oskit_netio_t *io)
{
        pthread_netio_impl_t *nio = (pthread_netio_impl_t *)io;

        if (nio->count == 0)
		return OSKIT_E_INVALIDARG;

        return ++nio->count;
}

static OSKIT_COMDECL_U
netio_release(oskit_netio_t *io)
{
        pthread_netio_impl_t *nio = (pthread_netio_impl_t *)io;
        unsigned newcount;

        if (nio == NULL || nio->count == 0)
		return OSKIT_E_INVALIDARG;

        newcount = --nio->count;
        if (newcount == 0) {
		/*
		 * Service threads hold references to our listener object
		 * so they should be dead before we reach here and thus it
		 * is safe to release the real listener object and free
		 * our memory.
		 */
		oskit_netio_release(nio->netio);

		/*
		 * Free up any pending bufios.
		 */
		while (nio->qhead != nio->qtail) {
			oskit_bufio_release(nio->packets[nio->qhead]);
			nio->qhead = NEXTI(nio->qhead);
		}
#ifdef NETIOSTATS
		if (nio->qpushes)
			printf("netio: %d pushes, %d drops\n",
			       nio->qpushes, nio->qdrops);
#endif
		sfree(nio, sizeof(*nio));
        }

        return newcount;
}

static OSKIT_COMDECL
netio_push(oskit_netio_t *io, oskit_bufio_t *b, oskit_size_t size)
{
        pthread_netio_impl_t *nio = (pthread_netio_impl_t *)io;
	int		     next;

	/*
	 * Spin lock the mutex and queue the bufio object.
	 * Signal a thread to invoke the netio. This could happen in
	 * an interrupt context, hence the spinning mutex lock. The
	 * thread will not keep this lock very long, so its okay.
	 * It will also disable interrupts to prevent interrupt deadlock.
	 *
	 * If no queue space is available, return an error to the caller.
	 */
	fast_mutex_spinlock(&nio->mutex);
	next = NEXTI(nio->qtail);
	if (next == nio->qhead) {
#ifdef NETIOSTATS
		nio->qdrops++;
#endif
		fast_mutex_unlock(&nio->mutex);
		return ENOMEM;
	}
	oskit_bufio_addref(b);
	nio->packets[nio->qtail] = b;
	nio->qtail = next;
#ifdef NETIOSTATS
	nio->qpushes++;
#endif
	fast_mutex_unlock(&nio->mutex);
	pthread_cond_signal(&nio->cond);

	return 0;
}

static OSKIT_COMDECL
netio_alloc_bufio(oskit_netio_t *io, oskit_size_t size,
		  oskit_bufio_t **out_bufio)
{
	return OSKIT_E_NOTIMPL;
}

static struct oskit_netio_ops oskit_pthread_netio_ops = {
	netio_query,
	netio_addref,
	netio_release,
	netio_push,
	netio_alloc_bufio
};

static void
netio_canceled(void *arg)
{
	pthread_netio_impl_t	*nio = (pthread_netio_impl_t *) arg;

	/* free this thread's reference */
	netio_release(&nio->iol);
}

/*
 * Turn the calling thread into a dedicated netio callback thread.
 * This thread holds a reference to the object, thus all such threads must
 * be destroyed before the object is freed.
 *
 * XXX currently the thread never returns, the thread must be explicitly
 * destroyed by the caller.
 */
void
oskit_threaded_netio_run(oskit_netio_t *io)
{
	pthread_netio_impl_t	*nio = (pthread_netio_impl_t *) io;
	oskit_bufio_t		*bufio;
	oskit_off_t		size;

	nio->count++;
	pthread_cleanup_push(netio_canceled, nio);

	/*
	 * Loop handling push events. The safe version of condwait allows
	 * interrupts to be disabled on entry, but does not check for
	 * cancelation.
	 */
	while (1) {
		assert_interrupts_enabled();
		disable_interrupts();
		
		pthread_mutex_lock(&nio->mutex);
		pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock,
				     &nio->mutex);
		while (nio->qhead == nio->qtail) {
			pthread_cond_wait_safe(&nio->cond, &nio->mutex);
			pthread_testcancel();
		}
		pthread_cleanup_pop(0);
		bufio = nio->packets[nio->qhead];
		nio->qhead = NEXTI(nio->qhead);
		pthread_mutex_unlock(&nio->mutex);
		
		enable_interrupts();

		oskit_bufio_getsize(bufio, &size);
		oskit_netio_push(nio->netio, bufio, (oskit_size_t)size);
		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
}

oskit_netio_t *
oskit_threaded_netio_create(oskit_error_t (*func)(void *data, oskit_bufio_t *b,
						  oskit_size_t pkt_size),
			    void *data)
{
	pthread_netio_impl_t	*nio;
	
	if ((nio = (pthread_netio_impl_t *) smalloc(sizeof(*nio))) == NULL)
		return 0;

	nio->iol.ops = &oskit_pthread_netio_ops;
	nio->count = 1;
	nio->qhead = nio->qtail = 0;
	nio->qmax  = MAXPAX;
#ifdef NETIOSTATS
	nio->qpushes = nio->qdrops = 0;
#endif

	/*
	 * Ensure the mutex is valid before we register the actual push
	 * function in case it triggers an immediate callback.
	 */
	pthread_mutex_init(&nio->mutex, NULL);
	pthread_cond_init(&nio->cond, NULL);
	pthread_mutex_lock(&nio->mutex);

	/*
	 * Create the underlying netio.
	 */
	nio->netio = oskit_netio_create(func, data);
	pthread_mutex_unlock(&nio->mutex);

	return &nio->iol;
}
