/*
 * Copyright (c) 1997-2001 University of Utah and the Flux Group.
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
 *  a driver for the system clock.
 */

#include <oskit/dev/dev.h>
#include <oskit/debug.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/com/listener.h>
#include <oskit/c/string.h>
#include <oskit/c/stdlib.h>
#include "pit_param.h"

#define VERBOSITY	0

/*
 * This file needs these osenv interfaces. The clock code is written on
 * top of the osenv interfaces, and is intended to be standalone from the
 * device library. It should be moved elsewhere, since it needs to use
 * the external interfaces, not the internal ones.
 */
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_timer.h>

#ifdef KNIT

#define BEGIN_CRITICAL_SECTION(intr,flags)   \
	flags = osenv_intr_enabled(); \
	osenv_intr_disable();

#define END_CRITICAL_SECTION(intr,flags) \
	if (flags) osenv_intr_enable();

#undef oskit_osenv_mem_alloc
#define oskit_osenv_mem_alloc(a,b,c,d) osenv_mem_alloc((b),(c),(d))
#undef oskit_osenv_mem_free
#define oskit_osenv_mem_free(a,b,c,d) osenv_mem_free((b),(c),(d))
#undef oskit_osenv_timer_spin
#define oskit_osenv_timer_spin(a,b) osenv_timer_spin((b))
#undef oskit_osenv_timer_register
#define oskit_osenv_timer_register(a,b,c) osenv_timer_register((b),(c))

#else /* !KNIT */
/*
 * Locally cached pointers to the COM objects needed.
 */
static oskit_osenv_log_t	*clock_osenv_log;

/*
 * For simplicity, lets map these older osenv_log calls to the new object
 * invocations to avoid a ton of textual changes.
 */
#undef  osenv_assert
#define osenv_assert(x) \
	oskit_osenv_assert(clock_osenv_log, x)

#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(clock_osenv_log, (pri), ##fmt)


#define BEGIN_CRITICAL_SECTION(intr,flags)   \
	flags = oskit_osenv_intr_save_disable(intr);

#define END_CRITICAL_SECTION(intr,flags) \
	if (flags) oskit_osenv_intr_enable(intr);

#endif /* !KNIT */

/*
 * What does a clock do, really?
 *
 * Two things:
 *	a) it entertains a counter which can be queried for the
 * 	          current time.
 *
 *	b) it entertains a bunch of oskit_timers which are to triggered
 *	   at specified intervals. Here we have basically two kinds:
 *
 *	   i)	periodic timers running at every clock tick
 *	   ii)	other timers. There are two subtypes:
 *		1. oneshot timers that are not being reloaded after they
 *	   	   expire
 *		2. periodic timers that are being reloaded.
 */

/*
 * this describes the data structure used to represent a timer
 */
typedef struct timer_impl {
	oskit_timer_t		iot;		/* COM timer interface */
	unsigned 		count;		/* reference count */
	oskit_s32_t 		left;		/* ticks to next alarm */
	oskit_s32_t 		reload;		/* the reload value in ticks */
	struct oskit_listener 	*listener;	/* my listener */
	oskit_itimerspec_t	itimer;		/* our itimerspec  */
	struct clock_impl	*clock;		/* our clock */
	struct timer_impl 	**clocklist;	/* head of list we're on */
	struct timer_impl 	*next;
} timer_impl_t;

/*
 * this describes the data structure used to represent our clock
 */
