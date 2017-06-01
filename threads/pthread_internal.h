/*
 * Copyright (c) 1996, 1998-2000 University of Utah and the Flux Group.
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
 * Internal definitions for threads package.
 */
#ifndef	_PTHREAD_INT_H_
#define _PTHREAD_INT_H_

#ifndef ASSEMBLER
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

/*
 * Turn on deadlock detection in spinlocks and interrupt mask checks.
 * Also turn on stackguards and deadlock detection.
 */
#ifdef  DEBUG
#undef  THREADS_DEBUG
#define THREADS_DEBUG
#endif

#if (defined(SMP) || defined(THREADS_DEBUG)) && !defined(OSKIT_ARM32)
#undef  THREADS_SPINLOCKS
#define THREADS_SPINLOCKS
#endif

#ifdef	USERMODE
#define OSKIT_INLINE	static inline
#endif

#include <oskit/compiler.h>	/* includes config.h */
#include <oskit/base_critical.h>

#include "machine/pcb.h"

#include <oskit/queue.h>
#include <oskit/debug.h>
#include <oskit/com/listener.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/dev/timer.h>
#include <oskit/dev/softirq.h>
#include <oskit/threads/pthread.h>
#include <threads/pthread_locking.h>
#ifdef  CPU_INHERIT
#include <threads/cpuinherit/pthread_cpuinherit.h>
#endif

#ifdef	SMP
#include <oskit/smp.h>
#endif

#if defined(RTSCHED_STATS) || defined(SCHED_STATS) || defined(IPC_STATS) || defined(THREAD_STATS)
#include "pthread_stats.h"
#else
#define PCOUNT(x)
#endif

/*
 * Thread structure. The PCB is stored in the thread structure, but
 * accessed indirectly to leave room for future expansion.
 */
