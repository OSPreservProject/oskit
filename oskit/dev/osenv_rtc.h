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
 * COMification of the osenv RTC (real time clock) interface.
 */
#ifndef _OSKIT_DEV_OSENV_RTC_H_
#define _OSKIT_DEV_OSENV_RTC_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/time.h>

/*
 * osenv RTC interface.
 *
 * IID 4aa7dff6-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_rtc {
	struct oskit_osenv_rtc_ops *ops;
};
typedef struct oskit_osenv_rtc oskit_osenv_rtc_t;

struct oskit_osenv_rtc_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_rtc_t)

	OSKIT_COMDECL   (*get)(oskit_osenv_rtc_t *o,
			struct oskit_timespec *rtctime);
	OSKIT_COMDECL_V (*set)(oskit_osenv_rtc_t *o,
			struct oskit_timespec *rtctime);
};

/* GUID for oskit_osenv_rtc interface */
extern const struct oskit_guid oskit_osenv_rtc_iid;
#define OSKIT_OSENV_RTC_IID OSKIT_GUID(0x4aa7dff6, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_rtc_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_rtc_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_rtc_addref(o) \
	((o)->ops->addref((oskit_osenv_rtc_t *)(o)))
#define oskit_osenv_rtc_release(o) \
	((o)->ops->release((oskit_osenv_rtc_t *)(o)))

#define oskit_osenv_rtc_set(o, rtctime) \
	((o)->ops->set((oskit_osenv_rtc_t *)(o), (rtctime)))
#define oskit_osenv_rtc_get(o, rtctime) \
	((o)->ops->get((oskit_osenv_rtc_t *)(o), (rtctime)))

/*
 * Return a reference to an osenv rtc interface object.
 */
oskit_osenv_rtc_t	*oskit_create_osenv_rtc(void);

#endif /* _OSKIT_DEV_OSENV_RTC_H_ */

