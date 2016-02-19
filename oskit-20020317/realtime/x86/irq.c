/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/pio.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/pc/base_irq.h>
#include <oskit/x86/pc/irq_list.h>
#include <oskit/x86/pc/pic.h>
#include <oskit/c/string.h>

/* linked list of functions for an IRQ */
struct int_handler {
	void	(*func)(void *);
	void	*data;
	struct int_handler *next;
	int	flags;
};

/* array of pointers to lists of interrupt handlers */
static struct int_handler *standard_handlers[BASE_IRQ_COUNT];

/* flag = 1 if sharing allowed; set to 0 if not */
static char shareable[BASE_IRQ_COUNT];

/* another array of pointers to lists of interrupt handlers */
static struct int_handler *realtime_handlers[BASE_IRQ_COUNT];

/* Pending IRQs that could not be delivered or have been soft scheduled. */
volatile unsigned int		realtime_irq_pending_irqs;

/* Realtime IRQs; these are IRQs that have a realtime handler installed. */
static volatile unsigned int	irq_realtime_irqs;

/* Software flag to control when the rest of the oskit enables/disables */
int				realtime_irq_intr_enabled = 1;

/* Forward decl. Deliver all pending IRQs to standard handlers */
void				realtime_deliver_pending_irqs(void);

/*
 * Tells us if the current thread is a realtime thread.
 * Note that this is a "common" symbol that will be overridden by the
 * pthread version when that library is linked in (since pthreads is
 * independent and optional of the realtime library).
 */
int				pthread_realtime_mode;

#ifdef	LBS
int do_kprintf = 0;
#endif

/*
 * Helper functions to deliver IRQs to its standard/realtime handlers.
 */
static inline void
DELIVER_IRQ(struct int_handler *current, struct trap_state *ts)
{
	while (current) {
		osenv_assert(current->func);
		if (current->flags & OSENV_IRQ_TRAPSTATE)
			current->func((void *) ts);
		else
			current->func(current->data);
		current = current->next;
	}
}

/*
 * Interrupt handler called from base_irq_inittab.
 * Called with IF flag cleared and target IRQ masked.
 * We should return the same way.
 */
static unsigned int
fdev_irq_handler(struct trap_state *ts)
{
	unsigned	irq = ts->err;
	int		was_enabled = realtime_irq_intr_enabled;

	osenv_assert ((irq >= 0) && (irq < BASE_IRQ_COUNT));
	osenv_assert ((get_eflags() & EFL_IF) == 0);

	/*
	 * EFL_IF is cleared when we get here.
	 * Sync the software state
	 */
	realtime_irq_intr_enabled = 0;

#ifdef	LBS
	if (do_kprintf) {
		unsigned int bt[24];

		store_stack_trace(bt, 12);

		kprintf("%d %d 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
			"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x"
			"\n", irq, was_enabled,
			bt[0], bt[1], bt[2], bt[3], bt[4], bt[5], bt[6],
			bt[7], bt[8], bt[9], bt[10], bt[11]);
	}
#endif
	/*
	 * Realtime handlers are always delivered immediately, and
	 * take precedence over the standard handlers. The implication
	 * for an IRQ that has both realtime *and* standard handlers
	 * (like the clock) is that the realtime handler must arrange
	 * for the standard handlers to be called by adding the IRQ to
	 * the pending set. This is done cause it allows the realtime
	 * handler to do an immediate context switch after renabling
	 * the PIC, and not have to worry about having the standard
	 * handlers be run when the current context gets switched back
	 * in.  Rather, they will get run at the next available
	 * opportunity, which is when interrupts get renabled and realtime
	 * mode is no longer in effect (that is, a designated "realtime"
	 * thread is no longer running).
	 *
	 * If there are no realtime handlers, the IRQ is added to the set
	 * of pending IRQs for standard handlers.
	 *
	 * As above, standard handlers will be invoked only when interrupts
	 * were software enabled *and* when realtime mode is no longer in
	 * effect.
	 *
	 * NOTE: The assembly stub disables the PIC for this IRQ, and by
	 * returning BASE_IRQ_LEAVE_MASKED below, the stub will leave it
	 * alone. So, all we need to worry about is enabling it if/when
	 * the IRQ is delivered to its handler.
	 */
	if (realtime_handlers[irq]) {
		pic_enable_irq(irq);
		DELIVER_IRQ(realtime_handlers[irq], ts);
	}
	else {
		osenv_assert(! (realtime_irq_pending_irqs & (1 << irq)));

		realtime_irq_pending_irqs |= (1 << irq);
	}

	/*
	 * Deliver if interrupts were software enabled and a realtime thread
	 * is not running.
	 */
	if (realtime_irq_pending_irqs && was_enabled && !pthread_realtime_mode)
		realtime_deliver_pending_irqs();

	realtime_irq_intr_enabled = was_enabled;

	/*
	 * Software interrupts are delivered from the delivery function
	 * below (realtime_deliver_pending_irqs()) when interrupts are
	 * enabled coming in, and when interrupts are renabled (and there
	 * were pending IRQs to deliver). As a result, always return the
	 * flag that prevents the softint handler from being called out of
	 * the assembly stub.
	 */
	return (BASE_IRQ_LEAVE_MASKED|BASE_IRQ_SKIP_SOFTINT);
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

#ifdef GPROF
	/* Profiling steals the RTC, so do not let someone have it. */
	if (irq == IRQ_RTC && !(flags & OSENV_IRQ_REALTIME))
		return OSKIT_EBUSY;
#endif
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
	temp->flags = flags;

	first_time = (standard_handlers[irq] == NULL &&
		      realtime_handlers[irq] == NULL);

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
	    !(flags & (OSENV_IRQ_OVERRIDE|OSENV_IRQ_REALTIME)) &&
	    (!shareable[irq] || !(flags & OSENV_IRQ_SHAREABLE))) {
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
		panic("osenv_irq_alloc: Attempt to share an IRQ!");
	    	return OSKIT_EBUSY;
	}

	/*
	 * Note that we only hook in the new handler
	 * after its structure has been fully initialized;
	 * this way we don't have to disable interrupts,
	 * because interrupt-level code only scans the list.
	 */
	for (p = ((flags & OSENV_IRQ_REALTIME) ?
		  &realtime_handlers[irq] : &standard_handlers[irq]);
		     *p != NULL; p = &(*p)->next);
	*p = temp;

	if (first_time)  {
		shareable[irq] = (flags & OSENV_IRQ_SHAREABLE) != 0;
		base_irq_handlers[irq] = fdev_irq_handler;
		osenv_irq_enable(irq);
	}

	if (flags & OSENV_IRQ_REALTIME)
		irq_realtime_irqs |= (1 << irq);

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
	int enabled;

	osenv_assert(irq >= 0 && irq < BASE_IRQ_COUNT);
	osenv_assert(base_irq_handlers[irq] == fdev_irq_handler);

	/*
	 * This used to be more fancy, not disabling the IRQ. Forget that.
	 * We don't free IRQs that often, so lets be simple about it.
	 */
	enabled = osenv_irq_enabled(irq);
	osenv_irq_disable(irq);

	/*
	 * Find and unlink the handler from the list.
	 */
	p = &standard_handlers[irq];
	while (((temp = *p) != NULL) &&
	       (temp->func != handler || temp->data != data))
		p = &temp->next;

	/* not found? */
	if (temp == NULL) {
		p = &realtime_handlers[irq];
		while (((temp = *p) != NULL) &&
		       (temp->func != handler || temp->data != data))
			p = &temp->next;
	}

	/* still not found? */
	if (temp == NULL) {
		osenv_log(OSENV_LOG_WARNING,
			"osenv_irq_free: interrupt handler not registered!\n");
		if (enabled)
			osenv_irq_enable(irq);
		return;
	}

	/* remove it! */
	*p = temp->next;

	/*
	 * If no more registered handlers, reset things.
	 */
	if (!realtime_handlers[irq])
		irq_realtime_irqs &= ~(1 << irq);

	if (!standard_handlers[irq] && !realtime_handlers[irq]) {
		base_irq_handlers[irq] = base_irq_default_handler;
		shareable[irq] = 0;
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
		return;
	}

	if (enabled)
		osenv_irq_enable(irq);

	osenv_mem_free(temp, 0, sizeof(struct int_handler));
}

