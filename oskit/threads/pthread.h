/*
 * Copyright (c) 1996, 1998-2001 University of Utah and the Flux Group.
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
 * External definitions for threads package.
 */
#ifndef	_OSKIT_THREADS_PTHREAD_H
#define _OSKIT_THREADS_PTHREAD_H

#include <oskit/types.h>
#include <oskit/time.h>
#include <oskit/queue.h>
#include <oskit/machine/spin_lock.h>
#include <sys/signal.h>
#include <oskit/threads/sched.h>

#define _POSIX_THREADS
#define _POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_PRIORITY_SCHEDULING
/* #define _POSIX_THREAD_PRIO_INHERIT   */
/* #define _POSIX_THREAD_PRIO_PROTECT   */
/* #define _POSIX_THREAD_PROCESS_SHARED */
#define _POSIX_THREAD_SAFE_FUNCTIONS

/*
 * Thread creation inheritance state.
 */
enum {
	PTHREAD_INHERIT_SCHED   = 1,
	PTHREAD_EXPLICIT_SCHED  = 2,
};

/*
 * Detach state.
 */
enum {
        PTHREAD_CREATE_JOINABLE = 1,
        PTHREAD_CREATE_DETACHED = 2,
};

/*
 * Thread object. The implementation is opaque.
 */
typedef void 	*pthread_t;

/*
 * Thread attributes object.
 */
struct pthread_attr {
        int		detachstate;
	int		priority;
	int		stacksize;
	void		*stackaddr;
	int		guardsize;
	int		policy;
	int		inherit;
	/*
	 * CPU Inheritance attributes. The opaque data is used to pass
	 * info to the scheduler, like tickets or reservation. It will
	 * do for now.
	 */
	pthread_t	scheduler;
	oskit_u32_t	opaque;
	/*
	 * More traditional realtime support.
	 */
	oskit_timespec_t	start;
	oskit_timespec_t	deadline;
	oskit_timespec_t	period;
	/*
	 * Stride and maybe lottery.
	 */
	int		tickets;
};

/*
 * Mutex object. The implementation is hidden, and allocated on demand
 * so that a static initializer can be used.
 */
struct pthread_mutex_impl;

struct pthread_mutex {
	struct pthread_mutex_impl	*impl;	/* implementation pointer */
};
typedef struct pthread_mutex pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER       { NULL }

/*
 * Mutex attributes object.
 */
struct pthread_mutexattr {
        int		priority_inherit;
	int		type;
};

/*
 * Mutex protocol attributes.
 */
enum {
	PTHREAD_PRIO_NONE	= 0,
	PTHREAD_PRIO_INHERIT,
};

/*
 * Mutex type attributes.
 */
enum {
	PTHREAD_MUTEX_NORMAL	= 0,
	PTHREAD_MUTEX_ERRORCHECK,
	PTHREAD_MUTEX_RECURSIVE,
	PTHREAD_MUTEX_DEFAULT,
};

/*
 * Condition variable object. The implementation is hidden, and
 * allocated on demand so that a static initializer can be used.
 */
struct pthread_cond_impl;

struct pthread_cond {
	struct pthread_cond_impl	*impl;	/* implementation pointer */
};
typedef struct pthread_cond pthread_cond_t;

#define PTHREAD_COND_INITIALIZER       { NULL }

/*
 * Condition variable attributes object.
 */
struct pthread_condattr {
        int attr;
};

typedef struct pthread_attr pthread_attr_t;
typedef struct pthread_mutexattr pthread_mutexattr_t;
typedef struct pthread_condattr pthread_condattr_t;

/*
 * Default attributes.
 */
extern pthread_attr_t		pthread_attr_default;
extern pthread_mutexattr_t	pthread_mutexattr_default;
extern pthread_condattr_t	pthread_condattr_default;

/*
 * Keys are just small ints (for now). Not the right way to do this.
 */
typedef int			pthread_key_t;
#define PTHREAD_KEYS_MAX	128

/*
 * Minimum stack size.
 */
#define PTHREAD_STACK_MIN	(5 * 0x1000)

/*
 * Cancel state stuff.
 * According to ANSI/IEEE Std 1003.1, these must be macros.
 */
#define	PTHREAD_CANCEL_ENABLE		0
#define	PTHREAD_CANCEL_DISABLE 		1
#define	PTHREAD_CANCEL_DEFERRED		2
#define	PTHREAD_CANCEL_ASYNCHRONOUS	3

#define PTHREAD_CANCELED	OSKIT_ECANCELED

/*
 * Cleanup stuff.
 */
/*
 * Cleanups are lexically scoped, and so the data structure can
 * be placed on the stack. This implements a backward chained
 * list of cleanups to be processed if the thread ends up in exit.
 */
typedef struct pthread_cleanup_data {
	void		(*func)(void *);	/* Function to call.  */
	void		*arg;			/* Its argument.  */
	struct pthread_cleanup_data *prev;	/* Chain of cleanups.  */
} pthread_cleanup_t;

/*
 * These macros handle pushing and popping cleanup handlers. The push routine
 * opens a lexical scope, while the pop routine closes that scope. This will
 * lead to slightly obscure compiler errors if the pushes and pops are not
 * matched properly.
 */
#define pthread_cleanup_push(func, arg)					\
	{								\
		pthread_cleanup_t _cleanup;				\
									\
		oskit_pthread_cleanup_push(&_cleanup, (func), (arg));

#define pthread_cleanup_pop(execute)					\
	oskit_pthread_cleanup_pop(&_cleanup, (execute));		\
	}

/*
 * Internal functions to handle the actual details of push/pop.
 */
