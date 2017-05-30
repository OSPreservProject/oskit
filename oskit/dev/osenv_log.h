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
 * COMification of the osenv log/panic interface. 
 */
#ifndef _OSKIT_DEV_OSENV_LOG_H_
#define _OSKIT_DEV_OSENV_LOG_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/driver.h>

/*
 * osenv log and panic interface.
 *
 * IID 4aa7dff1-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_log {
	struct oskit_osenv_log_ops *ops;
};
typedef struct oskit_osenv_log oskit_osenv_log_t;

struct oskit_osenv_log_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_log_t)

	OSKIT_COMDECL_V (*log)(oskit_osenv_log_t *o,
				int priority, const char *fmt, ...);
	OSKIT_COMDECL_V (*vlog)(oskit_osenv_log_t *o,
				int priority, const char *fmt, void *vl);
	OSKIT_COMDECL_V (*panic)(oskit_osenv_log_t *o,
			        const char *fmt, ...);
	OSKIT_COMDECL_V (*vpanic)(oskit_osenv_log_t *o,
			        const char *fmt, void *vl);
};

/* GUID for oskit_osenv_log interface */
extern const struct oskit_guid oskit_osenv_log_iid;
#define OSKIT_OSENV_LOG_IID OSKIT_GUID(0x4aa7dff1, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_log_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_log_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_log_addref(o) \
	((o)->ops->addref((oskit_osenv_log_t *)(o)))
#define oskit_osenv_log_release(o) \
	((o)->ops->release((oskit_osenv_log_t *)(o)))

#define oskit_osenv_log_log(o, pri, format...) \
	((o)->ops->log((oskit_osenv_log_t *)(o), (pri), ##format))
#define oskit_osenv_log_vlog(o, pri, format, vl) \
	((o)->ops->vlog((oskit_osenv_log_t *)(o), (pri), (format), (vl)))
#define oskit_osenv_log_panic(o, format...) \
	((o)->ops->panic((oskit_osenv_log_t *)(o), ##format))
#define oskit_osenv_log_vpanic(o, format, vl) \
	((o)->ops->vpanic((oskit_osenv_log_t *)(o), (format), (vl)))

/*
 * Return a reference to an osenv log/panic interface object.
 */
oskit_osenv_log_t	*oskit_create_osenv_log(void);

#if (defined DEBUG) || (defined OSKIT_DEBUG) || (defined FDEV_DEBUG)
#define oskit_osenv_assert(o, x)  { if (!(x))				\
	oskit_osenv_log_panic((o), "assertion `"#x"' failed in file "__FILE__ \
		", line %d\n", __LINE__ ); }
#else
#define oskit_osenv_assert(o, x)
#endif

#endif /* _OSKIT_DEV_OSENV_LOG_H_ */
