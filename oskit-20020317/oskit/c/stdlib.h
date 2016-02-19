/*
 * Copyright (c) 1994, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_STDLIB_H_
#define _OSKIT_C_STDLIB_H_

#include <oskit/types.h>
#include <oskit/compiler.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef oskit_size_t size_t;
#endif

#ifndef NULL
#define NULL 0
#endif

#define RAND_MAX 0x7fffffff

#define MB_CUR_MAX	1	/* We don't do locales, so it's always 1. */
#define MB_LEN_MAX	1

OSKIT_BEGIN_DECLS

/*
 * These function prototypes are roughly in the same order
 * as their descriptions in the ISO/ANSI C standard, section 7.10.
 */

#define atoi(str) ((int)atol(str))
long atol(const char *__str);
long strtol(const char *__p, char **__out_p, int __base);
unsigned long strtoul(const char *__p, char **__out_p, int __base);
double   strtod (const char *, char **);
double   atof (const char *);

int rand(void);
void srand(unsigned __seed);

/* Don't macro expand protos please. */
#ifndef MALLOC_IS_MACRO
void *malloc(size_t __size);
void *calloc(size_t __nelt, size_t __eltsize);
void *realloc(void *__buf, size_t __new_size);
void free(void *__buf);

/* OSKit additions */
void *mustmalloc(size_t __size);
void *mustcalloc(size_t __nelt, size_t __eltsize);

#endif /* MALLOC_IS_MACRO */

void abort(void) OSKIT_NORETURN;
void exit(int __status) OSKIT_NORETURN;

/* OSKit additions */
void panic(const char *__format, ...) OSKIT_NORETURN;

int atexit(void (*__function)(void));

char *getenv(const char *__name);

void qsort(void *__array, size_t __nelt, size_t __eltsize,
	   int (*__cmp)(void *, void *));

#define abs(n) __builtin_abs(n)


OSKIT_END_DECLS

#endif /* _OSKIT_C_STDLIB_H_ */
