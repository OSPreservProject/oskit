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
 * Linux software interrupts.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/system.h>
#include <asm/bitops.h>

#include <oskit/dev/dev.h>
#include <oskit/dev/softirq.h>
#include "osenv.h"

atomic_t bh_mask_count[32];

/*
 * Mask of pending interrupts.
 */
unsigned long bh_active = 0;

/*
 * Mask of enabled interrupts.
 */
unsigned long bh_mask = 0;

/*
 * List of software interrupt handlers.
 */
void (*bh_base[32])(void);

/*
 * Flag indicating a soft interrupt is being handled.
 */
unsigned int local_bh_count[1];		/* SMP */

/*
 * Our softirq vector that osenv gave back to us.
 */
int softintr_vector;

void
linux_softintr_init(void)
{
	int		err;
	static void	linux_softintr_handler(void *arg);

	err = osenv_softirq_alloc_vector(&softintr_vector);
	if (err)
		panic(__FUNCTION__ ": Could not allocate softirq vector\n");

	err = osenv_softirq_alloc(softintr_vector,
				  linux_softintr_handler, 0, 0);
	if (err)
		panic(__FUNCTION__ ": Could not allocate softirq handler\n");
}

/*
 * Software interrupt handler.
 * Should be called (and return) with interupts enabled.
 */
void
do_bottom_half(void)
{
	unsigned long active;
	unsigned long mask, left;
	void (**bh)(void);

	/* not reentrant */
	if (local_bh_count[0]++ == 0) {
		osenv_assert(osenv_intr_enabled() != 0);

		bh = bh_base;
		active = get_active_bhs();
		clear_active_bhs(active);
		for (mask = 1, left = ~0;
		     left & active; bh++, mask += mask, left += left) {
			if (mask & active) {
				void (*fn)(void);

				fn = *bh;
				if (fn == 0) {
					osenv_log(OSENV_LOG_WARNING,
						  "linux_soft_intr: "
						  "bad interrupt handler "
						  "entry 0x%08lx\n", mask);
					break;
				}
				fn();
			}
		}

		osenv_assert(osenv_intr_enabled() != 0);
	}
	local_bh_count[0]--;
}

/*
 * The handler, called from the osenv soft interrupt code.
 */
static void
linux_softintr_handler(void *arg)
{
	osenv_assert(local_irq_count[0] == 0);
	osenv_assert(local_bh_count[0] == 0);

	/*
	 * Interrupts will already be enabled.
	 */
	if (bh_mask & bh_active) {
		struct task_struct *cur = current;
		do_bottom_half();
		current = cur;
	}
}
