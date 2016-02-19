/*
 * Copyright (c) 1996-2001 The University of Utah and the Flux Group.
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
 * Initalization routines.
 */

/* Note: oskit PAGE_MASK is bit-inverse of Linux definition */
#include <asm/page.h>	/* linux PAGE_MASK def */
#include <oskit/dev/isa.h>
#include <oskit/dev/native.h>

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/wait.h>

#include <asm/system.h>

#define trunc_page(x) ((x) & PAGE_MASK)
#define round_page(x) (((x)+ ~PAGE_MASK) & PAGE_MASK)

#include "init.h"
#include "sched.h"
#include "shared.h"
#include "osenv.h"

#define PHYS_BIOS_START 0xe0000
unsigned BIOS_START;		/* VA for BIOS */
#define BIOS_SIZE 0x20000

#if 0
#define DEBUG_MSG() osenv_log(OSENV_LOG_DEBUG,"%s:%d:\n", __FILE__, __LINE__);
#else
#define DEBUG_MSG()
#endif

/*
 * Set if the machine has an EISA bus. Note, this can be a macro that
 * hardwires EISA_bus on or off.
 */
#ifndef OSKIT_ARM32_SHARK
int EISA_bus = 0;
#endif

/*
 * Hard drive parameters obtained from the BIOS.
 */
struct drive_info_struct {
	char dummy[32];
} drive_info;


/*
 * Linux initialization.
 */
oskit_error_t
oskit_linux_dev_init(void)
{
	static int inited = 0;
	char *p;
	oskit_addr_t addr, x;
	void *kaddr;
	struct task_struct ts;
	
	if (inited)
		return 0;
	inited = 1;

	oskit_linux_init();
	linux_softintr_init();
	linux_intr_init();

	OSKIT_LINUX_CREATE_CURRENT(ts);

	/*
	 * Register the Linux clock interrupt handler.
	 */
	osenv_timer_register(linux_timer_intr, HZ);
	current = &ts;	/* restore current pointer after possibly blocking */

#ifndef OSKIT_ARM32_SHARK
	/*
	 * Check if the machine has an EISA bus.
	 */
	/* Map the 128k BIOS region.  kvtophys() should equate
	 * BIOS_START & PHYS_BIOS_START, but just in case, we
	 * track them both.
	 */
	if (osenv_mem_map_phys(PHYS_BIOS_START, BIOS_SIZE,
			(void**)&BIOS_START, 0))
		panic("linux_init: unable to map physical memory");
	osenv_log(OSENV_LOG_DEBUG, "BIOS mapped to 0x%x\n", BIOS_START);
	kaddr = (void *) (BIOS_START + trunc_page(0xFFFD9) - PHYS_BIOS_START);
	current = &ts;	/* restore current pointer after possibly blocking */
	p = kaddr + (0xFFFD9 & (~PAGE_MASK));
	if (*p++ == 'E' && *p++ == 'I' && *p++ == 'S' && *p == 'A')
		EISA_bus = 1;
#endif

	/*
	 * Install software interrupt handlers.
	 */
	init_bh(TIMER_BH, timer_bh);
	init_bh(TQUEUE_BH, tqueue_bh);
	init_bh(IMMEDIATE_BH, immediate_bh);

#ifndef OSKIT_ARM32_SHARK
	/*
	 * Initialize drive info.
	 */
	if (osenv_mem_map_phys(0, PAGE_SIZE, &kaddr, 0))
		panic("%s:%d: unable to map phys memory", __FILE__, __LINE__);
	x = *((unsigned *)(kaddr + 0x104));
	addr = ((x >> 12) & 0xFFFF0) + (x & 0xFFFF);
	if (osenv_mem_map_phys(trunc_page(addr), PAGE_SIZE, (void **)&x, 0))
		panic("%s:%d: unable to map phys memory", __FILE__, __LINE__);
	memcpy(&drive_info, (void *)(x + (addr & (~PAGE_MASK))), 16);
	x = *((unsigned *)(kaddr + 0x118));
	addr = ((x >> 12) & 0xFFFF0) + (x & 0xFFFF);
	if (osenv_mem_map_phys(trunc_page(addr), PAGE_SIZE, (void **)&x, 0))
		panic("%s:%d: unable to map phys memory", __FILE__, __LINE__);
	memcpy((char *)&drive_info + 16,
	       (void *)(x + (addr & (~PAGE_MASK))), 16);
	current = &ts;	/* restore current pointer after possibly blocking */
#endif
	DEBUG_MSG();

	/*
	 * Allocate contiguous memory.
	 */

#ifdef OSKIT_ARM32_SHARK
	/*
	 * Initialize ISA DMA.
	 */
	init_dma();
#else
#ifdef CONFIG_PCI
	/*
	 * Initialize PCI bus.
	 */
	pci_init();

	DEBUG_MSG();

#endif	/* CONFIG_PCI */
#endif
	DEBUG_MSG();

#ifdef KNIT
        /* 
         * There really isn't anything to do - all that device_setup is
         * going to do is fiddle around and, eventually, reenable 
         * interrupts.  We can cut out the middle-man and avoid
         * hideous dependencies.
         */
#else
	/*
	 * Initialize devices.
	 */
	osenv_intr_disable();

	/* XXX shouldn't need the full genhd.c at all */
	device_setup();
#endif

	OSKIT_LINUX_DESTROY_CURRENT();

	return 0;
}
