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
 * This has declarations for things here that are not Linux functions.
 */
#ifndef _LINUX_SHARED_H_
#define _LINUX_SHARED_H_

#include <oskit/dev/dev.h>
#include <oskit/dev/osenv.h>

void oskit_linux_init(void);
void oskit_linux_osenv_init(oskit_osenv_t *osenv);

/*
 * These functions are simple wrappers around osenv_mem_alloc/free,
 * which save and restore the current pointer around the actual call;
 * this is necessary because the calls may block and current may be changed.
 */
void *oskit_linux_mem_alloc(oskit_size_t size, osenv_memflags_t flags,
			   unsigned align);
void oskit_linux_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size);

#endif

