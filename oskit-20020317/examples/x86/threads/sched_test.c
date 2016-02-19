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
 * A silly scheduler test program. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/threads/pthread.h>
#include <oskit/dev/timer.h>
#define DELAY		2000
#define TICKS2NANO(x)	(x * 1000000)

/*
 * Include this file in your main.c to drag in the realtime scheduler.
 * Do not include multiple times!
 */
#include <oskit/threads/sched_edf.h>

/*
 * A shared mutex to force priority inversion, and see if we recover.
 */
pthread_mutex_t shared_mutex;
int		shared_data;

/*
 * Cancelation handler.
 */
void		thread_canceled(void *arg);

/*
 * Realtime Clock Handler and local clock time.
 */
static void		realtime_handler(void *data);
static volatile int	realtime_ticks;

int verbose = 1;

/*
 * Low priority round robin thread. Runs for a short time, holding the
 * mutex. Then sleeps until the next time. 
 */
void *
LowPri(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "LPri");

	if (verbose)
		printf("LPri(%d/%d): Start at time: %d\n",
		       me, (int) self, oskit_pthread_realtime());

	while (1) {
		int	end;

		if (verbose)
			printf("LPri(%d): Loop %d, %d,%d\n", me, loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		pthread_mutex_lock(&shared_mutex);
		shared_data = 1;

		end = realtime_ticks + 3;
		while (realtime_ticks <= end)
			;
		
		shared_data = 0;
		pthread_mutex_unlock(&shared_mutex);

		pthread_testcancel();
		oskit_pthread_sleep(60);
		pthread_testcancel();

		loops++;
	}

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Hi Priority Realtime thread. Runs for a short time, holding the
 * mutex. Then yields until the next deadline. Note that time is
 * counted using the realtime clock handler installed in main, and
 * the local notion of the number of clock ticks that have passed
 * while in this routine. 
 */
void *
HiPri(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "HPri");

	if (verbose)
		printf("HPri(%d/%d): Start at time: %d\n",
		       me, (int) self, oskit_pthread_realtime());

	while (1) {
		int	end;

		if (verbose)
			printf("HPri(%d): Loop %d, %d,%d\n", me, loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		pthread_mutex_lock(&shared_mutex);
		shared_data = 1;

		end = realtime_ticks + 1;
		while (realtime_ticks <= end)
			;
		
		shared_data = 0;
		pthread_mutex_unlock(&shared_mutex);

		/*
		 * Yield till next period.
		 */
		pthread_testcancel();
		sched_yield();
		pthread_testcancel();

		loops++;
	}

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Medium priority thread. Wakes up and computes for a while, then sleeps
 * for a while.
 */
void *
MedPri(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "MPri");

	if (verbose)
		printf("MPri(%d/%d): Start at time: %d\n",
		       me, (int) self, oskit_pthread_realtime());

	while (1) {
		int     end;

		if (verbose)
			printf("MPri(%d): Loop %d, %d,%d\n", me, loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		if (shared_data)
			printf("Mutex is locked\n");


		end = realtime_ticks + 7;
		while (realtime_ticks <= end)
			;

		oskit_pthread_sleep(100);

		loops++;
	}

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Realtime clock handler. Bump local time.
 */
static void
realtime_handler(void *data)
{
	realtime_ticks++;
}
	
/*
 * Cancelation handler
 */
void
thread_canceled(void *arg)
{
	char	*str = (char *) arg;
	
	printf("Thread %s(%d) exiting ... \n", str, (int) pthread_self());
}

int
main()
{
	pthread_attr_t		threadattr;
	pthread_mutexattr_t	mutexattr;
	int			i, ntid = 0;
	void			*stat;
	struct sched_param	param;
	pthread_t		tids[8];
	
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();
#ifdef GPROF
	start_fs_bmod();
	start_gprof();
#endif
	/*
	 * Allocate a realtime handler for the Clock IRQ. This allows
	 * us to do demonstrate something interesting, as well as
	 * count time, since the rest of the oskit time keeping
	 * mechanisms do not run until the realtime thread switches
	 * out. This is okay, since realtime threads should not care
	 * about wall clock time anyway.
	 */
	if ((osenv_irq_alloc(0, realtime_handler, 0, OSENV_IRQ_REALTIME)))
		panic("Could not allocate realtime handler!");

	pthread_attr_init(&threadattr);

	/*
	 * The mutex is initialized to do priority inheritance so that
	 * when the realtime thread is blocked by the low priority thread,
	 * the low priority picks gets it priority bumped so that it
	 * can hurry up and release the mutex.
	 */
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&shared_mutex, &mutexattr);

	/*
	 * Create the Low Pri thread.
	 */
	param.priority = PRIORITY_NORMAL;
	pthread_attr_setschedparam(&threadattr, &param);
	pthread_attr_setschedpolicy(&threadattr, SCHED_RR);
	
	pthread_create(&tids[ntid++], &threadattr, LowPri, (void *) 0);

	/*
	 * Create the Med Pri thread.
	 */
	param.priority = PRIORITY_NORMAL + 5;
	pthread_attr_setschedparam(&threadattr, &param);
	pthread_attr_setschedpolicy(&threadattr, SCHED_RR);
	
	pthread_create(&tids[ntid++], &threadattr, MedPri, (void *) 0);

	/*
	 * Create the High Pri (realtime) thread.
	 */
	param.start.tv_sec   = 0;
	param.start.tv_nsec  = TICKS2NANO(100);
	param.period.tv_sec  = 0;
	param.period.tv_nsec = TICKS2NANO(50);
	pthread_attr_setschedparam(&threadattr, &param);
	pthread_attr_setschedpolicy(&threadattr, SCHED_EDF);
	
	pthread_create(&tids[ntid++], &threadattr, HiPri, (void *) 0);

	/*
	 * Delay for a while while the threads prove themselves.
	 */
	oskit_pthread_sleep(DELAY);
	
	/*
	 * Kill threads.
	 */
	printf("Killing Threads\n");
	for (i = 0; i < ntid; i++)
		pthread_cancel(tids[i]);

	/*
	 * Wait for threads to die.
	 */
	printf("Joining Threads\n");
	for (i = 0; i < ntid; i++)
		pthread_join(tids[i], &stat);

	oskit_pthread_sleep((long long) 100);
	pthread_exit(0);
	return 0;
}

