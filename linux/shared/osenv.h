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
#ifndef _LINUX_SHARED_OSENV_H_
#define _LINUX_SHARED_OSENV_H_

#include <oskit/dev/osenv_intr.h>
#include <oskit/dev/osenv_irq.h>
#include <oskit/dev/osenv_pci_config.h>
#include <oskit/dev/osenv_isa.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_driver.h>
#include <oskit/dev/osenv_device.h>
#include <oskit/dev/osenv_ioport.h>
#include <oskit/dev/osenv_timer.h>
#include <oskit/dev/osenv_sleep.h>
#include <oskit/dev/native.h>

#ifdef INDIRECT_OSENV

/*
 * Sleep/wakeup
 */
extern oskit_osenv_sleep_t	*linux_oskit_osenv_sleep;

#define osenv_sleep_init(sr) \
	oskit_osenv_sleep_init(linux_oskit_osenv_sleep, (sr))
#define osenv_sleep(sr) \
	oskit_osenv_sleep_sleep(linux_oskit_osenv_sleep, (sr))
#define osenv_wakeup(sr, status) \
	oskit_osenv_sleep_wakeup(linux_oskit_osenv_sleep, (sr), (status))

/*
 * Interrupt control.
 */
extern oskit_osenv_intr_t	*linux_oskit_osenv_intr;

#define osenv_intr_enable() \
	oskit_osenv_intr_enable(linux_oskit_osenv_intr)
#define osenv_intr_disable() \
	oskit_osenv_intr_disable(linux_oskit_osenv_intr)
#define osenv_intr_enabled() \
	oskit_osenv_intr_enabled(linux_oskit_osenv_intr)
#define osenv_intr_save_disable() \
	oskit_osenv_intr_save_disable(linux_oskit_osenv_intr)

/*
 * IRQ control.
 */
extern oskit_osenv_irq_t	*linux_oskit_osenv_irq;

#define osenv_irq_alloc(irq, handler, data, flags) \
	oskit_osenv_irq_alloc(linux_oskit_osenv_irq,\
			(irq), (handler), (data), (flags))
#define osenv_irq_free(irq, handler, data) \
	oskit_osenv_irq_free(linux_oskit_osenv_irq, (irq), (handler), (data))
#define osenv_irq_disable(irq) \
	oskit_osenv_irq_disable(linux_oskit_osenv_irq, (irq))
#define osenv_irq_enable(irq) \
	oskit_osenv_irq_enable(linux_oskit_osenv_irq, (irq))
#define osenv_irq_pending(irq) \
	oskit_osenv_irq_pending(linux_oskit_osenv_irq, (irq))

/*
 * PCI Configuration.
 */
extern oskit_osenv_pci_config_t	*linux_oskit_osenv_pci_config;

#define osenv_pci_config_init() \
	oskit_osenv_pci_config_init(linux_oskit_osenv_pci_config)
#define osenv_pci_config_read(bus, device, function, port, data) \
	oskit_osenv_pci_config_read(linux_oskit_osenv_pci_config, \
		(bus), (device), (function), (port), (data))
#define osenv_pci_config_write(bus, device, function, port, data) \
	oskit_osenv_pci_config_write(linux_oskit_osenv_pci_config, \
		(bus), (device), (function), (port), (data))

/*
 * ISA Goop
 */
extern oskit_osenv_isa_t	*linux_oskit_osenv_isa;

#define osenv_isabus_init() \
	oskit_osenv_isa_bus_init(linux_oskit_osenv_isa)
#define osenv_isabus_getbus() \
	oskit_osenv_isa_bus_getbus(linux_oskit_osenv_isa)
#define osenv_isabus_addchild(addr, dev) \
	oskit_osenv_isa_bus_addchild(linux_oskit_osenv_isa, (addr), (dev))
#define osenv_isabus_remchild(addr) \
	oskit_osenv_isa_bus_remchild(linux_oskit_osenv_isa, (addr))
#define osenv_isadma_alloc(channel) \
	oskit_osenv_isa_dma_alloc(linux_oskit_osenv_isa, (channel))
#define osenv_isadma_free(channel) \
	oskit_osenv_isa_dma_free(linux_oskit_osenv_isa, (channel))

/*
 * Log and panic.
 */
extern oskit_osenv_log_t	*linux_oskit_osenv_log;

