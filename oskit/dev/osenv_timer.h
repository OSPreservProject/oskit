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
/*
 * COMification of the osenv timer interface. 
 */
#ifndef _OSKIT_DEV_OSENV_TIMER_H_
#define _OSKIT_DEV_OSENV_TIMER_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>

/*
 * osenv timer interface.
 *
 * IID 4aa7dfee-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_timer {
	struct oskit_osenv_timer_ops *ops;
};
typedef struct oskit_osenv_timer oskit_osenv_timer_t;

struct oskit_osenv_timer_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_timer_t)

	OSKIT_COMDECL_V (*init)(oskit_osenv_timer_t *o);
	OSKIT_COMDECL_V (*shutdown)(oskit_osenv_timer_t *o);
	OSKIT_COMDECL_V (*spin)(oskit_osenv_timer_t *o, long nanosec);
	OSKIT_COMDECL_V (*Register)(oskit_osenv_timer_t *o,
				void (*func)(void), int freq);
	OSKIT_COMDECL_V (*unregister)(oskit_osenv_timer_t *o,
				void (*func)(void), int freq);
};

/* GUID for oskit_osenv_timer interface */
extern const struct oskit_guid oskit_osenv_timer_iid;
#define OSKIT_OSENV_TIMER_IID OSKIT_GUID(0x4aa7dfee, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_timer_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_timer_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_timer_addref(o) \
	((o)->ops->addref((oskit_osenv_timer_t *)(o)))
#define oskit_osenv_timer_release(o) \
	((o)->ops->release((oskit_osenv_timer_t *)(o)))

#define oskit_osenv_timer_init(o) \
	((o)->ops->init((oskit_osenv_timer_t *)(o)))
#define oskit_osenv_timer_shutdown(o) \
	((o)->ops->shutdown((oskit_osenv_timer_t *)(o)))
#define oskit_osenv_timer_spin(o, nanos) \
	((o)->ops->spin((oskit_osenv_timer_t *)(o), (nanos)))
#define oskit_osenv_timer_register(o, handler, freq) \
	((o)->ops->Register((oskit_osenv_timer_t *)(o), (handler), (freq)))
#define oskit_osenv_timer_unregister(o, handler, freq) \
	((o)->ops->unregister((oskit_osenv_timer_t *)(o), (handler), (freq)))

/*
 * Return a reference to an osenv timer interface object.
 */
oskit_osenv_timer_t	*oskit_create_osenv_timer(void);

#endif /* _OSKIT_DEV_OSENV_TIMER_H_ */

