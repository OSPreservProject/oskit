/*
 * linux/include/asm-arm/arch-ebsa285/hardware.h
 *
 * Copyright (C) 1998-1999 Russell King.
 *
 * This file contains the hardware definitions of the EBSA-285.
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/config.h>
#include <asm/arch/memory.h>

#ifdef OSKIT
/*
 * Most of this stuff is not necessary.
 */
/*
 * This needs to be defined in a more appropriate place!
 */
#include <oskit/arm32/shark/isa.h>

#define PCIO_BASE		ISA_IOB_VIRTADDR
#define PCIMEM_BASE		-1		/* XXX */
#else

#error Non-OSKit shark stuff goes here

#endif

#define XBUS_LEDS		((volatile unsigned char *)(XBUS_BASE + 0x12000))
#define XBUS_LED_AMBER		(1 << 0)
#define XBUS_LED_GREEN		(1 << 1)
#define XBUS_LED_RED		(1 << 2)
#define XBUS_LED_TOGGLE		(1 << 8)

#define XBUS_SWITCH		((volatile unsigned char *)(XBUS_BASE + 0x12000))
#define XBUS_SWITCH_SWITCH	((*XBUS_SWITCH) & 15)
#define XBUS_SWITCH_J17_13	((*XBUS_SWITCH) & (1 << 4))
#define XBUS_SWITCH_J17_11	((*XBUS_SWITCH) & (1 << 5))
#define XBUS_SWITCH_J17_9	((*XBUS_SWITCH) & (1 << 6))

#define PARAMS_OFFSET		0x0100
#define PARAMS_BASE		(PAGE_OFFSET + PARAMS_OFFSET)

#define FLUSH_BASE_PHYS		0x50000000


/* PIC irq control */
#define PIC_LO			0x20
#define PIC_MASK_LO		0x21
#define PIC_HI			0xA0
#define PIC_MASK_HI		0xA1

/* GPIO pins */
#define GPIO_CCLK		0x800
#define GPIO_DSCLK		0x400
#define GPIO_E2CLK		0x200
#define GPIO_IOLOAD		0x100
#define GPIO_RED_LED		0x080
#define GPIO_WDTIMER		0x040
#define GPIO_DATA		0x020
#define GPIO_IOCLK		0x010
#define GPIO_DONE		0x008
#define GPIO_FAN		0x004
#define GPIO_GREEN_LED		0x002
#define GPIO_RESET		0x001

/* CPLD pins */
#define CPLD_DSRESET		8
#define CPLD_UNMUTE		2

#ifndef __ASSEMBLY__
extern void gpio_modify_op(int mask, int set);
extern void gpio_modify_io(int mask, int in);
extern int  gpio_read(void);
extern void cpld_modify(int mask, int set);
#endif

#endif
