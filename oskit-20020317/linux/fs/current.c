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
 * Code to initialize `current' from a stack-allocated task_struct.
 */

#include <oskit/types.h>
#include <oskit/principal.h>
#include <oskit/error.h>

#include <linux/malloc.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include "current.h"

oskit_error_t
fs_linux_create_current(struct task_struct *ts)
{
	oskit_principal_t *prin;
	oskit_identity_t id;
	oskit_error_t err;
	int i;

	ts->state = TASK_RUNNING;

	ts->counter = 0;

	err = oskit_get_call_context(&oskit_principal_iid, (void *)&prin);
	if (!err) {
		err = oskit_principal_getid(prin, &id);
		if (err) {
			oskit_principal_release(prin);
			return err;
		}
		ts->fsuid = id.uid;
		ts->fsgid = id.gid;
		ts->uid = id.uid;
		ts->gid = id.gid;
		for (i = 0; i < id.ngroups && i < NGROUPS; i++)
			ts->groups[i] = id.groups[i];
		for ( ; i < NGROUPS; i++)
			ts->groups[i] = NOGROUP;
		oskit_principal_release(prin);
	}
	else {
		ts->fsuid = ts->fsgid = ts->uid = ts->gid = 0;
		
		for (i = 0; i < NGROUPS; i++)
			ts->groups[i] = NOGROUP;
	}
	ts->rlim[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	ts->rlim[RLIMIT_FSIZE].rlim_max = RLIM_INFINITY;

        ts->link_count = 0;

	ts->fs = kmalloc(sizeof(struct fs_struct), GFP_KERNEL);
	if (ts->fs == NULL)
		return OSKIT_ENOMEM;
	atomic_set(&ts->fs->count, 0);
	ts->fs->umask = 0;
	ts->fs->root = NULL;
	ts->fs->pwd = NULL;

	current = ts;

	return 0;
}

void
fs_linux_destroy_current(void)
{
	kfree(current->fs);
	current = (struct task_struct *)0xdeadcccc;
}

