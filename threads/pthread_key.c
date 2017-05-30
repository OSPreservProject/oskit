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
 * Keytable code.
 */
#include <threads/pthread_internal.h>

/*
 * Lock for key table.
 */
pthread_lock_t		threads_key_lock = PTHREAD_LOCK_INITIALIZER;

/*
 * The table itself.
 */
struct keytable		threads_key_table[PTHREAD_KEYS_MAX];

/*
 * Init the default key entries.
 */
void
pthread_init_keytable(void)
{
	pthread_lock_init(&threads_key_lock);
}

/*
 * Create a new key.
 */
int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int		newkey;

	assert_preemption_enabled();
	disable_preemption();
	
	pthread_lock(&threads_key_lock);

	for (newkey = 0; newkey < PTHREAD_KEYS_MAX; newkey++)
		if (! threads_key_table[newkey].inuse)
			break;

	if (newkey == PTHREAD_KEYS_MAX) {
		pthread_unlock(&threads_key_lock);
		enable_preemption();
		return EAGAIN;
	}
	threads_key_table[newkey].inuse      = 1;
	threads_key_table[newkey].destructor = destructor;
	
	pthread_unlock(&threads_key_lock);
	enable_preemption();

	*key = (pthread_key_t) newkey;
	return 0;
}

/*
 * Create a new key.
 */
int
pthread_key_delete(pthread_key_t key)
{
	assert_preemption_enabled();
	disable_preemption();

	pthread_lock(&threads_key_lock);

	if (! validkey(key)) {
		pthread_unlock(&threads_key_lock);
		enable_preemption();
		return EINVAL;
	}

	threads_key_table[key].inuse      = 0;
	threads_key_table[key].destructor = 0;
	
	pthread_unlock(&threads_key_lock);
	enable_preemption();
	return 0;
}

/*
 * Helper function. Call the key destructors for each valid and non-zero
 * key/value pair. Loop a reasonable amount of times, since it is required
 * that the destructor clear the key/value before this ends.
 */
void
pthread_call_key_destructors(void)
{
	pthread_thread_t	*pthread = CURPTHREAD();
	int			i, done, loopcount = 0;
	void			*value;

	do {
		if (loopcount++ > PTHREAD_KEYS_MAX)
			panic("pthread_call_key_destructors: "
			      "Looping in 0x%x(%d)\n",
			      (int) pthread, pthread->tid);

		done = 1;
		for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
			if (threads_key_table[i].inuse &&
			    (value = pthread->keyvalues[i])) {
				if (threads_key_table[i].destructor) {
					done = 0;
					threads_key_table[i].destructor(value);
				}
				else
					pthread->keyvalues[i] = 0;
			}
		}
	} while (! done);
}
