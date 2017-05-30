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
 * Map the osenv calls to appropriate environment calls. The MEMFS requires
 * only two interfaces, which we cache at initialization. I prefer this
 * approach for the moment, instead of rewriting all the calls inside of the
 * memfs code.
 */
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>

#ifdef KNIT

/*
 * Local log.
 * Uses a per-component log object.
 */
#define osenv_local_log(fsys, pri, fmt...) \
        osenv_log((pri), ##fmt)

/*
 * Memory stuff.
 * Uses a per-filesystem memory allocator.
 */

#define osenv_mem_alloc(fsys,size, flags, align) \
	osenv_mem_alloc((size), (flags), (align))
#define osenv_mem_free(fsys,block, flags, size) \
	osenv_mem_free((block), (flags), (size))
#define osenv_mem_get_phys(fsys,va) \
	osenv_mem_getphys((va))
#define osenv_mem_get_virt(fsys,pa) \
	osenv_mem_getvirt((pa))
#define osenv_mem_phys_max(fsys) \
	osenv_mem_physmax()
#define osenv_mem_map_phys(fsys,pa, size, addr, flags) \
	osenv_mem_mapphys((pa), (size), (addr), (flags))

#else

extern oskit_osenv_log_t      *memfs_oskit_osenv_log;

/*
 * Global log and panic.
 * Uses a global log object.
 */
#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(memfs_oskit_osenv_log, (pri), ##fmt)
#define osenv_vlog(pri, fmt, args) \
	oskit_osenv_log_vlog(memfs_oskit_osenv_log, (pri), (fmt), (args))
#define osenv_panic(fmt...) \
	oskit_osenv_log_panic(memfs_oskit_osenv_log, ##fmt)
#define osenv_vpanic(fmt, args) \
	oskit_osenv_log_vpanic(memfs_oskit_osenv_log, (fmt), (args))

/*
 * Local log.
 * Uses a per-component log object.
 */
#define osenv_local_log(fsys, pri, fmt...) \
	oskit_osenv_log_log((fsys)->log, (pri), ##fmt)

/*
 * Memory stuff.
 * Uses a per-filesystem memory allocator.
 */

#define osenv_mem_alloc(fsys,size, flags, align) \
	oskit_osenv_mem_alloc((fsys)->mem, (size), (flags), (align))
#define osenv_mem_free(fsys,block, flags, size) \
	oskit_osenv_mem_free((fsys)->mem, (block), (flags), (size))
#define osenv_mem_get_phys(fsys,va) \
	oskit_osenv_mem_getphys((fsys)->mem, (va))
#define osenv_mem_get_virt(fsys,pa) \
	oskit_osenv_mem_getvirt((fsys)->mem, (pa))
#define osenv_mem_phys_max(fsys) \
	oskit_osenv_mem_physmax((fsys)->mem)
#define osenv_mem_map_phys(fsys,pa, size, addr, flags) \
	oskit_osenv_mem_mapphys((fsys)->mem, \
				(pa), (size), (addr), (flags))

#endif /* !KNIT */
