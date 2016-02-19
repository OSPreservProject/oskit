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
 * Test pthread signal support, with tests for cleanup handlers and
 * pthread cancelation thrown in.
 */

/*
 * Set these accordingly. Also, make sure main() inits the appropriate
 * drivers.
 */
#define DISKNAME "wd1"
#define PARTITION "b"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <oskit/threads/pthread.h>
#include <oskit/startup.h>
#include <assert.h>
#include <oskit/clientos.h>

#ifdef  OSKIT_UNIX
void	start_fs_native_pthreads(char *root);
void	start_network_native_pthreads(void);
#endif

void	sigtest(void);

void
oskit_error(oskit_error_t errno, char *s)
{
	printf("%s: %s\n", s, strerror(errno));
}

int
main()
{
	oskit_clientos_init_pthreads();
	start_clock();
	start_pthreads();

	sigtest();

	return 0;
}

/*
 * Test pthread signal support. Create some threads, have them install
 * signal handlers, then send it some signals.
 */
#define NUMTESTERS	5
#define NUMCYCLES	5
volatile int		alarmed;
volatile int		killed;

void
sighandler(int sig, siginfo_t *info, struct sigcontext *scp)
{
	printf("SigHandler: pthread:%d sig:%d code:%d val:%d scp:%p\n",
	       (int) pthread_self(), info->si_signo, info->si_code,
	       info->si_value.sival_int, scp);
	killed = 1;
}

void
cleaner(void *arg)
{
	printf("Cleaner: pthread:%d arg:%d\n", (int)pthread_self(), (int) arg);
}

void
sigalarmer(int sig, int code, struct sigcontext *scp)
{
	printf("SigAlarmer: pthread:%d sig:%d code:%d scp:%p\n",
	       (int) pthread_self(), sig, code, scp);
	alarmed = 1;
}

/*
 * The thread body for the alarm() tests.
 */
void *
alarmtester(void *arg)
{
	int		i;

	for (i = 0; i < 3; i++) {
		printf("alarmtester:%d Calling Alarm ...\n",
		       (int) pthread_self());
		alarmed = 0;
		alarm(1);
		while (!alarmed)
			;
	}
	
	for (i = 0; i < 3; i++) {
		printf("alarmtester:%d Calling Alarm ...\n",
		       (int) pthread_self());
		alarm(1);
		oskit_pthread_sleep(2000);
	}
	
	pthread_exit(0);
	return 0;
}

/*
 * The thread body for the pthread_kill tests.
 */
void *
sigtester(void *arg)
{
	pthread_cleanup_push(cleaner, arg);

	printf("sigtester:%d ready\n", (int) pthread_self());
	oskit_pthread_sleep(100);
	
	while (1) {
		oskit_pthread_sleep(20);
	}

	pthread_cleanup_pop(1);
	return 0;
}

void
segvhandler(int sig, siginfo_t *info, struct sigcontext *scp)
{
	printf("SegvHandler: pthread:%d sig:%d code:%d val:%d scp:%p\n",
	       (int) pthread_self(), info->si_signo, info->si_code,
	       info->si_value.sival_int, scp);

	pthread_exit(0);
}

void *
segvtester(void *arg)
{
	pthread_cleanup_push(cleaner, arg);

	printf("sigtester:%d ready\n", (int) pthread_self());
	oskit_pthread_sleep(100);
	
	*((volatile int *) 0xDEADBEEF) = 69;

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Another thread body for the pthread_kill + sigwait tests.
 */
void *
sigwaiter(void *arg)
{
	int			rval;
	siginfo_t		info;
	sigset_t		set;
	oskit_timespec_t	timeout = { 1, 0 };
	
	pthread_cleanup_push(cleaner, arg);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, 0);
	
	printf("sigwaiter:%d ready\n", (int) pthread_self());
	
	while (1) {
		memset(&info, 0, sizeof(info));
		
		if ((rval = sigtimedwait(&set, &info, &timeout)) != 0)
			oskit_error(rval, "sigwaiter");
				    
		printf("sigwaiter:%d rval:0x%x sig:%d code:%d val:%d\n",
		       (int) pthread_self(), rval,
		       info.si_signo, info.si_code, info.si_value.sival_int);

		oskit_pthread_sleep(20);
	}

	pthread_cleanup_pop(1);
	return 0;
}

/*
 * Test process signals by blocking and unblocking signals that are
 * sent with kill().
 */
void *
sigkiller(void *arg)
{
	int			i;
	sigset_t		set;
	
	pthread_cleanup_push(cleaner, arg);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, 0);
	
	printf("sigkiller:%d ready\n", (int) pthread_self());
	
	for (i = 0; i < 3; i++) {
		printf("sigkiller: Sleeping ... \n");
		oskit_pthread_sleep(200);
		printf("sigkiller: Unblocking ... \n");
		pthread_sigmask(SIG_UNBLOCK, &set, 0);
		pthread_sigmask(SIG_BLOCK, &set, 0);
	}
	
	pthread_cleanup_pop(1);
	return 0;
}

