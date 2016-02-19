/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
 * Common, private stuff for libmemdebug.
 *
 * This is not the header file to include in clients of the library.
 */
#ifndef _OSKIT_MEMDEBUG_MEMDEBUG_H
#define _OSKIT_MEMDEBUG_MEMDEBUG_H

#include <malloc.h>

/*
 * Some configuration options for the library:
 */
  /* If non-zero, have the program panic() when a malloc() fails. */
#  define NO_MEM_FATAL 0

  /* If non-zero, then malloc(0)'s and free(NULL)'s are allowed. */
#  define ALLOW_MORALLY_QUESTIONABLE_PRACTICE 1

  /* If we're allowing morally questionably allocations, does a malloc(0) return
   * NULL or a vaild pointer? */
#  define MALLOC_0_RET_NULL 1

  /* if something suspicious is detected, do you want a stack trace? */
#  define DUMP_STACK_WITH_INFO 0
  /* Debug the debugger: */
#  if 0
#    define DPRINTF(fmt, args... ) memdebug_printf(__FUNCTION__  ":" fmt , ## args)
#  else
#    define DPRINTF(fmt, args... )
#  endif



/*
 * Since there are effectively two classes of memory allocation calls,
 * malloc()-style and smalloc()-style, we differentiate them, and
 * double check that allocation style matches free style.
 */
#define SMALLOC_CALLER  0x0
#define MALLOC_CALLER   0xF

/*
 * Memory chunks are wiped with different values depending on why.
 * Ideally this should aid in debugging.
 */
#define MALLOC_INIT  0x00bb
#define FREE_WIPE    0x00dd

#define SMALLOC_INIT 0x00aa
#define SFREE_WIPE   0x00ee

/*
 * Values for the "magic" field of the fences.  head and tail are set
 * in the respective fence, when its allocated (and checked when its
 * freed).  both are set to FREE_MAGIC after they've been released.
 */
#define HEAD_MAGIC 0x68454164   /* == "hEAd" */
#define TAIL_MAGIC 0x7441496C   /* == "tAIl" */
#define FREE_MAGIC 0x66524565   /* == "fREe" */

#define MHEAD_DEADBEEF 4        /* Length of over/under-run buffer */
#define MHEAD_BTLEN    8        /* Length of backtrace to store. */

#define NO_FLAGS        0
#define MARKED_FLAG     1
#define SMALLOC_CREATED 2

typedef struct memdebug_mhead
{
      unsigned magic;
      int  line;
      char *file;
      /* size is the *word aligned* user request. (NOT block plus fences) */
      unsigned size;
      /* reqsize of the original user request size, for debug printfs only */
      unsigned reqsize;
      struct memdebug_mhead *next;
      struct memdebug_mhead *prev;
      unsigned flags;
      unsigned backtrace[MHEAD_BTLEN];
      unsigned deadbeef[MHEAD_DEADBEEF];
} memdebug_mhead;

/*
 * Define the smallest power of two greater than
 * sizeof(memdebug_mhead).  An assert will back me up.  size == (7 + 8
 * + 4) * 4 == 96 bytes!!  Yeow.
 */
#define NP2_SZ_MHEAD (128)

#define MTAIL_DEADBEEF 4
typedef struct memdebug_mtail
{
      unsigned deadbeef[MTAIL_DEADBEEF];
      unsigned size;
      unsigned magic;
} memdebug_mtail;

/*
 * Alignment operator.
 */
#define word_align(x)		((((unsigned int) (x)) + 3) & ~3)

/*
 * A list of all the allocations made is linked off this fake
 * memdebug_mhead.
 */
extern memdebug_mhead memdebug_all_head;

/*
 * A printf routine that can be guaranteed not to generate any
 * malloc() calls.
 */
int memdebug_printf(const char *fmt, ...);

void * memdebug_untraced_alloc(oskit_u32_t size,
                               oskit_u32_t align_bits, oskit_u32_t align_ofs);
void memdebug_untraced_free(void *ptr, oskit_u32_t size);

/* All allocation requests come through this function */
void* memdebug_alloc(size_t alignment, size_t bytes, unsigned flags,
		     unsigned char initval, char caller, char *file, int line);

/* All release requests come through this function */
int memdebug_free(void* mem, int wipeval, char caller, int blocksize,
		  char *file, int line);

/* All getsize requests come through this function */
int memdebug_getsize(void *mem, int *blocksize, char *file, int line);

/* Prints a bogosity message and formats the backtrace stored in mhead */
void memdebug_bogosity(memdebug_mhead *head);

/* Run sanity checks on a pointer and its associated fence posts */
int memdebug_ptrchk(void *ptr);
int memdebug_traced_ptrchk(void *ptr, char *file, int line);

/* Print file and line info in a standard way. */
void memdebug_info_dump(const char *str, const char *file, int line);

/* Generate a machine-specific backtrace and store it in the buffer */
void memdebug_store_backtrace(unsigned *backtrace, int max_len);

#endif /* _OSKIT_MEMDEBUG_MEMDEBUG_H */
