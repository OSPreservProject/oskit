/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_SYS_MMAN_H_
#define _OSKIT_C_SYS_MMAN_H_

#include <oskit/compiler.h>
#include <oskit/c/sys/types.h>

/*
 * Protections are chosen from these bits, or-ed together.
 * NB: These are the same values as the FLUKE_PROT_xxx definitions,
 * and they can be used interchangeably.
 * They are also the same values used in traditional Unix, BSD, and Linux
 * though READ/EXEC are reversed from the traditional Unix filesystem values.
 */
#define	PROT_READ	0x01	/* Pages can be read */
#define	PROT_WRITE	0x02	/* Pages can be written */
#define	PROT_EXEC	0x04	/* Pages can be executed */
#define PROT_NONE	0x00	/* No access permitted */

/*
 * Flags for mmap().
 * These are the same values as in BSD.
 */
#define MAP_SHARED	0x0001	/* Writes change the underlying file */
#define MAP_PRIVATE	0x0002	/* Writes only change our mapping */
#define MAP_FIXED	0x0010	/* Must use specified address exactly */

#define MAP_FILE	0x0000	/* Map from file (default) */
#define MAP_ANON	0x1000	/* Allocate from memory */
#define MAP_ANONYMOUS	MAP_ANON /* some systems use this name for MAP_ANON */

/*
 * Failure return value for mmmap
 */
#define MAP_FAILED	((void *)-1)

/*
 * Flags for the msync() call.
 */
#define MS_ASYNC	0x0001	/* Perform asynchronous writes */
#define MS_SYNC		0x0000	/* Perform synchronous writes (default) */
#define	MS_INVALIDATE	0x0002	/* Invalidate cached data */

/*
 * Flags for the mlockall() call.
 */
#define MCL_CURRENT	0x0001	/* Lock all currently mapped memory */
#define MCL_FUTURE	0x0002	/* Lock all memory mapped in the future */

/*
 * Flags (possibly) returned by the mincore() call.
 */
#define MINCORE_INCORE           0x1 /* Page is in core */
#define MINCORE_REFERENCED       0x2 /* Page has been referenced by us */
#define MINCORE_MODIFIED         0x4 /* Page has been modified by us */
#define MINCORE_REFERENCED_OTHER 0x8 /* Page has been referenced */
#define MINCORE_MODIFIED_OTHER  0x10 /* Page has been modified */

/*
 * Advice to madvise (from NetBSD)
 */
#define	MADV_NORMAL	0	/* no further special treatment */
#define	MADV_RANDOM	1	/* expect random page references */
#define	MADV_SEQUENTIAL	2	/* expect sequential page references */
#define	MADV_WILLNEED	3	/* will need these pages */
#define	MADV_DONTNEED	4	/* dont need these pages */
#define	MADV_SPACEAVAIL	5	/* insure that resources are reserved */
#define	MADV_FREE	6	/* pages are empty, free them */

OSKIT_BEGIN_DECLS

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
int munmap(void *addr, size_t len);
int mprotect(const void *addr, size_t len, int prot);
int mincore(void *addr, size_t len, char *pagemap);

OSKIT_END_DECLS

#endif /* _OSKIT_C_SYS_MMAN_H_ */
