/*
 * Copyright (c) 1996, 1998-2000 University of Utah and the Flux Group.
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
 * Sleep and wakeup support for the C/POSIX library. Using the osenv
 * interfaces directly is wrong. This mimics the osenv_sleep stuff,
 * but since the C/POSIX library is not expected to be holding the
 * process lock when it calls sleep, we cannot use the osenv interfaces
 * directly, except when running single threaded, in which case it does
 * not really matter since there is no process lock.
 *
 * I do not like this arrangement much. The interface needs to be made
 * distinct from the osenv_sleep interfaces. 
 */

/* Code taken from clientos/libcenv.c */

#include <knit/c/sleep.h>


#ifndef PTHREADS
/*
 * Single threaded kernel. Can use the osenv interfaces since they do
 * not muck with the process lock.
 */

#include <oskit/com/listener.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <assert.h>

static oskit_timer_t	*sleep_timer = 0;
static osenv_sleeprec_t	*active_sleeprec = 0;

/* 
 * ToDo: I'm not sure if there's a race condition between timeout_handler
 * and oskit_wakeup.  ADR
 */

static oskit_error_t
timeout_handler(oskit_iunknown_t *ioobj, void *arg)
{
        if (active_sleeprec) {
                osenv_wakeup(active_sleeprec, OSENV_SLEEP_WAKEUP);
                active_sleeprec = 0; /* prevent a wakeup */
        }
	return 0;
}

oskit_error_t
init(osenv_sleeprec_t *sleeprec)
{
        oskit_listener_t *l;
        oskit_error_t     rc;
        oskit_clock_t	*clock = 0;

        clock = oskit_clock_init();
        if (!clock) return OSKIT_ENXIO;
		
        rc = oskit_clock_createtimer(clock, &sleep_timer);
        oskit_clock_release(clock);
        if (rc) return rc;

        l = oskit_create_listener(timeout_handler, (void *) 0);
        if (!l) return OSKIT_ENXIO;

        oskit_timer_setlistener(sleep_timer, l);
        oskit_listener_release(l);
        return 0;
}

void
oskit_sleep_init(osenv_sleeprec_t *sleeprec)
{
	osenv_sleep_init(sleeprec);
        assert(active_sleeprec == 0);
        active_sleeprec = sleeprec;
}

oskit_error_t
oskit_sleep(osenv_sleeprec_t *sleeprec, oskit_timespec_t *timeout)
{
	int		rval;
	
        assert( active_sleeprec == sleeprec );
	if (timeout) {
		oskit_itimerspec_t	to;

		to.it_interval.tv_sec  = 0;
		to.it_interval.tv_nsec = 0;
		to.it_value.tv_sec     = timeout->tv_sec;
		to.it_value.tv_nsec    = timeout->tv_nsec;

		oskit_timer_settime(sleep_timer, 0, &to);
	}

	rval = osenv_sleep(sleeprec);

	if (timeout) {
		oskit_itimerspec_t	zero = { {0}, {0} };
			
		oskit_timer_settime(sleep_timer, 0, &zero);
	}

	return ((rval == OSENV_SLEEP_CANCELED) ? ETIMEDOUT : 0);
}

void
oskit_wakeup(osenv_sleeprec_t *sleeprec)
{
        if (active_sleeprec) {
                assert( active_sleeprec == sleeprec );
                osenv_wakeup(sleeprec, OSENV_SLEEP_WAKEUP);
                active_sleeprec = 0; /* "cancel" the timer */
        }
}
#else /* PTHREADS */

#include <oskit/threads/pthread.h>
#include <assert.h>

/*
 * Now it gets trickier. Not SMP safe, but this will work for the
 * uniprocessor pthreads code. We could make this SMP safe by using
 * a condition variable, but maybe later.
 */
void
oskit_sleep_init(osenv_sleeprec_t *sleeprec)
{
	assert(sleeprec);

	sleeprec->data[0] = OSENV_SLEEP_WAKEUP;
	sleeprec->data[1] = (void *) -1;	/* Pre-sleep. NOT SMP SAFE! */
}

oskit_error_t
oskit_sleep(osenv_sleeprec_t *sleeprec, oskit_timespec_t *timeout)
{
	volatile osenv_sleeprec_t *vsr = sleeprec;
	int			   enabled = osenv_intr_enabled();
	int			   millis = 0;
	
	if (enabled)
		osenv_intr_disable();

	if (timeout) {
		millis = (timeout->tv_sec * 1000) +
			 (timeout->tv_nsec / 1000000);
	}

	if (vsr->data[1]) {
		vsr->data[1] = pthread_self();
		if (oskit_pthread_sleep(millis) == ETIMEDOUT) {
			vsr->data[0] = (void *) OSENV_SLEEP_CANCELED;
			vsr->data[1] = 0;
		}
	}

	if (enabled)
		osenv_intr_enable();

	return ((vsr->data[0] == (void *)OSENV_SLEEP_CANCELED) ?
		ETIMEDOUT : 0);
}

void
oskit_wakeup(osenv_sleeprec_t *sleeprec)
{
	pthread_t	tid;
	int		enabled = osenv_intr_enabled();
	
	if (enabled)
		osenv_intr_disable();

	/* Return immediately on spurious wakeup */
	if ((tid = (pthread_t) sleeprec->data[1]) == (pthread_t) NULL)
		goto done;

	sleeprec->data[0] = (void *)OSENV_SLEEP_WAKEUP;
	sleeprec->data[1] = (void *)0;
	
	/* Look for pre-sleep wakeup. NOT SMP SAFE! */
	if (tid == (pthread_t) -1)
		goto done;

	/* Okay, really asleep */
	oskit_pthread_wakeup(tid);

   done:	
	if (enabled)
		osenv_intr_enable();
}
#endif /* PTHREADS */
