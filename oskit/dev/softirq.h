/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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
 * Software Interrupts
 */
#ifndef _OSKIT_DEV_SOFTIRQ_H_
#define _OSKIT_DEV_SOFTIRQ_H_

#include <oskit/compiler.h>
#include <oskit/error.h>

/*
 * Maximum number of software interrupt vectors. The implementation
 * currently depends on this value fitting in a 32 bit word. 
 */
#define SOFT_IRQ_COUNT			32

/*
 * Split them up between some well-known vectors and the rest, which can
 * be allocated as needed.
 */
#define SOFT_IRQ_FREEBASE		16

/*
 * And some exported vector names.
 */
#define OSENV_SOFT_IRQ_CLOCK		0
#define OSENV_SOFT_IRQ_NET		1
#define OSENV_SOFT_IRQ_DISK		3
#define OSENV_SOFT_IRQ_CONSOLE		2
#define OSENV_SOFT_IRQ_SOFTWARE		30
#define OSENV_SOFT_IRQ_PTHREAD		31

OSKIT_BEGIN_DECLS

/*
 * Allocate a software interrupt request line and attach the specified
 * handler to it.  On interrupt, the `data' value will be passed to the
 * handler. Currently the flags value is ignored. If the irq is not one
 * of the well-known vectors below SOFT_IRQ_FREEBASE above, then the
 * vevtor must have been explicitly allocated with the alloc_vector()
 * call beforehand. Returns OSKIT_EINVAL if the vector is outside the
 * valid range, or if the previous statement is violated. 
 */
oskit_error_t
osenv_softirq_alloc(int irq, void (*handler)(void *), void *data, int flags);

/*
 * Removes the indicated interrupt handler. The handler is only removed if
 * it was registered with osenv_softirq_alloc() for the indicated request
 * line, and with the indicated `data' pointer.
 */
void
osenv_softirq_free(int irq, void (*handler)(void *), void *data);

/*
 * Allocate a software interrupt vector for use with osenv_softirq_alloc()
 * above, and returns the assigned vector in the location pointed to by
 * the integer pointer `out_irq.' Returns OSKIT_ERANGE if there are no more
 * free vectors (an unlikely event).
 */
oskit_error_t
osenv_softirq_alloc_vector(int *out_irq);

/*
 * Free a vector previously allocated with osenv_softirq_alloc_vector().
 * Prior to this call, all of the handlers must have been removed with
 * osenv_softirq_free(). Returns OSKIT_EINVAL if an attempt is made to
 * free a vector before all of the handlers have been removed.
 */
oskit_error_t
osenv_softirq_free_vector(int irq);

/*
 * Schedule a software interrupt to be delivered at the next opportunity.
 * Typically this will be called inside of a hardware interrupt handler
 * to schedule an activity that should be performed at the end of the IRQ,
 * before returning from the interrupt.
 */
void
osenv_softirq_schedule(int irq);

/*
 * Enable, disable, and test the enable status of the software interrupt
 * request mask. Like hardware interrupts, software interrupts can be
 * temporarily masked and unmasked.
 */
void	osenv_softintr_enable(void);
void	osenv_softintr_disable(void);
int	osenv_softintr_enabled(void);

OSKIT_END_DECLS

#endif /* _OSKIT_DEV_SOFTIRQ_H_ */
