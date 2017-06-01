/*
 * Copyright (c) 1998, 1999, 2001 University of Utah and the Flux Group.
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
 * Dining Philosophers. This example program solves the dining philosophers
 * problem using monitors. It is intended as a pthread demonstration, and
 * a small torture test of the core pthreads constructs like mutexes,
 * conditions, sleep, preemption, thread switching. It also turns on
 * priority inheritance in the monitor mutex to test that part of the
 * pthreads scheduling code.
 *
 * This was written by Jamie Painter a long, long time ago ...
 */

#include <stdlib.h>
#include <stdio.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/dev/osenv_intr.h>

#include <oskit/threads/pthread.h>

#define mutex_lock	pthread_mutex_lock
#define mutex_unlock    pthread_mutex_unlock
#define condition_wait  pthread_cond_wait
#define condition_signal pthread_cond_signal
#define condition_broadcast pthread_cond_broadcast

#define PHILOSOPHERS	7

/* This is used to ensure each monitor procedure is mutually exclusive*/
pthread_mutex_t	monitor_protect;
pthread_cond_t	self[PHILOSOPHERS];
enum {THINKING,HUNGRY,EATING} state[PHILOSOPHERS];

char *state_string[] = {"thinking", "hungry  ", "eating  "};

void random_spin(int max);

/*
 *  These are to guarantee mutual exclusion to stderr for output
 * and to the random clibrary function.
 */
pthread_mutex_t random_protect, stderr_protect;

void test(int k)
{
    /* 
     * Not a monitor entry so we DON'T lock the monitor mutex 
     * presumably it is already held by our caller.
     */
    if (state[(k+(PHILOSOPHERS - 1))%PHILOSOPHERS] != EATING &&
	state[k] == HUNGRY &&
	state[(k+1)%PHILOSOPHERS] != EATING) {
	state[k] = EATING;
	condition_signal(&self[k]);
    }
}

/* Philospher i picks up a pair of forks */
void pickup(int i)
{
    mutex_lock(&monitor_protect);
    random_spin(1000);
    state[i] = HUNGRY;
    test(i);
    while (state[i] != EATING)
	condition_wait(&self[i], &monitor_protect);
    mutex_unlock(&monitor_protect);
}


/* Philospher i puts down his forks */
void putdown(int i)
{
    mutex_lock(&monitor_protect);
    random_spin(1000);
    state[i] = THINKING;
    test((i+(PHILOSOPHERS - 1)) %PHILOSOPHERS);
    test((i+1) %PHILOSOPHERS);
    mutex_unlock(&monitor_protect);
}



void cycle_soaker(void ) {}

/*
 * A random wait time mixes things up a bit.
 */
void random_spin(int max)
{
    int spin_time;
    int i;

    max = max * 100;

    mutex_lock(&random_protect);
      spin_time = rand() % max;
    mutex_unlock(&random_protect);

    for(i=0; i<spin_time; i++)
	cycle_soaker();
}

/* 
 * Check that current state makes sense.
 *   i must be EATING.  The neighbors must not be.
 *
 */
void validate_state(int i)
{
    mutex_lock(&monitor_protect);
    
    if (state[i] != EATING ||
	state[(i+1)%PHILOSOPHERS] == EATING ||
        state[(i+(PHILOSOPHERS - 1))%PHILOSOPHERS] == EATING)  {
	mutex_lock(&stderr_protect);
	  printf("Invalid state detect by philosopher %d\n", i );
	mutex_unlock(&stderr_protect);
	abort();
    }
    mutex_unlock(&monitor_protect);
}
	
/*
 * In OSKit/unix, sleep time is not real time, rather time the process
 * is actually running.  So there is a time dilation we compensate for
 * (roughly a factor of 3-4).
 */
#ifdef OSKIT_UNIX
#define SHORT_SLEEP	((long long)(100/4))
#define LONG_SLEEP	((long long)(1000/4))
#else
#define SHORT_SLEEP	((long long)100)
#define LONG_SLEEP	((long long)1000)
#endif

void *
philosopher(void *arg)
{
    int i = (int) arg;
    int j;

    oskit_pthread_sleep(LONG_SLEEP);

    /* a year in a life of a philospher ... */
    for(j=0;j<150;j++) {
	pickup(i);		/* get forks */

	  validate_state(i);

/*  	  mutex_lock(&stderr_protect);
	  printf("%d Eating\n", i );
	  mutex_unlock(&stderr_protect); */

       if ((j % 10) == 0) {
		int k;
		
		mutex_lock(&stderr_protect);
		printf("p:%d(%d) j:%d ", i, oskit_pthread_whichcpu(), j);
		for (k = 0; k < PHILOSOPHERS; k++) {
			printf("%s ", state_string[state[k]]);
		}
		printf("\n");
		mutex_unlock(&stderr_protect);
	}

	  oskit_pthread_sleep(SHORT_SLEEP);

/*	  mutex_lock(&stderr_protect);
	  printf("%d Thinking\n", i );
	  mutex_unlock(&stderr_protect); */

	putdown(i);		/* release forks */

	random_spin(500);	/* hungry fellow: eats more than he thinks */
    }

    return 0;
}

int 
main()
{
    int i;
    void *stat;
    pthread_t foo[PHILOSOPHERS];
    pthread_mutexattr_t mutexattr;

#ifndef KNIT
    oskit_clientos_init_pthreads();
    start_clock();
    start_pthreads();
#ifdef GPROF
    start_fs_bmod();
    start_gprof();
#endif
#endif

    srand(1);

    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setprotocol(&mutexattr, 1);

/*    pthread_mutex_init(&stderr_protect, 0);
      pthread_mutex_init(&random_protect, 0); */
    pthread_mutex_init(&monitor_protect, &mutexattr);
    
    for(i=0; i<PHILOSOPHERS; i++) {
	state[i] = THINKING;
	pthread_cond_init(&self[i], 0);
    }

    for(i=0; i<PHILOSOPHERS; i++) {
	pthread_create(&foo[i], 0, philosopher, (void *) i);
	if ((unsigned) i & 1)
		oskit_pthread_setprio(foo[i], PRIORITY_NORMAL + 1);
    }
    
    for(i=0; i<PHILOSOPHERS; i++) {
	    pthread_join(foo[i], &stat);
    }

    oskit_pthread_sleep(SHORT_SLEEP);
    printf("exiting ...\n");
    exit(0);

    return 0;
}
