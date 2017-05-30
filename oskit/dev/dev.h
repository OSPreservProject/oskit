/*
 * Copyright (c) 1996-2001 University of Utah and the Flux Group.
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
 * OSKit device driver interface definitions.
 */
#ifndef _OSKIT_DEV_DEV_H_
#define _OSKIT_DEV_DEV_H_

#include <oskit/machine/types.h>
#include <oskit/dev/error.h>
#include <oskit/machine/dev.h>
#include <oskit/compiler.h>
#include <oskit/dev/osenv.h>
#include <oskit/io/blkio.h>

OSKIT_BEGIN_DECLS

#ifndef NULL
#define NULL 0
#endif

struct oskit_device;

/*
 * Main initialization.
 */
void oskit_dev_init(oskit_osenv_t *osenv);
void oskit_dev_probe(void);

/*
 * Memory allocation/deallocation.
 */
typedef unsigned osenv_memflags_t;

void *osenv_mem_alloc(oskit_size_t size, osenv_memflags_t flags, unsigned align);
void osenv_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size);
oskit_addr_t osenv_mem_get_phys(oskit_addr_t va);
oskit_addr_t osenv_mem_get_virt(oskit_addr_t pa);
oskit_addr_t osenv_mem_phys_max(void);

#define OSENV_AUTO_SIZE		0x01
#define OSENV_PHYS_WIRED	0x02
#define OSENV_VIRT_EQ_PHYS	0x04
#define OSENV_PHYS_CONTIG	0x08
#define OSENV_NONBLOCKING	0x10
#define OSENV_ISADMA_MEM	0x20
#define OSENV_X861MB_MEM	0x40

/*
 * Physical memory mapping.
 */
#define PHYS_MEM_WRITETHROUGH	0x01	/* No writeback caching allowed */
#define PHYS_MEM_NOCACHE	0x02	/* No caching at all allowed */

int osenv_mem_map_phys(oskit_addr_t pa, oskit_size_t size, void **addr, int flags);


/*** Function entrypoints for the root "pseudo-bus" driver */

/*
 * This function returns a reference to the root "bus" device node,
 * which is the standard hang-out for device nodes that have no obvious parent,
 * such as the root PCI bus in a PCI machine.
 * This device node represents the root of the hardware tree;
 * every other device node in the system should be reachable from it.
 * (Be sure and release() the reference when you are done with it.)
 */
struct oskit_bus *osenv_rootbus_getbus(void);

/*
 * Add a device node to the root pseudo-bus.
 */
oskit_error_t osenv_rootbus_addchild(struct oskit_device *dev);

/*
 * Remove a device node from the root pseudo-bus,
 * e.g., because it finally figured out where it was supposed to go.
 */
oskit_error_t osenv_rootbus_remchild(struct oskit_device *dev);


/*
 * Interrupts.
 */
#define OSENV_IRQ_SHAREABLE 0x10000
#define OSENV_IRQ_REALTIME  0x20000
#define OSENV_IRQ_OVERRIDE  0x40000
#define OSENV_IRQ_TRAPSTATE 0x80000

int osenv_irq_alloc(int irq, void (*handler)(void *), void *data, int flags);
void osenv_irq_free(int irq, void (*handler)(void *), void *data);
void osenv_irq_disable(int irq);
void osenv_irq_enable(int irq);
int osenv_irq_enabled(int irq);
int osenv_irq_pending(int irq);
/* realtime support */
void osenv_irq_schedule(int irq);

void osenv_intr_disable(void);
void osenv_intr_enable(void);
int osenv_intr_enabled(void);
int osenv_intr_save_disable(void);

/*
 * The following functions have been deprecated by the
 * oskit_clock COM object.
 */
void osenv_timer_init(void);
void osenv_timer_shutdown(void);
void osenv_timer_spin(long nanosec);
long osenv_timer_spin_resolution(void);
void osenv_timer_register(void (*func)(void), int freq);
void osenv_timer_unregister(void (*func)(void), int freq);

/*
 * Declarations for the osenv_timer_pit layer upon which the osenv_timer
 * functions are implemented.
 * Alternatively, if there are no users of osenv_timer* functions in the
 * system, the oskit_clock COM object may use these directly.
 */
int osenv_timer_pit_init(int freq, void (*timer_intr)(void));
void osenv_timer_pit_shutdown(void);
int osenv_timer_pit_read(void);

/*
 * Flags to *_block_open routines.
 */
