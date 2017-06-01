/*
 * Copyright (c) 1998-2000 University of Utah and the Flux Group.
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
#ifndef	_PTHREAD_STATS_H_
#define _PTHREAD_STATS_H_

typedef int stat_count_t;		/* count instances of an event */
typedef unsigned long stat_time_t;	/* individual time value */
typedef unsigned long stat_accum_t;	/* accumulated time for an event */
typedef unsigned long long stat_stamp_t;/* arch specific time stamp */

#include <oskit/machine/proc_reg.h>

#define STAT_STAMPGET()		get_tsc()
#define STAT_STAMPDIFF(s)	(stat_time_t)(get_tsc() - (s))

#ifdef THREAD_STATS
struct pthread_stats {
	stat_count_t	scheds;		/* # of times scheduled from other */
	stat_count_t	rescheds;	/* # of times scheduled from us */
	stat_count_t	preempts;	/* # of times preempted */
	stat_count_t	blocks;		/* # of times preempted */
	stat_accum_t	rtime;		/* accumulated run time */
	stat_time_t	rmin, rmax;	/* min/max run times */
	stat_stamp_t	rstamp;		/* start run time stamp */
	stat_accum_t	qtime;		/* accumulated time on run queue */
	stat_time_t	qmin, qmax;	/* min/max queue times */
	stat_stamp_t	qstamp;		/* start time stamp for queueing */
};

struct pthread_history {
	pthread_t tid;
	int priority;
	struct pthread_stats stats;
};

#define PTHREAD_HISTORY_SIZE	1024
#endif

#ifdef  RTSCHED_STATS
struct pthread_counts {
	stat_count_t	softint_preempt_count;
	stat_count_t	softint_yield_count;
	stat_count_t	realtime_clockticks;
	stat_count_t	standard_clockticks;
	stat_count_t	missed_clockticks;
	stat_accum_t	handler_latency_total;
	stat_accum_t	handler_latency_min;
	stat_accum_t	handler_latency_max;
};
extern struct pthread_counts pthread_counts;
#define PCOUNT(x)	(x)
#else
#define PCOUNT(x)
#endif

#ifdef IPC_STATS
struct ipc_stats {
	stat_count_t	sends;
	stat_accum_t	send_cycles;
	stat_count_t	sblocks;
	stat_accum_t	sblock_cycles;
	stat_count_t	recvs;
	stat_accum_t	recv_cycles;
	stat_count_t	rblocks;
	stat_accum_t	rblock_cycles;
	stat_count_t	waits;
	stat_accum_t	wait_cycles;
	stat_count_t	wfails;
	stat_accum_t	wfail_cycles;
	stat_count_t	wblocks;
	stat_accum_t	wblock_cycles;
	stat_count_t	waitfails;
	stat_accum_t	waitfail_cycles;
	stat_count_t	scheds;
	stat_accum_t	sched_cycles;
	stat_count_t	schednrs;
	stat_accum_t	schednr_cycles;
	stat_count_t	schedws;
	stat_accum_t	schedw_cycles;
};
#endif

#ifdef	SCHED_STATS
struct pthread_gstats {
	stat_count_t	wakeups;
	stat_accum_t	wakeup_cycles;
	stat_count_t	switches;
	stat_accum_t	switch_cycles;
	stat_count_t	switchessame;
	stat_accum_t	switchsame_cycles;
};
#endif

#ifdef	THREAD_STATS
#endif

#endif /* _PTHREADS_STATS_H_ */
