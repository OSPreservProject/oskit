/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * A very simple CPU Inheritance Scheduler demonstration example.
 * To build this example, the threads directory will need to be
 * compiled with CPU_INHERIT defined. See the GNUmakerules in that
 * directory.
 *
 * The test runs for a fixed amount of time and then exits. Here is the
 * scheduling hierarchy:
 *             
              --- T1
             /
        --- RM
       /     \
      /       --- T2
     /
    /
   RS ----- LO -- T3
    \
     \
      \
       \
        --- RR -- T4

   RS - Root Scheduler
   RM - Rate Mono Scheduler
   LO - Lotto Scheduler
   RR - Round Robin Scheduler
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/dev/timer.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>

pthread_mutex_t shared_mutex;
int		shared_data;
pthread_t	tids[16];
int		ntid;
void		thread_canceled(void *arg);
int		verbose = 1;
int		tk = 0;

/*
 * High priority Rate Monotonic threads. Lock a shared resource.
 */
void
rmono_worker(void *arg)
{
	int		me  = (int) arg;
	int		duration, period;
	int		loops     = 0;
	int		end, now, next = 0, dur;
	pthread_t	self  = pthread_self();

	if (me == 0) {
		duration = 10;
		period   = 50;
	}
	else {
		duration = 10;
		period   = 80;
	}
	next = oskit_pthread_realtime();

	pthread_cleanup_push(thread_canceled, "RM");

	if (verbose)
		printf("RM(%d/%d): Start at time: %d\n",
		       me, (int) pthread_self(), oskit_pthread_realtime());

	if (tk)
		printf("||-a:graph linecolor the_graph tid%d_line green||\n"
		       "||-a:graph linename  the_graph tid%d_line RM%d||\n",
		       (int) pthread_self(), (int) pthread_self(), me);
	while (1) {

		/*
		 * Compute
		 */
		now = oskit_pthread_realtime();
		pthread_mutex_lock(&shared_mutex);
		shared_data = 1;

		cpuprintf("RM(%d): A %d,%d\n", me,
			  oskit_pthread_realtime(),
			  oskit_pthread_cputime(self));

		if (oskit_pthread_realtime() - now > 30)
			printf("RM(%d): Lock took %d\n",
			       me, oskit_pthread_realtime() - now);

		end = oskit_pthread_cputime(self) + duration;

		while (oskit_pthread_cputime(self) < end)
			pthread_testcancel();

		cpuprintf("RM(%d): B %d,%d\n", me,
			  oskit_pthread_realtime(),
			  oskit_pthread_cputime(self));

		shared_data = 0;
		pthread_mutex_unlock(&shared_mutex);

		cpuprintf("RM(%d): C %d,%d\n", me,
			  oskit_pthread_realtime(),
			  oskit_pthread_cputime(self));

		/*
		 * Sleep to beginning of next period.
		 */
		next += period;
		dur   = next - oskit_pthread_realtime();

		if (dur < 0)
			panic("rmono_worker");

		if (dur > 0)
			oskit_pthread_sleep(dur);
		
		if (verbose)
			printf("RM(%d): Loop %d, %d,%d\n", me, loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		loops++;
	}
	
	pthread_cleanup_pop(1);
}

/*
 * Medium priority thread. This one just spins around for a while.
 */
void
LOTTO(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "LO");

	if (tk)
		printf("||-a:graph linecolor the_graph tid%d_line blue||\n"
		       "||-a:graph linename  the_graph tid%d_line LO%d||\n",
		       (int) pthread_self(), (int) pthread_self(), me);

	while (1) {
		int	t1 = rand() % 200;
		int	t2 = 200 - t1;
		int     end = oskit_pthread_cputime(self) + t1;

		if (verbose)
			printf("LO: Loop %d, %d,%d\n", loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		if (shared_data)
			printf("LO: Mutex is locked\n");

		while (oskit_pthread_cputime(self) <= end)
			pthread_testcancel();

		oskit_pthread_sleep(t2);

		loops++;
	}
	pthread_cleanup_pop(1);
}

/*
 * Medium priority thread. This one just spins around for a while.
 */
void
STRIDE(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "ST");

	if (tk)
		printf("||-a:graph linecolor the_graph tid%d_line purple||\n"
		       "||-a:graph linename  the_graph tid%d_line ST%d||\n",
		       (int) pthread_self(), (int) pthread_self(), me);

	while (1) {
		int	t1 = rand() % 200;
		int	t2 = 200 - t1;
		int     end = oskit_pthread_cputime(self) + t1;

		if (verbose)
			printf("ST: Loop %d, %d,%d\n", loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		if (shared_data)
			printf("LO: Mutex is locked\n");

		while (oskit_pthread_cputime(self) <= end)
			pthread_testcancel();

		oskit_pthread_sleep(t2);

		loops++;
	}
	pthread_cleanup_pop(1);
}

/*
 * Low priority thread. Lock the same shared resource.
 */
void
RR(void *arg)
{
	int		me    = (int) arg;
	int		loops = 0;
	pthread_t	self  = pthread_self();

	pthread_cleanup_push(thread_canceled, "RR");

	if (tk)
		printf("||-a:graph linecolor the_graph tid%d_line red||\n"
		       "||-a:graph linename  the_graph tid%d_line RR%d||\n",
		       (int) pthread_self(), (int) pthread_self(), me);

	while (1) {
		int	end;

		if (verbose)
			printf("RR: Loop %d, %d,%d\n", loops,
			       oskit_pthread_realtime(),
			       oskit_pthread_cputime(self));

		pthread_mutex_lock(&shared_mutex);
		shared_data = 1;
		
		end = oskit_pthread_cputime(self) + 30;
		while (oskit_pthread_cputime(self) <= end)
			pthread_testcancel();
		
		shared_data = 0;
		pthread_mutex_unlock(&shared_mutex);

		end = (rand() % 100) + oskit_pthread_cputime(self);
		while (oskit_pthread_cputime(self) <= end)
			pthread_testcancel();
		
		loops++;
	}
	pthread_cleanup_pop(1);
}

int
main()
{
	pthread_mutexattr_t mutexattr;
	pthread_attr_t threadattr;
	pthread_t s1,s2,s3;
	struct sched_param schedparam;
	int i;
	void *stat;

	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();

	srand(time(0));
	
	/*
	 * Create a shared resource.
	 */
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&shared_mutex, &mutexattr);

	pthread_attr_init(&threadattr);

	/*
	 * Create Rate mono scheduler and its children.
	 */
	schedparam.priority = PRIORITY_NORMAL + 5;
	pthread_attr_setschedparam(&threadattr, &schedparam);
	create_ratemono_scheduler(&s1, &threadattr, 0);

	pthread_attr_setscheduler(&threadattr, s1);
	pthread_attr_setopaque(&threadattr, 50);	/* Period */
	pthread_create(&tids[ntid++], &threadattr, rmono_worker, (void *) 0);

	pthread_attr_setopaque(&threadattr, 80);	/* Period */
	pthread_create(&tids[ntid++], &threadattr, rmono_worker, (void *) 1);

	/*
	 * Create the other two schedulers.
	 */
	pthread_attr_setscheduler(&threadattr, 0);
	schedparam.priority = PRIORITY_NORMAL;
	pthread_attr_setschedparam(&threadattr, &schedparam);
	create_lotto_scheduler(&s2, &threadattr, 1);

	schedparam.priority = PRIORITY_NORMAL - 5;
	pthread_attr_setschedparam(&threadattr, &schedparam);
	create_fixedpri_scheduler(&s3, &threadattr, 1);

	/*
	 * Create threads for LOTTO scheduler
	 */
	pthread_attr_setscheduler(&threadattr, s2);
	pthread_attr_setopaque(&threadattr, 10);	/* TICKETS */
	pthread_create(&tids[ntid++], &threadattr, LOTTO, (void *) 0);

	/*
	 * Create threads for round robin scheduler
	 */
	pthread_attr_setscheduler(&threadattr, s3);
	schedparam.priority = PRIORITY_NORMAL;
	pthread_attr_setschedparam(&threadattr, &schedparam);
	pthread_attr_setschedpolicy(&threadattr, SCHED_RR);
	
	pthread_create(&tids[ntid++], &threadattr, RR, (void *) 0);

	/*
	 * Test runs runs for a fixed time.
	 */
	oskit_pthread_sleep((long long) 10000);

	/*
	 * Kill threads.
	 */
	for (i = 0; i < ntid; i++)
		pthread_cancel(tids[i]);

	/*
	 * Wait for threads to die.
	 */
	for (i = 0; i < ntid; i++)
		pthread_join(tids[i], &stat);

	/*
	 * Kill the schedulers.
	 */
	pthread_cancel(s1);
	pthread_cancel(s2);
	pthread_cancel(s3);

	/*
	 * And wait for them to die ...
	 */
	pthread_join(s1, &stat);
	pthread_join(s2, &stat);
	pthread_join(s3, &stat);
	
	oskit_pthread_sleep((long long) 100);
	printf("exiting ...\n");
	pthread_exit(0);
	return 0;
}

void
thread_canceled(void *arg)
{
	char	*str = (char *) arg;
	
	printf("Thread %s(%d) exiting. TIME: %d ticks, CPUTIME: %d ticks\n",
	       str, (int) pthread_self(),
	       oskit_pthread_realtime(),
	       oskit_pthread_cputime(pthread_self()));
}
