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
 * Map older osenv interface to new COMified osenv interface.
 */
#ifndef _NETBSD_FS_OSENV_H_
#define _NETBSD_FS_OSENV_H_

#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_timer.h>
#include <oskit/dev/osenv_rtc.h>

#ifdef INDIRECT_OSENV
void	fs_netbsd_oskit_osenv_init(oskit_osenv_t *osenv);

/*
 * Sleep/wakeup
 */
extern oskit_osenv_sleep_t	*fs_netbsd_oskit_osenv_sleep;

#define osenv_sleep_init(sr) \
	oskit_osenv_sleep_init(fs_netbsd_oskit_osenv_sleep, (sr))
#define osenv_sleep(sr) \
	oskit_osenv_sleep_sleep(fs_netbsd_oskit_osenv_sleep, (sr))
#define osenv_wakeup(sr, status) \
	oskit_osenv_sleep_wakeup(fs_netbsd_oskit_osenv_sleep, (sr), (status))

/*
 * Interrupt control.
 */
extern oskit_osenv_intr_t	*fs_netbsd_oskit_osenv_intr;

#define osenv_intr_enable() \
	oskit_osenv_intr_enable(fs_netbsd_oskit_osenv_intr)
#define osenv_intr_disable() \
	oskit_osenv_intr_disable(fs_netbsd_oskit_osenv_intr)
#define osenv_intr_enabled() \
	oskit_osenv_intr_enabled(fs_netbsd_oskit_osenv_intr)
#define osenv_intr_save_disable() \
	oskit_osenv_intr_save_disable(fs_netbsd_oskit_osenv_intr)

/*
 * Log and panic.
 */
extern oskit_osenv_log_t	*fs_netbsd_oskit_osenv_log;

#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(fs_netbsd_oskit_osenv_log, (pri), ##fmt)
#define osenv_vlog(pri, fmt, args) \
	oskit_osenv_log_vlog(fs_netbsd_oskit_osenv_log, (pri), (fmt), (args))
#define osenv_panic(fmt...) \
	oskit_osenv_log_panic(fs_netbsd_oskit_osenv_log, ##fmt)
#define osenv_vpanic(fmt, args) \
	oskit_osenv_log_vpanic(fs_netbsd_oskit_osenv_log, (fmt), (args))

#undef  osenv_assert
#define osenv_assert(x) \
	oskit_osenv_assert(fs_netbsd_oskit_osenv_log, x)
	
/*
 * Memory stuff.
 */
extern oskit_osenv_mem_t	*fs_netbsd_oskit_osenv_mem;

#define osenv_mem_alloc(size, flags, align) \
	oskit_osenv_mem_alloc(fs_netbsd_oskit_osenv_mem, \
				(size), (flags), (align))
#define osenv_mem_free(block, flags, size) \
	oskit_osenv_mem_free(fs_netbsd_oskit_osenv_mem, \
				(block), (flags), (size))
#define osenv_mem_get_phys(va) \
	oskit_osenv_mem_getphys(fs_netbsd_oskit_osenv_mem, (va))
#define osenv_mem_get_virt(pa) \
	oskit_osenv_mem_getvirt(fs_netbsd_oskit_osenv_mem, (pa))
#define osenv_mem_phys_max() \
	oskit_osenv_mem_physmax(fs_netbsd_oskit_osenv_mem)
#define osenv_mem_map_phys(pa, size, addr, flags) \
	oskit_osenv_mem_mapphys(fs_netbsd_oskit_osenv_mem, \
				(pa), (size), (addr), (flags))

/*
 * Timer control.
 */
extern oskit_osenv_timer_t		*fs_netbsd_oskit_osenv_timer;

#define osenv_timer_init() \
	oskit_osenv_timer_init(fs_netbsd_oskit_osenv_timer)
#define osenv_timer_shutdown() \
	oskit_osenv_timer_shutdown(fs_netbsd_oskit_osenv_timer)
#define osenv_timer_spin(nanosec) \
	oskit_osenv_timer_spin(fs_netbsd_oskit_osenv_timer, (nanosec))
#define osenv_timer_register(func, freq) \
	oskit_osenv_timer_register(fs_netbsd_oskit_osenv_timer, (func), (freq))
#define osenv_timer_unregister(func, freq) \
	oskit_osenv_timer_unregister(fs_netbsd_oskit_osenv_timer, \
				(func), (freq))

/*
 * RTC control.
 */
extern oskit_osenv_rtc_t		*fs_netbsd_oskit_osenv_rtc;

#define osenv_rtc_set(time) \
	oskit_osenv_rtc_set(fs_netbsd_oskit_osenv_rtc, (time))
#define osenv_rtc_get(time) \
	oskit_osenv_rtc_get(fs_netbsd_oskit_osenv_rtc, (time))

#endif /* !INDIRECT_OSENV */

#endif
