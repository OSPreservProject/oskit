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
 * COMification of the osenv interrupt interface. 
 */
#ifndef _OSKIT_DEV_OSENV_INTR_H_
#define _OSKIT_DEV_OSENV_INTR_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>

/*
 * osenv interrupt interface.
 * IID 4aa7dfec-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_intr {
	struct oskit_osenv_intr_ops *ops;
};
typedef struct oskit_osenv_intr oskit_osenv_intr_t;

struct oskit_osenv_intr_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_intr_t)

	OSKIT_COMDECL_V	(*enable)(oskit_osenv_intr_t *s);
	OSKIT_COMDECL_V	(*disable)(oskit_osenv_intr_t *s);
	OSKIT_COMDECL_U	(*enabled)(oskit_osenv_intr_t *s);
	OSKIT_COMDECL_U	(*savedisable)(oskit_osenv_intr_t *s);
};

/* GUID for oskit_osenv_intr interface */
extern const struct oskit_guid oskit_osenv_intr_iid;
#define OSKIT_OSENV_INTR_IID OSKIT_GUID(0x4aa7dfec, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_intr_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_osenv_intr_t *)(s), (iid), (out_ihandle)))
#define oskit_osenv_intr_addref(s) \
	((s)->ops->addref((oskit_osenv_intr_t *)(s)))
#define oskit_osenv_intr_release(s) \
	((s)->ops->release((oskit_osenv_intr_t *)(s)))

#define oskit_osenv_intr_enable(s) \
	((s)->ops->enable((oskit_osenv_intr_t *)(s)))
#define oskit_osenv_intr_disable(s) \
	((s)->ops->disable((oskit_osenv_intr_t *)(s)))
#define oskit_osenv_intr_enabled(s) \
	((s)->ops->enabled((oskit_osenv_intr_t *)(s)))
#define oskit_osenv_intr_save_disable(s) \
	((s)->ops->savedisable((oskit_osenv_intr_t *)(s)))

/*
 * Return a reference to an osenv interrupt object.
 */
oskit_osenv_intr_t	*oskit_create_osenv_intr(void);

#endif /* _OSKIT_DEV_OSENV_INTR_H_ */

