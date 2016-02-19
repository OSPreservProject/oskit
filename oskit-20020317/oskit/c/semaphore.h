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

#ifndef _OSKIT_C_SEMAPHORE_H_
#define _OSKIT_C_SEMAPHORE_H_

#include <oskit/queue.h>
#include <oskit/machine/spin_lock.h>

typedef struct {
	char			*sem_name; /* Unnamed semaphore has NULL */
	spin_lock_t		sem_lock;
	queue_head_t		sem_waiters;
	int			sem_value;
	int			sem_flag;
#define SEM_UNLINK_FLAG		1
	int			sem_refcount;
} sem_t;

/*
 * POSIX API Prototypes
 */
int	sem_init(sem_t *sem, int pshared, unsigned int value);
int	sem_destroy(sem_t *sem);
sem_t	*sem_open(const char *name, int oflag, 
		  /* oskit_mode_t mode, unsigned int value */ ...);
int	sem_close(sem_t *sem);
int	sem_unlink(const char *name);
int	sem_getvalue(sem_t *sem, int *sval);

/* call directly to eliminate the overhead of procedure call */
#define sem_wait	oskit_sem_wait
#define sem_trywait	oskit_sem_trywait
#define sem_post	oskit_sem_post

/*
 * OSKIT API Prototypes
 */
int	oskit_sem_init(sem_t *sem, int pshared, unsigned int value);
int	oskit_sem_destroy(sem_t *sem);
int	oskit_sem_open(const char *name, int oflag, sem_t **sem,
		       /* oskit_mode_t mode, unsigned int value */ ... );
int	oskit_sem_close(sem_t *sem);
int	oskit_sem_unlink(const char *name);
int	oskit_sem_wait(sem_t *sem);
int	oskit_sem_trywait(sem_t *sem);
int	oskit_sem_post(sem_t *sem);
int	oskit_sem_getvalue(sem_t *sem, int *sval);

#endif /* _OSKIT_C_SEMAPHORE_H_ */
