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
 * create a /dev/mem
 */
#include <oskit/c/fcntl.h>
#include <oskit/c/unistd.h>
#include <oskit/c/stdio.h>
#include <oskit/c/errno.h>
#include <oskit/c/assert.h>
#include <oskit/c/fs.h>

#include <oskit/c/fd.h>
#include <oskit/fs/memfs.h>
#include <oskit/machine/base_multiboot.h>

/*
 * create a /dev/mem device under dev_name, directory for dev_name must exist!
 */
oskit_error_t
make_dev_mem(char *dev_name)
{
	int fd;
	oskit_error_t	rc;
	oskit_file_t	*mfile;
	struct multiboot_info *m = &boot_info;
	void		*start = 0;
	oskit_off_t	len;

	if (!(m->flags & MULTIBOOT_MEMORY))
		return OSKIT_EINVAL;

	if (m->mem_upper)
		len = (0x100000 + (m->mem_upper << 10));
	else 
		len = (m->mem_lower << 10);		/* ??? */

	fd = open(dev_name, O_RDWR | O_CREAT, 0x777);
	if (fd == -1)
		return errno;

	assert(fd_array[fd].openfile);

	rc = oskit_openfile_getfile(fd_array[fd].openfile, &mfile);
	if (rc)
		return rc;

	printf(">>> %s %p-%p (%d MBytes)\n", dev_name,
		start, start+len, (unsigned)(len >> 20));

	rc = oskit_memfs_file_set_contents(mfile,
	    start /* data */, 
	    len /* size */,
	    len /* allocsize, says writable length */,
	    0 /* can_sfree */, 
	    1 /* inhibit_resize */);

	oskit_file_release(mfile);
	close(fd);

	if (rc)
		return rc;
	return 0;
}
