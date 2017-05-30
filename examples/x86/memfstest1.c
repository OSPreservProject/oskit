/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * Test
 */

#include <stdio.h>
#include <string.h>
#include <oskit/fs/memfs.h>
#include <oskit/clientos.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/dev/osenv_mem.h>

#include <oskit/debug.h>

oskit_osenv_mem_t *
oskit_create_osenv_memdebug(oskit_osenv_mem_t *mem);

void
main()
{
        oskit_osenv_log_t *log;
        oskit_osenv_mem_t *raw_mem;
        oskit_osenv_mem_t *mem;
	int 	rc;
	oskit_filesystem_t *memfs;

	oskit_clientos_init();

	log = oskit_create_osenv_log();
	assert(log);
	raw_mem = oskit_create_osenv_mem();
	assert(raw_mem);
	mem = oskit_create_osenv_memdebug(raw_mem);
	assert(mem);

	printf("Grabbing memfs\n");
	rc = oskit_memfs_init_ex(mem, log, &memfs);
	if (rc)
		panic("Can't create memfs.");
	oskit_filesystem_release(memfs);

	exit(0);
}
