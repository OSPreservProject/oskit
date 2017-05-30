/*
 * Copyright (c) 1996-1999 University of Utah and the Flux Group.
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
 * IRQ management.
 *
 * Shared IRQs are now supported.
 */

#include <oskit/error.h>
#include <oskit/debug.h>
#include <oskit/dev/dev.h>
#include <oskit/arm32/proc_reg.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/shark/base_irq.h>
#include <stdio.h>

/* linked list of functions for an IRQ */
struct int_handler {
	void	(*func)(void *);
	void	*data;
	struct int_handler *next;
};

/* array of pointers to lists of interrupt handlers */
static struct int_handler *handlers[BASE_IRQ_COUNT];

/* flag = 1 if sharing allowed; set to 0 if not */
static char shareable[BASE_IRQ_COUNT];

static void
fdev_irq_handler(struct trap_state *ts)
{
	unsigned irq = ts->intno;
	struct int_handler *current = handlers[irq];

	osenv_assert ((irq >= 0) && (irq < BASE_IRQ_COUNT));

	while (current) {
		osenv_assert(current->func);
		current->func(current->data);
		current = current->next;
	}
}

/*
 * Allocate an IRQ.
 */
int
osenv_irq_alloc(int irq, void (*handler)(void *), void *data, int flags)
{
	struct int_handler *temp, **p;
	int first_time;

	if (irq < 0 || irq >= BASE_IRQ_COUNT)
		return OSKIT_EINVAL;

	/*
	 * This is a blocking operation,
	 * so to avoid races we need to do it
	 * before we start mucking with data structures.
	 */
	temp = osenv_mem_alloc(sizeof(struct int_handler), 0, 1);
	if (temp == NULL) 
		return OSKIT_ENOMEM;
	temp->func = handler;
	temp->data = data;
	temp->next = NULL;

	first_time = (handlers[irq] == NULL);

	/*
	 * Make sure the entry in the base_irq_handlers table is available.
	 */
	if (first_time && base_irq_handlers[irq] != base_irq_default_handler) {
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
	    	return OSKIT_EBUSY;
	}

	/*
	 * Fail if the irq is already in use
	 * and either the new handler or the existing handler
	 * is not shareable.
	 */
	if (!first_time &&
	    (!shareable[irq] || !(flags & OSENV_IRQ_SHAREABLE))) {
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
	    	return OSKIT_EBUSY;
	}

	/*
	 * Note that we only hook in the new handler
	 * after its structure has been fully initialized;
	 * this way we don't have to disable interrupts,
	 * because interrupt-level code only scans the list.
	 */
	for (p = &handlers[irq]; *p != NULL; p = &(*p)->next);
	*p = temp;

	if (first_time)  {
		shareable[irq] = (flags & OSENV_IRQ_SHAREABLE) != 0;
		base_irq_handlers[irq] = fdev_irq_handler;
		osenv_irq_enable(irq);
	}

	return 0;
}

/*
 * Deallocate an IRQ.
 * Need a handle so know WHICH interrupt handler to remove.
 */
void
osenv_irq_free(int irq, void (*handler)(void *), void *data)
{
	struct int_handler *temp, **p;

	osenv_assert(irq >= 0 && irq < BASE_IRQ_COUNT);
	osenv_assert(base_irq_handlers[irq] == fdev_irq_handler);

	/*
	 * If this is the only handler for this IRQ,
	 * then disable the IRQ before unregistering it,
	 * to avoid a possible infinite interrupt loop
	 * if the interrupt comes at just the wrong time.
	 * Not that this is ever likely to happen,
	 * but you never know with PC hardware...
	 */
	temp = handlers[irq];
	if (temp != NULL &&
	    temp->func == handler && temp->data == data &&
	    temp->next == NULL) {
		osenv_irq_disable(irq);
		handlers[irq] = NULL;
		base_irq_handlers[irq] = base_irq_default_handler;
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
		return;
	}

	/*
	 * Find and unlink the handler from the list.
	 * Interrupt handlers may safely scan the list during this time,
	 * but we know no one else will concurrently modify it
	 * because only process-level code modifies the list.
	 */
	p = &handlers[irq];
	while (((temp = *p) != NULL) &&
	       (temp->func != handler || temp->data != data))
		p = &temp->next;

	/* not found? */
	if (temp == NULL) {
		osenv_log(OSENV_LOG_WARNING,
			"osenv_irq_free: interrupt handler not registered!\n");
		return;
	}
	
	/* remove it! */
	*p = temp->next;

	osenv_mem_free(temp, 0, sizeof(struct int_handler));
}


