/*
 * linux/include/asm-arm/arch-ebsa285/memory.h
 *
 * Copyright (c) 1996-1999 Russell King.
 *
 * Changelog:
 *  20-Oct-1996	RMK	Created
 *  31-Dec-1997	RMK	Fixed definitions to reduce warnings.
 *  17-May-1998	DAG	Added __virt_to_bus and __bus_to_virt functions.
 *  21-Nov-1998	RMK	Changed __virt_to_bus and __bus_to_virt to macros.
 *  21-Mar-1999	RMK	Added PAGE_OFFSET for co285 architecture.
 *			Renamed to memory.h
 *			Moved PAGE_OFFSET and TASK_SIZE here
 */
#ifndef __ASM_ARCH_MMU_H
#define __ASM_ARCH_MMU_H

#ifdef OSKIT
#ifndef __ASSEMBLY__
extern unsigned long virt_to_phys(volatile void *address);
extern void *phys_to_virt(unsigned long address);
#define __virt_to_bus(x)	(virt_to_phys((x)))
#define __bus_to_virt(x)	((void *)(phys_to_virt((unsigned long)(x))))
#endif
#endif

#endif
