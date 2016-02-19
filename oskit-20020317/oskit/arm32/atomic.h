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

#ifndef	_OSKIT_ARM32_ATOMIC_H_
#define	_OSKIT_ARM32_ATOMIC_H_

/*
 * Use the interrupt disable approach for now ... Since the amount of
 * time that interrupts are going to be disabled is tiny, do not go
 * through the osenv interfaces; just disable interrupts directly. 
 */

#include <oskit/compiler.h>
#include <oskit/config.h>
#include <oskit/arm32/types.h>
#include <oskit/arm32/proc_reg.h>

typedef struct { oskit_s32_t counter; } atomic_t;

#define atomic_read(v)		((v)->counter)
#define atomic_set(v, i)	(((v)->counter) = (i))

OSKIT_INLINE void
atomic_add(volatile atomic_t *v, oskit_s32_t i)
{
	int	enabled = interrupts_enabled();

	if (enabled)
		disable_interrupts();

	v->counter += i;

	if (enabled)
		enable_interrupts();
}

OSKIT_INLINE void
atomic_sub(volatile atomic_t *v, oskit_s32_t i)
{
	int	enabled = interrupts_enabled();

	if (enabled)
		disable_interrupts();

	v->counter -= i;

	if (enabled)
		enable_interrupts();
}

OSKIT_INLINE void
atomic_inc(volatile atomic_t *v)
{
	int	enabled = interrupts_enabled();

	if (enabled)
		disable_interrupts();

	v->counter++;

	if (enabled)
		enable_interrupts();
}

OSKIT_INLINE void
atomic_dec(volatile atomic_t *v)
{
	int	enabled = interrupts_enabled();

	if (enabled)
		disable_interrupts();

	v->counter--;

	if (enabled)
		enable_interrupts();
}

/*
 * Return true when result of decrement is zero.
 */
OSKIT_INLINE int
atomic_dec_and_test(volatile atomic_t *v)
{
	int	result;
	int	enabled = interrupts_enabled();

	if (enabled)
		disable_interrupts();

	v->counter--;
	result = (v->counter == 0);

	if (enabled)
		enable_interrupts();

	return result;
}

#endif /* _OSKIT_ARM32_ATOMIC_H_ */