typedef struct pthread_thread {
	queue_chain_t		runq;		/* Runq/free link */
	pcb_t			*ppcb;		/* Pointer to PCB */
	void			*pstk;		/* Pointer to stack alloc */
	void			*(*func)(void *);/* Start function */
	void			*cookie;	/* Argument to start func */
	size_t			ssize;		/* Allocated stack size */
	size_t			guardsize;	/* Stack Guard size */
	pthread_t		tid;		/* Thread ID */
	oskit_u32_t		flags;		/* Thread state flags */
	pthread_lock_t		lock;		/* Lock */
	queue_chain_t		chain;		/* Queuing element */
	int			preempt;	/* Preempt this thread */

	/*
	 * The exit code is painful. Use a mutex/cond pair and special dead
	 * flag. This avoids specialized handling in the cancel and signal
	 * code (looks just like a condwait). 
	 */
	pthread_mutex_t		mutex;		/* Protect dead flag */
	pthread_cond_t		cond;		/* Wait for dead flag=1 */
	oskit_u32_t		exitval;	/* Exit value for joiner */
	int			dead;		/* Thread is finally dead */

	/*
	 * Resource accounting stuff. Added for the demo!
	 */
#ifdef HIGHRES_THREADTIMES
	long long		rstamp;		/* start of run timestamp */
	long long		cpucycles;	/* accumulated CPU cycles */
#endif
	int			cputime;	/* Total ticks consumed */
	int			cpticks;	/* Ticks in last second */
	oskit_u32_t		pctcpu;		/* Percent of CPU usage */
	int			childtime;	/* Accumulation of child CPU */

	/*
	 * To implement cancelation and signals, must know the object
	 * the thread is blocked on. This set of fields is protected
	 * by its own lock to avoid having to take the pthread lock
	 * everywhere.
	 */
	pthread_lock_t		waitlock;	/* Lock */
	oskit_u32_t		waitflags;	/* Wait state flags */
	pthread_cond_t		*waitcond;	/* Condition variable */
	struct osenv_sleeprec	*sleeprec;	/* pthread in an osenv_sleep */

	/*
	 * IPC stuff.
	 */
	struct {
		void		        *msg;		/* Message pointer */
		oskit_size_t		msg_size;
		pthread_t		tid;
		void			*reply;
		oskit_size_t		reply_size;
		queue_head_t		senders;
		queue_chain_t		senders_chain;
	} ipc_state;

	/*
	 * Timed condition wait is implemented using a clock timer.
	 */
	struct oskit_timer	*condtimer;

	/*
	 * pthread sleep/wakeup is implemented using a separate clock timer.
	 */
	struct oskit_timer	*sleeptimer;

	/*
	 * Signal stuff.
	 */
	pthread_lock_t		siglock;	/* Protect signal state */
	sigset_t		sigmask;	/* Blocked signals */
	sigset_t		sigpending;	/* Pending signals */
	sigset_t		sigwaiting;	/* Waiting for signals */
	oskit_u32_t		eip;		/* For page fault handling */

	/*
	 * Keys values are currently a fixed array.
	 */
	void			*keyvalues[PTHREAD_KEYS_MAX]; /* Key/Value */

	/*
	 * Cancelation state and cleanup handlers.
	 */
	pthread_cleanup_t	*cleanups;	/* Cleanup handlers chain */
	char			cancelstate;	/* Cancelation State */
	char			canceltype;	/* Cancelation Type */

	/*
	 * The scheduler uses a separate lock.
	 */
	pthread_lock_t		schedlock;

#ifdef  RTAI_SCHEDULER
	void			*rtai_priv;
	int			policy;		/* Scheduling Policy */
	int			priority;	/* Current Priority */
#endif
#ifdef  DEFAULT_SCHEDULER
	struct scheduler_entry	*scheduler;	/* Scheduler entry */
	int			policy;		/* Scheduling Policy */
	int			priority;	/* Current Priority */
	int			base_priority;  /* Original priority */
	int			ticks;		/* Scheduling ticks left */
#ifdef  PTHREAD_SCHED_STRIDE
	int			tickets;
	int			stride;
	int			remain;
	long long		pass;
	long long		start;
#endif
#ifdef  PTHREAD_REALTIME
	/*
	 * Should do for most things, but add as needed.
	 */
	oskit_timespec_t	start;		/* Time of next run */
	oskit_timespec_t	deadline;	/* Time of next run */
	oskit_timespec_t	period;		/* Delay between runs */
#endif
#endif
#ifdef  PRI_INHERIT
	/*
	 * Priority inheritance support.
	 */
	queue_head_t		waiters;	/* Threads waiting on me */
	queue_chain_t		waiters_chain;	/* Waiters queueing element */
	struct pthread_thread	*waiting_for;	/* Thread being waited for */
	struct pthread_thread	*inherits_from;	/* Thread inheriting from */
#endif
#ifdef  CPU_INHERIT
	/*
	 * CPU inheritance support.
	 */
	struct pthread_thread   *scheduler;	/* The threads scheduler */
	schedmsg_t		unblockmsg;
	schedmsg_queue_t	*msgqueue;
	sched_wakecond_t	wakeup_cond;	/* Wakeup condition */
	schedflags_t		schedflags;	/* Flags */
	int			donate_rc;	/* Return code from donation */
	int			timeout;	/* In milliseconds */
	queue_head_t		donors;		/* Threads donating to me */
	queue_chain_t		donors_chain;	/* Donors queueing element */
	struct pthread_thread	*donating_to;	/* Thread being donated to */
	struct pthread_thread	*inherits_from;	/* Thread inheriting from */
	struct pthread_thread	*nextup;	/* Next thread to run. Chain */
#endif
#ifdef	THREAD_STATS
	struct pthread_stats	stats;
#endif
	pcb_t			pcb;		/* Actual PCB */
} pthread_thread_t;
#define NULL_THREADPTR		((pthread_thread_t *) 0)

/*
 * These are generic flags. They can be modified once preemptions are
 * disabled and the pthread lock is held.
 */
#define THREAD_EXITING		0x00001
#define THREAD_DETACHED		0x00004
#define THREAD_CANCELED		0x00008
#define THREAD_KILLED		0x00010
#define THREAD_USERSTACK	0x00020

/*
 * These are the waitstate flags. They are modified from interrupt handlers,
 * so interrupts must be disabled and the pthread waitlock held.
 */
#define THREAD_WS_SLEEPING	0x00001		/* These must be disjoint */
#define THREAD_WS_TIMEDOUT	0x00002
#define THREAD_WS_TIMERSET	0x00004
#define THREAD_WS_SLEEPFLAGS    0x00007

