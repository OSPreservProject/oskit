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
 * The osenv database.
 */

#ifndef _OSKIT_DEV_OSENV_H_
#define _OSKIT_DEV_OSENV_H_

#include <oskit/com/services.h>
struct oskit_mem;

/*
 * The osenv structure is just a plain old services database for now.
 * I create an new GUID for it, but eventually it will be a first class
 * object of its own. I am doing this until I get a better picture of
 * what the interface for the osenv structure should be.
 */
typedef struct oskit_services oskit_osenv_t;

/* GUID for oskit_osenv interface */
extern const struct oskit_guid oskit_osenv_iid;
#define OSKIT_OSENV_IID OSKIT_GUID(0x4aa7dff7, 0x7c74, 0x11cf, \
		0xb5, 0x00, 0x08, 0x00, 0x09, 0x53, 0xad, 0xc2)

#define oskit_osenv_create() ({						\
	oskit_services_t	*__osenv = 0;				\
									\
	oskit_services_create((struct oskit_mem *) 0, &__osenv);	\
	__osenv;							\
})

#define oskit_osenv_addref(osenv) \
	oskit_services_addref((osenv));
#define oskit_osenv_release(osenv) \
	oskit_services_release((osenv));
#define oskit_osenv_register(osenv, iid, intf) \
	oskit_services_addservice((osenv), (iid), (intf));
#define oskit_osenv_unregister(osenv, iid, intf) \
	oskit_services_remservice((osenv), (iid), (intf));
#define oskit_osenv_lookup(osenv, iid, out_intf) \
	oskit_services_lookup_first((osenv), (iid), (out_intf));

/*
 * Helper macro to create and install an interface. This relies on the
 * naming convention that was chosen for all of the init routines and
 * typedefs.
 */
#define INIT_OSENV_IFACE(osenv, name) ({				\
        oskit_osenv_##name##_t *iface = oskit_create_osenv_##name();	\
	osenv_assert(iface);						\
	oskit_osenv_register((osenv),					\
			     &(oskit_osenv_##name##_iid), (void *) iface); \
})

/*
 * Same, but specify the actual interface.
 */
#define SET_OSENV_IFACE(osenv, name, iface) ({				\
	oskit_osenv_register((osenv),					\
			     &(oskit_osenv_##name##_iid), (void *) (iface)); \
})

/*
 * Same thing, but some of the osenv interfaces need to parameterized.
 */
#define INIT_OSENV_IFACE_PARMED(osenv, name, args) ({			\
        oskit_osenv_##name##_t *iface = oskit_create_osenv_##name( args ); \
	osenv_assert(iface);						\
	oskit_osenv_register((osenv),					\
			&(oskit_osenv_##name##_iid), (void *) iface);   \
})

/*
 * Retrieve an interface object from an osenv object.
 */
#define GET_OSENV_IFACE(osenv, name, var) ({				    \
	oskit_services_lookup_first((osenv),				    \
				    &(oskit_osenv_##name##_iid),	    \
				    (void *) (var));			    \
})

/*
 * Create a default osenv implementation that uses all of the existing
 * interfaces.
 */
oskit_osenv_t	*oskit_osenv_create_default(void);
#endif
