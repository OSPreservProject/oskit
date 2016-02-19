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
 * Locking?
 */

/*
 * COM interface for the default implementation of the LIBC environment.
 */
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/com/services.h>
#include <oskit/com/libcenv.h>
#include <oskit/com/mem.h>
#include <oskit/com/stream.h>
#include <oskit/io/ttystream.h>
#include <fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#ifdef PTHREADS
#include <oskit/threads/pthread.h>
#define	oskit_libcenv_create	oskit_libcenv_create_pthreads
#else
#include <oskit/com/listener.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/dev/osenv_sleep.h>
#endif

/* The default console */
extern struct oskit_stream *default_console_stream;

/* A common symbol that is overridden by the kernel library */
void (*oskit_libc_exit)(int);

/* The default signal init function. */
extern void
oskit_sendsig_init(int (*deliver_function)(int, int, void *));

static struct oskit_libcenv_ops libcenv_ops;

#ifdef KNIT
oskit_libcenv_t	*initial_clientos_libcenv;
#endif

/*
 * This is the interface object.
 */
struct genv {
	oskit_libcenv_t		libcenvi;	/* COM interface */
	int			count;		/* Reference count */
	oskit_fsnamespace_t	*fsn;
	char			hostname[256];
	oskit_ttystream_t	*console;
	void			(*exit)(int);
	void			(*siginit)(int (*func)(int, int, void *));
#ifndef PTHREADS
	/*
	 * This stuff for sleep/wakeup support in single threaded mode.
	 * Must request these interfaces dynamically to avoid linktime
	 * dependencies.
	 */
	oskit_timer_t		*sleep_timer;
	oskit_osenv_sleep_t	*sleep_iface;
	osenv_sleeprec_t	*sleeprec;
#endif	
};

/*
 * The initial object is statically constructed.
 */
static struct genv	default_libcenv;

static OSKIT_COMDECL 
libcenv_query(oskit_libcenv_t *s, const oskit_iid_t *iid, void **out_ihandle)
{ 
	struct genv	*g = (struct genv *) s;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_libcenv_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &g->libcenvi;
		g->count++;
                return 0; 
        }
  
        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
libcenv_addref(oskit_libcenv_t *s)
{
	struct genv	*g = (struct genv *) s;

	assert(g->count);
	return ++g->count;
}

static OSKIT_COMDECL_U
libcenv_release(oskit_libcenv_t *s)
{
	struct genv	*g = (struct genv *) s;

	assert(g->count);

	if (--g->count == 0) {
		if (g->fsn)
			oskit_fsnamespace_release(g->fsn);
		if (g->console)
			oskit_ttystream_release(g->console);
#ifndef PTHREADS
		if (g->sleep_timer)
			oskit_timer_release(g->sleep_timer);
		if (g->sleep_iface)
			oskit_osenv_sleep_release(g->sleep_iface);
#endif
		sfree(g, sizeof(g));
	}
	return g->count;
}

/*
 * Get the root directory.
 *
 * NOTE: We do not add a reference for it. Let the caller deal with it.
 */
static OSKIT_COMDECL
libcenv_getfsnamespace(oskit_libcenv_t *s, oskit_fsnamespace_t **intf)
{
	struct genv	*g = (struct genv *) s;

	if (g->fsn) {
		*intf = g->fsn;
		return 0;
	}
	return OSKIT_E_NOINTERFACE;
}

/*
 * Set the root directory. Clear out the old one.
 */
static OSKIT_COMDECL
libcenv_setfsnamespace(oskit_libcenv_t *s, oskit_fsnamespace_t *fsn)
{
	struct genv	*g = (struct genv *) s;

	if (g->fsn) {
		oskit_fsnamespace_release(g->fsn);
		g->fsn = 0;
	}
	if (fsn) {
		g->fsn = fsn;
		oskit_fsnamespace_addref(fsn);
	}
	return 0;
}

/*
 * Get the hostname.
 */
static OSKIT_COMDECL
libcenv_gethostname(oskit_libcenv_t *s, char *name, int len)
{
	struct genv	*g = (struct genv *) s;

	strncpy(name, g->hostname, len);
	return 0;
}

/*
 * Set the hostname
 */
static OSKIT_COMDECL
libcenv_sethostname(oskit_libcenv_t *s, const char *name, int len)
{
	struct genv	*g = (struct genv *) s;

	if (len > 256)
		return OSKIT_EINVAL;

	strncpy(g->hostname, name, 256);
	return 0;
}