#define THREAD_WS_CONDWAIT	0x00010
#define THREAD_WS_OSENVSLEEP	0x00020
#define THREAD_WS_CPUIRECV_WAIT	0x00040
#define THREAD_WS_SIGWAIT	0x00080
#define THREAD_WS_IPCSEND_WAIT	0x01000
#define THREAD_WS_IPCRECV_WAIT	0x02000
#define THREAD_WS_IPCREPL_WAIT	0x04000
#define THREAD_WS_IPCWAIT_FLAG	0x07000
#define THREAD_WS_SEMWAIT	0x08000

/*
 * When a thread gives up the CPU, tell the reschedule code why. This allows
 * for more control with regards to scheduling policy.
 */
typedef enum {
	RESCHED_PREEMPT		= 1,	/* Time based preemption yield */
	RESCHED_YIELD,			/* Involuntary yield */
	RESCHED_USERYIELD,		/* User directed yield */
	RESCHED_BLOCK,			/* Waiting for a resource (mutex) */
	RESCHED_INTERNAL,		/* Internal rescheduling call */
} resched_flags_t;

/* Default scheduling interval for SCHED_RR (in ticks) */
#define SCHED_RR_INTERVAL	1

/*
 * Currently executing thread and idlethreads.
 */
extern pthread_thread_t		*threads_curthreads[];
extern pthread_thread_t		*threads_idlethreads[];

#ifdef SMP
#define CURPTHREAD()		threads_curthreads[smp_find_cur_cpu()]
#define SETCURPTHREAD(p)	threads_curthreads[smp_find_cur_cpu()] = (p)
#define IDLETHREAD		threads_idlethreads[smp_find_cur_cpu()]
#else
#define CURPTHREAD()		threads_curthreads[0]
#define SETCURPTHREAD(p)	threads_curthreads[0] = (p)
#define IDLETHREAD		threads_idlethreads[0]
#endif

/*
 * Interrupts.
 *
 * Include machine dependent method for dealing with interrupt control
 */
#include "machine/interrupt.h"

/*
 * Define INLINED_INTR_FLAGS and you get and you will get the inlinable 
 * versions of these functions used throughout the threads code, unless
 * profiling or doing realtime. Breaks UNIX mode emulation, though.
 */
#if !defined(INLINED_INTR_FLAGS) || defined(GPROF) || defined(PTHREAD_REALTIME)
#define disable_interrupts()		osenv_intr_disable()
#define enable_interrupts()		osenv_intr_enable()
#define interrupts_enabled()		osenv_intr_enabled()
#define save_disable_interrupts()	osenv_intr_save_disable()
#define save_interrupt_enable(f)	(f) = osenv_intr_enabled()
#define restore_interrupt_enable(f)	if (f) osenv_intr_enable()
#define assert_interrupts_enabled()	assert(osenv_intr_enabled())
#define assert_interrupts_disabled()	assert(!osenv_intr_enabled())
#else
#define disable_interrupts()		machine_intr_disable()
#define enable_interrupts()		machine_intr_enable()
#define interrupts_enabled()		machine_intr_enabled()
#define save_disable_interrupts()	machine_intr_save_disable()
#define save_interrupt_enable(f)	(f) = machine_intr_enabled()
#define restore_interrupt_enable(f)	if (f) enable_interrupts()
#define assert_interrupts_enabled()	assert(machine_intr_enabled())
#define assert_interrupts_disabled()	assert(!machine_intr_enabled())
#endif

/*
 * Preemption.
 */
extern int			threads_preempt_enable[];
extern int			threads_preempt_needed[];
extern int			threads_preempt_ready;

#ifdef SMP
#define PREEMPT_ENABLE		threads_preempt_enable[smp_find_cur_cpu()]
#define PREEMPT_NEEDED		threads_preempt_needed[smp_find_cur_cpu()]
#else
#define PREEMPT_ENABLE		threads_preempt_enable[0]
#define PREEMPT_NEEDED		threads_preempt_needed[0]
#endif

#define disable_preemption()						\
	(PREEMPT_ENABLE = 0)
