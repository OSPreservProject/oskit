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
 * COMification of the sleep/wakeup interface. 
 */
#ifndef _OSKIT_DEV_OSENV_SLEEP_H_
#define _OSKIT_DEV_OSENV_SLEEP_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_intr.h>

/*
 * Wakeup status values are the same as those in oskit/dev/dev.h
 */
#define OSKIT_SLEEP_WAKEUP	OSENV_SLEEP_WAKEUP
#define	OSKIT_SLEEP_CANCELED	OSENV_SLEEP_CANCELED

/*
 * Sleep/wakeup object. Intended to replace the older sleep_rec_t interface.
 * 
 * IID 4aa7dfea-7c74-11cf-b500-08000953adc2
 */
struct oskit_sleep {
	struct oskit_sleep_ops *ops;
};
typedef struct oskit_sleep oskit_sleep_t;

struct oskit_sleep_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_sleep_t)

	/*
	 * Give up the CPU. Upon return from sleep, the sleep object
	 * is left intact, and reinitialized for another call to sleep.
	 * This allows easy reuse of a single sleep object, without
	 * having to reinitialize it before the next use.
	 */
	OSKIT_COMDECL_U	(*sleep)(oskit_sleep_t *s);

	/*
	 * Same as above, except that the sleep object is consumed
	 * when the sleep terminates. No release is necessary.
	 */
	OSKIT_COMDECL_U	(*sleep_consume)(oskit_sleep_t *s);

	/*
	 * Wakeup the sleeper. The status indicator is returned to caller
	 * of sleep. 
	 */
	OSKIT_COMDECL_V	(*wakeup)(oskit_sleep_t *s, oskit_u32_t status);
};

/* GUID for oskit_sleep interface */
extern const struct oskit_guid oskit_sleep_iid;
#define OSKIT_SLEEP_IID OSKIT_GUID(0x4aa7dfea, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_sleep_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_sleep_t *)(s), (iid), (out_ihandle)))
#define oskit_sleep_addref(s) \
	((s)->ops->addref((oskit_sleep_t *)(s)))
#define oskit_sleep_release(s) \
	((s)->ops->release((oskit_sleep_t *)(s)))

#define oskit_sleep_sleep(s) \
	((s)->ops->sleep((oskit_sleep_t *)(s)))
#define oskit_sleep_sleep_consume(s) \
	((s)->ops->sleep_consume((oskit_sleep_t *)(s)))
#define oskit_sleep_wakeup(s, status) \
	((s)->ops->wakeup((oskit_sleep_t *)(s), (status)))

/*
 * osenv sleep interface. This is a comification of the original sleep
 * interface. It can be used with the older sleep_rec_t data structure.
 * Or, if you are brave, you can use the create method, and get back
 * an oskit_sleep_t object (above). Eventually, all uses of sleep_rec_t
 * will be replaced with the sleep object approach.
 * 
 * IID 4aa7dfeb-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_sleep {
	struct oskit_osenv_sleep_ops *ops;
};
typedef struct oskit_osenv_sleep oskit_osenv_sleep_t;

struct oskit_osenv_sleep_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_sleep_t)

	/*
	 * The original sleep/wakeup interface!
	 */
	OSKIT_COMDECL_V (*init)(oskit_osenv_sleep_t *o,
				osenv_sleeprec_t *sr);
	OSKIT_COMDECL_U (*sleep)(oskit_osenv_sleep_t *o,
				osenv_sleeprec_t *sr);
	OSKIT_COMDECL_V (*wakeup)(oskit_osenv_sleep_t *o,
				osenv_sleeprec_t *sr, int wakeup_status);

	/*
	 * The factory for creating new-style sleep objects.
	 */
	OSKIT_COMDECL	(*create)(oskit_osenv_sleep_t *f,
				  oskit_sleep_t **out_sleep);
};

/* GUID for oskit_osenv_sleep interface */
extern const struct oskit_guid oskit_osenv_sleep_iid;
#define OSKIT_OSENV_SLEEP_IID OSKIT_GUID(0x4aa7dfeb, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_sleep_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_sleep_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_sleep_addref(o) \
	((o)->ops->addref((oskit_osenv_sleep_t *)(o)))
#define oskit_osenv_sleep_release(o) \
	((o)->ops->release((oskit_osenv_sleep_t *)(o)))

#define oskit_osenv_sleep_init(o, sr) \
	((o)->ops->init((oskit_osenv_sleep_t *)(o), (sr)))
#define oskit_osenv_sleep_sleep(o, sr) \
	((o)->ops->sleep((oskit_osenv_sleep_t *)(o), (sr)))
#define oskit_osenv_sleep_wakeup(o, sr, status) \
	((o)->ops->wakeup((oskit_osenv_sleep_t *)(o), (sr), (status)))
#define oskit_osenv_sleep_create(o, out) \
	((o)->ops->create((oskit_osenv_sleep_t *)(o), (out)))

/*
 * Return a reference to an osenv sleep object. This must be parametrized
 * with the appropriate osenv_intr interface object.
 */
oskit_osenv_sleep_t	*oskit_create_osenv_sleep(oskit_osenv_intr_t *iface);

#endif /* _OSKIT_DEV_OSENV_SLEEP_H_ */

