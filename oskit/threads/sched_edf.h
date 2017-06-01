/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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

#ifndef	_OSKIT_THREADS_SCHED_EDF_H_
#define _OSKIT_THREADS_SCHED_EDF_H_

/*
 * Looks strange, I know. Include this file just once!
 *
 * The scheduler symbols are defined "weak" so that you don't have to
 * mess with your makefiles if you build the threads library with a
 * realtime scheduler, but do not actually use it. Referencing this
 * non-weak symbol will drag in the file from the library when linking.
 */
extern int __drag_in_edf_scheduler__;
int *__using_edf_scheduler__ = &__drag_in_edf_scheduler__;

#endif  /* _OSKIT_THREADS_SCHED_EDF_H_ */