/*
 * Schedule a standard IRQ to be delivered when interrupts are next enabled.
 */
void
osenv_irq_schedule(int irq)
{
	osenv_assert(base_irq_handlers[irq] == fdev_irq_handler);
	osenv_assert(standard_handlers[irq]);
	osenv_assert(! realtime_irq_intr_enabled);

	realtime_irq_pending_irqs |= (1 << irq);
}

/*
 * Deliver pending IRQs. When we deliver pending IRQs, we want to turn
 * on interrupts and enable realtime IRQs at the PIC (they already
 * will be enabled), but leave all other IRQs disabled. This way realtime
 * IRQs will continue to be delivered, but the set of standard IRQs to be
 * delivered will be masked during this time. When all the IRQs have been
 * delivered, turn off interrupts and renable all those IRQs at the PIC.
 *
 * When all IRQshave  been delivered, check for a software interrupt. The IRQ
 * handler above always returns BASE_IRQ_SKIP_SOFTINT. By checking for
 * software interrupts here, we maintain the invariants that a software
 * interrupt is delivered only as the result of a hardware interrupt, and
 * the softint is delivered at the end of the hardware interrupt delivery.
 */
void
realtime_deliver_pending_irqs(void)
{
	static int		doing_deliver;
	unsigned int		irqs_delivered = 0;
	struct trap_state	ts;

	if (doing_deliver)
		return;
	doing_deliver = 1;

	memset(&ts, 0, sizeof(ts));

	while (realtime_irq_pending_irqs) {
		int			i;

		for (i = 0; i < BASE_IRQ_COUNT; i++) {
			if (realtime_irq_pending_irqs & (1 << i)) {
				realtime_irq_pending_irqs &= ~(1 << i);
				irqs_delivered |= (1 << i);
				ts.err = i;
				realtime_irq_intr_enabled = 0;
				sti();
				DELIVER_IRQ(standard_handlers[i], &ts);
				cli();
			}
		}
	}
	if (irqs_delivered) {
		pic_set_irqmask(pic_get_irqmask() & ~irqs_delivered);

		/*
		 * Deliver soft interrupts.  We must do this now since the
		 * HW IRQ handler above returns the flag that prevents this
		 * from happening in the assembly stub when interrupts were
		 * disabled.
		 *
		 * The SW int handler is called with interrupts enabled so
		 * we need to make sure that HW ints can be delivered
		 * (doing_deliver == 0) but that software interrupts are
		 * disabled.
		 */
		if (base_irq_softint_pending() && base_irq_softint_enabled()) {
			base_irq_softint_disable();
			base_irq_softint_clear();
			ts.err = 0;
			doing_deliver = 0;
			realtime_irq_intr_enabled = 1;
			sti();
			base_irq_softint_handler(&ts);
			cli();
			base_irq_softint_enable();
		}
	}

	doing_deliver = 0;
}
