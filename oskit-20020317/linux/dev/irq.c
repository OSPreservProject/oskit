/*
 * Copyright (c) 1996-2000 The University of Utah and the Flux Group.
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
 * Emulate Linux IRQ management.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kernel_stat.h>

#include <oskit/dev/softirq.h>
#include "irq.h"
#include "osenv.h"

static void linux_intr(void *data);
static void probe_intr(void *data);

static unsigned probe_mask;

#if 0
#define DEBUG_MSG() osenv_log(OSENV_LOG_DEBUG, "%s:%d:\n", __FILE__, __LINE__);
#else
#define DEBUG_MSG()
#endif

#define NHWI 16

/* flag = 1 if sharing allowed; set to 0 if not */
static char shared[NHWI];

/* linked list of functions for an IRQ */
struct int_handler {
	void	(*func)(int, void*, struct pt_regs *);
	int	flags;
        unsigned long mask;
        const char *name;
        void *dev_id;
	struct int_handler *next;
};

/* array of pointers to lists of interrupt handlers */
static struct int_handler *handlers[NHWI];


/*
 * Flag indicating a hard interrupt is being handled.
 */
unsigned int local_irq_count[1];		/* SMP */


void
linux_intr_init()
{
	int i;

	for (i = 0; i < NHWI; i++) {
		handlers[i] = NULL;
		shared[i] = 1;
	}
}



/*
 * Generic Linux interrupt handler.
 * I believe that the Linux equivalent to this routine expects interrupts
 * to be disabled (EFL_IF) coming in and going out.
 */
static void
linux_intr(void *data)
{
	int irq = (int)data;
	struct pt_regs regs;
	struct int_handler *hand;
	struct task_struct *cur = current;

	kstat.irqs[0][irq]++;
	local_irq_count[0]++;

	if ((handlers[irq]->flags & SA_INTERRUPT) == 0)
		linux_sti();

	hand = handlers[irq];

	while (hand) {
		(*hand->func)(irq, hand->dev_id, &regs);
		hand = hand->next;
	}

	local_irq_count[0]--;

	/* If any linux software handlers pending, schedule a softirq */
	if (bh_mask & bh_active)
		osenv_softirq_schedule(softintr_vector);

	linux_cli();

	current = cur;
}


/*
 * Probe interrupts set the bit indicating they were received
 */
static void
probe_intr(void *data)
{
	int irq = (int)data;
#ifdef IRQ_DEBUG
	osenv_log(OSENV_LOG_DEBUG, "probe IRQ%d\n", irq);
#endif
	probe_mask |= (1 << irq);
}

/*
 * Attach a handler to an IRQ.
 */
int
request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
	    unsigned long flags, const char *device, void *dev_id)
{
	void (*intr_handler)(void *);
	struct task_struct *cur = current;
	int rc;
	int fdev_flags;
	struct int_handler *temp;

	/* Make sure it is valid to assign this handler */
#ifdef IRQ_DEBUG
	osenv_log(OSENV_LOG_DEBUG, "IRQ %d requested by %s at %p\n",
		irq, (device ? device : ""), handler);
#endif

	if (!handler) {
		return (-EINVAL);
	}

	if (!shared[irq]) {
		return (-EBUSY);
	}

	/* If owned by sharing, and we won't share, we're done too */
	if (handlers[irq] && ((flags & SA_SHIRQ) == 0)) {
		return (-EBUSY);
	}

	/* okay, now set up the handler */

	temp = osenv_mem_alloc(sizeof(struct int_handler), 0, 1);
	if (!temp)
		return (-ENOMEM);

	temp->next = handlers[irq];
	handlers[irq] = temp;

	handlers[irq]->func = handler;
	handlers[irq]->flags = flags;
	handlers[irq]->dev_id = dev_id;

	if (handlers[irq]->next) { /* already have the IRQ from FDEV. */
		osenv_log(OSENV_LOG_DEBUG, 
			"adding another linux handler to irq %d, tag = %x\n",
			irq, (unsigned)handler);
	} else {

		intr_handler = (flags & SA_PROBE) ? probe_intr : linux_intr;
        	fdev_flags = (flags & SA_SHIRQ) ? OSENV_IRQ_SHAREABLE : 0;

		rc=osenv_irq_alloc(irq, intr_handler, (void *)irq, fdev_flags);
		current = cur;	/* restore current after possibly blocking */
		if (rc) {
			temp = handlers[irq];
			handlers[irq] = handlers[irq]->next;
			osenv_mem_free(temp, 0, sizeof(struct int_handler));

			return (-EBUSY);
		}
	}

	/* update the shared flag */
	if ((flags & SA_SHIRQ) == 0)
		shared[irq] = 0;

	DEBUG_MSG();
	return (0);
}

