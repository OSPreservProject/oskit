/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_SYS_TIME_H_
#define _OSKIT_C_SYS_TIME_H_

#include <oskit/compiler.h>
#include <oskit/time.h>

struct timeval {
	oskit_time_t	tv_sec;
	oskit_s32_t	tv_usec;
};

struct timezone;		/* never defined */

#define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}

#define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}

/*
 * definitions for interval timer
 */
struct itimerval {
	struct  timeval it_interval;    /* timer interval */
	struct  timeval it_value;       /* current value */
};

#define ITIMER_REAL      0
#define ITIMER_VIRTUAL   1
#define ITIMER_PROF      2

/*
 * according to X/Open CAE Issue 4, Version 2, fd_set et al go in here
 */
#define NBBY    8               /* number of bits in a byte */

/*
 * Select uses bit masks of file descriptors in longs.  These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE      256
#endif

typedef long    fd_mask;
#define NFDBITS (sizeof(fd_mask) * NBBY)        /* bits per mask */

#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif

typedef struct fd_set { 
        fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];  
} fd_set;

#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_COPY(f, t)   memcpy(t, f, sizeof(*(f)))
#define FD_ZERO(p)      memset(p, 0, sizeof(*(p)))

OSKIT_BEGIN_DECLS

int gettimeofday(struct timeval *tp, struct timezone *tzp);
int select(int n, fd_set *in, fd_set *out, fd_set *exc, struct timeval *tout);
int getitimer(int which, struct itimerval *value);
int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
int utimes(const char *, const struct timeval *);

/* the clock kept by the minimal C library */
extern struct oskit_clock *sys_clock;
void set_system_clock(struct oskit_clock *clock);

OSKIT_END_DECLS

#endif /* _OSKIT_C_SYS_TIME_H_ */
