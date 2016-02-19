/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
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
 * Linux port management.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/ioport.h>
#include <linux/sched.h>
#include "osenv.h"

int
check_region(unsigned long port, unsigned long size)
{
	struct task_struct *cur = current;
	int avail;

	avail = osenv_io_avail((oskit_addr_t) port, (oskit_size_t) size);

	current = cur;
	return avail ? 0 : -EBUSY;
}

void
request_region(unsigned long port, unsigned long size, const char *name)
{
	struct task_struct *cur = current;

	osenv_io_alloc((oskit_addr_t) port, (oskit_size_t) size);

	current = cur;
}

void
release_region(unsigned long port, unsigned long size)
{
	struct task_struct *cur = current;

	osenv_io_free((oskit_addr_t) port, (oskit_size_t) size);

	current = cur;
}