/*
 * Deallocate an irq.
 */
void
free_irq(unsigned int irq, void *dev_id)
{
	struct task_struct *cur = current;
	struct int_handler *temp, *last;

	temp = handlers[irq];

	if (!temp)
		return;

	/* one element -- free the irq */
	if (!temp->next) {
		void (*intr_handler)(void *);

		if (temp->dev_id != dev_id)
			return;
		handlers[irq]=NULL;
		intr_handler = (temp->flags & SA_PROBE) ? 
			probe_intr : linux_intr; 
		osenv_irq_free(irq, intr_handler, (void*)irq);
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
		shared[irq] = 1;

		current = cur;
		return;
	}

	last = temp;

	/* find the correct element and remove it */
	while (temp && (temp->dev_id != dev_id)) {
		last = temp;
		temp = temp->next;
	}

	if (!temp)	/* not found */
		return;

	if (temp == last) {	/* first node */
		handlers[irq] = temp->next;
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
	} else {
		last->next = temp->next;
		osenv_mem_free(temp, 0, sizeof(struct int_handler));
	}

	current = cur;
}

void
disable_irq(unsigned int irq)
{
	osenv_irq_disable(irq);
}

#ifndef OSKIT_ARM32
void
disable_irq_nosync(unsigned int irq)
{
	osenv_irq_disable(irq);
}
#endif

void
enable_irq(unsigned int irq)
{
	osenv_irq_enable(irq);
}


static void no_action(int cpl, void *arg, struct pt_regs * regs) { }

/*
 * Get ready to probe for the interrupts.
 * Set IRQs received to 0 (probe_mask).
 * Allocate all available IRQs, and return that set.
 * If receive any IRQs here, we don't want that IRQ
 */
unsigned long 
probe_irq_on (void)
{
	unsigned int i, irqs = 0, irqmask;
	unsigned long delay;

	/* first, snaffle up any unassigned irqs */
	probe_mask = 0;
	for (i = (NHWI - 1); i > 0; i--) {
		if (!request_irq(i, no_action, SA_PROBE, "probe", NULL)) {
			enable_irq(i);
			irqs |= (1 << i);
		}
	}

	/* wait for spurious interrupts to mask themselves out again */
	for (delay = jiffies + 5; delay > jiffies; );	/* min 40ms delay */

	irqmask = probe_mask;	/* if bit is set, it is spurious */
	/* now filter out any obviously spurious interrupts */
	for (i = (NHWI - 1); i > 0; i--) {
		if (irqs & (1 << i) & irqmask) {
#ifdef IRQ_DEBUG
			osenv_log(OSENV_LOG_NOTICE, "Spurious IRQ%d\n", i);
#endif
			irqs ^= (1 << i);
			free_irq(i, NULL);
		}
	}

#ifdef IRQ_DEBUG
	printk("probe_irq_on:  irqs=0x%04x irqmask=0x%04x\n\n", irqs, 
	       probe_mask);
#endif
	return irqs;
}

/*
 * Free all allocated IRQs.
 * If only one of the IRQs grabbed was received, return it.
 * Otherwise, autoprobe failed.
 */
int 
probe_irq_off (unsigned long irqs)
{
	unsigned int i, irqmask;

	irqmask = probe_mask;	/* IRQs received */
	/* free all allocated IRQs */
	for (i = (NHWI - 1); i > 0; i--) {
		if (irqs & (1 << i)) {
			free_irq(i, NULL);
		}
	}
#ifdef IRQ_DEBUG
	printk("probe_irq_off: irqs=0x%04x irqmask=0x%04x\n", (unsigned)irqs, 
	       irqmask);
#endif
	/* what allocated IRQs did I receive? */
	irqs &= irqmask;
	if (!irqs)	/* none found... */
		return 0;
	i = ffz(~irqs);
	if (irqs != (irqs & (1 << i)))	/* multiple IRQs found? */
		i = -i;
	return i;
}
