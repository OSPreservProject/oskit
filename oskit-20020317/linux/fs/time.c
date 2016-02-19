/*
 * Copyright (c) 1997, 1998, 1999 The University of Utah and the Flux Group.
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
/*
 * Define a function to emulate Linux's CURRENT_TIME macro.
 */

#include <oskit/types.h>
#include <oskit/fs/fs.h>
#include <assert.h>

#include <linux/sched.h>

/*
 * Linux CURRENT_TIME in <linux/sched.h> is #defined to this.
 */
int
fs_linux_current_time()
{
	struct timeval tv, *tvp = &tv;
	struct task_struct *task;

	task = current;
	/* Right thing to do? */
	if (gettimeofday(tvp, 0))
		panic("fs_linux_current_time: gettimeofday failed");
	current = task;

	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	
	return tv.tv_sec;
}

