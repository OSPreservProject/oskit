/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_ARM32_SPIN_LOCK_H_
#define _OSKIT_ARM32_SPIN_LOCK_H_

/*
 * Just fake it.
 */

typedef volatile int spin_lock_t;

#define SPIN_LOCK_INITIALIZER	0

#define spin_lock_init(s)	(*(s) = SPIN_LOCK_INITIALIZER)
#define spin_lock_locked(s)	(*(s) != SPIN_LOCK_INITIALIZER)
#define spin_lock_unlocked(s)	(*(s) == SPIN_LOCK_INITIALIZER)

#define	spin_unlock(s)		(*(s) = SPIN_LOCK_INITIALIZER)
#define spin_lock(s)		(*(s) = 1)
#define spin_try_lock(s)	({ *(s) = 1; 1;})

#endif /* _OSKIT_ARM32_SPIN_LOCK_H_ */
