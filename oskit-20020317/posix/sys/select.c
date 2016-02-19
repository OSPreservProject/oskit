/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

#include <oskit/time.h>
#include <oskit/com/listener.h>
#include <oskit/io/asyncio.h>
#include <oskit/dev/dev.h>
#include <oskit/c/environment.h>

#if VERBOSITY > 2
#include <stdio.h>
#endif
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include "posix.h"
#include "fd.h"

#ifdef KNIT
#include <knit/c/sleep.h>

#undef oskit_libcenv_sleep_init(s, sr) 
#undef oskit_libcenv_sleep(s, sr, timeout) 
#undef oskit_libcenv_wakeup(s, sr) 
#define oskit_libcenv_sleep_init(s, sr)     oskit_sleep_init((sr))
#define oskit_libcenv_sleep(s, sr, timeout) oskit_sleep((sr),(timeout))
#define oskit_libcenv_wakeup(s, sr)         oskit_wakeup((sr))
#endif

static oskit_error_t	callback(oskit_iunknown_t *ioobj, void *arg);

#define VERBOSITY	0
#if VERBOSITY > 20
#define DMARK printf(__FILE__ ":%d: " __FUNCTION__ "\n", __LINE__)
#else
#define DMARK ((void)0)         /* no-op */
#endif

/*
 * Single and Multi threaded select combined.
 */
