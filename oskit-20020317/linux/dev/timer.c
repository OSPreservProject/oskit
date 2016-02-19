/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000 The University of Utah and the Flux Group.
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
 * Linux timer support.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tqueue.h>
#include <linux/interrupt.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/softirq.h>

#include "irq.h"

/*
 * Mask of active timers.
 */
unsigned long timer_active = 0;

/*
 * List of timeout routines.
 */
struct timer_struct timer_table[32];

/*
 * The head for the timer-list has a "expires" field of MAX_UINT,
 * and the sorting routine counts on this..
 */
static struct timer_list timer_head =
{
  &timer_head, &timer_head, ~0, 0, NULL
};

#undef SLOW_BUT_DEBUGGING_TIMERS

void
add_timer(struct timer_list *timer)
{
	unsigned long flags;
	struct timer_list *p;

#if SLOW_BUT_DEBUGGING_TIMERS
	if (timer->next || timer->prev) {
		printk("add_timer() called with non-zero list from %p\n",
			__builtin_return_address(0));
		return;
	}
#endif
	p = &timer_head;
	flags = linux_save_flags();
	linux_cli();
	do {
		p = p->next;
	} while (timer->expires > p->expires);
	timer->next = p;
	timer->prev = p->prev;
	p->prev = timer;
	timer->prev->next = timer;
	linux_restore_flags(flags);
}

int
del_timer(struct timer_list *timer)
{
	unsigned long flags;
#if SLOW_BUT_DEBUGGING_TIMERS
	struct timer_list * p;

	p = &timer_head;
	flags = linux_save_flags();
	linux_cli();
	while ((p = p->next) != &timer_head) {
		if (p == timer) {
			timer->next->prev = timer->prev;
			timer->prev->next = timer->next;
			timer->next = timer->prev = NULL;
			linux_restore_flags(flags);
			return 1;
		}
	}
	if (timer->next || timer->prev)
		printk("del_timer() called from %p with timer not initialized\n",
			__builtin_return_address(0));
	linux_restore_flags(flags);
	return 0;
#else
	struct timer_list * next;
	int ret = 0;
	flags = linux_save_flags();
	linux_cli();
	if ((next = timer->next) != NULL) {
		(next->prev = timer->prev)->next = next;
		timer->next = timer->prev = NULL;
		ret = 1;
	}
	linux_restore_flags(flags);
	return ret;
#endif
}

void
mod_timer(struct timer_list *timer, unsigned long expires)
{
	unsigned long flags;

	flags = linux_save_flags();
	linux_cli();

	del_timer(timer);
	timer->expires = expires;
	add_timer(timer);

	linux_restore_flags(flags);
}

/*
 * Timer software interrupt handler.
 * Should be called (and return) with interrupts enabled.
 */
void
timer_bh()
{
	unsigned long mask;
	struct timer_struct *tp;
	struct timer_list * timer;

	osenv_assert(osenv_intr_enabled() != 0);

	linux_cli();

	while ((timer = timer_head.next) != &timer_head
	       && timer->expires <= jiffies) {
		void (*fn)(unsigned long) = timer->function;
		unsigned long data = timer->data;

		timer->next->prev = timer->prev;
		timer->prev->next = timer->next;
		timer->next = timer->prev = NULL;
		linux_sti();
		fn(data);
		linux_cli();
	}
	linux_sti();

	for (mask = 1, tp = timer_table; mask; tp++, mask <<= 1) {
		if (mask > timer_active)
			break;
		if ((mask & timer_active)
		    && tp->expires > jiffies) {
			timer_active &= ~mask;
			(*tp->fn)();
			linux_sti();
		}
	}

	osenv_assert(osenv_intr_enabled() != 0);
}

/*
 * Timer interrupt handler.
 * Should be called (and return) with interrupts disabled.
 */
void
linux_timer_intr()
{
	unsigned long mask;
	struct timer_struct *tp;

	osenv_assert(osenv_intr_enabled() == 0);

	for (mask = 1, tp = timer_table; mask; tp++, mask += mask) {
		if (mask > timer_active)
			break;
		if (!(mask & timer_active))
			continue;
		if (tp->expires > jiffies)
			continue;
		mark_bh(TIMER_BH);
	}
	if (timer_head.next->expires <= jiffies)
		mark_bh(TIMER_BH);

	if (tq_timer)
		mark_bh(TQUEUE_BH);

	/* If any linux software handlers pending, schedule a softirq */
	if (bh_mask & bh_active)
		osenv_softirq_schedule(softintr_vector);

	osenv_assert(osenv_intr_enabled() == 0);
}
