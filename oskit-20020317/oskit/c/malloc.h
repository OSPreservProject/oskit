/*
 * Copyright (c) 1994, 1998, 1999 University of Utah and the Flux Group.
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
 * This header file defines common types and functions used by
 * the minimal C library's default memory allocation functions.
 * This header file is NOT a standard POSIX or Unix header file;
 * instead its purpose is to expose the implementation of this facility
 * so that the client can fully control it and use it in arbitrary contexts.
 */
#ifndef _OSKIT_C_MALLOC_H_
#define _OSKIT_C_MALLOC_H_

#include <oskit/types.h>
#include <oskit/compiler.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef oskit_size_t size_t;
#endif

OSKIT_BEGIN_DECLS

/*
 * Don't macro expand protos please.
 */
#ifndef MALLOC_IS_MACRO

void *malloc(size_t size);
void *mustmalloc(size_t size);
void *memalign(size_t alignment, size_t size);
void *calloc(size_t nelt, size_t eltsize);
void *mustcalloc(size_t nelt, size_t eltsize);
void *realloc(void *buf, size_t new_size);
void free(void *buf);

/*
 * Alternate version of the standard malloc functions that expect the
 * caller to keep track of the size of allocated chunks.  These
 * versions are _much_ more memory-efficient when allocating many
 * chunks naturally aligned to their (natural) size (e.g. allocating
 * naturally-aligned pages or superpages), because normal memalign
 * requires a prefix between each chunk which will create horrendous
 * fragmentation and memory loss.  Chunks allocated with these
 * functions must be freed with sfree() rather than the ordinary
 * free().
 */
void *smalloc(size_t size);
void *smemalign(size_t alignment, size_t size);
void *scalloc(size_t size);
void *srealloc(void *buf, size_t old_size, size_t new_size);
void sfree(void *buf, size_t size);

#endif /* MALLOC_IS_MACRO */

/*
 * These functions by default do nothing, but can be overridden by the
 * C library when it detects that there is a registered lock manager,
 * and thus needs to provide proper locking.
 */
void mem_lock(void);
void mem_unlock(void);
void mem_lock_init(void);

OSKIT_END_DECLS

#endif /* _OSKIT_C_MALLOC_H_ */
