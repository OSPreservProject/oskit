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

#include <stdio.h>
#include <oskit/x86/tss.h>

void tss_dump(struct x86_tss *tss)
{
	printf("Dump of TSS at %p:\n", tss);
	printf("back_link %04x\n", tss->back_link & 0xffff);
	printf("ESP0 %08x SS0 %04x\n", tss->esp0, tss->ss0 & 0xffff);
	printf("ESP1 %08x SS1 %04x\n", tss->esp1, tss->ss1 & 0xffff);
	printf("ESP2 %08x SS2 %04x\n", tss->esp2, tss->ss2 & 0xffff);
	printf("CR3 %08x\n", tss->cr3);
	printf("EIP %08x EFLAGS %08x\n", tss->eip, tss->eflags);
	printf("EAX %08x EBX %08x ECX %08x EDX %08x\n",
		tss->eax, tss->ebx, tss->ecx, tss->edx);
	printf("ESI %08x EDI %08x EBP %08x ESP %08x\n",
		tss->esi, tss->edi, tss->ebp, tss->esp);
	printf("CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n",
		tss->cs & 0xffff, tss->ss & 0xffff,
		tss->ds & 0xffff, tss->es & 0xffff,
		tss->fs & 0xffff, tss->gs & 0xffff);
	printf("LDT %04x\n", tss->ldt & 0xffff);
	printf("trace_trap %04x\n", tss->trace_trap);
	printf("IOPB offset %04x\n", tss->io_bit_map_offset);
}

