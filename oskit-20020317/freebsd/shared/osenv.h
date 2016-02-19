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
#ifndef _FREEBSD_SHARED_OSENV_H_
#define _FREEBSD_SHARED_OSENV_H_

#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_isa.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_driver.h>
#include <oskit/dev/osenv_device.h>
#include <oskit/dev/osenv_ioport.h>
#include <oskit/dev/osenv_timer.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/osenv_rtc.h>

#ifdef KNIT

#define osenv_rtc_set(time) oskit_rtc_set(time) 
#define osenv_rtc_get(time) oskit_rtc_set(time) 

#else /* !KNIT */

void	freebsd_oskit_osenv_init(oskit_osenv_t *osenv);

/*
 * Sleep/wakeup
 */
extern oskit_osenv_sleep_t	*freebsd_oskit_osenv_sleep;

#define osenv_sleep_init(sr) \
	oskit_osenv_sleep_init(freebsd_oskit_osenv_sleep, (sr))
#define osenv_sleep(sr) \
	oskit_osenv_sleep_sleep(freebsd_oskit_osenv_sleep, (sr))
#define osenv_wakeup(sr, status) \
	oskit_osenv_sleep_wakeup(freebsd_oskit_osenv_sleep, (sr), (status))

/*
 * Interrupt control.
 */
extern oskit_osenv_intr_t	*freebsd_oskit_osenv_intr;

#define osenv_intr_enable() \
	oskit_osenv_intr_enable(freebsd_oskit_osenv_intr)
#define osenv_intr_disable() \
	oskit_osenv_intr_disable(freebsd_oskit_osenv_intr)
#define osenv_intr_enabled() \
	oskit_osenv_intr_enabled(freebsd_oskit_osenv_intr)
#define osenv_intr_save_disable() \
	oskit_osenv_intr_save_disable(freebsd_oskit_osenv_intr)

/*
 * IRQ control.
 */
extern oskit_osenv_irq_t	*freebsd_oskit_osenv_irq;

#define osenv_irq_alloc(irq, handler, data, flags) \
	oskit_osenv_irq_alloc(freebsd_oskit_osenv_irq,\
			(irq), (handler), (data), (flags))
#define osenv_irq_free(irq, handler, data) \
	oskit_osenv_irq_free(freebsd_oskit_osenv_irq, (irq), (handler), (data))
#define osenv_irq_disable(irq) \
	oskit_osenv_irq_disable(freebsd_oskit_osenv_irq, (irq))
#define osenv_irq_enable(irq) \
	oskit_osenv_irq_enable(freebsd_oskit_osenv_irq, (irq))
#define osenv_irq_pending(irq) \
	oskit_osenv_irq_pending(freebsd_oskit_osenv_irq, (irq))

/*
 * ISA Goop
 */
extern oskit_osenv_isa_t	*freebsd_oskit_osenv_isa;

#define osenv_isabus_init() \
	oskit_osenv_isa_bus_init(freebsd_oskit_osenv_isa)
#define osenv_isabus_getbus() \
	oskit_osenv_isa_bus_getbus(freebsd_oskit_osenv_isa)
#define osenv_isabus_addchild(addr, dev) \
	oskit_osenv_isa_bus_addchild(freebsd_oskit_osenv_isa, (addr), (dev))
#define osenv_isabus_remchild(addr) \
	oskit_osenv_isa_bus_remchild(freebsd_oskit_osenv_isa, (addr))
#define osenv_isadma_alloc(channel) \
	oskit_osenv_isa_dma_alloc(freebsd_oskit_osenv_isa, (channel))
#define osenv_isadma_free(channel) \
	oskit_osenv_isa_dma_free(freebsd_oskit_osenv_isa, (channel))

/*
 * Log and panic.
 */
extern oskit_osenv_log_t	*freebsd_oskit_osenv_log;

#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(freebsd_oskit_osenv_log, (pri), ##fmt)
#define osenv_vlog(pri, fmt, args) \
	oskit_osenv_log_vlog(freebsd_oskit_osenv_log, (pri), (fmt), (args))