/*
 * Call the kernel exit function.
 */
static OSKIT_COMDECL_V
libcenv_exit(oskit_libcenv_t *s, oskit_u32_t exitval)
{
	struct genv	*g = (struct genv *) s;

	if (g->exit)
		g->exit(exitval);
	else {
		printf("Exited with return code %d.\n"
		       "Looping forever...\n", exitval);
		while (1)
			;
	}
}

/*
 * Set the exit function.
 */
static OSKIT_COMDECL
libcenv_setexit(oskit_libcenv_t *s, void (*exitfunc)(int))
{
	struct genv	*g = (struct genv *) s;

	g->exit = exitfunc;
	return 0;
}

/*
 * Get the console object. 
 */
static OSKIT_COMDECL
libcenv_getconsole(oskit_libcenv_t *s, oskit_ttystream_t **out_intf)
{
	struct genv	*g = (struct genv *) s;
	
	if (!g->console)
		return OSKIT_E_NOINTERFACE;

	*out_intf = g->console;
	return 0;
}

/*
 * Set the console object. 
 */
static OSKIT_COMDECL
libcenv_setconsole(oskit_libcenv_t *s, oskit_ttystream_t *console)
{
	struct genv	*g = (struct genv *) s;
	
	g->console = console;
	oskit_ttystream_addref(g->console);
	return 0;
}

/*
 * Call the signal init function.
 */
static OSKIT_COMDECL
libcenv_signals_init(oskit_libcenv_t *s, int (*func)(int, int, void *))
{
	struct genv	*g = (struct genv *) s;

	if (g->siginit)
		g->siginit(func);

	return 0;
}

/*
 * Set the signal init function.
 */
static OSKIT_COMDECL
libcenv_setsiginit(oskit_libcenv_t *s,
		   void (*sigfunc)(int (*func)(int, int, void *)))
{
	struct genv	*g = (struct genv *) s;

	g->siginit = sigfunc;
	return 0;
}

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
#ifndef PTHREADS
/*
 * Single threaded kernel. Can use the osenv interfaces since they do
 * not muck with the process lock.
 */
static oskit_error_t
timeout_handler(oskit_iunknown_t *ioobj, void *arg)
{
	struct genv	*g = (struct genv *) arg;
	
        if (g->sleeprec) {
                oskit_osenv_sleep_wakeup(g->sleep_iface,
                                         g->sleeprec, OSENV_SLEEP_WAKEUP);
                g->sleeprec = 0; /* prevent a wakeup */
        }
	return 0;
}

static OSKIT_COMDECL_V
libcenv_sleep_init(oskit_libcenv_t *s, osenv_sleeprec_t *sleeprec)
{
	struct genv	*g = (struct genv *) s;

	if (!g->sleep_timer) {
		oskit_clock_t		*sys_clock = (oskit_clock_t *) NULL;
		oskit_listener_t	*l;
		oskit_timer_t		*timeout_timer;
		oskit_osenv_t		*osenv;
		
		oskit_lookup_first(&oskit_clock_iid, (void *) &sys_clock);

		if (!sys_clock)
			panic("libcenv_sleep_init: No system clock");

		oskit_lookup_first(&oskit_osenv_iid, (void *) &osenv);
		if (!osenv)
			panic("libcenv_sleep_init: No osenv interface");

		oskit_osenv_lookup(osenv, &oskit_osenv_sleep_iid,
				   (void *) &(g->sleep_iface));
		if (!g->sleep_iface)
			panic("libcenv_sleep_init: No sleep interface");
		oskit_osenv_release(osenv);

		oskit_clock_createtimer(sys_clock, &timeout_timer);
		l = oskit_create_listener(timeout_handler, (void *) g);

		g->sleep_timer = timeout_timer;
		oskit_timer_setlistener(timeout_timer, l);
		oskit_listener_release(l);
		oskit_clock_release(sys_clock);
	}

	g->sleeprec = sleeprec;
	oskit_osenv_sleep_init(g->sleep_iface, sleeprec);
}

