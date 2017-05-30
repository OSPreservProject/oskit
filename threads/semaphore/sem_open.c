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

#include "sem.h"

/* Semaphore Name Space */
struct semn_space	semn_space;

static sem_t *sem_namespace_lookup(const char *name);
static int sem_namespace_register(sem_t *sem);

/*
 *  Initialize/Open a Named Semaphore
 */
int
oskit_sem_open(const char *name, int oflag, sem_t **sem_ret, ...)
{
	int ret = 0;
	int rc;
	sem_t *sem;
	oskit_mode_t mode;
	unsigned int value;
	va_list pvar;

	va_start(pvar, sem_ret);
	mode = va_arg(pvar, oskit_mode_t);
	value = va_arg(pvar, unsigned int);
	va_end(pvar);

	/* lock the queue name space */
	pthread_mutex_lock(&semn_space.semn_lock);

	sem = sem_namespace_lookup(name);
	if (sem == NULL) {
		if (!(oflag & O_CREAT)) {
			ret = ENOENT;
			goto bailout;
		}
		sem = malloc(sizeof(sem_t));
		if (sem == NULL) {
			ret = ENOMEM;
			goto bailout;
		}
		rc = oskit_sem_init(sem, 1, value);
		if (rc != 0) {
			free(sem);
			ret = rc;
			goto bailout;
		}
		sem->sem_name = strdup(name);
		if (sem->sem_name == NULL) {
			free(sem);
			ret = ENOMEM;
			goto bailout;
		}
		if (sem_namespace_register(sem) != 0) {
			oskit_sem_destroy(sem);
			free(sem);
			ret = ENOMEM;
			goto bailout;
		}
	} else {
		if ((oflag & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)) {
			pthread_mutex_unlock(&semn_space.semn_lock);
			return EEXIST;
		}
		sem->sem_refcount++;
	}

	*sem_ret = sem;

 bailout:
	pthread_mutex_unlock(&semn_space.semn_lock);
        return ret;
}

static sem_t *
sem_namespace_lookup(const char *name)
{
	int i;

	for (i = 0 ; i < semn_space.semn_arraylen ; i++) {
		if (semn_space.semn_array[i]
		    && (strcmp(semn_space.semn_array[i]->sem_name, name)
			== 0)) {
			return semn_space.semn_array[i];
		}
	}
	return NULL;
}

/* Assume that semn_space.semn_lock is locked */
static int
sem_namespace_register(sem_t *sem)
{
	int i;

	/* Register to the queue name space.  XXX should be hash */
	for (i = 0 ; i < semn_space.semn_arraylen ; i++) {
		if (semn_space.semn_array[i] == NULL) {
			semn_space.semn_array[i] = sem;
			break;
		}
	}

	if (i == semn_space.semn_arraylen) {
		sem_t **tmp;

		/* realloc the queue name space */
		tmp = realloc(semn_space.semn_array, 
			      (semn_space.semn_arraylen * 2)
			      * sizeof(struct sem*));
		if (tmp == NULL) {
			return -1;
		}
		bzero(tmp + semn_space.semn_arraylen, 
		      semn_space.semn_arraylen * sizeof(struct sem*));
		tmp[semn_space.semn_arraylen] = sem;
		semn_space.semn_arraylen *= 2;
		semn_space.semn_array = tmp;
	}
	return 0;
}
