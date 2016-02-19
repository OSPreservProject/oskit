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
 * COMification of the osenv isa configuration interface. 
 */
#ifndef _OSKIT_DEV_OSENV_ISA_H_
#define _OSKIT_DEV_OSENV_ISA_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/device.h>

/*
 * osenv isa configuration interface.
 *
 * IID 4aa7dff3-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_isa {
	struct oskit_osenv_isa_ops *ops;
};
typedef struct oskit_osenv_isa oskit_osenv_isa_t;

struct oskit_osenv_isa_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_isa_t)

	OSKIT_COMDECL   (*bus_init)(oskit_osenv_isa_t *o);
	struct oskit_isabus *
	OSKIT_COMCALL   (*bus_getbus)(oskit_osenv_isa_t *o);
	OSKIT_COMDECL   (*bus_addchild)(oskit_osenv_isa_t *o,
				oskit_u32_t addr, oskit_device_t *dev);
	OSKIT_COMDECL   (*bus_remchild)(oskit_osenv_isa_t *o,
				oskit_u32_t addr);

	/*
	 * DRQ manipulation.
	 */
	OSKIT_COMDECL   (*dma_alloc)(oskit_osenv_isa_t *o, int channel);
	OSKIT_COMDECL_V (*dma_free)(oskit_osenv_isa_t *o, int channel);
};

/* GUID for oskit_osenv_isa interface */
extern const struct oskit_guid oskit_osenv_isa_iid;
#define OSKIT_OSENV_ISA_IID OSKIT_GUID(0x4aa7dff3, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_isa_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_osenv_isa_t *)(s), (iid), (out_ihandle)))
#define oskit_osenv_isa_addref(s) \
	((s)->ops->addref((oskit_osenv_isa_t *)(s)))
#define oskit_osenv_isa_release(s) \
	((s)->ops->release((oskit_osenv_isa_t *)(s)))

#define oskit_osenv_isa_bus_init(s) \
	((s)->ops->bus_init((oskit_osenv_isa_t *)(s)))
#define oskit_osenv_isa_bus_getbus(s) \
	((s)->ops->bus_getbus((oskit_osenv_isa_t *)(s)))
#define oskit_osenv_isa_bus_addchild(s, addr, dev) \
	((s)->ops->bus_addchild((oskit_osenv_isa_t *)(s), (addr), (dev)))
#define oskit_osenv_isa_bus_remchild(s, addr) \
	((s)->ops->bus_remchild((oskit_osenv_isa_t *)(s), (addr)))

#define oskit_osenv_isa_dma_alloc(s, channel) \
	((s)->ops->dma_alloc((oskit_osenv_isa_t *)(s), (channel)))
#define oskit_osenv_isa_dma_free(s, channel) \
	((s)->ops->dma_free((oskit_osenv_isa_t *)(s), (channel)))

/*
 * Return a reference to an osenv isa goop interface object.
 */
oskit_osenv_isa_t	*oskit_create_osenv_isa(void);

#endif /* _OSKIT_DEV_OSENV_ISA_H_ */
