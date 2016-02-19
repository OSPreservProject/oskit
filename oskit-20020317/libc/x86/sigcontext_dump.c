/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
#include <signal.h>

void
sigcontext_dump_stack_trace(struct sigcontext *scp)
{
	unsigned int *fp, i;

	fp = (unsigned int *)scp->sc_ebp;

	printf("Backtrace:");
	for (i = 0; i < 16; i++) {
		if ((i % 8) == 0)
			printf("\n");

		fp = (int *)(*fp);
		if (!(*(fp + 1) && *fp))
			break;

		printf(" %08x", *(fp + 1));
	}
	printf("\n");
}

void
sigcontext_dump(struct sigcontext *scp)
{
	printf("EAX %08x EBX %08x ECX %08x EDX %08x\n",
		scp->sc_eax, scp->sc_ebx, scp->sc_ecx, scp->sc_edx);
	printf("ESI %08x EDI %08x EBP %08x ESP %08x\n",
		scp->sc_esi, scp->sc_edi, scp->sc_ebp, scp->sc_esp);
	printf("EIP %08x EFLAGS %08x\n", scp->sc_eip, scp->sc_efl);
	printf("CS %04x SS %04x DS %04x ES %04x\n",
		scp->sc_cs & 0xffff, scp->sc_ss & 0xffff,
		scp->sc_ds & 0xffff, scp->sc_es & 0xffff);
	sigcontext_dump_stack_trace(scp);
}
