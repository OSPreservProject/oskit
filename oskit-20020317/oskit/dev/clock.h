/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
 * Definitions specific to clock (timing) devices.
 * This basic clock device interface doesn't assume
 * that the clock it represents is programmable or settable;
 * it only provides the basic services of waiting for a period of time
 * and registering clock interrupt callbacks.
 * Also, clocks exporting this interface
 * do not necessarily report real-world wall-clock time.
 */
#ifndef _OSKIT_DEV_CLOCK_H_
#define _OSKIT_DEV_CLOCK_H_

#include <oskit/time.h>
#include <oskit/dev/device.h>

struct oskit_timer;


struct oskit_clock_info {
	oskit_u32_t		flags;

	/*
	 * Precision with which the clock can trigger timer callbacks.
	 * This is generally the period of the clock.
	 */
	oskit_timespec_t		timer_precision;

	/*
	 * Precision with which the clock can be explicitly queried.
	 * This is often smaller (higher precision) than timer_precision.
	 */
	oskit_timespec_t		query_precision;

	/*
	 * Precision with which the clock supports blocking waits.
	 * Usually the same as timer_precision.
	 */
	oskit_timespec_t		sleep_precision;

	/*
	 * Precision with which the clock supports nonblocking waits.
	 * Usually the same as query_precision.
	 */
	oskit_timespec_t		spin_precision;
};

#define OSKIT_CLOCK_SETTABLE	0x00000001	/* Time can be set */
#define OSKIT_CLOCK_ADJUSTABLE	0x00000002	/* ...gradually */
#define OSKIT_CLOCK_PROGRAMMABLE	0x00000004	/* Period can be set */
#define OSKIT_CLOCK_REAL_TIME	0x00000008	/* Reports real time */

/*
 * Standard clock device node interface, derived from oskit_device_t,
 * IID 4aa7dfa1-7c74-11cf-b500-08000953adc2.
 */
struct oskit_clock
{
	struct oskit_clock_ops *ops;
};
typedef struct oskit_clock oskit_clock_t;

struct oskit_clock_ops
{
	/* COM-specified IUnknown interface operations */
	OSKIT_COMDECL_IUNKNOWN(oskit_clock_t)

	/* Base fdev device interface operations */
	OSKIT_COMDECL	(*getinfo)(oskit_clock_t *fdev, oskit_devinfo_t *out_info);
	OSKIT_COMDECL	(*getdriver)(oskit_clock_t *fdev,
				      oskit_driver_t **out_driver);

	/* Clock device interface operations */

	/* Return basic information about this clock, defined above */
	OSKIT_COMDECL_V	(*getclockinfo)(oskit_clock_t *dev,
					  struct oskit_clock_info *out_info);

	/* Query the clock's current time. */
	OSKIT_COMDECL_V	(*gettime)(oskit_clock_t *dev,
				    struct oskit_timespec *out_time);

	/*
	 * Wait for a period of time, possibly blocking during the wait.
	 */
	OSKIT_COMDECL_V	(*sleep)(oskit_clock_t *dev,
				 const struct oskit_timespec *sltime);

	/*
	 * Wait for a period of time without blocking
	 * or enabling interrupts if interrupts are disabled on call.
	 * This should only be used for very short waits.
	 */
	OSKIT_COMDECL_V	(*spin)(oskit_clock_t *dev,
				const struct oskit_timespec *sptime);

	/*
	 * Create a new timer based on this clock.
	 */
	OSKIT_COMDECL	(*createtimer)(oskit_clock_t *dev,
					struct oskit_timer **out_timer);

	/*
	 * Set the clock's current time immediately.
	 * Clocks that aren't settable will return OSKIT_E_NOTIMPL.
	 */
	OSKIT_COMDECL	(*settime)(oskit_clock_t *dev,
				    const struct oskit_timespec *new_time);

	/*
	 * Set the clock's current time gradually,
	 * so that the clock's value does not jump or go backward.
	 * Clocks that aren't adjustable will return OSKIT_E_NOTIMPL.
	 */
	OSKIT_COMDECL	(*adjtime)(oskit_clock_t *dev,
				       const struct oskit_timespec *new_time);

	/*
	 * Set the clock's period, or timer_precision.
	 * Clocks that aren't programmable will return OSKIT_E_NOTIMPL.
	 * Even if the clock is programmable,
	 * it may not support the exact period requested;
	 * the caller can getclockinfo() and check timer_precision
	 * to determine the actual period for which the clock was set.
	 * If the requested period is completely outside of the range
	 * supported by the clock, this method returns OSKIT_E_DEV_BAD_PERIOD.
	 */
	OSKIT_COMDECL	(*setperiod)(oskit_clock_t *dev,
				      const struct oskit_timespec *period);

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
	OSKIT_COMDECL	(*open)(oskit_clock_t *dev, oskit_clock_t **out_opendev);
};

/* GUID for fdev clock device interface */
extern const struct oskit_guid oskit_clock_iid;
#define OSKIT_CLOCK_IID OSKIT_GUID(0x4aa7dfa1, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_clock_query(io, iid, out_ihandle) \
	((io)->ops->query((oskit_clock_t *)(io), (iid), (out_ihandle)))
#define oskit_clock_addref(io) \
	((io)->ops->addref((oskit_clock_t *)(io)))
#define oskit_clock_release(io) \
	((io)->ops->release((oskit_clock_t *)(io)))
#define oskit_clock_getinfo(fdev, out_info) \
	((fdev)->ops->getinfo((oskit_clock_t *)(fdev), (out_info)))
#define oskit_clock_getdriver(fdev, out_driver) \
	((fdev)->ops->getdriver((oskit_clock_t *)(fdev), (out_driver)))
#define oskit_clock_getclockinfo(dev, out_info) \
	((dev)->ops->getclockinfo((oskit_clock_t *)(dev), (out_info)))
#define oskit_clock_gettime(dev, out_time) \
	((dev)->ops->gettime((oskit_clock_t *)(dev), (out_time)))
#define oskit_clock_sleep(dev, sltime) \
	((dev)->ops->sleep((oskit_clock_t *)(dev), (sltime)))
#define oskit_clock_spin(dev, sptime) \
	((dev)->ops->spin((oskit_clock_t *)(dev), (sptime)))
#define oskit_clock_createtimer(dev, out_timer) \
	((dev)->ops->createtimer((oskit_clock_t *)(dev), (out_timer)))
#define oskit_clock_settime(dev, new_time) \
	((dev)->ops->settime((oskit_clock_t *)(dev), (new_time)))
#define oskit_clock_adjtime(dev, new_time) \
	((dev)->ops->adjtime((oskit_clock_t *)(dev), (new_time)))
#define oskit_clock_setperiod(dev, period) \
	((dev)->ops->setperiod((oskit_clock_t *)(dev), (period)))
#define oskit_clock_open(dev, out_opendev) \
	((dev)->ops->open((oskit_clock_t *)(dev), (out_opendev)))


/* This is how the default implementation gives you one. */
struct oskit_clock *oskit_clock_init(void);

#endif /* _OSKIT_DEV_CLOCK_H_ */