void		oskit_pthread_cleanup_push(struct pthread_cleanup_data *data,
			void (*routine)(void *), void *arg);
void		oskit_pthread_cleanup_pop(struct pthread_cleanup_data *data,
			int execute);

/*
 * This is a local hack to get the current thread state.
 */
struct pthread_state {
	oskit_size_t	stacksize;
	oskit_u32_t	stackbase;	/* Pointer to base of memory alloc */
	oskit_u32_t	stackptr;
	oskit_u32_t	frameptr;
};
typedef struct pthread_state pthread_state_t;

/*
 * API Prototypes.
 */
void		pthread_init(int preemptible);
void		pthread_init_withhooks(int preemptible,
			void *(*allocator)(oskit_size_t),
			void (*deallocator)(void*));
int		pthread_create(pthread_t *tid, const pthread_attr_t *pattr,
			void *(*function)(void *),
			void *argument);
int		pthread_detach(pthread_t tid);
int		pthread_join(pthread_t tid, void **status);
int		pthread_cancel(pthread_t tid);
int		pthread_attr_init(pthread_attr_t *pattr);
int		pthread_attr_setstacksize(pthread_attr_t *pattr,
			oskit_size_t threadStackSize);
int		pthread_attr_getstacksize(const pthread_attr_t *attr,
					  oskit_size_t *stacksize);
int		pthread_attr_setstackaddr(pthread_attr_t *attr,
					  void *stackaddr);
int		pthread_attr_getstackaddr(const pthread_attr_t *attr,
					  void **stackaddr);
int		pthread_attr_setguardsize(pthread_attr_t *attr,
			oskit_size_t guardsize);
int		pthread_mutexattr_init(pthread_mutexattr_t *attr);
int		pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int		pthread_condattr_init(pthread_condattr_t *attr);
int		pthread_condattr_destroy(pthread_condattr_t *attr);
int		pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr,
			int inherit);
int		pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int		pthread_setschedparam(pthread_t tid, int policy,
			const struct sched_param *param);
int		pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int		pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int		pthread_attr_setschedparam(pthread_attr_t *attr,
			const struct sched_param *param);
int		pthread_attr_setscheduler(pthread_attr_t *attr, pthread_t tid);
int		pthread_kill(pthread_t tid, int sig);

#ifndef NOPTHREADPROTOS
int		pthread_cond_init(pthread_cond_t *c,
			const pthread_condattr_t *attr);
int		pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m);
int		pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m,
			oskit_timespec_t *abstime);
int		pthread_cond_destroy(pthread_cond_t *c);
int		pthread_cond_signal(pthread_cond_t *c);
int		pthread_cond_broadcast(pthread_cond_t *c);

void		pthread_exit(void *status) OSKIT_NORETURN;
int		pthread_key_create(pthread_key_t *key,
			void (*destructor)(void *));
int		pthread_key_delete(pthread_key_t key);

int             pthread_mutex_init(pthread_mutex_t *m,
			const pthread_mutexattr_t *attr);
int		pthread_mutex_lock(pthread_mutex_t *m);
int		pthread_mutex_trylock(pthread_mutex_t *m);
int		pthread_mutex_unlock(pthread_mutex_t *m);
int		pthread_mutex_destroy(pthread_mutex_t *m);

pthread_t	pthread_self(void);
int		pthread_setcancelstate(int state, int *oldstate);
int		pthread_setcanceltype(int type, int *oldtype);
int		pthread_setspecific(pthread_key_t key, const void *value);
void	       *pthread_getspecific(pthread_key_t key);
int		pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
void		pthread_testcancel(void);
void		sched_yield(void);
#endif

/*
 * Local stuff, not part of the POSIX API.
 */
int		oskit_pthread_attr_setprio(pthread_attr_t *pattr, int pri);
int		oskit_pthread_attr_setopaque(pthread_attr_t *attr,
			oskit_u32_t op);
#ifndef NOPTHREADPROTOS
int		oskit_pthread_sleep(oskit_s64_t millis);
int		oskit_pthread_wakeup(pthread_t tid);
int		oskit_pthread_setprio(pthread_t tid, int prio);
int		oskit_pthread_getstate(pthread_t tid, pthread_state_t *pstate);
#endif

int		oskit_pthread_cpucycles(pthread_t tid, long long *cycles);
oskit_u32_t	oskit_pthread_cputime(pthread_t tid);
oskit_u32_t	oskit_pthread_realtime(void);
oskit_u32_t	oskit_pthread_childtime(pthread_t tid);
int		oskit_pthread_whichcpu(void);

struct oskit_socket_factory;
void		pthread_init_socketfactory(struct oskit_socket_factory *sf);

int		oskit_pthread_setspecific(pthread_t tid, pthread_key_t key,
			const void *value);
void	       *oskit_pthread_getspecific(pthread_t tid, pthread_key_t key);

typedef void (*oskit_pthread_create_hook_t)(pthread_t tid);
oskit_pthread_create_hook_t
	oskit_pthread_create_hook_set(oskit_pthread_create_hook_t hook);

typedef void (*oskit_pthread_destroy_hook_t)(pthread_t tid);
oskit_pthread_destroy_hook_t
	oskit_pthread_destroy_hook_set(oskit_pthread_destroy_hook_t hook);

typedef void (*oskit_pthread_csw_hook_t)(void);
oskit_pthread_csw_hook_t
	oskit_pthread_aftercsw_hook_set(oskit_pthread_csw_hook_t hook);

/*
 * Timer. The resolution is 10ms.
 */
#define PTHREAD_HZ	100
#define PTHREAD_TICK	(1000/PTHREAD_HZ)
#endif /* _OSKIT_THREADS_PTHREAD_H */
