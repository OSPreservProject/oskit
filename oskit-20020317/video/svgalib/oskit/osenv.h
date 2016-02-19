/*
 * Copyright (c) 1997-2000 The University of Utah and the Flux Group.
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
#ifndef _VIDEO_SVGALIB_OSKIT_OSENV_H_
#define _VIDEO_SVGALIB_OSKIT_OSENV_H_

#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_log.h>

#ifndef KNIT
extern void			oskit_svgalib_osenv_init(void);

/*
 * Interrupt control.
 */
extern oskit_osenv_intr_t	*svgalib_oskit_osenv_intr;

#define osenv_intr_enable() \
	oskit_osenv_intr_enable(svgalib_oskit_osenv_intr)
#define osenv_intr_disable() \
	oskit_osenv_intr_disable(svgalib_oskit_osenv_intr)
#define osenv_intr_enabled() \
	oskit_osenv_intr_enabled(svgalib_oskit_osenv_intr)
#define osenv_intr_save_disable() \
	oskit_osenv_intr_save_disable(svgalib_oskit_osenv_intr)

/*
 * Memory stuff.
 */
extern oskit_osenv_mem_t	*svgalib_oskit_osenv_mem;

#define osenv_mem_alloc(size, flags, align) \
	oskit_osenv_mem_alloc(svgalib_oskit_osenv_mem, \
				(size), (flags), (align))
#define osenv_mem_free(block, flags, size) \
	oskit_osenv_mem_free(svgalib_oskit_osenv_mem, \
				(block), (flags), (size))
#define osenv_mem_get_phys(va) \
	oskit_osenv_mem_getphys(svgalib_oskit_osenv_mem, (va))
#define osenv_mem_get_virt(pa) \
	oskit_osenv_mem_getvirt(svgalib_oskit_osenv_mem, (pa))
#define osenv_mem_phys_max() \
	oskit_osenv_mem_physmax(svgalib_oskit_osenv_mem)
#define osenv_mem_map_phys(pa, size, addr, flags) \
	oskit_osenv_mem_mapphys(svgalib_oskit_osenv_mem, \
				(pa), (size), (addr), (flags))

/*
 * Log and panic.
 */
extern oskit_osenv_log_t	*svgalib_oskit_osenv_log;

#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(svgalib_oskit_osenv_log, (pri), ##fmt)
#define osenv_vlog(pri, fmt, args) \
	oskit_osenv_log_vlog(svgalib_oskit_osenv_log, (pri), (fmt), (args))
#define osenv_panic(fmt...) \
	oskit_osenv_log_panic(svgalib_oskit_osenv_log, ##fmt)
#define osenv_vpanic(fmt, args) \
	oskit_osenv_log_vpanic(svgalib_oskit_osenv_log, (fmt), (args))

#undef  osenv_assert
#define osenv_assert(x) \
	oskit_osenv_assert(svgalib_oskit_osenv_log, x)

#endif /* !KNIT */
	
#endif
