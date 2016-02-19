/*
 * Copyright (c) 1995-2000 University of Utah and the Flux Group.
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
 * This header defines the OSKIT's base IRQ handling facilities.
 * The default environment provides a high-level, easy-to-use,
 * and not-too-horrendously-inefficient mechanism
 * for hooking into hardware interrupts and installing interrupt handlers.
 * It also provides a basic software interrupt facility.
 */
#ifndef _OSKIT_ARM32_SHARK_BASE_IRQ_H_
#define _OSKIT_ARM32_SHARK_BASE_IRQ_H_

struct trap_state;


/* On normal PCs, there are always 16 IRQ lines.  */
#define BASE_IRQ_COUNT		0x10

/* This is the default location in the IDT at which we program the PICs. */
#define BASE_IRQ_MASTER_BASE	0x20
#define BASE_IRQ_SLAVE_BASE	0x28


/*
 * In the base oskit environment, IRQs come in on a trap vector. When
 * irqs are initialized, that trap vector is redefined to point to an
 * assembly language stub which deals with the extra details of IRQs,
 * such as disabling and acknowledging the hardware interrupt,
 * and calling a designated high-level handler routine through this table.
 *
 * Initially, all the entries in this table point to
 * base_irq_default_handler(); to install a custom handler, simply
 * change the appropriate table entry.  Custom interrupt handlers can
 * freely examine and modify the processor state of the interrupted
 * activity, e.g., to implement threads and preemption. 
 */
extern void (*base_irq_handlers[])(struct trap_state *ts);


/*
 * Program the PICs to the standard vector base addresses above,
 * and initialize the corresponding entries in the base IDT.
 * Assumes the processor's interrupt flag is disabled throughout.
 */
void base_irq_init(void);


/*
 * Default IRQ handler for unexpected hardware interrupts
 * simply displays a warning message and returns.
 */
void base_irq_default_handler(struct trap_state *ts);

/*
 * This is the trap handler thats get plugged into base_trap_handlers.
 * Note: IRQs come in a trap vector, so we use the trap mechanism to
 * create the frame, and then transfer to the IRQ handler which has
 * been plugged into the trap handlers array.
 */
int base_irq_trap_handler(struct trap_state *ts);


/*** Software Interrupt Handling ***/

/*
 * Hardware interrupt nesting counter,
 * used to ensure that the software interrupt handler isn't called
 * until all outstanding hardware interrupts have been processed.
 *
 * In addition, this word also acts as the software interrupt pending
 * and enabled flag:
 * if the high bit is set, no software interrupt is pending;
 * if the high bit is clear, a software interrupt is pending;
 * if the next-highest bit is set, software interrupts are disabled;
 * if the next-highest bit is clear, software interrupts are enabled.
 * This design allows the interrupt path to decide
 * whether to call the software interrupt handler
 * simply by testing the word for zero (pending and enabled).
 *
 * XXX note that this is an int not a char as on the x86.
 * XXX should be reconciled.
 */
extern unsigned int base_irq_nest;

/*
 * Request a software interrupt (usually from a hardware interrupt handler).
 * The base_irq_softint_handler() function will be called later
 * after all hardware interrupt handlers have completed processing.
 */
#define BASE_IRQ_SOFTINT_CLEARED		0x80
#define BASE_IRQ_SOFTINT_DISABLED		0x40

#define base_irq_softint_request()	\
	base_irq_nest &= ~BASE_IRQ_SOFTINT_CLEARED

#define base_irq_softint_pending()	\
	(! (base_irq_nest & BASE_IRQ_SOFTINT_CLEARED))

#define base_irq_softint_clear()	\
	base_irq_nest |= BASE_IRQ_SOFTINT_CLEARED

#define base_irq_softint_enable()	\
	base_irq_nest &= ~BASE_IRQ_SOFTINT_DISABLED

#define base_irq_softint_enabled()	\
	(! (base_irq_nest & BASE_IRQ_SOFTINT_DISABLED))

#define base_irq_softint_disable()	\
	base_irq_nest |= BASE_IRQ_SOFTINT_DISABLED

/*
 * Software interrupt handler called by the entry/exit stub code
 * when a software interrupt has been requested and needs to be run.
 * The default implementation of this routine simply returns;
 * to use software interrupts, the kernel must override it.
 */
void base_irq_softint_default_handler(struct trap_state *ts);

/*
 * And the pointer to hold it so that it can be easily overridden!
 */
extern void (*base_irq_softint_handler)(struct trap_state *ts);

#endif /* _OSKIT_ARM32_SHARK_BASE_IRQ_H_ */
