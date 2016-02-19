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
 * Initialize the threads subsystem.
 */
#include <threads/pthread_internal.h>
#include <threads/pthread_ipc.h>
#include <math.h>
#include <assert.h>

/*
 * Current thread and idlethread, per processor.
 */
pthread_thread_t       *threads_curthreads[MAXCPUS]      = { 0 };
pthread_thread_t       *threads_idlethreads[MAXCPUS]     = { 0 };
pthread_thread_t        threads_mainthread;
int			threads_preempt_enable[MAXCPUS]  = { 0 };
int			threads_preempt_needed[MAXCPUS]  = { 0 };
int			threads_preempt_ready            = 0;
int			threads_alive                    = 1; /* Main */

#ifdef SMP
pthread_lock_t		threads_smpboot_lock = PTHREAD_LOCK_INITIALIZER;
#endif

int		        threads_initialized  = 0;
int		        threads_preemptible  = 0;
int			threads_debug        = 0;
int		        threads_num_processors = 0;
int		        threads_base_processor = 0;
int			threads_switch_mode  = 0;
oskit_u32_t		threads_realticks    = 0;
oskit_timespec_t	threads_realtime     = { 0, 0 };

/*
 * The array of thread structure pointers, indexed by TIDs.
 *
 * The only reason for doing this is so application code can be presented
 * with small integers for thread IDs, while internally we use the address
 * of the thread structure.
 */
pthread_thread_t	*threads_tidtothread[THREADS_MAX_THREAD];

/*
 * Hooks for user defined callouts.
 */
void	       *(*threads_allocator)(size_t)   = pthread_alloc_memory;
void		(*threads_deallocator)(void *) = pthread_dealloc_memory;

void	        pthread_init_internal(int preemptible, pthread_thread_t *main);
void		pthread_softint_handler(void *arg);

#ifdef THREADS_DEBUG
/*
 * Deadlock detection.
 */
int			threads_sleepers      = 0;
pthread_lock_t		threads_sleepers_lock = PTHREAD_LOCK_INITIALIZER;
#endif

#ifdef  RTSCHED_STATS
struct pthread_counts   pthread_counts;
static void		dump_stats(void);
#endif

#ifdef	LATENCY_THREAD
pthread_thread_t *hiprio;
unsigned long hiprio_missed, hiprio_delayed;

void *
thread_hiprio(void *arg)
{
	while (1)
		pthread_sched_reschedule(RESCHED_BLOCK, (spin_lock_t *)0);
}
#endif

void
pthread_init_withhooks(int preemptible,
		       void *(*allocator)(size_t), 
		       void (*deallocator)(void *))
{
	pthread_init_internal(preemptible, &threads_mainthread);

	threads_allocator   = allocator;
	threads_deallocator = deallocator;
}

void
pthread_init(int preemptible)
{
	pthread_init_internal(preemptible, &threads_mainthread);
}

