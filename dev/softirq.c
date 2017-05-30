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
 * Software Interrupt management.
 */
#include <oskit/error.h>
#include <oskit/debug.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/softirq.h>
#include <oskit/machine/base_irq.h>
#include <stdio.h>

/* linked list of functions for a software interrupt */
struct softint_handler {
	void	(*func)(void *);
	void	*data;
	struct softint_handler *next;
	int	flags;
};

/* array of pointers to lists of interrupt handlers */
static struct softint_handler *softint_handlers[SOFT_IRQ_COUNT];

/* Mask of allocated vectors, for the benefit of osenv_softirq_request() */
static unsigned int		softint_allocated =
			((1 << OSENV_SOFT_IRQ_SOFTWARE) |
			 (1 << OSENV_SOFT_IRQ_PTHREAD));

/* Pending softirqs that have been software scheduled. */
static volatile unsigned int	softint_pending;

/*
 * This will override the stub function in the kernel library. It is the
 * entrypoint from the hardware interrupt handler, and is invoked when
 * it is safe to deliver a software interrupt, and one or more software
 * interrupts is pending.
 *
 * Called with hardware interrupts enabled and software interrupts disabled.
 * Upon return to the caller, hardware interrupts will be disabled again and
 * software interrupts will be renabled.
 *
 * The handler will be invoked with hardware interrupts enabled. Since
 * software interrupts are still disabled, if the handler results in a context
 * switch, software interrupts will need to be renabled. The pthreads
 * code does this in its software interrupt handler, but I think thats the
 * wrong place since some other operation could result in a context switch,
 * which would leave softints disabled. Perhaps there needs to be something
 * in the context switch code?
 *
 * A software interrupt handler should obviously not allocate or deallocate
 * a software interrupt. It may schedule one of course.
 */
static void
softint_handler(struct trap_state *ts)
{
	osenv_intr_disable();

	while (softint_pending) {
		int			i;
		struct softint_handler  *current;

		for (i = 0; i < SOFT_IRQ_COUNT; i++) {
			if (softint_pending & (1 << i)) {
				softint_pending &= ~(1 << i);
				
				current = softint_handlers[i];
				osenv_intr_enable();
				while (current) {
					current->func(current->data);
					current = current->next;
				}
				osenv_intr_disable();
			}
		}
	}

	assert(!base_irq_softint_enabled());
	osenv_intr_enable();
}

/*
 * Allocate a (well-known) software interrupt handler.
 */
oskit_error_t
osenv_softirq_alloc(int irq, void (*handler)(void *), void *data, int flags)
{
	struct softint_handler *temp, **p;

	if (irq < 0 || irq >= SOFT_IRQ_COUNT)
		return OSKIT_EINVAL;

	/*
	 * Do this to ensure that the free vectors are explicitly allocated
	 * before having a handler assigned.
	 */
	if ((irq >= SOFT_IRQ_FREEBASE) && !(softint_allocated & (1 << irq)))
		return OSKIT_EINVAL;

	/*
	 * This is a blocking operation,
	 * so to avoid races we need to do it
	 * before we start mucking with data structures.
	 */
	temp = osenv_mem_alloc(sizeof(struct softint_handler), 0, 1);
	if (temp == NULL) 
		return OSKIT_ENOMEM;
	temp->func = handler;
	temp->data = data;
	temp->next = NULL;
	temp->flags = flags;

	/* Do this first */
	base_irq_softint_handler = softint_handler;

	/*
	 * Note that we only hook in the new handler
	 * after its structure has been fully initialized;
	 * this way we don't have to disable interrupts,
	 * because interrupt-level code only scans the list.
	 */
	for (p = &softint_handlers[irq]; *p != NULL; p = &(*p)->next)
		;
	*p = temp;

	return 0;
}

/*
 * Request an unused software interrupt handler vector. This just reserves
 * and returns the vector number for use with osenv_softirq_alloc() above.
 */
oskit_error_t
osenv_softirq_alloc_vector(int *out_irq)
{
	int		   i, enabled;
	
	/*
	 * Go ahead and block interrupts, cause its easy.
	 */
	enabled = osenv_intr_enabled();
	if (enabled)
		osenv_intr_disable();

	/*
	 * Search for an empty slot above the well-known vectors.
	 */
	for (i = SOFT_IRQ_FREEBASE; i < SOFT_IRQ_COUNT; i++) {
		if (! (softint_allocated & (1 << i)))
			break;
	}
	if (i == SOFT_IRQ_COUNT) {
		if (enabled)
			osenv_intr_enable();
		return OSKIT_ERANGE;
	}

	softint_allocated |= (1 << i);
	*out_irq = i;
	
	if (enabled)
		osenv_intr_enable();
	return 0;
}

/*
 * Free up a vector.
 */
oskit_error_t
osenv_softirq_free_vector(int irq)
{
	/* Must be allocated */
	if (! (softint_allocated & (1 << irq)))
		return OSKIT_EINVAL;

	/* Must no longer be in use */
	if (softint_handlers[irq])
		return OSKIT_EINVAL;

	softint_allocated &= ~(1 << irq);
	
	return 0;
}

/*
 * Deallocate a software interrupt.
 * Need a handle so know WHICH interrupt handler to remove.
 */
void
osenv_softirq_free(int irq, void (*handler)(void *), void *data)
{
	struct softint_handler *temp, **p;

	osenv_assert(irq >= 0 && irq < SOFT_IRQ_COUNT);

	/*
	 * Find and unlink the handler from the list.
	 */
	p = &softint_handlers[irq];
	while (((temp = *p) != NULL) &&
	       (temp->func != handler || temp->data != data))
		p = &temp->next;

	/* not found? */
	if (temp == NULL) {
		osenv_log(OSENV_LOG_WARNING,
			  "osenv_softirq_free: handler not registered!\n");
		return;
	}
	
	/* remove it! */
	*p = temp->next;

	osenv_mem_free(temp, 0, sizeof(struct softint_handler));
}

/*
 * Schedule a software interrupt to be delivered next time its appropriate
 */
void
osenv_softirq_schedule(int irq)
{
	osenv_assert(softint_handlers[irq]);

	softint_pending |= (1 << irq);
	base_irq_softint_request();
}

/*
 * These chain to the base_irq_softint routines. They are the equiv
 * of osenv_intr_enable() and osenv_intr_disable().
 */
void
osenv_softintr_enable(void)
{
	base_irq_softint_enable();
}

void
osenv_softintr_disable(void)
{
	base_irq_softint_disable();
}

int
osenv_softintr_enabled(void)
{
	return base_irq_softint_enabled();
}

