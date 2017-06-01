/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * Map older osenv interface to new COMified osenv interface.
 */
#ifndef _X11_VIDEO_SHARED_OSENV_H_
#define _X11_VIDEO_SHARED_OSENV_H_

#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>

#ifndef KNIT

extern oskit_osenv_intr_t	*oskit_x11_oskit_osenv_intr;

#define osenv_intr_enable() \
	oskit_osenv_intr_enable(oskit_x11_oskit_osenv_intr)
#define osenv_intr_disable() \
	oskit_osenv_intr_disable(oskit_x11_oskit_osenv_intr)
#define osenv_intr_enabled() \
	oskit_osenv_intr_enabled(oskit_x11_oskit_osenv_intr)

extern oskit_osenv_mem_t	*oskit_x11_oskit_osenv_mem;

#define osenv_mem_alloc(size, flags, align) \
	oskit_osenv_mem_alloc(oskit_x11_oskit_osenv_mem, (size), (flags), (align))
#define osenv_mem_free(block, flags, size) \
	oskit_osenv_mem_free(oskit_x11_oskit_osenv_mem, (block), (flags), (size))
#define osenv_mem_get_phys(va) \
	oskit_osenv_mem_getphys(oskit_x11_oskit_osenv_mem, (va))
#define osenv_mem_get_virt(pa) \
	oskit_osenv_mem_getvirt(oskit_x11_oskit_osenv_mem, (pa))
#define osenv_mem_phys_max() \
	oskit_osenv_mem_physmax(oskit_x11_oskit_osenv_mem)
#define osenv_mem_map_phys(pa, size, addr, flags) \
	oskit_osenv_mem_mapphys(oskit_x11_oskit_osenv_mem, \
				(pa), (size), (addr), (flags))


void				oskit_x11_osenv_init(void);

#endif /* !KNIT */

#endif
