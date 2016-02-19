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

struct frame {
	unsigned int   fr_fp;
        unsigned int   fr_sp;        
	unsigned int   fr_lr;
        unsigned int   fr_pc;
};

#define GET_FP(x) ((struct frame *)(x) - \
		   ((sizeof(struct frame) - sizeof(unsigned int))))

void
sigcontext_dump_stack_trace(struct sigcontext *scp)
{
	unsigned int i;
	struct frame *fp, *lastfp;


	lastfp = NULL;
	fp = GET_FP(scp->sc_r11);

	printf("Backtrace:");
	for (i = 0; i < 16; i++) {
		if ((i % 8) == 0)
			printf("\n");
		
		lastfp = fp;
		fp = GET_FP(fp->fr_fp);
		if (fp == NULL)
			break;
		
		if (fp <= lastfp) {
			printf("bad frame pointer %p\n", fp);
			break;
		} else
			printf(" %08x", fp->fr_pc);
	}
	printf("\n");
}

void
sigcontext_dump(struct sigcontext *scp)
{
	int i;

	printf("R0  %08x R1  %08x R2  %08x R3  %08x R4  %08x\n",
	       scp->sc_r0, scp->sc_r1, scp->sc_r2, scp->sc_r3, scp->sc_r4); 
	printf("R5  %08x R6  %08x R7  %08x R8  %08x R9  %08x\n",
	       scp->sc_r5, scp->sc_r6, scp->sc_r7, scp->sc_r8, scp->sc_r9); 
	printf("R10 %08x R11 %08x R12 %08x\n", 
	       scp->sc_r10, scp->sc_r11, scp->sc_r12);
	
	printf("SPSR %08x USR_SP %08x USR_LR %08x SVC_LR %08x PC %08x\n",
	       scp->sc_spsr, scp->sc_usr_sp, scp->sc_usr_lr, 
	       scp->sc_svc_lr, scp->sc_pc);

	sigcontext_dump_stack_trace(scp);
}
