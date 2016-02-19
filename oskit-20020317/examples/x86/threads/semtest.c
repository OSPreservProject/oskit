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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/threads/ipc.h>
#include <oskit/machine/proc_reg.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/error.h>
#include <oskit/c/semaphore.h>
#include <oskit/c/fcntl.h>

#define NTHREAD 2
pthread_t thread[NTHREAD];
sem_t	*sem;

void
wait_and_post()
{
	int rc;
	int neat = 0;
	int me = (int)pthread_self();
	int x;

	while (1) {
		sem_getvalue(sem, &x);
		printf("%x: getvalue = %d\n", me, x);

		printf("%x: sem_wait()...\n", me);
		rc = sem_wait(sem);
		printf("%x: sem_wait returns %d\n", me, rc);
		printf("%x: Yum yum.. (%d)\n", me, neat++);

		sleep(1);

		printf("%x: sem_post()...\n", me);
		rc = sem_post(sem);
		printf("%x: sem_post returns %d\n", me, rc);
	}
}

int
main()
{
	int rc;
	int i;

	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();

	printf("Semaphore Test Program Started\n");

	/* Create a semaphore object */
	sem = sem_open("/test", O_CREAT|O_EXCL, 0777, 1);
	printf("sem_open returns %p\n", sem);

	for (i = 0 ; i < NTHREAD ; i++) {
		/* Create a thread */
		rc = pthread_create(&thread[i], NULL,
				    (void*(*)(void*))wait_and_post, NULL);
		if (rc != 0) {
			printf("pthread_create returns %d", rc);
			exit(1);
		}
		usleep(100*1000);
	}

	sleep(5);

	for (i = 0 ; i < NTHREAD ; i++) {
		printf("canceling thread %d\n", thread[i]);
		pthread_cancel(thread[i]);
		pthread_join(thread[i], NULL);
	}

	exit(0);
}
