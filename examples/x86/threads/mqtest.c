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

#ifdef OSKIT
#include <oskit/threads/pthread.h>
#include <oskit/threads/cpuinherit.h>
#include <oskit/threads/ipc.h>
#include <oskit/machine/proc_reg.h>
#include <oskit/clientos.h>
#include <oskit/startup.h>
#include <oskit/error.h>
#include <oskit/c/mqueue.h>
#include <oskit/c/fcntl.h>
#else
#include <pthread.h>
#include <fcntl.h>
#include <mqueue.h>
#endif

#define QUEUE_NAME	"/test"

pthread_t t1, t2;
mqd_t	mq, mq2;

void
reader(int num)
{
	char rcvbuf[100];
	int rcvprio;
	int rc;
	while (1) {
#if 0
		struct mq_attr a;
		mq_getattr(mq2, &a);
		printf("mq_flags = %d\n", a.mq_flags);
		printf("mq_curmsgs = %d\n", a.mq_curmsgs);
		printf("mq_maxmsg = %d\n", a.mq_maxmsg);
		printf("mq_msgsize = %d\n", a.mq_msgsize);
#endif
	retry:
		rc = mq_receive(mq2, (char*)&rcvbuf, 100, &rcvprio);
		if (rc < 0) {
			printf("mq_receive returns %d\n", rc);
			if (errno == EAGAIN) {
				sched_yield();
				goto retry;
			}
			abort();
		}
		if (rc > 100) {
			printf("mq_receive returns %d\n", rc);
			abort();
		}
		rcvbuf[rc] = 0;
		printf("[%s]", rcvbuf);
		if (rcvbuf[0] == 'Z') {
			putchar('\n');
		}
	}
}

void
writer()
{
	int rc;
	char buf[100];
	int c = 'A';

	while (1) {
	retry:
		memset(buf, c, 100);
		rc = mq_send(mq, buf, c - 'A' + 1, 0);
		if (rc != 0) {
			printf("mq_send returns %d\n", rc);
			if (errno == EAGAIN) {
				sched_yield();
				goto retry;
			}
			abort();
		}
		if (c++ == 'Z') 
			c = 'A';
	}
}


int
main()
{
	struct mq_attr attr;
	int rc;

#ifdef OSKIT
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();
#endif

	printf("Message Queue Test Program Started\n");

	attr.mq_maxmsg = 1;
	attr.mq_msgsize = 100;
	attr.mq_flags = 0;

	/* Create a message queue */
	mq = mq_open(QUEUE_NAME, O_WRONLY|O_CREAT, 0777, &attr);
	printf("mq_open(\"%s\") returns %d\n", QUEUE_NAME, mq);
	if (mq < 0) {
		perror("mq_open");
		exit(1);
	}

	/* Open it again.  This is not necessary.  For test only */
	mq2 = mq_open(QUEUE_NAME, O_RDONLY, 0777, &attr);
	printf("mq_open(\"%s\") returns %d\n", QUEUE_NAME, mq2);
	if (mq2 < 0) {
		perror("mq_open");
		exit(1);
	}

	/* Create a writer thread */
	rc = pthread_create(&t1, NULL, (void*(*)(void*))writer, NULL);
	if (rc != 0) {
		printf("pthread_create returns %d", rc);
		exit(1);
	}

	/* Create a reader thread */
	rc = pthread_create(&t2, NULL,
			    (void*(*)(void*))reader, (void*)1);
	if (rc != 0) {
		printf("pthread_create returns %d", rc);
		exit(1);
	}

	/* Make them run for a while */
	sleep(1);
	pthread_cancel(t1);
	pthread_cancel(t2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	/* Close the message queue */
	rc = mq_close(mq);
	if (rc != 0) {
		printf("mq_close(%ld) returns %d", (long)mq, rc);
		exit(1);
	}
	rc = mq_close(mq2);
	if (rc != 0) {
		printf("mq_close(%ld) returns %d", (long)mq, rc);
		exit(1);
	}
	/* Unlink the message queue */
	rc = mq_unlink(QUEUE_NAME);
	if (rc != 0) {
		printf("mq_unlink(\"%s\") returns %d\n", QUEUE_NAME, rc);
		exit(1);
	}

	putchar('\n');

	exit(0);
	return 0;
}
