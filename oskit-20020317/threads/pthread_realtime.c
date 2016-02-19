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

#ifdef PTHREAD_REALTIME
/*
 * Support for realtime schedulers.
 */

#include <threads/pthread_internal.h>
#include <threads/pthread_realtime.h>

/* Clock IRQ handlers */
static void		realtime_clock_handler(void *data);
static void		standard_clock_handler(void *data);

/*
 * Clock counts. This allows us to keep track of missed clock
 * interrupts. For every realtime interrupt, there should be a
 * corresponding callback on the standard handler. When interrupts are
 * software disabled, we get callback on the realtime handler, but not
 * on the standard handler. In most cases this one miss will get
 * called later when interrupts are software enabled. However, it is
 * possible to miss more than one if interrupts are disabled a long
 * time. By keeping track of the difference in count, we can schedule
 * enough clock ticks to keep the rest of the oskit time keeping
 * mechanism happy. This essentially operates as an up/down counter.
 */
static unsigned int	realtime_clock_ticks;

/*
 * Just what the name sez.
 */
void
realtime_clock_init(void)
{
	int		err, enabled;

	enabled = machine_intr_enabled();
	machine_intr_disable();
	
	/*
	 * Add a realtime handler for IRQ 0 (Clock).
	 */
	err = osenv_irq_alloc(0, realtime_clock_handler,
			      0, OSENV_IRQ_REALTIME);
	if (err)
		panic("realtime_clock_init: Could not add realtime IRQ\n");

	/*
	 * Add a normal handler for the clock. The override flag allows
	 * a non-shared IRQ to become shared. I prefer this to changing
	 * existing uses of IRQ 0 in other parts of the oskit.
	 */
	err = osenv_irq_alloc(0, standard_clock_handler,
			      0, OSENV_IRQ_OVERRIDE);
	if (err)
		panic("realtime_clock_init: Could not add standard IRQ\n");


	if (enabled)
		machine_intr_enable();
}

/*
 * Realtime clock handler. 
 */
static void
realtime_clock_handler(void *data)
{
#if     defined(RTSCHED_STATS) && defined(OSKIT_X86_PC)
	/*
	 * Count latency cycles. Depends on some code in the low level
	 * IRQ handler. Not portable.
	 */
	extern stat_time_t interrupt_start_high, interrupt_start_low;
	stat_stamp_t	then, now;
	stat_accum_t	diff;

	now  = STAT_STAMPGET();
	then = ((stat_stamp_t) interrupt_start_high << 32) |
		interrupt_start_low;
	diff = (stat_accum_t) (now - then);
	
	pthread_counts.handler_latency_total += diff;

	if (diff < pthread_counts.handler_latency_min)
		pthread_counts.handler_latency_min = diff;
	if (diff > pthread_counts.handler_latency_max)
		pthread_counts.handler_latency_max = diff;
#endif
	PCOUNT(pthread_counts.realtime_clockticks++);
	realtime_clock_ticks++;
	
	/*
	 * Always pend a corresponding standard clock IRQ so that the
	 * rest of the oskit stays happy and in time. This will result
	 * in a call to the pthread timer callback.
	 */
	osenv_irq_schedule(0);

	/*
	 * In the RTAI/RTL systems, a realtime thread preempts whatever is
	 * running immediately. Thats because there is a very clear line
	 * between what is Linux and what is the RT system. In other words,
	 * linux and RT threads will not overlap in their behaviour, so its
	 * okay to preempt the linux kernel no matter what it is doing,
	 * cause an RT thread will never do anything that overlaps it.
	 * Well, thats how it appears to me.
	 *
	 * In the oskit, we do not have such a clear line (yet?). Preempting
	 * a thread when interrupts or preemption is disabled could really
	 * mess up things. This aspect needs more thought/work. Additionally,
	 * the scheduler code proper will need to hardware disable interrupts
	 * when taking the scheduler lock since it would be bad to context
	 * switch when in the context switch path!
	 *
	 * For now, use the existing thread preemption mechanism, which is
	 * a software interrupt. The existing pthread timer callback
	 * will enable the software interrupt flag. A preemption will
	 * happen as soon as interrupts are enabled and the preemption
	 * disable flag is cleared.
	 */
}

/*
 * All this does is keep the oskit time in sync by making sure that we
 * get as many standard interrupts handler calls as there are realtime
 * handler calls. 
 */
static void
standard_clock_handler(void *data)
{
	PCOUNT(pthread_counts.standard_clockticks++);

	osenv_assert(realtime_clock_ticks > 0);

	/*
	 * If multiple missed ticks, schedule another IRQ to be delivered.
	 */
	if (--realtime_clock_ticks > 0) {
		PCOUNT(pthread_counts.missed_clockticks++);
		osenv_irq_schedule(0);
	}
}
#endif
