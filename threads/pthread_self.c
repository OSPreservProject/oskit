/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#include <threads/pthread_internal.h>

/*
 * Obvious
 */
pthread_t
pthread_self(void)
{
	/*
	 * For convenience, lets not have this seg fault if called
	 * before the thread system is initialized.
	 */
	if (CURPTHREAD())
		return CURPTHREAD()->tid;
	else
		return 0;
}

/*
 * Whats the point of this function?
 */
int
pthread_equal(pthread_t tida, pthread_t tidb)
{
	return tida == tidb;
}

/*
 * Internal helper.
 */
pthread_thread_t *
pthread_current(void)
{
	return CURPTHREAD();
}