int 
select(int n, fd_set *in, fd_set *out, fd_set *exc, struct timeval *tout)
{
	int		fd, first, state, count;
	fd_set		inr, outr, excr;
	fd_set		locks;
	osenv_sleeprec_t sleeprec;
	oskit_timespec_t timeout = {0, 0};
#ifdef THREAD_SAFE
	int		gotlocks;

	pthread_testcancel();
#endif

	count    = 0;
	first    = 1;
	FD_ZERO(&locks);
	FD_ZERO(&inr);
	FD_ZERO(&outr);
	FD_ZERO(&excr);
#ifdef THREAD_SAFE
	gotlocks = 1;

	/*
	 * Try and lock all the descriptor locks. Don't want to deadlock,
	 * so if some are already locked, the select returns 0 so the
	 * caller can try again.
	 */
	for (fd = 0; fd < n; fd++) {
		if (in && FD_ISSET(fd, in)) {
			if (FD_BAD(fd))
				goto badf;
			if (!fd_trylock(fd) ||
			    !fd_access_trylock(fd, FD_RDWR)) {
				gotlocks = 0;
				break;
			}
			FD_SET(fd, &locks);
		}
		else if (out && FD_ISSET(fd, out)) {
			if (FD_BAD(fd))
				goto badf;
			if (!fd_trylock(fd) ||
			    !fd_access_trylock(fd, FD_RDWR)) {
				break;
			}
			FD_SET(fd, &locks);
		}
		else if (exc && FD_ISSET(fd, exc)) {
			if (FD_BAD(fd))
				goto badf;
			if (!fd_trylock(fd) ||
			    !fd_access_trylock(fd, FD_RDWR)) {
				gotlocks = 0;
				break;
			}
			FD_SET(fd, &locks);
		}
	}
	if (!gotlocks) {
		for (fd = 0; fd < n; fd++) {
			if (FD_ISSET(fd, &locks))
				fd_access_unlock(fd, FD_RDWR);
		}
		return 0;
	}
#else
	/*
	 * Just verify the descriptors. Use the same lock set though since
	 * its convenient.
	 */
	for (fd = 0; fd < n; fd++) {
		if (in && FD_ISSET(fd, in)) {
			if (FD_BAD(fd))
				goto badf;
			FD_SET(fd, &locks);
		}
		else if (out && FD_ISSET(fd, out)) {
			if (FD_BAD(fd))
				goto badf;
			FD_SET(fd, &locks);
		}
		else if (exc && FD_ISSET(fd, exc)) {
			if (FD_BAD(fd))
				goto badf;
			FD_SET(fd, &locks);
		}
	}
#endif
  retry:
	/*
	 * Check descriptors to see if anything is ready.
	 */
	for (fd = 0; fd < n; fd++) {
		fd_t	*fdp  = fd_array + fd;	/* this fd */
		int	ready = 0;

		/*
		 * See if we care about this descriptor.
		 */
		if (! FD_ISSET(fd, &locks))
			continue;

		fdp->selecting_on = 0;	/* Flag for creating listener */
		
		/* 
		 * if the fd doesn't support the interface, 
		 * we just ignore it
		 */
		if (!FD_HAS_INTERFACE(fd, asyncio)) 
			continue;

		/*
		 * Get the state and see if there is something ready
		 * that this select was interested in.
		 */
		state = oskit_asyncio_poll(fdp->asyncio);

		if (in && FD_ISSET(fd, in)) {
			if (state & OSKIT_ASYNCIO_READABLE) {
				FD_SET(fd, &inr);
				ready = 1;
			}
			fdp->selecting_on |= OSKIT_ASYNCIO_READABLE;
		}

		if (out && FD_ISSET(fd, out)) {
			if (state & OSKIT_ASYNCIO_WRITABLE) {
				FD_SET(fd, &outr);
				ready = 1;
			}
			fdp->selecting_on |= OSKIT_ASYNCIO_WRITABLE;
		}

		if (exc && FD_ISSET(fd, exc)) {
			if (state & OSKIT_ASYNCIO_EXCEPTION) {
				FD_SET(fd, &excr);
				ready = 1;
			}
			fdp->selecting_on |= OSKIT_ASYNCIO_EXCEPTION;
		}
		/* count each descriptor only once! */
		if (ready)
			count++;
	}

	/* Either got something, nothing ready after wakeup, or zero timeout? */
	if (count || !first || (tout && !tout->tv_sec && !tout->tv_usec))
		goto done;

	/*
	 * Nothing. Need to create the listeners. Initialize the sleeprec
	 * first to avoid premature and spurious wakeups.
	 */
	oskit_libcenv_sleep_init(libc_environment, &sleeprec);
	
	for (fd = 0; fd < n; fd++) {
		fd_t		  *fdp  = fd_array + fd;	/* this fd */
		
		/*
		 * See if we care about this descriptor.
		 */
		if (! FD_ISSET(fd, &locks))
			continue;

		/*
		 * Create the listener for this fd if we actually care
		 * about it. Use proper flags!
		 *
		 * Note that we reuse the listener if its there.
		 */
		if (fdp->selecting_on) {
			/*
			 * Stash the sleeprec in the fdp so the callback
			 * can do the wakeup.
			 */
			fdp->select_handle = &sleeprec;
			
			if (! fdp->listener)
				fdp->listener =
					oskit_create_listener(callback,
							      (void *) fd);
			
			state = oskit_asyncio_add_listener(fdp->asyncio,
							   fdp->listener,
							   fdp->selecting_on);

			/* Guard against lost wakeup. */
			if (state & fdp->selecting_on)
				count++;
		}
	}
	first = 0;

	/*
	 * if (un)lucky enough for something to come in while setting up
	 * the listeners, go back and see what actually happened.
	 */
	if (count) {
		count = 0;
		goto retry;
	}

	/*
	 * Okay, convert the timeout to a timespec and sleep. If the sleep
	 * returns with a timeout, then nothing came in and we can finish up.
	 */
	if (tout) {
		TIMEVAL_TO_TIMESPEC(tout, &timeout);
	}

	if (oskit_libcenv_sleep(libc_environment, &sleeprec, &timeout)
	    != ETIMEDOUT)
		goto retry;

   done:
	/* Cleanup descriptors to avoid spurious calls to wakeup. */
	if (!first) {
		for (fd = 0; fd < n; fd++) {
			fd_t	*fdp  = fd_array + fd;
			
			if (FD_ISSET(fd, &locks) && fdp->asyncio)
				oskit_asyncio_remove_listener(fdp->asyncio,
							      fdp->listener);
		}
	}
	
	/* copy out results */
	if (in) 
		FD_COPY(&inr, in);
	if (out) 
		FD_COPY(&outr, out);
	if (exc) 
		FD_COPY(&excr, exc);
#ifdef THREAD_SAFE
	/* unlock the descriptors */
	for (fd = 0; fd < n; fd++) {
		if (FD_ISSET(fd, &locks))
			fd_access_unlock(fd, FD_RDWR);
	}
#endif
	return count;
		
   badf:
	for (fd = 0; fd < n; fd++) {
		if (FD_ISSET(fd, &locks))
			fd_access_unlock(fd, FD_RDWR);
	}
	errno = EBADF;
	return -1;
}

static oskit_error_t
callback(oskit_iunknown_t *ioobj, void *arg)
{
	oskit_asyncio_t		*as   = (void *)ioobj;
	int			fd    = (int) arg;
	fd_t			*fdp  = fd_array + fd;
	oskit_error_t		mask;

	mask = oskit_asyncio_poll(as);
#if VERBOSITY > 2
	if (mask < 0) {
		printf("fd %d, poll on %p failed: %s\n", 
			fd, as, strerror(mask));
	} else {
		printf("fd %d is ready, mask = 0x%x ", fd, mask);
		if (mask & OSKIT_ASYNCIO_READABLE)
			printf(" READ ");
		if (mask & OSKIT_ASYNCIO_WRITABLE)
			printf(" WRITE ");
		if (mask & OSKIT_ASYNCIO_EXCEPTION)
			printf(" EXCEPTION ");
		printf("\n");
	}
#endif
	if (fdp->selecting_on & mask) {
		oskit_libcenv_wakeup(libc_environment,
				     (osenv_sleeprec_t *) fdp->select_handle);
	}

	return 0;
}
