/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
 * Definitions specific to timer (timing) devices.
 * This basic timer device interface doesn't assume
 * that the timer it represents is programmable or settable;
 * it only provides the basic services of waiting for a period of time
 * and registering timer interrupt callbacks.
 * Also, timers exporting this interface
 * do not necessarily report real-world wall-timer time.
 */
#ifndef _OSKIT_DEV_TIMER_H_
#define _OSKIT_DEV_TIMER_H_

#include <oskit/com.h>
#include <oskit/time.h>

struct oskit_timespec;
struct oskit_listener;

/*
 * possible values for flags parameter in timer::settime
 */
#define OSKIT_TIMER_ABSTIME	0x1	/* time in it_value is absolute */

/*
 * Standard timer interface, corresponding to POSIX's timer functions.
 * IID 4aa7dfa4-7c74-11cf-b500-08000953adc2.
 */
struct oskit_timer
{
	struct oskit_timer_ops *ops;
};
typedef struct oskit_timer oskit_timer_t;

struct oskit_timer_ops
{
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL_IUNKNOWN(oskit_timer_t)

	/* Set a listener to be notifed when timer expires */
	OSKIT_COMDECL_V	(*setlistener)(oskit_timer_t *timer,
				   struct oskit_listener *listener);

	/* Set the timer's expiration and period. */
	OSKIT_COMDECL	(*settime)(oskit_timer_t *timer, int flags,
				   const struct oskit_itimerspec *value);

	/* Query the timer's current expiration and period. */
	OSKIT_COMDECL_V	(*gettime)(oskit_timer_t *timer,
				   struct oskit_itimerspec *out_value);

	/* Return number of overruns */
	OSKIT_COMDECL	(*getoverrun)(oskit_timer_t *timer);

	/*
	 * note that we don't provide an interface to query the time
	 * of the clock with which the timer is associated - the user 
	 * is required to keep track of this association
	 */
};

#define oskit_timer_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_timer_t *)(io), (iid), (out_ihandle)))
#define oskit_timer_addref(io) \
	((io)->ops->addref((oskit_timer_t *)(io)))
#define oskit_timer_release(io) \
	((io)->ops->release((oskit_timer_t *)(io)))
#define oskit_timer_setlistener(timer, listener) \
	((timer)->ops->setlistener((oskit_timer_t *)(timer), (listener)))
#define oskit_timer_settime(timer, flags, value) \
	((timer)->ops->settime((oskit_timer_t *)(timer), (flags), (value)))
#define oskit_timer_gettime(timer, out_value) \
	((timer)->ops->gettime((oskit_timer_t *)(timer), (out_value)))
#define oskit_timer_getoverrun(timer) \
	((timer)->ops->getoverrun((oskit_timer_t *)(timer)))

/* GUID for oskit timer device interface */
extern const struct oskit_guid oskit_timer_iid;
#define OSKIT_TIMER_IID OSKIT_GUID(0x4aa7dfa4, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#endif /* _OSKIT_DEV_TIMER_H_ */
