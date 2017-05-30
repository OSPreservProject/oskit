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
 * COMification of the osenv memory allocation interface. 
 */
#ifndef _OSKIT_DEV_OSENV_MEM_H_
#define _OSKIT_DEV_OSENV_MEM_H_

#include <oskit/com.h>
#include <oskit/error.h>
#include <oskit/dev/dev.h>

/*
 * osenv memory allocation interface.
 *
 * IID 4aa7dff5-7c74-11cf-b500-08000953adc2
 */
struct oskit_osenv_mem {
	struct oskit_osenv_mem_ops *ops;
};
typedef struct oskit_osenv_mem oskit_osenv_mem_t;

struct oskit_osenv_mem_ops {

	/*** COM-specified IUnknown interface operations ***/
	OSKIT_COMDECL_IUNKNOWN(oskit_osenv_mem_t)
		
	void *
	OSKIT_COMCALL	(*alloc)(oskit_osenv_mem_t *o, oskit_size_t size,
				osenv_memflags_t flags, unsigned align);
	OSKIT_COMDECL_V (*free)(oskit_osenv_mem_t *o, void *block,
				osenv_memflags_t flags, oskit_size_t size);
	oskit_addr_t
	OSKIT_COMCALL	(*getphys)(oskit_osenv_mem_t *o, oskit_addr_t va);
	oskit_addr_t
	OSKIT_COMCALL	(*getvirt)(oskit_osenv_mem_t *o, oskit_addr_t pa);
	oskit_addr_t
	OSKIT_COMCALL	(*physmax)(oskit_osenv_mem_t *o);
	OSKIT_COMDECL_U (*mapphys)(oskit_osenv_mem_t *o, oskit_addr_t pa,
				oskit_size_t size, void **addr, int flags);
};

/* GUID for oskit_osenv_mem interface */
extern const struct oskit_guid oskit_osenv_mem_iid;
#define OSKIT_OSENV_MEM_IID OSKIT_GUID(0x4aa7dff5, 0x7c74, 0x11cf, \
				0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_mem_query(s, iid, out_ihandle) \
	((s)->ops->query((oskit_osenv_mem_t *)(s), (iid), (out_ihandle)))
#define oskit_osenv_mem_addref(s) \
	((s)->ops->addref((oskit_osenv_mem_t *)(s)))
#define oskit_osenv_mem_release(s) \
	((s)->ops->release((oskit_osenv_mem_t *)(s)))

#define oskit_osenv_mem_alloc(s, size, flags, align) \
	((s)->ops->alloc((oskit_osenv_mem_t *)(s), (size), (flags), (align)))
#define oskit_osenv_mem_free(s, block, flags, size) \
	((s)->ops->free((oskit_osenv_mem_t *)(s), (block), (flags), (size)))
#define oskit_osenv_mem_getphys(s, va) \
	((s)->ops->getphys((oskit_osenv_mem_t *)(s), (va)))
#define oskit_osenv_mem_getvirt(s, pa) \
	((s)->ops->getvirt((oskit_osenv_mem_t *)(s), (pa)))
#define oskit_osenv_mem_physmax(s) \
	((s)->ops->physmax((oskit_osenv_mem_t *)(s)))
#define oskit_osenv_mem_mapphys(s, pa, size, addr, flags) \
	((s)->ops->mapphys((oskit_osenv_mem_t *)(s), \
			   (pa), (size), (addr), (flags)))

/*
 * Return a reference to an osenv memory allocation interface object.
 */
oskit_osenv_mem_t	*oskit_create_osenv_mem(void);

#endif /* _OSKIT_DEV_OSENV_MEM_H_ */
