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

#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#include "fd.h"
#include "fs.h"
#include <oskit/fs/soa.h>

int
open(const char *name, int flags, ...)
{
	oskit_file_t *f;
	oskit_openfile_t *fsfd;
	int fd, mode, rc = 0, excl = 0;
	va_list pvar;

	va_start(pvar, flags);
	mode = va_arg(pvar, int);
	va_end(pvar);

#ifdef	THREAD_SAFE
	pthread_testcancel();
#endif

	if (flags & O_EXCL)
		excl = 1;

#if VERBOSITY > 1
        printf("open '%s' flags %08x, mode %o\n", name, flags, mode);
#endif

	rc = fs_lookup(name, FSLOOKUP_FOLLOW, &f);

	/* create if not found and creat flag set */
	if ((rc == OSKIT_ENOENT) && (flags & O_CREAT)) {
		oskit_dir_t *dir;

		if ((rc = fs_lookup_dir(&name, FSLOOKUP_NOFLAGS, &dir)) != 0)
			return errno = rc, -1;

		rc = oskit_dir_create(dir, name, excl, mode &~ fs_cmask, &f);
		oskit_dir_release(dir);
	}

	if (rc) {
		errno = rc;
		return -1;
	}

	/*
	 * now, the file has either been found or created -
	 * we hold a reference to it in `f'
	 */

	rc = oskit_file_open(f, flags & ~(O_CREAT | O_EXCL), &fsfd);
        if (rc) {
		oskit_file_release(f);
#if VERBOSITY > 0
                printf("FAIL: open '%s' flags %08x, errno=0x%x\n",
                        name, flags, rc);
#endif
		errno = rc;
                return -1;
        }
	if (fsfd == NULL) {
		/*
		 * oskit_file_open may succeed but return a NULL openfile,
		 * if the file system does not implement per-open state.
		 * In that case we must do it ourselves.
		 */
		rc = oskit_soa_openfile_from_file(f,
					  flags & (O_RDWR | O_APPEND), &fsfd);
		if (rc) {
			oskit_file_release(f);
			errno = rc;
			return -1;
		}
	}
	/*
	 * COM open doesn't truncate, so we must do it
	 */
	if (flags & O_TRUNC) {
		struct oskit_stat	st;
		st.size = 0;
		rc = oskit_file_setstat(f, OSKIT_STAT_SIZE, &st);
		if (rc)
			return errno = rc, -1;
	}
	oskit_file_release(f);

	/* assign a file descriptor to the new openfile */
        fd = fd_alloc((oskit_iunknown_t*)fsfd, 0);
	if (fd < 0) {
		return -1;
	}
	fd_lock(fd);
	fd_array[fd].openfile = fsfd;
	fd_unlock(fd);

        return fd;
}