void
pthread_init_internal(int preemptible, pthread_thread_t *mainthread)
{
	if (threads_initialized)
		return;

#ifdef  RTSCHED_STATS
	atexit(dump_stats);
	pthread_counts.handler_latency_min = 100000000; /* XXX */
#endif
	pthread_init_attributes();
#ifndef KNIT
	pthread_init_comlock();
	pthread_init_osenv_sleep();
#endif
	pthread_init_keytable();
	pthread_init_process_lock();
	pthread_init_exit();
	pthread_init_ipc();
#ifndef KNIT
	oskit_init_libc();
	oskit_clock_init();
#endif
	pthread_init_scheduler();
#ifdef	STACKGUARD
	pthread_init_guard();
#endif
	thread_machdep_init();

	threads_base_processor = 0;
	threads_num_processors = 1;
	
#ifdef	SMP
	/*
	 * Look for more processors
	 */
	if (!smp_init()) {
		threads_num_processors = smp_get_num_cpus();		
		threads_base_processor = smp_find_cur_cpu();
	}
	printf("pthread_init: %d Processors, Processor %d\n",
		threads_num_processors, threads_base_processor);
#endif
	/*
	 * Create the main thread.
	 */
	threads_curthreads[threads_base_processor] =
		pthread_init_mainthread(mainthread);

#ifndef CPU_INHERIT
	/*
	 * Create the idle thread(s).
	 */
	threads_idlethreads[threads_base_processor] =
		pthread_create_internal(pthread_idle_function, 0, 0);
#ifdef THREAD_STATS
	/* XXX idle threads are never explicitly q'd */
	threads_idlethreads[threads_base_processor]->stats.qstamp =
		STAT_STAMPGET();
#endif
#endif
#ifdef LATENCY_THREAD
	/*
	 * Create a high priority thread that wakes up periodically
	 * (so we can measure scheduling latency).
	 * XXX UP only
	 */
	{
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		attr.detachstate = PTHREAD_CREATE_DETACHED;
		attr.priority = PRIORITY_MAX;
		attr.stacksize = PTHREAD_STACK_MIN;
		hiprio = pthread_create_internal(thread_hiprio, 0, &attr);
	}
#endif
#ifdef	SMP
	/*
	 * Look for more processors
	 */
	if (threads_num_processors > 1) {
		int	num, cur;
		void	pthread_smp_booted(void *ignored);
		void	pthread_ipi_handler(void *ignored);
		char    *pstkmem;

		if (osenv_irq_alloc(SMP_IPI_VECTOR, pthread_ipi_handler, 0, 0))
			panic("pthread_init: osenv_irq_alloc");

		/* allow receiving IPI */
		smp_message_pass_enable[smp_find_cur_cpu()] = 1;
				    
		pstkmem = pthread_alloc_memory(512 *
					       (threads_num_processors - 1));

		num = 1;
		cur = -1;

		while (num < threads_num_processors)  {
			cur = smp_find_cpu(cur);
			if (cur != smp_find_cur_cpu()) {
				threads_idlethreads[cur] =
				 pthread_create_internal(pthread_idle_function,
							 0, 0);

#ifdef THREAD_STATS
				/* XXX idle threads are never explicitly q'd */
				threads_idlethreads[cur]->stats.qstamp =
					STAT_STAMPGET();
#endif
				printf("starting cpu %d\n", cur);
				smp_start_cpu(cur, pthread_smp_booted, 0,
					      pstkmem + 512);

				pstkmem += 512;
				num++;
			}
		}
	}
#endif
	/*
	 * Now it is safe to do this ...
	 */
#ifndef KNIT
	if (pthread_register_interface())
		panic("pthread_init: Could not register interface");
#endif

	if (preemptible) {
		void	pthread_interrupt_handler(void);

		osenv_timer_register(pthread_interrupt_handler, PTHREAD_HZ);
	}
	osenv_softirq_alloc(OSENV_SOFT_IRQ_PTHREAD,
			    pthread_softint_handler, 0, 0);

	PREEMPT_ENABLE = 1;
	threads_preempt_ready = 1;
	
#ifdef CPU_INHERIT
	/*
	 * Bootstrap the root scheduler. 
	 */
	{
		void		*(*function)(void *);
		void		*argument;

		bootstrap_root_scheduler(mainthread->tid, preemptible,
					 &function, &argument);

		mainthread->scheduler = pthread_root_scheduler =
			pthread_create_internal(function, argument, 0);

		/* XXX */
		threads_tidtothread[0] = pthread_root_scheduler;
		pthread_root_scheduler->tid = 0;

		/*
		 * Create the idle thread. This thread is never actually
		 * scheduled, but instead is run out of the rescheduler 
		 * when the root scheduler has nothing to do.
		 *
		 * Why an idle thread? A place to call to pthread_delay()
		 * in usermode, but mostly to avoid spinning on a stack
		 * belonging to some arbitrary thread that was switching
		 * out.
		 */
		threads_idlethreads[threads_base_processor] =
			pthread_create_internal(pthread_idle_function, 0, 0);

		pthread_sched_switchto(pthread_root_scheduler);
	}
#endif
	/*
	 * Running in main thread again ...
	 */
	pthread_init_signals();
	
	threads_initialized = 1;
}

/*
 * Preemption interrupt.
 */