typedef struct clock_impl {
	oskit_clock_t		ioc;		/* COM clock interface */
	unsigned 		count;		/* reference count */
	oskit_timespec_t 	time;		/* "current" time */

#ifndef KNIT
        /*
	 * Locally cached pointers to the COM objects needed.
	 */
	oskit_osenv_mem_t	*mem;
	oskit_osenv_timer_t	*timer;
	oskit_osenv_intr_t	*intr;
#endif

	/*
	 * Please see the distinction I made above.
	 * Godmar feels it is important to optimize the case where a timer
	 * expires at every clock tick
	 * Thus, we keep two queues. Both of them are used by
	 * the interrupt code, so changes to them must be protected.
	 */

	/* this is our "callout" queue for one-shot timers - a differential
	 * list of events essentially. This list is ordered by
	 * timer_impl::left. Negative values indicate overdue events.
	 */
	struct timer_impl	*callout;

	/*
	 * these timers are triggered at every clock interrupt
	 */
	struct timer_impl	*everytime;
} clock_impl_t;

/*
 * Currently, we only support one instance
 * Note the COM code is written for dynamic allocation/deallocation,
 * which is why we keep a pointer here
 */
clock_impl_t	*oskit_system_clock;

/* forward declarations */
static timer_impl_t * create_timer(clock_impl_t *clock);
static void add_synchronous_timer(oskit_osenv_intr_t *intr, timer_impl_t **head, timer_impl_t *t);
static void add_oneshot_timer(oskit_osenv_intr_t *intr, timer_impl_t **head,
	timer_impl_t *n, oskit_s32_t ticks);


/*
 * these macros should probably go elsewhere, maybe in oskit/time.h
 */
#define TIMESPEC2NANOSEC(t)	((t)->tv_sec * 1000000000 + (t)->tv_nsec)
#define NULL_TIMESPEC(t)	((t)->tv_sec = (t)->tv_nsec = 0)
#define ADDNANO2TIMESPEC(n, t) { \
	assert(n <= 1000000000); \
	(t)->tv_nsec += (n); \
	if ((t)->tv_nsec >= 1000000000) { \
		(t)->tv_sec++; \
		(t)->tv_nsec -= 1000000000; \
	} \
}
#define SUBTIMESPEC(s1, s2, d)					\
	((d)->tv_nsec = ((s1)->tv_nsec >= (s2)->tv_nsec) ?	\
	    (((d)->tv_sec = (s1)->tv_sec - (s2)->tv_sec),	\
	     (s1)->tv_nsec - (s2)->tv_nsec)			\
	    :							\
	    (((d)->tv_sec = (s1)->tv_sec - (s2)->tv_sec - 1),	\
	     (1000000000 + (s1)->tv_nsec - (s2)->tv_nsec)))


/*
 * timer interrupt handler.
 */
static void fdev_handle_clock(clock_impl_t *clock);

static void
timer_intr()
{
	if (oskit_system_clock)
		fdev_handle_clock(oskit_system_clock);
}

/*
 * take any action that needs to be taken at a (hardware) clock tick
 */
static void
fdev_handle_clock(clock_impl_t *clock)
{
	timer_impl_t  *th;
	int	mustcallout = 0;

	/* update our current time */
	ADDNANO2TIMESPEC(NANOPERTICK, &clock->time);

	/* handle simple, synchronously running timers */
	for (th = clock->everytime; th; th = th->next)
		oskit_listener_notify(th->listener, (oskit_iunknown_t *)th);

	/* handle callout queue */
	for (th = clock->callout; th; th = th->next) {
		if (--th->left > 0)
			break;
		mustcallout = 1;
		if (th->left == 0)
			break;
	}

	if (!mustcallout)
		return;

	/* Call and remove all handlers that are due */
	while (((th = clock->callout) != NULL) && th->left <= 0) {

		/* remove it from list */
		clock->callout = th->next;
		th->next = 0;
		th->clocklist = 0;

		/* notify the object */
		oskit_listener_notify(th->listener, (oskit_iunknown_t *)th);

		/*
		 * reload timer if necessary
		 */
		if (th->reload) {
#ifdef KNIT
		        if (th->reload == 1)
				add_synchronous_timer(0,
						      &clock->everytime, th);
			else
				add_oneshot_timer(0,
						  &clock->callout, th,
						  th->reload);
#else /* !KNIT */
			if (th->reload == 1)
				add_synchronous_timer(clock->intr,
						      &clock->everytime, th);
			else
				add_oneshot_timer(clock->intr,
						  &clock->callout, th,
						  th->reload);
#endif /* !KNIT */
		}
	}
}