#define enable_preemption()						\
	(PREEMPT_ENABLE = 1,						\
	 (PREEMPT_NEEDED && interrupts_enabled()) ?			\
				pthread_yield() : (void)0)
#define save_preemption_enable(flag)					\
	((flag) = PREEMPT_ENABLE)
#define restore_preemption_enable(flag)					\
	((flag) ? enable_preemption() : (void)0)
#define check_preemption()						\
	 (PREEMPT_ENABLE && PREEMPT_NEEDED && interrupts_enabled()) ?	\
				pthread_yield() : (void)0)

#define assert_preemption_disabled() \
	assert(!threads_preempt_ready || !PREEMPT_ENABLE)
#define assert_preemption_enabled() \
	assert(!threads_preempt_ready || PREEMPT_ENABLE)

/*
 * Scheduler definitions. The scheduler module needs to define these
 * symbols.
 */
void		pthread_init_scheduler(void);
int		pthread_sched_reschedule(resched_flags_t reason,
				pthread_lock_t *lock);
void		pthread_sched_init_schedstate(pthread_thread_t *pthread,
				int policy, const struct sched_param *param);
int		pthread_sched_setrunnable(pthread_thread_t *pthread);
int		pthread_sched_change_state(pthread_thread_t *pthread,
				int policy, const struct sched_param *param);
void		pthread_sched_handoff(int waitstate,
				pthread_thread_t *pnext);
void		pthread_sched_priority_transfer_undo(pthread_thread_t *p);
void		pthread_sched_priority_transfer_and_wait(pthread_thread_t *p,
				pthread_lock_t *plock);

/*
 * The maximum number of allowed threads.
 */
#define THREADS_MAX_THREAD	512

/*
 * The array of thread structure pointers, indexed by TID.
 */
extern pthread_thread_t		*threads_tidtothread[];

OSKIT_INLINE pthread_thread_t *
tidtothread(pthread_t tid)
{
	int		 realid = (int) tid;
	pthread_thread_t *pthread;

	if (realid < 0 || realid >= THREADS_MAX_THREAD ||
	    ((pthread = threads_tidtothread[realid]) == NULL_THREADPTR))
		return NULL;

	return pthread;
}

/*
 * Callouts.
 */
extern void		       *(*threads_allocator)(size_t);
extern void			(*threads_deallocator)(void *);

/*
 * Key table stuff.
 */
struct keytable {
	int		inuse;
	void		(*destructor)(void *);
};

extern pthread_lock_t		threads_key_lock;
extern struct keytable		threads_key_table[];

OSKIT_INLINE int
validkey(int key)
{
	return (key >= 0 && key < PTHREAD_KEYS_MAX &&
		threads_key_table[key].inuse);
}

/*
 * The switch routine is dependent on user/real mode operation, so its a
 * function pointer.
 */
extern void	  (*thread_switch)(pthread_thread_t *pnext,
				   pthread_lock_t *l, pthread_thread_t *cur);

/*
 * Internal Prototypes.
 */
pthread_thread_t *pthread_init_mainthread(pthread_thread_t *pthread);
void		  thread_setup(pthread_thread_t *pthread);
void		  thread_destroy(pthread_thread_t *pthread);
void	         *pthread_alloc_memory(size_t size);
int		  pthread_mprotect(oskit_addr_t addr, size_t length,
			int writeable);
void		  pthread_dealloc_memory(void *pmem);
void		  *pthread_idle_function(void *argument);
pthread_thread_t *pthread_create_internal(void *(*function)(void *),
			void *argument, const pthread_attr_t *pattr);
void		  pthread_destroy_internal(pthread_thread_t *pthread);
void		  pthread_block_onQ(queue_head_t *q, pthread_lock_t *plock);
pthread_thread_t *pthread_dequeue_fromQ(queue_head_t *q);
pthread_thread_t *pthread_remove_fromQ(queue_head_t *q, pthread_thread_t *pth);
void		  pthread_resume_allQ(queue_head_t *q);
void		  pthread_blockints(void);
void		  pthread_unblockints(void);
int		  pthread_wakeup_locked(pthread_thread_t *pthread);
int		  pthread_wakeup_unlocked(pthread_thread_t *pthread);
pthread_thread_t *pthread_current(void);
int		  pthread_setprio_internal(pthread_thread_t *pthread, int pri);
void		  pthread_preempt(void);
void		  pthread_yield(void);
int		  threads_stack_back_trace(int tid, int max_st_levels);
void		  thread_getstate(pthread_thread_t *pth, pthread_state_t *pst);
void		  pthread_call_key_destructors(void);
void		  pthread_call_cleanup_handlers(void);
void		  osenv_process_release(void);
void		  dump_all_threads(void);
void		  pthread_delay(void);