void
pthread_interrupt_handler(void)
{
	if (! threads_preempt_ready)
		return;
	
	if (CURPTHREAD()) {
		CURPTHREAD()->cputime++;
		CURPTHREAD()->cpticks++;
#ifdef CPU_INHERIT
		{
			pthread_thread_t	*ptmp;

			ptmp = CURPTHREAD()->inherits_from;
			while (ptmp) {
				ptmp->childtime++;
				ptmp = ptmp->inherits_from;
			}

			pthread_sched_clocktick();
		}
#endif
	}
	threads_realticks++;

	/*
	 * Better way to keep time ...
	 */
	threads_realtime.tv_nsec += PTHREAD_TICK * 1000000;
	if (threads_realtime.tv_nsec >= 1000000000) {
		threads_realtime.tv_sec++;
		threads_realtime.tv_nsec -= 1000000000;
	}

	/*
	 * Recompute CPU percentages once a second.
	 */
	if ((threads_realticks % PTHREAD_HZ) == 0) {
		int			i;
		pthread_thread_t	*pthread;
#if	defined(CPU_INHERIT) && defined(DEMO)
		char			foo[BUFSIZ], *bp = foo;
		int			count = 0;
#endif
		for (i = 0; i < THREADS_MAX_THREAD; i++) {
			if ((pthread = threads_tidtothread[i]) != 0) {
				pthread->pctcpu  = pthread->cpticks;
				pthread->cpticks = 0;
			}
		}
#if	defined(CPU_INHERIT) && defined(DEMO)
		/*
		 * Demo stuff. Output percentages.
		 */
		bp += sprintf(bp, "||root:Time:%d ", threads_realticks);

		for (i = 0; i < THREADS_MAX_THREAD; i++) {
			if ((pthread = threads_tidtothread[i]) != 0) {
				if (pthread->pctcpu > 0) {
#if 0
					printf("||-a:graph the_graph ");
					printf("tid%d_line %d %d||\n",
					       pthread->tid,
					       threads_realticks,
					       count + pthread->pctcpu);
#endif
					bp += sprintf(bp, "%d:%d ",
						      pthread->tid,
						      pthread->pctcpu);

					count += pthread->pctcpu;
				}
			}
		}
		printf("%s - %d||\n", foo, count);
#endif
	}

#ifndef CPU_INHERIT
#ifdef SMP
	if (THISCPU == threads_base_processor) {
		int	curr = -1;
		int	i;

		for (i = 0; i < threads_num_processors; i++) {
			curr = smp_find_cpu(curr);			
			if (curr != threads_base_processor)
				smp_message_pass(curr);
		}
	}
	else {
		if (PREEMPT_ENABLE && CURPTHREAD() != IDLETHREAD) 
			pthread_preempt();
		return;
	}
#endif
#ifdef LATENCY_THREAD
	if (hiprio) {
		if (hiprio->runq.next != 0)
			hiprio_missed++;
		else {
			if (PREEMPT_ENABLE == 0)
				hiprio_delayed++;
			pthread_sched_setrunnable(hiprio);
			softint_request(SOFTINT_TIMEOUT);
		}
	} else
#endif
	if (CURPTHREAD() != IDLETHREAD)
		softint_request(SOFTINT_TIMEOUT);
#endif
	return;
}

/*
 * Handle an async interrupt request. 
 */
void
pthread_softint_handler(void *arg)
{
	int	switch_mode = threads_switch_mode;

	if (! PREEMPT_ENABLE) {
		if (switch_mode & SOFTINT_TIMEOUT)
			PREEMPT_NEEDED = 1;
		threads_switch_mode = 0;
		return;
	}

	assert_interrupts_enabled();
	disable_interrupts();

	/* XXX reenable softints before context switching */
	osenv_softintr_enable();

	threads_switch_mode = 0;
	
	if (switch_mode & SOFTINT_TIMEOUT) {
		PCOUNT(pthread_counts.softint_preempt_count++);
		pthread_preempt();
	}
	else if (switch_mode & SOFTINT_ASYNCREQ) {
		PCOUNT(pthread_counts.softint_yield_count++);
		pthread_yield();
	}
	
	osenv_softintr_disable();

	assert_interrupts_disabled();
	enable_interrupts();
}

#ifdef SMP
void
pthread_smp_booted(void *ignored)
{
	printf("pthread_smp_booted: CPU %d\n", smp_find_cur_cpu());

	splpreempt();
	
	/* allow receiving IPI */
	smp_message_pass_enable[smp_find_cur_cpu()] = 1;
	
	pthread_lock(&threads_smpboot_lock);
	thread_switch(IDLETHREAD, &threads_smpboot_lock, CURPTHREAD());

	printf("pthread_smp_booted: CPU %d returned\n", smp_find_cur_cpu());

	exit(1);
}

void
pthread_ipi_handler(void *ignored)
{
	/* ack the interrupt */
	smp_apic_ack();

	pthread_interrupt();
}
#endif

int
oskit_pthread_whichcpu(void)
{
	return THISCPU;
}

#ifdef  RTSCHED_STATS
static void
dump_stats(void)
{
#define P(x)	pthread_counts.x
	
	printf("Threads Stats:\n\n");
	printf("\tsoftint_preempt_count:      %d\n", P(softint_preempt_count));
	printf("\tsoftint_yield_count:        %d\n", P(softint_yield_count));
	printf("\trealtime_clockticks:        %d\n", P(realtime_clockticks));
	printf("\tstandard_clock_ticks:       %d\n", P(standard_clockticks));
	printf("\thandler_latency (total):    %d\n",
	       (int) P(handler_latency_total));
	printf("\thandler_latency (min):      %d\n",
	       (int) P(handler_latency_min));
	printf("\thandler_latency (max):      %d\n",
	       (int) P(handler_latency_max));
	printf("\thandler_latency (avg):      %d\n",
	       (P(realtime_clockticks) ?
		((int) (P(handler_latency_total) / P(realtime_clockticks)))
		: 0));
	printf("\n");
}
#endif