#define osenv_panic(fmt...) \
	oskit_osenv_log_panic(freebsd_oskit_osenv_log, ##fmt)
#define osenv_vpanic(fmt, args) \
	oskit_osenv_log_vpanic(freebsd_oskit_osenv_log, (fmt), (args))

#undef  osenv_assert
#define osenv_assert(x) \
	oskit_osenv_assert(freebsd_oskit_osenv_log, x)
	
/*
 * Memory stuff.
 */
extern oskit_osenv_mem_t	*freebsd_oskit_osenv_mem;

#define osenv_mem_alloc(size, flags, align) \
	oskit_osenv_mem_alloc(freebsd_oskit_osenv_mem, \
				(size), (flags), (align))
#define osenv_mem_free(block, flags, size) \
	oskit_osenv_mem_free(freebsd_oskit_osenv_mem, \
				(block), (flags), (size))
#define osenv_mem_get_phys(va) \
	oskit_osenv_mem_getphys(freebsd_oskit_osenv_mem, (va))
#define osenv_mem_get_virt(pa) \
	oskit_osenv_mem_getvirt(freebsd_oskit_osenv_mem, (pa))
#define osenv_mem_phys_max() \
	oskit_osenv_mem_physmax(freebsd_oskit_osenv_mem)
#define osenv_mem_map_phys(pa, size, addr, flags) \
	oskit_osenv_mem_mapphys(freebsd_oskit_osenv_mem, \
				(pa), (size), (addr), (flags))

/*
 * Driver registration.
 */
extern oskit_osenv_driver_t		*freebsd_oskit_osenv_driver;

#define osenv_driver_register(driver, iids, iid_count) \
	oskit_osenv_driver_register(freebsd_oskit_osenv_driver, \
				    (driver), (iids), (iid_count))
#define osenv_driver_unregister(driver, iids, iid_count) \
	oskit_osenv_driver_unregister(freebsd_oskit_osenv_driver, \
				    (driver), (iids), (iid_count))
#define osenv_driver_lookup(iid, out_interface_array) \
	oskit_osenv_driver_lookup(freebsd_oskit_osenv_driver, \
				    (iid), (out_interface_array))

/*
 * Device registration.
 */
extern oskit_osenv_device_t		*freebsd_oskit_osenv_device;

#define osenv_device_register(device, iids, iid_count) \
	oskit_osenv_device_register(freebsd_oskit_osenv_device, \
				    (device), (iids), (iid_count))
#define osenv_device_unregister(device, iids, iid_count) \
	oskit_osenv_device_unregister(freebsd_oskit_osenv_device, \
				    (device), (iids), (iid_count))
#define osenv_device_lookup(iid, out_interface_array) \
	oskit_osenv_device_lookup(freebsd_oskit_osenv_device, \
				    (iid), (out_interface_array))

/*
 * I/O Port goo.
 */
extern oskit_osenv_ioport_t		*freebsd_oskit_osenv_ioport;

#define osenv_io_avail(port, size) \
	oskit_osenv_ioport_avail(freebsd_oskit_osenv_ioport, (port), (size))
#define osenv_io_alloc(port, size) \
	oskit_osenv_ioport_alloc(freebsd_oskit_osenv_ioport, (port), (size))
#define osenv_io_free(port, size) \
	oskit_osenv_ioport_free(freebsd_oskit_osenv_ioport, (port), (size))

/*
 * Timer control.
 */
extern oskit_osenv_timer_t		*freebsd_oskit_osenv_timer;

#define osenv_timer_init() \
	oskit_osenv_timer_init(freebsd_oskit_osenv_timer)
#define osenv_timer_shutdown() \
	oskit_osenv_timer_shutdown(freebsd_oskit_osenv_timer)
#define osenv_timer_spin(nanosec) \
	oskit_osenv_timer_spin(freebsd_oskit_osenv_timer, (nanosec))
#define osenv_timer_register(func, freq) \
	oskit_osenv_timer_register(freebsd_oskit_osenv_timer, (func), (freq))
#define osenv_timer_unregister(func, freq) \
	oskit_osenv_timer_unregister(freebsd_oskit_osenv_timer, (func), (freq))


/*
 * RTC control.
 */
extern oskit_osenv_rtc_t		*freebsd_oskit_osenv_rtc;

#define osenv_rtc_set(time) \
	oskit_osenv_rtc_set(freebsd_oskit_osenv_rtc, (time))
#define osenv_rtc_get(time) \
	oskit_osenv_rtc_get(freebsd_oskit_osenv_rtc, (time))

#endif /* !KNIT */

#endif