int		  pthread_cond_wait_safe(pthread_cond_t *c,
			pthread_mutex_t *m);

void		  pthread_exit_locked(void *status) OSKIT_NORETURN;
void		  pthread_reap_threads(void);

int		  pthread_init_comlock(void);
void		  pthread_init_attributes(void);
void		  pthread_init_guard(void);
void		  pthread_init_libc_locks(void);
void		  pthread_init_osenv_sleep(void);
void		  pthread_init_osenv_intr(void);
void		  pthread_init_keytable(void);
void		  pthread_init_process_lock(void);
void		  pthread_init_fs_sleep(void);
void		  pthread_init_exit(void);
void		  pthread_init_signals(void);
void		  pthread_ipc_cancel(pthread_thread_t *pthread);
void		  oskit_init_libc(void);
void		  thread_machdep_init(void);
oskit_error_t	  pthread_register_interface(void);
int		  oskit_pthread_sleep_withflags(int waitflags,
			const oskit_timespec_t *timeout);
int		  sigqueue_internal(pid_t pid, int signo,
				    const union sigval value, int code);

/*
 * Internal mutex prototypes.
 */
void		  pthread_mutex_panic(pthread_mutex_t *m,
			char *t, char *msg);

#ifndef PAGE_SIZE
#define PAGE_SIZE	0x1000
#endif

#ifdef SMP
#define MAXCPUS		8
#define THISCPU		(smp_find_cur_cpu())
#else
#define MAXCPUS		1
#define THISCPU		0
#endif

#ifdef THREADS_DEBUG
#undef  STACKGUARD
#define STACKGUARD

/*
 * Deadlock detection.
 */
extern int		threads_sleepers;
extern pthread_lock_t	threads_sleepers_lock;
#endif

#ifdef	STACKGUARD
#define DEFAULT_STACKGUARD	PAGE_SIZE
#else
#define DEFAULT_STACKGUARD	0
#endif

extern int		threads_debug;
extern pthread_thread_t threads_mainthread;

/*
 * Soft interrupt support.
 *
 * There are two situations in which a softint handler will be requested.
 * The first is from the time-based preemption code. The second is
 * when a wakeup operation (resume or timed condtion timeout) occurs,
 * and the priorities have changed, requiring a reschedule operation.
 */
extern int			threads_switch_mode;

#define SOFTINT_TIMEOUT		0x01
#define SOFTINT_ASYNCREQ	0x02

/*
 * Seems kinda non-portable, don't ya think?
 */
#include <oskit/machine/base_irq.h>

/* Ack */
#define IN_AN_INTERRUPT()	\
	(base_irq_nest & ~(BASE_IRQ_SOFTINT_CLEARED|BASE_IRQ_SOFTINT_DISABLED))

OSKIT_INLINE void
softint_request(int type)
{
	threads_switch_mode |= type;
	osenv_softirq_schedule(OSENV_SOFT_IRQ_PTHREAD);
}

OSKIT_INLINE void
queue_check(queue_head_t *queue, pthread_thread_t *pthread)
{
	pthread_thread_t	*pnext;

	queue_iterate(queue, pnext, pthread_thread_t *, chain) {
		if (pnext == pthread)
			panic("queue_check");
	}
}

#ifdef THREADS_DEBUG
#define DPRINTF(fmt, args... ) \
	{ \
		if (threads_debug) \
			printf(__FUNCTION__  ":" fmt , ## args); \
	}
#else
#define DPRINTF(fmt, args... )
#endif

#endif /* ASSEMBLER */

/*
 * Define assembly language constants.
 */
#define THREAD_NEXT		0x0
#define THREAD_PREV		0x4
#define THREAD_PPCB		0x8

#endif /* _THREADS_INT_H_ */
