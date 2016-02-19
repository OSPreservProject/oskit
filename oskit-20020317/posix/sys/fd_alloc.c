/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Allocate a file descriptor
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "fd.h"

/* start out with NDFILE */
#define NDFILE          20 
#define NDEXTENT        50

/* then extend by `extendby' - which is doubled every time */
static	int extendby = NDFILE;

#ifdef THREAD_SAFE
static pthread_mutexattr_t	mutex_attributes;
pthread_mutex_t			libc_fd_array_mutex;
#endif

#ifdef KNIT
oskit_error_t fd_init2(void);
#else
static  int didinit  = 0;
void	fd_init(void);
#endif

/*
 * The 3 stdio descriptors have to be statically initialized so that
 * console printf works from the get go. This makes the first real
 * allocation a little messy, since we are going to toss this array
 * away once we get the intialization call. Life is messy.
 */
extern oskit_stream_t console_stream;

int	fd_arraylen = 3;
fd_t    initial_fd_array[3] = {
	{ (oskit_iunknown_t*) &console_stream },
	{ (oskit_iunknown_t*) &console_stream },
	{ (oskit_iunknown_t*) &console_stream },
};
fd_t	*fd_array = initial_fd_array;

int	
fd_alloc(oskit_iunknown_t *obj, int min_fd)
{
	int		i;
	oskit_size_t 	bytes;
	fd_t		*newarray;

#ifndef KNIT
	if (! didinit) {
#ifdef  THREAD_SAFE
		panic("fd_alloc: oskit_init_libc was not called!");
#else
		fd_init();
#endif
	}
#endif /* !KNIT */
	fd_array_lock();

try_again:
	/* search for a free fd */
	for (i = min_fd; i < fd_arraylen; i++) {
		if (fd_array[i].obj == 0) {
			/* found an unused slot */
#ifdef THREAD_SAFE
			pthread_mutex_init(fd_array[i].lock,
					   &mutex_attributes);
			pthread_cond_init(fd_array[i].readcond,
					  &pthread_condattr_default);
			pthread_cond_init(fd_array[i].writecond,
					  &pthread_condattr_default);
			fd_array[i].writer = 0;
			fd_array[i].reader = 0;
			fd_array[i].writecount = 0;
			fd_array[i].readcount = 0;
			fd_array[i].readwait = 0;
			fd_array[i].writewait = 0;
			spin_lock_init(&fd_array[i].spinlock);
#ifdef DEBUG
			fd_array[i].creator = pthread_self();
#endif
#endif
			oskit_iunknown_addref(obj);
			fd_array[i].obj = obj;
			fd_array_unlock();
			return i;
		}
	}

	/*
	 * Create the fd_array if we don't have one yet, or realloc
	 * if we do.
	 */
	if (fd_array == initial_fd_array) {
		extendby = NDFILE;
		bytes = (fd_arraylen + extendby) * sizeof(fd_t);
		newarray = (fd_t *)malloc(bytes);
		memcpy(newarray, initial_fd_array, sizeof(initial_fd_array));
        }
	else {
		bytes = (fd_arraylen + extendby) * sizeof(fd_t);
		newarray = (fd_t *)realloc(fd_array, bytes);
	}
	if (newarray == NULL) {
		errno = ENOMEM;
		fd_array_unlock();
		return -1;
	}
	/* zero out new part of the array */
	memset(newarray + fd_arraylen, 0, extendby * sizeof(fd_t));
	
#ifdef  THREAD_SAFE
	/* allocate the mutex and condition variable structures. */
	{
		pthread_mutex_t	*pmutex;
		pthread_cond_t  *pcond;
		fd_t		*pfd = newarray + fd_arraylen;
		
		if ((pmutex = (pthread_mutex_t *)
		     calloc(extendby, sizeof(pthread_mutex_t))) == NULL) {
			errno = ENOMEM;
			fd_array_unlock();
			return -1;
		}
		if ((pcond = (pthread_cond_t *)
		     calloc(extendby * 2, sizeof(pthread_cond_t))) == NULL) {
			free(pmutex);
			errno = ENOMEM;
			fd_array_unlock();
			return -1;
		}
		for (i = 0; i < extendby; i++) {
			pfd[i].lock      = pmutex++;
			pfd[i].readcond  = pcond++;
			pfd[i].writecond = pcond++;
		}
	}
#endif
	fd_array = newarray;
	fd_arraylen += extendby;
	extendby *= 2;
	goto try_again;
}

#ifdef KNIT	
oskit_error_t fd_init2(void)
#define exit return
#else
void fd_init(void)
#endif
{
#ifdef THREAD_SAFE
	int		i;
#endif

#ifndef KNIT	
	if (didinit)
		return;
#endif
	
#ifdef THREAD_SAFE
	pthread_mutex_init(&libc_fd_array_mutex, &pthread_mutexattr_default);

	/*
	 * Because interrupt routines can call printf, the mutex needs
	 * to allow recursive lock attempts.
	 */
	pthread_mutexattr_init(&mutex_attributes);
	pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);

	/*
	 * Initialize the mutex and condition variables in the stdio
	 * descriptors.
	 */
	for (i = 0; i < fd_arraylen; i++) {
		pthread_mutex_t		*pmutex;
		pthread_cond_t		*pcond;

		if ((pmutex = (pthread_mutex_t *)
		     calloc(1, sizeof(pthread_mutex_t))) == NULL) {
			exit(969);
		}
		if ((pcond = (pthread_cond_t *)
		     calloc(2, sizeof(pthread_cond_t))) == NULL) {
			free(pmutex);
			exit(9696);
		}

		pthread_mutex_init(pmutex, &mutex_attributes);
		pthread_cond_init(pcond, &pthread_condattr_default);
		pthread_cond_init(&pcond[1], &pthread_condattr_default);

		fd_array[i].lock      = pmutex;
		fd_array[i].readcond  = pcond++;
		fd_array[i].writecond = pcond;
		
	}
#endif
#ifdef KNIT	
        return 0;
#else
	didinit = 1;
#endif
}
#ifdef KNIT
#undef exit
#endif

/*
 * Cleanup routine.
 */
void
fd_cleanup(void)
{
	int		i;

	for (i = 3; i < fd_arraylen; i++) {
		if (fd_array[i].obj != 0) {
			fd_free(i);
		}
	}
}