#define OSKIT_DEV_OPEN_READ	0x01	/* open device for reading */
#define OSKIT_DEV_OPEN_WRITE	0x02	/* open device for writing */
#define OSKIT_DEV_OPEN_ALL	(OSKIT_DEV_OPEN_WRITE|OSKIT_DEV_OPEN_READ)


/*
 * Sleep/wakeup support.
 */
struct osenv_sleeprec {
	void 	*data[2];
};
typedef struct osenv_sleeprec osenv_sleeprec_t;

/*
 * Initialize a sleeprec such that if osenv_wakeup() is called on it
 * after osenv_sleep_init() but before osenv_sleep(),
 * then osenv_sleep() will return immediately and not "lose" the signal.
 */
void osenv_sleep_init(osenv_sleeprec_t *sr);

/*
 * Sleep until osenv_wakeup() is called on this sleeprec,
 * which must have been previously initialized by osenv_sleep_init().
 */
int osenv_sleep(osenv_sleeprec_t *sr);

/*
 * Wake up a waiting process.
 */
void osenv_wakeup(osenv_sleeprec_t *sr, int wakeup_status);

/*
 * Wakeup status values. Very limited set for now.
 */
#define OSENV_SLEEP_WAKEUP	0	/* Wakeup normally */
#define	OSENV_SLEEP_CANCELED	1	/* Interrupted wakeup */

/*
 * Attempt to lock the process lock. If the lock cannot be immediately
 * granted, the thread is put to sleep until it can be. The process
 * lock is provided so that the client operating system can protect
 * the device driver framework from concurrent execution. It is
 * expected than any entry into the device framework will first take
 * the process lock. If the thread executing inside the device driver
 * framework blocks by calling {\tt osenv_sleep}, the process lock
 * will be released so that another thread may enter it safely. When
 * the thread is woken up later, it will take the process lock again
 * before returning from the sleep.
 * 
 * Attempts to recursively lock the process lock will result in a
 * panic. This is intended as a debugging measure to prevent
 * indiscriminate nesting of components that try to take the lock.
 */
void osenv_process_lock(void);

/*
 * Release the process lock. If another thread is waiting to lock the
 * process lock, it will be woken up. The process lock is provided so
 * that the client operating system can protect the device driver
 * framework from concurrent execution.
 */
void osenv_process_unlock(void);

/*
 * Test if the current thread has the process lock. Kludge for usermode.
 */
int
osenv_process_locked(void);

/*
 * Apply a process_lock wrapper to a blkio device.
 */
oskit_blkio_t*
osenv_wrap_blkio(oskit_blkio_t *bio);


/* Dump the driver database and device hardware tree */
void oskit_dump_drivers(void);
void oskit_dump_devices(void);


/*
 * Display or otherwise record a message from this driver set.
 * The log priorities below indicate its general nature.
 */
void osenv_log(int priority, const char *fmt, ...);
void osenv_vlog(int priority, const char *fmt, void *vl);

/*
 * Here we define the different log priorities.
 * Lower number is a higher priority.
 * These numbers are the same as those used in BSD and Linux -
 * for once they agree in something!?!
 */
#define OSENV_LOG_EMERG		0
#define OSENV_LOG_ALERT		1
#define OSENV_LOG_CRIT		2
#define OSENV_LOG_ERR		3
#define OSENV_LOG_WARNING	4
#define OSENV_LOG_NOTICE	5
#define OSENV_LOG_INFO		6
#define OSENV_LOG_DEBUG		7

/*
 * This function is called if there's an error in the driver set
 * that's so critical we just have to bail immediately.
 * Note that this may not necessarily terminate the whole OS;
 * it might just terminate the driver set it's called from.
 */
void osenv_panic(const char *fmt, ...) OSKIT_NORETURN;
void osenv_vpanic(const char *fmt, void *vl) OSKIT_NORETURN;

#if (defined DEBUG) || (defined OSKIT_DEBUG) || (defined FDEV_DEBUG)
#define osenv_assert(x)  { if (!(x))				\
	osenv_panic("assertion `"#x"' failed in file "__FILE__	\
		", line %d\n", __LINE__ ); }
#else
#define osenv_assert(x)
#endif

int osenv_pci_config_read(char bus, char device, char function, char port,
	unsigned *data);
int osenv_pci_config_write(char bus, char device, char function, char port,
	unsigned data);
/* returns 0 on success, non-zero on failure */
int osenv_pci_config_init(void);

OSKIT_END_DECLS

#endif /* _OSKIT_DEV_DEV_H_ */
