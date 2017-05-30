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
#ifndef _OSKIT_MEMDEBUG_H_
#define _OSKIT_MEMDEBUG_H_

#ifndef __ASSEMBLER__
/*
 * We define this so malloc.h/stdlib.h knows not to prototype
 * malloc and friends.
 * Otherwise the prototypes will get macro expanded and bad things will
 * happen.
 */
#define MALLOC_IS_MACRO

#include <oskit/compiler.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef oskit_size_t size_t;
#endif

#define malloc(s) memdebug_traced_malloc(s, 0, __FILE__, __LINE__)
#define memalign(a, s) memdebug_traced_memalign(a, s, 0, __FILE__, __LINE__)
#define calloc(n, s) memdebug_traced_calloc(n, s, __FILE__, __LINE__)
#define realloc(m, s) memdebug_traced_realloc(m, s, __FILE__, __LINE__)
#define free(p) memdebug_traced_free(p, __FILE__, __LINE__)

#define smalloc(s) memdebug_traced_smalloc(s, 0, __FILE__, __LINE__)
#define smemalign(a, s) memdebug_traced_smemalign(a, s, 0, __FILE__, __LINE__)
/* XXX these two are yet to be implemented. */
/* #define scalloc(n, s) memdebug_traced_scalloc(n, s, __FILE__, __LINE__) */
/* #define srealloc(m, s) memdebug_traced_srealloc(m, s, __FILE__, __LINE__) */
#define sfree(p, s) memdebug_traced_sfree(p, s, __FILE__, __LINE__)

OSKIT_BEGIN_DECLS

/*
 * Mark all currently allocated blocks, for a future memdebug_check().
 */
void memdebug_mark(void);

/*
 * Look for allocated blocks that are unmarked--ie, alloced and
 * not free'd since the last memdebug_mark().
 */
void memdebug_check(void);

/*
 * Run a set of vailidity checks on the given memory block ptr.
 */
#define memdebug_ptrchk(ptr) memdebug_traced_ptrchk(ptr, __FILE__, __LINE__);
int memdebug_traced_ptrchk(void *__ptr, char *file, int line);

/*
 * Sweep through all allocated memory blocks and do a validity check.
 */
#define memdebug_sweep() memdebug_traced_sweep(__FILE__, __LINE__);
void memdebug_traced_sweep(char *file, int line);

/*
 * These functions are defined in libmemdebug, and are not meant to be
 * used directly, but via the macros defined above.  But who am I to
 * tell you what to do.
 */
void *memdebug_traced_malloc(size_t bytes, unsigned flags, char *file, int line);
void *memdebug_traced_memalign(size_t alignment, size_t size, unsigned flags, char *file, int line);
void *memdebug_traced_calloc(size_t n, size_t elem_size, char *file, int line);
void memdebug_traced_free(void* mem, char *file, int line);
void *memdebug_traced_realloc(void* oldmem, size_t bytes, char *file, int line);
void *memdebug_traced_smalloc(size_t size, unsigned flags, char *file, int line);
void *memdebug_traced_smemalign(size_t alignment, size_t size, unsigned flags, char *file, int line);
void memdebug_traced_sfree(void* mem, size_t chunksize, char *file, int line);

int memdebug_printf(const char *fmt, ...);

/*
 * ``Real,'' untracked allocation functions.
 * Used by memdebug itself to acquire memory.  Provided by the client OS.
 */
void *memdebug_untraced_alloc(oskit_u32_t size,
			      oskit_u32_t align_bits, oskit_u32_t align_ofs);
void memdebug_untraced_free(void *ptr, oskit_u32_t size);

OSKIT_END_DECLS

#endif /* !__ASSEMBLER__ */
#endif _OSKIT_MEMDEBUG_H_
