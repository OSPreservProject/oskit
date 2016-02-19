/*
 * Copyright (c) 1996-1998 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit SMP Support Library, which is free software,
 * also known as "open source;" you can redistribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), version 2, as published
 * by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*
 * This is the critical_section code.  It is NOT designed to provide
 * a locking mechanism for general use; however, it does provide a
 * simple recursive lock which can be used for debugging and speed-
 * insensitive code (such as puts/putc).  Critical sections should 
 * have their own locks, rather than all sharing a common lock.
 */

#include <oskit/smp.h>
#include <oskit/x86/smp.h>
#include <oskit/base_critical.h>
#include <oskit/x86/proc_reg.h>
#include "linux-smp.h"

/* This also acts as the spin lock; -1 means unlocked. */
static volatile unsigned critical_cpu_id = -1;

static unsigned critical_nest_count;
static unsigned critical_saved_eflags;

extern unsigned int num_processors;

void base_critical_enter(void)
{
	unsigned old_eflags = get_eflags();
	unsigned cpu_id = (num_processors > 1) ? smp_find_cur_cpu() : 0;

	/* First make sure we get no interference from interrupt activity. */
	cli();

	/* If we already own the lock, just increment the count and return. */
	if (critical_cpu_id == cpu_id) {
		critical_nest_count++;
		return;
	}

	/* Lock the global spin lock, waiting if another processor has it. */
	asm volatile("1: movl $-1,%%eax; lock; cmpxchgl %0,(%1); jne 1b"
		     : : "r" (cpu_id), "r" (&critical_cpu_id) : "eax");

	critical_nest_count = 0;
	critical_saved_eflags = old_eflags;
}

void base_critical_leave(void)
{
	unsigned old_eflags;

	if (critical_nest_count > 0) {
		critical_nest_count--;
		return;
	}

	old_eflags = critical_saved_eflags;

	/* Unlock the global spin lock. */
	asm volatile("movl %0,(%1)"
		     : : "r" (-1), "r" (&critical_cpu_id));

	set_eflags(old_eflags);
}

