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
 * COMification of the osenv IRQ interface.
 */
#ifndef _OSKIT_DEV_OSENV_IRQ_H_
#define _OSKIT_DEV_OSENV_IRQ_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>

/*
 * osenv irq interface.
 * IID 4aa7dfed-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_irq {
	struct oskit_osenv_irq_ops *ops;
};
typedef struct oskit_osenv_irq oskit_osenv_irq_t;

struct oskit_osenv_irq_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_irq_t)

	OSKIT_COMDECL_U (*alloc)(oskit_osenv_irq_t *o, int irq,
			       void (*handler)(void *), void *data, int flags);
	OSKIT_COMDECL_V (*free)(oskit_osenv_irq_t *o, int irq,
			       void (*handler)(void *), void *data);
	OSKIT_COMDECL_V (*disable)(oskit_osenv_irq_t *o, int irq);
	OSKIT_COMDECL_V (*enable)(oskit_osenv_irq_t *o, int irq);
	OSKIT_COMDECL_U (*pending)(oskit_osenv_irq_t *o, int irq);
};

/* GUID for oskit_osenv_irq interface */
extern const struct oskit_guid oskit_osenv_irq_iid;
#define OSKIT_OSENV_IRQ_IID OSKIT_GUID(0x4aa7dfed, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_irq_query(o, iid, out_ihandle) \
	((o)->ops->query((oskit_osenv_irq_t *)(o), (iid), (out_ihandle)))
#define oskit_osenv_irq_addref(o) \
	((o)->ops->addref((oskit_osenv_irq_t *)(o)))
#define oskit_osenv_irq_release(o) \
	((o)->ops->release((oskit_osenv_irq_t *)(o)))

#define oskit_osenv_irq_alloc(o, irq, handler, data, flags) \
	((o)->ops->alloc((oskit_osenv_irq_t *)(o), \
			(irq), (handler), (data), (flags)))
#define oskit_osenv_irq_free(o, irq, handler, data) \
	((o)->ops->free((oskit_osenv_irq_t *)(o), (irq), (handler), (data)))
#define oskit_osenv_irq_disable(o, irq) \
	((o)->ops->disable((oskit_osenv_irq_t *)(o), (irq)))
#define oskit_osenv_irq_enable(o, irq) \
	((o)->ops->enable((oskit_osenv_irq_t *)(o), (irq)))
#define oskit_osenv_irq_pending(o, irq) \
	((o)->ops->pending((oskit_osenv_irq_t *)(o), (irq)))

/*
 * Return a reference to an osenv interrupt object.
 */
oskit_osenv_irq_t	*oskit_create_osenv_irq(void);

#endif /* _OSKIT_DEV_OSENV_IRQ_H_ */

