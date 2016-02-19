/*
 * Copyright (c) 1998, 1999 University of Utah and the Flux Group.
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
 * A simple program demonstrating the use of the pthread IPC primitives.
 * A number of client and server threads are started up, where each client
 * sends a specific number of messages to a server. 
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

#define	SERVERS		3	/* Number of server threads */
#define CLIENTS		10	/* Number of client threads */
#define CLIENT_MSGS	10000	/* Number of messages each client sends */

/*
 * Define this to send zero length messages.
 */
#define NULLMSG 
#define NULLREPLY

/*
 * Define this to create a scheduler "hierarchy". This option is valid only
 * when the threads library has been compiled with cpu inheritance scheduling
 * defined.
 */
/* #define CPUI */

void
oskit_error(oskit_error_t errno, char *s)
{
	printf("%s: %s\n", s, strerror(errno));
}

void		       *client(void *arg);
void		       *server(void *arg);
pthread_mutex_t		randlock;
int			verbose = 0;
int			barrier;
pthread_mutex_t		barrier_mutex;
pthread_cond_t		barrier_cond;

/*
 * A dopey little message structure.
 */
typedef struct message {
	int	foo;
	int	bar;
	int	fee;
	int     buf[22];
} message_t;

pthread_t		clients[CLIENTS], servers[SERVERS];

int
main()
{
	int		i;
	void		*status;
	unsigned long long before_cycles, after_cycles;
	unsigned long	before, after;
	pthread_attr_t	attr;
#ifdef	CPUI
	pthread_t	s1, s2;
#endif
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();

	pthread_mutex_init(&randlock, 0);
	pthread_mutex_init(&barrier_mutex, 0);
	pthread_cond_init(&barrier_cond, 0);
	pthread_attr_init(&attr);
	barrier = SERVERS+CLIENTS;

#ifdef	CPUI
	create_fixedpri_scheduler(&s1, &attr, 0);
	create_fixedpri_scheduler(&s2, &attr, 0);
	pthread_attr_setscheduler(&attr, s1);
#endif

	for (i = 0; i < SERVERS; i++) {
		pthread_create(&servers[i], &attr, server, (void *) i);
	}
		
#ifdef	CPUI
	pthread_attr_setscheduler(&attr, s2);
#endif
	for (i = 0; i < CLIENTS; i++) {
		pthread_create(&clients[i], &attr, client, (void *) i);
	}

	/*
	 * Make everyone wait at the barrier.
	 */
	pthread_mutex_lock(&barrier_mutex);
	while (barrier != 0) {
		pthread_mutex_unlock(&barrier_mutex);
		pthread_mutex_lock(&barrier_mutex);
	}
	pthread_mutex_unlock(&barrier_mutex);

	/*
	 * Okay, send them off and wait for them to finish.
	 */
	before_cycles = get_tsc();
	before = oskit_pthread_realtime();
	pthread_cond_broadcast(&barrier_cond);

	for (i = 0; i < CLIENTS; i++)
		pthread_join(clients[i], &status);

	after_cycles = get_tsc();
	after = oskit_pthread_realtime();
	printf("Test took %qu cycles\n",
	       (after_cycles - before_cycles));

	for (i = 0; i < SERVERS; i++)
		pthread_cancel(servers[i]);

	for (i = 0; i < SERVERS; i++)
		pthread_join(servers[i], &status);

#ifdef  CPUI
	pthread_cancel(s1);
	pthread_cancel(s2);
	pthread_join(s1, &status);
	pthread_join(s2, &status);	
#endif

	return 0;
}

/*
 * IPC test. Server side.
 */
void *
server(void *arg)
{
	pthread_t	me = pthread_self();
	message_t	msg, reply;
	char		buf[BUFSIZ];

	pthread_t	client;
	oskit_size_t	actual;
	oskit_error_t	rc;
	int		msize;

	pthread_mutex_lock(&barrier_mutex);
	barrier--;
	pthread_cond_wait(&barrier_cond, &barrier_mutex);
	pthread_mutex_unlock(&barrier_mutex);

	printf("server(%d): Starting up\n", (int) me);

	while (1) {
		pthread_testcancel();

		/*
		 * Wait for a message.
		 */
		if ((rc = oskit_ipc_wait(&client,
					 &msg, sizeof(msg), &actual, -1))) {
			oskit_error(rc, "oskit_ipc_recv");
			pthread_exit((void *) 1);
		}

		if (verbose) {
			sprintf(buf, "server(%d): %8d  %8d  %8d\n", (int) me,
				msg.foo, msg.bar, msg.fee);
			write(1, buf, strlen(buf));
		}
#ifdef NULLREPLY
		msize  = 0;
#else
		memcpy(&reply, &msg, sizeof(msg));
		msize  = sizeof(reply);
#endif
		if ((rc = oskit_ipc_reply(client, &reply, msize))) {
			oskit_error(rc, "oskit_ipc_reply");
			pthread_exit((void *) 1);
		}
	}

	return 0;
}

/*
 * Client side.
 */
void *
client(void *arg)
{
	pthread_t	me = pthread_self();
	message_t	msg, reply;
	char		buf[BUFSIZ];
	oskit_size_t	replysize;
	int		msize, count = CLIENT_MSGS;
	long long	total = 0, before, after;
	pthread_t	server;
	oskit_error_t	rc;

	pthread_mutex_lock(&barrier_mutex);
	barrier--;
	pthread_cond_wait(&barrier_cond, &barrier_mutex);
	pthread_mutex_unlock(&barrier_mutex);
	
	server = servers[(int) me % SERVERS];

	printf("client(%d): Starting up; sending to %d\n",
	       (int) me, (int) server);

	while (count--) {
		pthread_testcancel();
		sched_yield();

		/*
		 * Send a message.
		 */
#if 0
#ifndef NULLMSG
		pthread_mutex_lock(&randlock);
		msg.foo = rand() % 100000;
		msg.bar = rand() % 100000;
		msg.fee = rand() % 100000;
		pthread_mutex_unlock(&randlock);
#endif
#endif
		if (verbose) {
			sprintf(buf, "client(%d): %8d  %8d  %8d\n", (int) me,
				msg.foo, msg.bar, msg.fee);
			write(1, buf, strlen(buf));
		}
#ifdef NULLMSG
		msize  = 0;
#else
		msize  = sizeof(msg);
#endif
		before = get_tsc();
		if ((rc = oskit_ipc_call(server, &msg, msize,
					 &reply, sizeof(msg), &replysize, 0))){
			oskit_error(rc, "oskit_ipc_call");
			pthread_exit((void *) 1);
		}
		after = get_tsc();
		total += (after - before);
#ifndef NULLREPLY
		assert(replysize == sizeof(reply));
		assert(msg.foo == reply.foo);
		assert(msg.bar == reply.bar);
		assert(msg.fee == reply.fee);
#endif
	}

	sprintf(buf, "%d IPCs took %qu cycles (%d cycles each)\n",
		CLIENT_MSGS, total, (int) (total / CLIENT_MSGS));
	write(1, buf, strlen(buf));

	return 0;
}