#define osenv_log(pri, fmt...) \
	oskit_osenv_log_log(linux_oskit_osenv_log, (pri), ##fmt)
#define osenv_vlog(pri, fmt, args) \
	oskit_osenv_log_vlog(linux_oskit_osenv_log, (pri), (fmt), (args))
#define osenv_panic(fmt...) \
	oskit_osenv_log_panic(linux_oskit_osenv_log, ##fmt)
#define osenv_vpanic(fmt, args) \
	oskit_osenv_log_vpanic(linux_oskit_osenv_log, (fmt), (args))

#undef  osenv_assert
#define osenv_assert(x) \
	oskit_osenv_assert(linux_oskit_osenv_log, x)
	
/*
 * Memory stuff.
 */
extern oskit_osenv_mem_t	*linux_oskit_osenv_mem;

#define osenv_mem_alloc(size, flags, align) \
	oskit_osenv_mem_alloc(linux_oskit_osenv_mem, (size), (flags), (align))
#define osenv_mem_free(block, flags, size) \
	oskit_osenv_mem_free(linux_oskit_osenv_mem, (block), (flags), (size))
#define osenv_mem_get_phys(va) \
	oskit_osenv_mem_getphys(linux_oskit_osenv_mem, (va))
#define osenv_mem_get_virt(pa) \
	oskit_osenv_mem_getvirt(linux_oskit_osenv_mem, (pa))
#define osenv_mem_phys_max() \
	oskit_osenv_mem_physmax(linux_oskit_osenv_mem)
#define osenv_mem_map_phys(pa, size, addr, flags) \
	oskit_osenv_mem_mapphys(linux_oskit_osenv_mem, \
				(pa), (size), (addr), (flags))

/*
 * Driver registration.
 */
extern oskit_osenv_driver_t		*linux_oskit_osenv_driver;

#define osenv_driver_register(driver, iids, iid_count) \
	oskit_osenv_driver_register(linux_oskit_osenv_driver, \
				    (driver), (iids), (iid_count))
#define osenv_driver_unregister(driver, iids, iid_count) \
	oskit_osenv_driver_unregister(linux_oskit_osenv_driver, \
				    (driver), (iids), (iid_count))
#define osenv_driver_lookup(iid, out_interface_array) \
	oskit_osenv_driver_lookup(linux_oskit_osenv_driver, \
				    (iid), (out_interface_array))

/*
 * Device registration.
 */
extern oskit_osenv_device_t		*linux_oskit_osenv_device;

#define osenv_device_register(device, iids, iid_count) \
	oskit_osenv_device_register(linux_oskit_osenv_device, \
				    (device), (iids), (iid_count))
#define osenv_device_unregister(device, iids, iid_count) \
	oskit_osenv_device_unregister(linux_oskit_osenv_device, \
				    (device), (iids), (iid_count))
#define osenv_device_lookup(iid, out_interface_array) \
	oskit_osenv_device_lookup(linux_oskit_osenv_device, \
				    (iid), (out_interface_array))

/*
 * I/O Port goo.
 */
extern oskit_osenv_ioport_t		*linux_oskit_osenv_ioport;

#define osenv_io_avail(port, size) \
	oskit_osenv_ioport_avail(linux_oskit_osenv_ioport, (port), (size))
#define osenv_io_alloc(port, size) \
	oskit_osenv_ioport_alloc(linux_oskit_osenv_ioport, (port), (size))
#define osenv_io_free(port, size) \
	oskit_osenv_ioport_free(linux_oskit_osenv_ioport, (port), (size))

/*
 * Timer control.
 */
extern oskit_osenv_timer_t		*linux_oskit_osenv_timer;

#define osenv_timer_init() \
	oskit_osenv_timer_init(linux_oskit_osenv_timer)
#define osenv_timer_shutdown() \
	oskit_osenv_timer_shutdown(linux_oskit_osenv_timer)
#define osenv_timer_spin(nanosec) \
	oskit_osenv_timer_spin(linux_oskit_osenv_timer, (nanosec))
#define osenv_timer_register(func, freq) \
	oskit_osenv_timer_register(linux_oskit_osenv_timer, (func), (freq))
#define osenv_timer_unregister(func, freq) \
	oskit_osenv_timer_unregister(linux_oskit_osenv_timer, (func), (freq))

#endif /* !KNIT */

#endif
