/*
 * Copyright (c) 1994-1998, 2000, 2001 University of Utah and the Flux Group.
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
 * Threaded file descriptor support that puts the current thread to
 * sleep if input/output is not available.
 */

#include <oskit/dev/dev.h>
#include <sys/types.h> /* Out of /usr/include/ */

#include "native.h"
#include "native_errno.h"
#include "support.h"

/*
 * The callback routine used in oskitunix_threaded_fd_wait().
 */
static void
threaded_callback(void *arg)
{
	osenv_sleeprec_t	*sleeprec = (osenv_sleeprec_t *) arg;

	osenv_wakeup(sleeprec, OSENV_SLEEP_WAKEUP);
}

/*
 * Wait for I/O.
 */
int
oskitunix_threaded_fd_wait(int fd, unsigned iotype)
{
	int			rcode;
	osenv_sleeprec_t	sleeprec;

	osenv_sleep_init(&sleeprec);
	oskitunix_register_async_fd(fd, iotype,
				    threaded_callback, &sleeprec);

	/*
	 * If interrupted, then clear the async wait.
	 */
	if ((rcode = osenv_sleep(&sleeprec)) != OSENV_SLEEP_WAKEUP)
		oskitunix_unregister_async_fd(fd);

	return rcode;
}

/*
 * Non-blocking read from an fd.
 */
int
oskitunix_threaded_read(int fd, void* buf, size_t len)
{
	int		count;
	int		enabled;

	enabled = osenv_intr_save_disable();

	for (;;) {
		count = NATIVEOS(read)(fd, buf, len);
		if (count >= 0)
			break;

		/* a "real" error */
		if (!((NATIVEOS(errno) == EAGAIN)
		      || (NATIVEOS(errno) == EINTR))) {
			errno = NATIVEOS(errno);
			break;
		}

		/* an interruption */
		if (oskitunix_threaded_fd_wait(fd, IOTYPE_READ)) {
			errno = EINTR;
			count = -1;
			break;
		}
	}

	if (enabled)
		osenv_intr_enable();
	return count;
}

/*
 * Non-blocking write to an fd.
 */
int
oskitunix_threaded_write(int fd, const void* buf, size_t len)
{
	int		count = 1;
	const void*	ptr;
	int		enabled;

	ptr = buf;
	enabled = osenv_intr_save_disable();

	while (len > 0 && count > 0) {
		count = NATIVEOS(write)(fd, ptr, len);
		if (count >= 0) {
			ptr += count;
			len -= count;
			count = ptr - buf;
			continue;
		}

		/* a regular error */
		if (!((NATIVEOS(errno) == EAGAIN)
		      || (NATIVEOS(errno) == EINTR))) {
			errno = NATIVEOS(errno);
			break;
		}

		/* an interruption */
		if (oskitunix_threaded_fd_wait(fd, IOTYPE_WRITE)) {
			errno = EINTR;
			count = -1;
			break;
		}
		count = 1;
	}
	if (enabled)
		osenv_intr_enable();
	return count;
}

/*
 * Non-blocking socket connect.
 */
int
oskitunix_threaded_connect(int fd, struct sockaddr* addr, size_t len)
{
	int		rc;
	int		enabled;

	enabled = osenv_intr_save_disable();

	for (;;) {
		rc = NATIVEOS(connect)(fd, addr, len);
		if (rc == 0)
			break;

		/* A "real" error */
		if (!((NATIVEOS(errno) == EINPROGRESS)
		      || (NATIVEOS(errno) == EALREADY)
		      || (NATIVEOS(errno) == EINTR))) {
			errno = NATIVEOS(errno);
			break;
		}

		/* an interruption */
		if (oskitunix_threaded_fd_wait(fd, IOTYPE_WRITE)) {
			errno = EINTR;
			rc    = -1;
			break;
		}
	}
	/* annul EISCONN error */
	if ((rc < 0)
	    && (NATIVEOS(errno) == EISCONN)) 
		rc = 0;

	if (enabled)
		osenv_intr_enable();
	return rc;
}

/*
 * Non-blocking socket accept.
 */
int
oskitunix_threaded_accept(int fd, struct sockaddr* addr, size_t* len)
{
	int		newfd;
	int		enabled;

	enabled = osenv_intr_save_disable();
	for (;;) {
		newfd = NATIVEOS(accept)(fd, addr, len);
		if (newfd >= 0)
			break;

		if (!((NATIVEOS(errno) == EAGAIN)
		      || (NATIVEOS(errno) == EINTR))) {
			errno = NATIVEOS(errno);
			break;
		}

		if (oskitunix_threaded_fd_wait(fd, IOTYPE_READ)) {
			errno = EINTR;
			newfd = -1;
			break;
		}
	}

	if (enabled)
		osenv_intr_enable();
	return newfd;
}

/*
 * Non-blocking socket recvfrom
 */
int
oskitunix_threaded_recvfrom(int fd, void* buf, size_t len, int flags,
	struct sockaddr* from, int* fromlen)
{
	int		count;
	int		enabled;

	enabled = osenv_intr_save_disable();
	for (;;) {
		count = NATIVEOS(recvfrom)(fd, buf, len, flags, from, fromlen);
		if (count >= 0)
			break;

		if (!((NATIVEOS(errno) == EAGAIN)
		      || (NATIVEOS(errno) == EINTR)))
		{
			errno = NATIVEOS(errno);
			break;
		}

		if (oskitunix_threaded_fd_wait(fd, IOTYPE_READ)) {
			errno = EINTR;
			count = -1;
			break;
		}
	}

	if (enabled)
		osenv_intr_enable();
	return count;
}

/*
 * Non-blocking socket sendto
 */
int
oskitunix_threaded_sendto(int fd, const void* buf, size_t len, int flags,
			  struct sockaddr* to, int tolen)
{
	int		total = 0, count;
	int		enabled;

	enabled = osenv_intr_save_disable();
	while (len > 0) {
		count = NATIVEOS(sendto)(fd, buf, len, flags, to, tolen);
		if (count >= 0) {
			buf   += count;
			len   -= count;
			total += count;
			continue;
		}

		if (!((NATIVEOS(errno) == EAGAIN)
		      || (NATIVEOS(errno) == EINTR))) 
		{
			errno = NATIVEOS(errno);
			break;
		}

		if (oskitunix_threaded_fd_wait(fd, IOTYPE_WRITE)) {
			errno = EINTR;
			total = -1;
			break;
		}
		count = 1;
	}

	if (enabled)
		osenv_intr_enable();
	return total;
}


/*
 * Set a file descriptor to non-blocking, async notify I/O. 
 *
 * Returns the native errno if such is necessary.  0, otherwise.
 */
int
oskitunix_set_async_fd(int fd)
{
	int	rc;
	int     mypid = NATIVEOS(getpid)();

	rc = NATIVEOS(fcntl)(fd, F_SETOWN, mypid);
	if (rc == -1) {
		return NATIVEOS(errno);
	}

	rc = NATIVEOS(fcntl)(fd, F_SETFL, O_ASYNC | O_NONBLOCK);
	if (rc == -1) {
		return NATIVEOS(errno);
	}

	return 0;
}

/*
 * Turn off async notify / non-blocking for a file descriptor.
 */
int
oskitunix_unset_async_fd(int fd)
{
	int	rc;

	rc = NATIVEOS(fcntl)(fd, F_SETFL, 0);
	if (rc == -1) {
		return NATIVEOS(errno);
	}

	return 0;
}
