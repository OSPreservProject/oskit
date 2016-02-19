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
 * Get and set thread-specific data values.
 */
#include <threads/pthread_internal.h>

int
pthread_setspecific(pthread_key_t key, const void *value)
{
	pthread_thread_t	*pthread = CURPTHREAD();

	/*
	 * Don't allow the builtin keys to be changed either.
	 */
	if (! validkey(key)) {
		return EINVAL;
	}

	/*
	 * Now change the key since its valid.
	 */
	pthread->keyvalues[key] = (void *) value;

	return 0;
}

void *
pthread_getspecific(pthread_key_t key)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	void			*value;

	if (! validkey(key)) {
		return 0;	/* No error returned! */
	}

	value = pthread->keyvalues[key];
	
	return value;
}

/* Not POSIX */
int
oskit_pthread_setspecific(pthread_t tid, pthread_key_t key, const void *value)
{
	pthread_thread_t	*pthread;

	if (! validkey(key)) {
		return 0;	/* No error returned! */
	}

	pthread = tidtothread(tid);
	if (pthread == 0) {
	    return 0;
	}

	pthread->keyvalues[key] = (void *) value;
	
	return 0;
}

/* Not POSIX */
void *
oskit_pthread_getspecific(pthread_t tid, pthread_key_t key)
{
	pthread_thread_t	*pthread;
	void			*value;

	if (! validkey(key)) {
		return 0;	/* No error returned! */
	}

	pthread = tidtothread(tid);
	if (pthread == 0) {
	    return 0;
	}

	value = pthread->keyvalues[key];
	
	return value;
}

