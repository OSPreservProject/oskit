/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#include <malloc.h>
#include <errno.h>

#include <oskit/io/bufio.h>
#include <oskit/fs/openfile.h>

#include <oskit/svm.h>

static inline int
convert_prot(int prot)
{
	int svm_prot = 0;
	if (prot & PROT_READ)
		svm_prot |= SVM_PROT_READ;
	if (prot & PROT_WRITE)
		svm_prot |= SVM_PROT_WRITE;
	return svm_prot;
}

void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	oskit_error_t rc;

	if (len == 0)
		return 0;

	if (flags & MAP_ANON) {
		rc = svm_alloc((oskit_addr_t *)&addr, len, convert_prot(prot),
			       (flags & MAP_FIXED) ? SVM_ALLOC_FIXED : 0);
		if (rc) {
			errno = rc;
			return MAP_FAILED;
		}
		return addr;
	}

	errno = ENOSYS;
	return MAP_FAILED;
}

int
munmap(void *addr, size_t len)
{
	oskit_error_t rc;

	if (len == 0)
		return 0;

	rc = svm_dealloc((oskit_addr_t) addr, len);
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}

int
mprotect(const void *addr, size_t len, int prot)
{
	oskit_error_t rc;

	if (len == 0)
		return 0;

	rc = svm_protect((oskit_addr_t) addr, len, convert_prot(prot));
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}
