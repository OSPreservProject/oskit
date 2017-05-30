/*
 * Copyright (c) 1996, 1997, 1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

struct wait_queue;
struct buffer_head;

/*
 * This macro must be invoked in every process-level fdev_linux entrypoint;
 * it initializes the specified 'task_struct',
 * which must have been allocated locally on the stack,
 * and sets the 'current' pointer to point to it.
 */
#define OSKIT_LINUX_CREATE_CURRENT(ts)					\
	(ts).fsuid = 0;							\
	current = &(ts);

/*
 * This macro is invoked on exit from process-level fdev_linux entrypoints;
 * it clears the 'current' pointer back to the "idle" task
 * ("idle" meaning the driver set is idle, not that the system is idle).
 */
#ifdef OSKIT_DEBUG
#define OSKIT_LINUX_DESTROY_CURRENT()					\
	current = (struct task_struct*)0xdeadcccc
#define OSKIT_LINUX_CHECK_CURRENT()					\
	if (cur == (struct task_struct*)0xdeadcccc)			\
		panic("OSKIT_LINUX_CHECK_CURRENT: no current task");
#else
#define OSKIT_LINUX_DESTROY_CURRENT()	/* do nothing */
#define OSKIT_LINUX_CHECK_CURRENT()	/* do nothing */
#endif