#if VERBOSITY > 0
/*
 * for debugging - print out the contents of a queue
 */
static void
print_queue(timer_impl_t **head)
{
	timer_impl_t	*t = *head;
	while (t) {
		osenv_log(OSENV_LOG_DEBUG, "(%d, %d, %p) ", t->left, t->reload,
			t->clocklist);
		t = t->next;
	}
	osenv_log(OSENV_LOG_DEBUG, "\n");
}
#endif

/*
 * add a timer to an unordered list of timers
 */
static void
add_synchronous_timer(oskit_osenv_intr_t *intr, timer_impl_t **head, timer_impl_t *t)
{
	unsigned int flags;
	BEGIN_CRITICAL_SECTION(intr, flags);
	t->clocklist = head;
	t->next = *head;
	*head = t;
	t->left = t->reload;
	END_CRITICAL_SECTION(intr, flags);
}

/*
 * remove a timer from an unordered list of timers
 */
static void
remove_synchronous_timer(oskit_osenv_intr_t *intr, timer_impl_t **head, timer_impl_t *t)
{
	unsigned int flags;
	BEGIN_CRITICAL_SECTION(intr, flags);
	while (*head) {
		if (*head == t) {
			*head = t->next;
			t->clocklist = 0;
			break;
		}
		head = &(*head)->next;
	}
	END_CRITICAL_SECTION(intr, flags);
}

/*
 * add a timer to be fired in "ticks"
 * this is programmed analogous to BSD's timeout function
 */
static void
add_oneshot_timer(oskit_osenv_intr_t *intr, timer_impl_t **head, timer_impl_t *n, oskit_s32_t ticks)
{
	timer_impl_t	*t, **p;
	unsigned int flags;

	BEGIN_CRITICAL_SECTION(intr, flags);
	n->clocklist = head;

	/* walk through list, finding spot, adjusting ticks parameter */
	for (p = head; (t = *p) && ticks > t->left; p = &t->next)
		if (t->left > 0)
			ticks -= t->left;

	n->left = ticks;
	/* adjust next entry */
	if (t)
		t->left -= ticks;

	n->next = t;
	*p = n;
	END_CRITICAL_SECTION(intr, flags);
}

/*
 * remove a one-shot timer, correcting the next queue entry
 */
static void
remove_oneshot_timer(oskit_osenv_intr_t *intr, timer_impl_t **head, timer_impl_t *f)
{
	timer_impl_t    *t, **p;
	unsigned int flags;

	BEGIN_CRITICAL_SECTION(intr, flags);
	for (p = head; (t = *p) != NULL; p = &t->next) {
		if (t == f) {
			if (t->next && t->left > 0)
				t->next->left += t->left;
			*p = t->next;
			f->clocklist = 0;
			f->next = 0;
			break;
		}
	}
	END_CRITICAL_SECTION(intr, flags);
}

/*
 * figure out how much many ticks are left on a particular timer
 */
static oskit_s32_t
ticks_left_on_oneshot_timer(timer_impl_t *t, timer_impl_t *f)
{
	unsigned int flags;
	oskit_s32_t left = 0;

	BEGIN_CRITICAL_SECTION(f->clock->intr, flags);

	while (t) {
		if (t->left > 0)
			left += t->left;
		if (t == f)
			break;
		t = t->next;
	}

	END_CRITICAL_SECTION(f->clock->intr,flags);
	/*
	 * if we didn't find t, lets return zero instead of returning
	 * the accumulated sum of all time diffs
	 */
	return t ? left : 0;
}

/*
 * sometimes we have the need to remove a timer from its clock
 */
