/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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

#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "fd.h"

#include <oskit/io/bufio.h>

/*
 * mmap a file - we totally ignore `addr', `prot' and `flags' here
 */
void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	oskit_file_t *file;
	oskit_error_t rc;
	void	*outaddr;
	fd_t	*fdp;

	if (fd == -1) {		/* anonymous mapping */
		return malloc(len);
	}

	if (fd_check(fd))
		return (void*)-1;

	/* 
	 * our COM interfaces cannot guarantee that modifications 
	 * will be private.
	 */
	if ((flags & MAP_PRIVATE) && (prot & PROT_WRITE)) {
		rc = ENOMEM;
		goto bad;
	}

	if (!FD_HAS_INTERFACE(fd, openfile)) {
		rc = ENOSYS;
		goto bad;
	}

	fdp = fd_array + fd;
	rc = oskit_openfile_getfile(fdp->openfile, &file);
	if (rc)
		goto bad;

	rc = oskit_file_query(file, &oskit_bufio_iid, (void **)&fdp->bufio);
	oskit_file_release(file);
	if (rc)
		goto bad;

	rc = oskit_bufio_map(fdp->bufio, &outaddr, off, len);
	if (rc)
		goto bad;

	fd_unlock(fd);
	return outaddr;

bad:
	fd_unlock(fd);
	errno = rc;
	return (void *)-1;
}

int 
munmap(void *addr, size_t len)
{
	return 0;	/* please, unmap me */
}

int 
mprotect(const void *addr, size_t len, int prot)
{
#if 0
	/* isn't that simply that? */
	return mmap(addr, len, prot, MAP_ANON, -1, 0);
#endif
	return 0;	/* oh, protect me */
}
