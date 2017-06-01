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
 * Define the environment services needed by the library to access
 * stuff that is external to it.
 */
#ifndef _OSKIT_C_ENV_H_
#define _OSKIT_C_ENV_H_

#include <oskit/com/services.h>
#include <oskit/com/libcenv.h>
#include <oskit/com/mem.h>

/*
 * The library services object. In the simple case, this is a COM object
 * representing the one and only registry database. Later, it might be
 * something more complicated. 
 */
extern oskit_services_t *libc_services_object;

/*
 * Within the library, use this function to request an interface.
 */
#define oskit_library_services_lookup(iid, out_interface) \
	(oskit_services_lookup_first(libc_services_object, \
				     (iid), (out_interface)))

/*
 * The libc environment object. A random collection of stuff the C library
 * needs in order to be functional. Some stuff is optional, some is not.
 */
extern oskit_libcenv_t	*libc_environment;

/*
 * The memory object for malloc.
 */
extern oskit_mem_t	*libc_memory_object;

#ifdef KNIT
static inline int check_memory_object(void)
{
        return 1;
}
#else
/*
 * Function to get the memory object. Used in the malloc routines.
 */
static inline int check_memory_object(void)
{
        if (libc_memory_object == NULL) { 
		if (libc_services_object) { 
			oskit_library_services_lookup(&oskit_mem_iid, 
                                              (void **)&libc_memory_object); 
		} else { 
			oskit_lookup_first(&oskit_mem_iid, 
					      (void **)&libc_memory_object); 
		} 
		if (libc_memory_object == NULL) 
			return 0; 
	}
        return 1;
}
#endif /* !KNIT */

#endif /* _OSKIT_C_ENV_H_ */