static void
remove_from_clocklist(timer_impl_t *ti)
{
	clock_impl_t *clock = ti->clock;
#ifdef KNIT
	if (ti->clocklist == &ti->clock->everytime)
		remove_synchronous_timer(0, &clock->everytime, ti);
	else
	if (ti->clocklist == &ti->clock->callout)
		remove_oneshot_timer(0, &clock->callout, ti);
#else
	if (ti->clocklist == &ti->clock->everytime)
		remove_synchronous_timer(clock->intr, &clock->everytime, ti);
	else
	if (ti->clocklist == &ti->clock->callout)
		remove_oneshot_timer(clock->intr, &clock->callout, ti);
#endif
}


/*************************************************************************
 *
 *	oskit_timer COM interface implementation
 *
 ************************************************************************/

/* Basic COM functions */
/*
 * Query a timer object for its interfaces.
 */
static OSKIT_COMDECL
timer_query(oskit_timer_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	timer_impl_t *po = (timer_impl_t *)io;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_timer_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &po->iot;
		++po->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


/*
 * Clone a reference to a timer
 */
static OSKIT_COMDECL_U
timer_addref(oskit_timer_t *io)
{
	timer_impl_t *po = (timer_impl_t *)io;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	return ++po->count;
}

/*
 * release a timer
 */
static OSKIT_COMDECL_U
timer_release(oskit_timer_t *io)
{
	timer_impl_t *ti = (timer_impl_t *)io;
	unsigned newcount;

	osenv_assert(ti != NULL);
	osenv_assert(ti->count != 0);

	newcount = --ti->count;
	if (newcount == 0) {
		/* if we're still on a list, remove ourselves! */
		if (ti->clocklist)
			remove_from_clocklist(ti);
		if (ti->listener)
			oskit_listener_release(ti->listener);
		oskit_osenv_mem_free(ti->clock->mem, ti, 0, sizeof(*ti));
	}

	return newcount;
}

/* Set a listener to be notifed when timer expires */
static OSKIT_COMDECL_V
timer_setlistener(oskit_timer_t *timer, struct oskit_listener *listener)
{
	timer_impl_t	*ti = (timer_impl_t *)timer;

	osenv_assert(ti != NULL);
	osenv_assert(ti->count != 0);

	oskit_listener_addref(listener);
	if (ti->listener)
		oskit_listener_release(ti->listener);
	ti->listener = listener;
}

/* Set the timer's expiration and period. */
static OSKIT_COMDECL
timer_settime(oskit_timer_t *timer, int flags,
	const struct oskit_itimerspec *value)
{
	timer_impl_t	*ti = (timer_impl_t *)timer;
	clock_impl_t	*clock;
	oskit_s32_t	left, reload;

	osenv_assert(ti != NULL);
	osenv_assert(ti->count != 0);

	clock = ti->clock;

	/*
	 * Step 1: compute the new parameters
	 */
	ti->itimer = *value;
	if (flags & OSKIT_TIMER_ABSTIME) {
		oskit_timespec_t	exp;

		/* get time from clock and subtract from value->it_value */
		SUBTIMESPEC(&value->it_value, &clock->time, &exp);
		ti->itimer.it_value = exp;
	}
	left = TIMESPEC2TICKS(&ti->itimer.it_value);
	reload = TIMESPEC2TICKS(&ti->itimer.it_interval);

#if VERBOSITY > 0
	osenv_log(OSENV_LOG_DEBUG,
			__FUNCTION__" it_value = { %d, %d }, left = %d\n",
			ti->itimer.it_value.tv_sec,
			ti->itimer.it_value.tv_nsec,
			left);
	osenv_log(OSENV_LOG_DEBUG,
			__FUNCTION__" it_interval = { %d, %d }, reload = %d\n",
			ti->itimer.it_interval.tv_sec,
			ti->itimer.it_interval.tv_nsec,
			reload);
#endif
	/*
	 * Step 2: remove ourselves from any lists - should we be on one
	 */
	if (ti->clocklist)
		remove_from_clocklist(ti);

	/*
	 * If left and reload are zero, do nothing. Might have been a
	 * timer removal.
	 */
	if (left == 0 && reload == 0)
		return 0;

	/*
	 * set reload in any event so that one-shot timers with a period
	 * can be inserted in the periodic queue after they first expire
	 */
	ti->reload = reload;

	/*
	 * Step 3: Figure out just what kind of a timer we are
	 */
#ifdef KNIT
	if (left == 1 && left == reload) {
		/* this timer expires at every clock tick */
		add_synchronous_timer(0, &clock->everytime, ti);
	} else {
		/* this timer starts out as a one-shot timer */
		add_oneshot_timer(0, &clock->callout, ti, left);
	}
#else
	if (left == 1 && left == reload) {
		/* this timer expires at every clock tick */
		add_synchronous_timer(clock->intr, &clock->everytime, ti);
	} else {
		/* this timer starts out as a one-shot timer */
		add_oneshot_timer(clock->intr, &clock->callout, ti, left);
	}
#endif

	return 0;
}

/* Query the timer's current expiration and period. */
static OSKIT_COMDECL_V
timer_gettime(oskit_timer_t *timer, struct oskit_itimerspec *out_value)
{
	timer_impl_t	*ti = (timer_impl_t *)timer;
	oskit_s32_t	left = 0;

	osenv_assert(ti != NULL);
	osenv_assert(ti->count != 0);

	/* set current period */
	out_value->it_interval = ti->itimer.it_interval;

	/*
	 * try to figure out how much time is left to next expiration
	 */
	if (ti->clocklist == &ti->clock->everytime)
		left = 1;		/* one tick is left */
	else
	if (ti->clocklist == &ti->clock->callout)
		left = ticks_left_on_oneshot_timer(ti->clock->callout, ti);

	/* set expiration time left */
	TICKS2TIMESPEC(left, &out_value->it_value);
}

/* Return number of overruns */
static OSKIT_COMDECL
timer_getoverrun(oskit_timer_t *timer)
{
	/* we don't run over, do we? */
	return 0;
}


/*************************************************************************
 *
 *	oskit_clock COM interface implementation
 *
 ************************************************************************/

/* Basic COM functions */
/*
 * Query a clock object for its interfaces.
 */
static OSKIT_COMDECL
clock_query(oskit_clock_t *io, const oskit_iid_t *iid, void **out_ihandle)
{
	clock_impl_t *po = (clock_impl_t *)io;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_clock_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &po->ioc;
		++po->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}


/*
 * Clone a reference to a clock device
 */
static OSKIT_COMDECL_U
clock_addref(oskit_clock_t *io)
{
	clock_impl_t *po = (clock_impl_t *)io;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	return ++po->count;
}

/*
 * Close ("release") a clock device.
 */
static OSKIT_COMDECL_U
clock_release(oskit_clock_t *io)
{
	clock_impl_t *po = (clock_impl_t *)io;
	unsigned newcount;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	newcount = --po->count;
	if (newcount == 0) {
#ifdef KNIT
		osenv_timer_unregister(timer_intr, TIMER_FREQ);
		osenv_mem_free(po, 0, sizeof(*po));
#else
		oskit_osenv_mem_t *mem = po->mem;
		oskit_osenv_timer_unregister(po->timer,
					     timer_intr, TIMER_FREQ);
		oskit_osenv_timer_release(po->timer);
		oskit_osenv_intr_release(po->intr);
		oskit_osenv_mem_free(mem, po, 0, sizeof(*po));
		oskit_osenv_mem_release(mem);
#endif
	}

	return newcount;
}

/* Base fdev device interface operations */
static OSKIT_COMDECL
clock_getinfo(oskit_clock_t *fdev, oskit_devinfo_t *out_info)
{
	/*
	 * this gives out info about the actual hardware device in use
	 */
	out_info->name = "clock";
	out_info->description = "Simple pit-based clock device";
	out_info->vendor = "UofU/CSL";
	out_info->author = "Godmar Back";
	out_info->version = "1.0";
	return 0;
}

static OSKIT_COMDECL
clock_getdriver(oskit_clock_t *fdev, oskit_driver_t **out_driver)
{
	/*
	 * this a reference about the "more general" driver of
	 * which we are an instance
	 *
	 * for now, just return an additional reference to us
	 */
	return clock_query(fdev, &oskit_clock_iid,(void **)out_driver);
}

/* Clock device interface operations */

/* Return basic information about this clock, defined above */
static OSKIT_COMDECL_V
clock_getclockinfo(oskit_clock_t *dev, struct oskit_clock_info *out_info)
{
	oskit_timespec_t	res = { 0, NANOPERTICK };
	oskit_timespec_t	pitns = { 0, PIT_NS };

	out_info->flags = OSKIT_CLOCK_SETTABLE;
	out_info->timer_precision = res;
	out_info->query_precision = res;
	out_info->sleep_precision = res;
	out_info->spin_precision = pitns;
}

/* Query the clock's current time. */
static OSKIT_COMDECL_V
clock_gettime(oskit_clock_t *dev, struct oskit_timespec *out_time)
{
	clock_impl_t *po = (clock_impl_t *)dev;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	*out_time = po->time;
}

/*
 * Wait for a period of time, possibly blocking during the wait.
 */
static OSKIT_COMDECL_V
clock_sleep(oskit_clock_t *dev, const struct oskit_timespec *time)
{
	/*
	 * this would be implemented by posting an "internal"
	 * timeout and then sleeping
	 */
}

/*
 * Wait for a period of time without blocking
 * or enabling interrupts if interrupts are disabled on call.
 * This should only be used for very short waits.
 */
static OSKIT_COMDECL_V
clock_spin(oskit_clock_t *dev, const struct oskit_timespec *time)
{
	clock_impl_t *po = (clock_impl_t *)dev;
	oskit_osenv_timer_spin(po->timer, TIMESPEC2NANOSEC(time));
}

/*
 * Create a new timer based on this clock.
 */
static OSKIT_COMDECL
clock_createtimer(oskit_clock_t *dev, struct oskit_timer **out_timer)
{
	clock_impl_t *po = (clock_impl_t *)dev;
	timer_impl_t	*t;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	/* that's all we can do here since we don't know nothing about it yet */
	t = create_timer(po);
	*out_timer = &t->iot;
	return 0;
}

/*
 * Set the clock's current time immediately.
 * Clocks that aren't settable will return OSKIT_E_NOTIMPL.
 */
static OSKIT_COMDECL
clock_settime(oskit_clock_t *dev, const struct oskit_timespec *new_time)
{
	/* yeah, set me - like you know what you're doing... */
	clock_impl_t *po = (clock_impl_t *)dev;

	osenv_assert(po != NULL);
	osenv_assert(po->count != 0);

	po->time = *new_time;
	return 0;
}

/*
 * Set the clock's current time gradually,
 * so that the clock's value does not jump or go backward.
 * Clocks that aren't adjustable will return OSKIT_E_NOTIMPL.
 */
static OSKIT_COMDECL
clock_adjtime(oskit_clock_t *dev, const struct oskit_timespec *new_time)
{
	/* there's no way I'm going to implement that... */
	return OSKIT_E_NOTIMPL;
}

/*
 * Set the clock's period, or timer_precision.
 * Clocks that aren't programmable will return OSKIT_E_NOTIMPL.
 * Even if the clock is programmable,
 * it may not support the exact period requested;
 * the caller can get_clock_info() and check timer_precision
 * to determine the actual period for which the clock was set.
 * If the requested period is completely outside of the range
 * supported by the clock, this method returns OSKIT_E_DEV_BAD_PERIOD.
 */
static OSKIT_COMDECL
clock_setperiod(oskit_clock_t *dev, const struct oskit_timespec *period)
{
	return OSKIT_E_NOTIMPL;
}

/*
 * Open this clock device for exclusive access.
 * The returned object exports the same clock interface,
 * but changes made to its period using set_period() only apply
 * until the open clock object reference is released;
 * then the clock reverts to its original period.
 * Callbacks registered through either interface will still be called,
 * though their exactness may change when the period changes.
 * This method may return OSKIT_E_NOTIMPL
 * if exclusive access to the clock is not supported.
 */
static OSKIT_COMDECL
clock_open(oskit_clock_t *dev, oskit_clock_t **out_opendev)
{
	return OSKIT_E_NOTIMPL;
}

/***********************************************************************/

/*
 * vtables
 */
static struct oskit_clock_ops oskit_clock_ops = {
	clock_query,
	clock_addref,
	clock_release,
	clock_getinfo,
	clock_getdriver,
	clock_getclockinfo,
	clock_gettime,
	clock_sleep,
	clock_spin,
	clock_createtimer,
	clock_settime,
	clock_adjtime,
	clock_setperiod,
	clock_open
};

static struct oskit_timer_ops oskit_timer_ops = {
	timer_query,
	timer_addref,
	timer_release,
	timer_setlistener,
	timer_settime,
	timer_gettime,
	timer_getoverrun
};

static timer_impl_t *
create_timer(clock_impl_t *clock)
{
	timer_impl_t *t;
	t = (timer_impl_t *)oskit_osenv_mem_alloc(clock->mem,
						  sizeof *t, 0, 0);
	memset(t, 0, sizeof *t);
	t->iot.ops = &oskit_timer_ops;
	t->count = 1;
	t->clock = clock;
	return t;
}

/*
 * return a reference to the system clock; creating the object if necessary
 *
 * Requires the osenv interface.
 */
oskit_clock_t *
oskit_clock_init()
{
	clock_impl_t	*c;
#ifndef KNIT
	oskit_osenv_t	*osenv;
	oskit_osenv_mem_t   *mem;
	oskit_osenv_timer_t *timer;
	oskit_osenv_intr_t  *intr;
#endif

	/* there can be only one */
	if (oskit_system_clock)	{
		++oskit_system_clock->count;
		return &oskit_system_clock->ioc;
	}

#ifndef KNIT
	/*
	 * Find the osenv, and then the various osenv interfaces this
	 * component needs.
	 */
	oskit_lookup_first(&oskit_osenv_iid, (void *) &osenv);

	if (!osenv)
		panic("oskit_clock_init(): osenv not yet registered.");

	oskit_osenv_lookup(osenv,
			   &oskit_osenv_mem_iid, (void *) &mem);
	oskit_osenv_lookup(osenv,
			   &oskit_osenv_intr_iid, (void *) &intr);
	oskit_osenv_lookup(osenv,
			   &oskit_osenv_timer_iid, (void *)&timer);
	oskit_osenv_lookup(osenv,
			   &oskit_osenv_log_iid, (void *) &clock_osenv_log);

	oskit_osenv_release(osenv);
#endif /* KNIT */

	/*
	 * initialize the COM object
	 */
	c = (clock_impl_t *)oskit_osenv_mem_alloc(mem,
						  sizeof(*c), 0, 0);
        if (c == NULL)
                return NULL;

	memset(c, 0, sizeof *c);
        c->ioc.ops = &oskit_clock_ops;
        c->count = 1;
#ifndef KNIT
        c->mem   = mem;
        c->timer = timer;
        c->intr  = intr;

	/* safe to call multiple times */
	oskit_osenv_timer_init(timer);
#endif
	oskit_system_clock = c;
	oskit_osenv_timer_register(timer, timer_intr, TIMER_FREQ);
	return &c->ioc;
}

#ifdef KNIT
oskit_error_t
init(void)
{
        if (oskit_clock_init()) {
                return 0;
        } else {
                return OSKIT_ENXIO;
        }
}
#endif
