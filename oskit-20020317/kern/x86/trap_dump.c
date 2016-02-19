/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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

#include <stdio.h>
#include <oskit/x86/trap.h>
#include <oskit/x86/eflags.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/base_trap.h>

void trap_dump(const struct trap_state *st)
{
	int from_user = (st->cs & 3) || (st->eflags & EFL_VM);
	int i;

	printf("Dump of trap_state at %p:\n", st);
	printf("EAX %08x EBX %08x ECX %08x EDX %08x\n",
		st->eax, st->ebx, st->ecx, st->edx);
	printf("ESI %08x EDI %08x EBP %08x ESP %08x\n",
		st->esi, st->edi, st->ebp,
		from_user ? st->esp : (unsigned)&st->esp);
	printf("EIP %08x EFLAGS %08x\n", st->eip, st->eflags);
	printf("CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n",
		st->cs & 0xffff, from_user ? st->ss & 0xffff : get_ss(),
		st->ds & 0xffff, st->es & 0xffff,
		st->fs & 0xffff, st->gs & 0xffff);
	if (st->eflags & EFL_VM)
		printf("v86:            DS %04x ES %04x FS %04x GS %04x\n",
			st->v86_ds & 0xffff, st->v86_es & 0xffff,
			st->v86_gs & 0xffff, st->v86_gs & 0xffff);
	printf("trapno %d, error %08x, from %s mode\n",
		st->trapno, st->err, from_user ? "user" : "kernel");
	if (st->trapno == T_PAGE_FAULT)
		printf("page fault linear address %08x\n", st->cr2);

	/* Dump the top of the stack too.  */
	if (!from_user)
	{
		for (i = 0; i < 32; i++)
		{
			printf("%08x%c", (&st->esp)[i],
				((i & 7) == 7) ? '\n' : ' ');
		}
	}
}

