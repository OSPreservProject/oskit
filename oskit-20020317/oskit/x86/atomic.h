/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

#ifndef	_OSKIT_X86_ATOMIC_H_
#define	_OSKIT_X86_ATOMIC_H_

#include <oskit/compiler.h>
#include <oskit/config.h>
#include <oskit/x86/types.h>

typedef struct { oskit_s32_t counter; } atomic_t;

#define atomic_read(v)		((v)->counter)
#define atomic_set(v, i)	(((v)->counter) = (i))

/*
 * In case we do SMP
 */
#define MPLOCK ""

OSKIT_INLINE void
atomic_add(volatile atomic_t *v, oskit_s32_t i)
{
	__asm__ __volatile__(
		MPLOCK "addl %1,%0"
		: "=m" (v->counter)
		: "ir" (i), "m" (v->counter));
}

OSKIT_INLINE void
atomic_sub(volatile atomic_t *v, oskit_s32_t i)
{
	__asm__ __volatile__(
		MPLOCK "subl %1,%0"
		: "=m" (v->counter)
		: "ir" (i), "m" (v->counter));
}

OSKIT_INLINE void
atomic_inc(volatile atomic_t *v)
{
	__asm__ __volatile__(
		MPLOCK "incl %0"
		: "=m" (v->counter)
		: "m" (v->counter));
}

OSKIT_INLINE void
atomic_dec(volatile atomic_t *v)
{
	__asm__ __volatile__(
		MPLOCK "decl %0"
		: "=m" (v->counter)
		: "m" (v->counter));
}

/*
 * Return true when result of decrement is zero.
 */
OSKIT_INLINE int
atomic_dec_and_test(volatile atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		MPLOCK "decl %0; sete %1"
		: "=m" (v->counter), "=qm" (c)
		: "m" (v->counter));
	
	return c != 0;
}

#endif /* _OSKIT_X86_ATOMIC_H_ */