static OSKIT_COMDECL_U
libcenv_sleep(oskit_libcenv_t *s,
	      osenv_sleeprec_t *sleeprec, oskit_timespec_t *timeout)
{
	struct genv	*g = (struct genv *) s;
	int		rval;
	
	if (timeout) {
		oskit_itimerspec_t	to;

		to.it_interval.tv_sec  = 0;
		to.it_interval.tv_nsec = 0;
		to.it_value.tv_sec     = timeout->tv_sec;
		to.it_value.tv_nsec    = timeout->tv_nsec;

		oskit_timer_settime(g->sleep_timer, 0, &to);
	}

	rval = oskit_osenv_sleep_sleep(g->sleep_iface, sleeprec);

	if (timeout) {
		oskit_itimerspec_t	zero = { {0}, {0} };
			
		oskit_timer_settime(g->sleep_timer, 0, &zero);
	}

	return ((rval == OSENV_SLEEP_CANCELED) ? ETIMEDOUT : 0);
}

static OSKIT_COMDECL_V
libcenv_wakeup(oskit_libcenv_t *s, osenv_sleeprec_t *sleeprec)
{
	struct genv	*g = (struct genv *) s;

        if (g->sleeprec) {
                oskit_osenv_sleep_wakeup(g->sleep_iface, 
                                         sleeprec, OSENV_SLEEP_WAKEUP);
                g->sleeprec = 0; /* "cancel" the timer */
        }
                
}
#else
/*
 * Now it gets trickier. Not SMP safe, but this will work for the
 * uniprocessor pthreads code. We could make this SMP safe by using
 * a condition variable, but maybe later.
 */
static OSKIT_COMDECL_V
libcenv_sleep_init(oskit_libcenv_t *s, osenv_sleeprec_t *sleeprec)
{
	assert(sleeprec);

	sleeprec->data[0] = OSENV_SLEEP_WAKEUP;
	sleeprec->data[1] = (void *) -1;	/* Pre-sleep. NOT SMP SAFE! */
}

static OSKIT_COMDECL_U
libcenv_sleep(oskit_libcenv_t *s,
	      osenv_sleeprec_t *sleeprec, oskit_timespec_t *timeout)
{
	volatile osenv_sleeprec_t *vsr = sleeprec;
	int			   enabled;
	int			   millis = 0;
	
	enabled = osenv_intr_save_disable();

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

static OSKIT_COMDECL_V
libcenv_wakeup(oskit_libcenv_t *s, osenv_sleeprec_t *sleeprec)
{
	pthread_t	tid;
	int		enabled;
	
	enabled = osenv_intr_save_disable();

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
#endif

/*
 * Clone an interface object.
 */
static OSKIT_COMDECL
libcenv_clone(oskit_libcenv_t *s, oskit_libcenv_t **out_intf)
{
	struct genv	*g = (struct genv *) s;
	struct genv	*newg;
	
	newg = (struct genv *) smalloc(sizeof(*newg));
	if (newg == NULL)
		return OSKIT_E_OUTOFMEMORY;

	newg->libcenvi.ops = &libcenv_ops;
	newg->count        = 1;

	if (g->fsn) {
		newg->fsn = g->fsn;
		oskit_fsnamespace_addref(newg->fsn);
	}
	if (g->console) {
		newg->console = g->console;
		oskit_ttystream_addref(newg->console);
	}
	strcpy(newg->hostname, g->hostname);
	newg->exit      = g->exit;
	newg->siginit   = g->siginit;

	*out_intf = &newg->libcenvi;
	return 0;
}


static struct oskit_libcenv_ops libcenv_ops = {
	libcenv_query,
	libcenv_addref,
	libcenv_release,
	libcenv_getfsnamespace,
	libcenv_setfsnamespace,
	libcenv_gethostname,
	libcenv_sethostname,
	libcenv_exit,
	libcenv_setexit,
	libcenv_getconsole,
	libcenv_setconsole,
	libcenv_signals_init,
	libcenv_setsiginit,
	libcenv_sleep_init,
	libcenv_sleep,
	libcenv_wakeup,
	libcenv_clone,
};

/*
 * This initializes the default (or initial) libcenv structure. It is
 * static.
 */
oskit_error_t
oskit_libcenv_create(oskit_libcenv_t **out_iface)
{
	struct genv	*g = &default_libcenv;

	if (g->count) {
		*out_iface = &g->libcenvi;
		return 0;
	}
		
	g->libcenvi.ops = &libcenv_ops;
	g->count        = 10000;
	g->console      = (oskit_ttystream_t *) default_console_stream;
	g->exit         = oskit_libc_exit;
	g->siginit      = oskit_sendsig_init;

	initial_clientos_libcenv = *out_iface = &g->libcenvi;

	return 0;
}
