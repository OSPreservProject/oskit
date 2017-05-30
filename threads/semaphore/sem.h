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

#ifndef _THREAD_SEMAPHORE_SEM_H_
#define _THREAD_SEMAPHORE_SEM_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <oskit/c/fcntl.h>
#include <oskit/threads/pthread.h>
#include <oskit/c/semaphore.h>
#include "pthread_internal.h"
#include "oskit/queue.h"

/*
 * Queue Name Space      array of ptr	   
 * (semn_space)          to sem_t               sem_t
 * +---------------+     +-------+		+---------------+
 * |               |---->|       |------------->|               |
 * |     Lock      |     |-------|              |               |
 * +---------------+     |       |              +---------------+
 *                       |-------|
 *
 *  -  Only named semaphore is linked from semn_space.
 */

/* # of semaphore name space allocated at first */
#define SEM_INITIAL_SIZE	20

/*
 * Queue Namespace.  Per one system.
 */
struct semn_space {
	sem_t			**semn_array;
	int			semn_arraylen;
	pthread_mutex_t		semn_lock;
};
extern struct semn_space	semn_space;

/*
 * Prototypes
 */
void	sem_remove(sem_t *sem);

#endif /* _THREAD_SEMAPHORE_SEM_H_ */
