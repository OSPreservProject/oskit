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
 * A silly scheduler test program for the Stride scheduler.
 */

#include <stdlib.h>
#include <stdio.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/threads/pthread.h>
#include <oskit/dev/timer.h>
#define DELAY		(60000 * 2)

/*
 * Include this file in your main.c to drag in the stride scheduler.
 * Do not include multiple times!
 */
#include <oskit/threads/sched_stride.h>

/*
 * Cancelation handler.
 */
void		thread_canceled(void *arg);

int verbose = 1;

/*
 * Just spin wildly. Each thread should get an amount proportional to
 * their ticket value, as evidenced by the printout at the end which
 * gives you the amount of CPU time burned by each thread.
 */
void *
Thread1(void *arg)
{
	int		me    = (int) arg;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "T1");

	if (verbose)
		printf("T1(%d/%d): Start at time: %d\n",
		       me, (int) self, oskit_pthread_realtime());

	oskit_pthread_sleep(60);

	while (1) {
		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Alternate spinning and sleeping, with each thread doing equal amounts
 * in equal proportion to their ticket values. This is to prove that
 * the scheduler can handle threads dynamically leaving and joining the set.
 */
void *
Thread2(void *arg)
{
	int		me    = (int) arg;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "T2");

	if (verbose)
		printf("T1(%d/%d): Start at time: %d\n",
		       me, (int) self, oskit_pthread_realtime());

	sched_yield();

	while (1) {
		int	end = oskit_pthread_cputime(self) + 60;

		while (oskit_pthread_cputime(self) < end)
			;
		
		pthread_testcancel();
		/*
		 * Everyone sleeps an amount proportional to thier tickets
		 */
		oskit_pthread_sleep(10);
		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
	return 0;
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
	pthread_attr_init(&threadattr);

	param.tickets = 1000;
	pthread_attr_setschedparam(&threadattr, &param);
	pthread_attr_setschedpolicy(&threadattr, SCHED_STRIDE);

	/*
	 * Change ourself to the STRIDE scheduler so that we get enough
	 * CPU time to run.
	 */
	pthread_setschedparam(pthread_self(), SCHED_STRIDE, &param);

	/*
	 * Create some threads, each with different ticket amounts.
	 * The threads should compute in ratios proportional to these
	 * ticket amounts. 
	 */
	for (i = 0; i < 4; i++) {
		param.tickets = 100 * (i+1);
		pthread_attr_setschedparam(&threadattr, &param);

		printf("Creating thread with %d tickets\n", param.tickets);

		pthread_create(&tids[ntid++],
			       &threadattr, Thread2, (void *) i);
	}

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

	for (i = 0; i < ntid; i++)
		printf("Thread %d burned %d cputime\n", i,
		       oskit_pthread_cputime(tids[i]));

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