void
sigtest(void)
{
	int		i, j;
	void		*stat;
	pthread_t	tids[NUMTESTERS];
	sigset_t	set;
	union sigval	value;
	struct sigaction sa;

	/*
	 * Install SIGUSR1 handler for pthread_kill tests.
	 */
	sa.sa_handler = sighandler;
	sa.sa_flags   = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGUSR1, &sa, 0) < 0) {
		perror("sigaction SIGUSR1");
		exit(1);
	}

	if (sigaction(SIGUSR2, &sa, 0) < 0) {
		perror("sigaction SIGUSR2");
		exit(1);
	}

	/*
	 * Install SIGALRM handler for alarm() tests.
	 */
	sa.sa_handler = sigalarmer;
	sa.sa_flags   = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGALRM, &sa, 0) < 0) {
		perror("sigaction SIGALRM");
		exit(1);
	}

	/*
	 * Install SIGSEGV handler for segv test.
	 */
	sa.sa_handler = segvhandler;
	sa.sa_flags   = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGSEGV, &sa, 0) < 0) {
		perror("sigaction SIGSEGV");
		exit(1);
	}

	/*
	 * Create a thread for the alarm test. Wait for it to do its
	 * thing, and then go onto the pthread_kill test.
	 */
	pthread_create(&tids[0], 0, alarmtester, (void *) 0);
	pthread_join(tids[0], &stat);

	/*
	 * Create threads for pthread_kill tests.
	 */
	for (i = 0; i < NUMTESTERS; i++) {
		pthread_create(&tids[i], 0, sigtester, (void *) i);
		oskit_pthread_sleep(20);
	}

	/*
	 * Send some signals
	 */
	printf("\n*** Sending sigtester threads a SIGUSR1 ...\n");
	for (j = 0; j < NUMCYCLES; j++) 
		for (i = 0; i < NUMTESTERS; i++) {
			pthread_kill(tids[i], SIGUSR1);
			oskit_pthread_sleep(20);
		}

	printf("\n*** Canceling sigtester threads ...\n");

	/*
	 * Cancel the threads.
	 */
	for (i = 0; i < NUMTESTERS; i++) {
		pthread_cancel(tids[i]);
	}

	for (i = 0; i < NUMTESTERS; i++) {
		pthread_join(tids[i], &stat);
	}

	/*
	 * Create threads for pthread_kill + sigwait tests.
	 */
	for (i = 0; i < NUMTESTERS; i++) {
		pthread_create(&tids[i], 0, sigwaiter, (void *) i);
		oskit_pthread_sleep(20);
	}

	/*
	 * Send some signals
	 */
	printf("\n*** Sending sigwaiter threads a SIGUSR1 ...\n");
	for (j = 0; j < NUMCYCLES; j++) 
		for (i = 0; i < NUMTESTERS; i++) {
			pthread_kill(tids[i], SIGUSR1);
			oskit_pthread_sleep(20);
		}

	/*
	 * Send them a different signal.
	 */
	printf("\n*** Sending sigwaiter threads a SIGUSR2 ...\n");
	for (i = 0; i < NUMTESTERS; i++) {
		pthread_kill(tids[i], SIGUSR2);
		oskit_pthread_sleep(20);
	}
		
	/*
	 * Let them all take a timeout.
	 */
	printf("\n*** Letting sigwaiter threads timeout ...\n");
	oskit_pthread_sleep(1500);

	/*
	 * Send a kill to ourselves and see what happens. One of the
	 * sigwaiters should get it.
	 */
	printf("\n*** Sending a kill(0, SIGUSR1) ...\n");
	kill(0, SIGUSR1);
	oskit_pthread_sleep(100);

	printf("\n*** Canceling sigwaiter threads ...\n");

	/*
	 * Cancel the threads.
	 */
	for (i = 0; i < NUMTESTERS; i++) {
		pthread_cancel(tids[i]);
	}

	for (i = 0; i < NUMTESTERS; i++) {
		pthread_join(tids[i], &stat);
	}

	printf("\n*** Sending another kill(0, SIGUSR1) ...\n");
	kill(0, SIGUSR1);
	oskit_pthread_sleep(100);

	printf("\n*** Creating sigkiller threads ...\n");
	for (i = 0; i < 3; i++) {
		pthread_create(&tids[i], 0, sigkiller, (void *) i);
		oskit_pthread_sleep(20);
	}

	/*
	 * Make sure the main thread does not take these.
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &set, 0);

	/*
	 * Send a couple of kills() and see what happens. It should become a
	 * process pending thread since its blocked in all threads, and then
	 * delivered when one of the sigkiller threads unblocks it.
	 */
	for (i = 0; i < 3; i++) {
		killed = 0;
		kill(0, SIGUSR1);
		while (! killed)
			oskit_pthread_sleep(10);
		oskit_pthread_sleep(100);
	}

	printf("\n*** Canceling sigkiller threads ...\n");

	/*
	 * Cancel the threads.
	 */
	for (i = 0; i < 3; i++) {
		pthread_cancel(tids[i]);
	}

	for (i = 0; i < 3; i++) {
		pthread_join(tids[i], &stat);
	}

	/*
	 * Create a single sigwaiter thread for the sigqueue tests.
	 * The POSIX spec does mention that having multiple threads waiting
	 * for the same signal is a problem, and should be avoided.
	 * Not only is it difficult to maintain order when queueing 
	 * for the signal, but there is the problem of lost signals too.
	 */
	printf("\n*** Creating sigwaiter thread for sigqueue test ...\n");

	pthread_create(&tids[0], 0, sigwaiter, (void *) 0);
	oskit_pthread_sleep(20);

	printf("\n*** Queueing to sigwaiter thread ...\n");
	value.sival_int = 69;
	for (i = 0; i < 5; i++) {
		sigqueue(0, SIGUSR1, value);
		value.sival_int++;
	}
	oskit_pthread_sleep(200);

	printf("\n*** Canceling sigwaiter thread ...\n");

	/*
	 * Cancel the threads.
	 */
	pthread_cancel(tids[0]);
	pthread_join(tids[0], &stat);

	oskit_pthread_sleep((long long) 100);
	printf("exiting ...\n");
	exit(0);
	return;
}
