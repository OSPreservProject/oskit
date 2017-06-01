/*
 * Copyright (c) 1997 M.I.T.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by MIT.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Optimization routines for dpf.  They are optional.  
 * 
 * One thing that we should do is make this exploit 64-bit loads.
 * See coalesce for ramblings on how to make the optimizations optimal.
 */
#include <dpf-internal.h>

/* 
 * Consumes a sorted ir and produces a coalesced version.
 * Very simplistic: coalesces only when bitfields abut. 
 *
 * Sophistication (if it would be useful) would come from
 * coalescing with gaps and (the biggie) making machine 
 * specific decisions as to when to coalesce (as opposed
 * the current greedy strategy) by trading the number of
 * chunks we create vs. the cost of any unaligned loads
 * we introduce.  Fortunately from ease of implementation
 * but unfortunately from a puzzle standpoint, I don't think
 * the extra optimization is particularly important.
 */
void dpf_coalesce(struct dpf_ir *irs) {
/* should make this the wordsize of the machine. */
#define DPF_MAXBITS 32
/* ulword */
	struct ir *ir, *p;
	int i,n, coalesced;
	struct eq *eq1, *eq2;

	for(p = ir = &irs->ir[0], coalesced = 0, i = 1, n = irs->irn; i < n; i++) {
		eq1 = &p->u.eq;
		eq2 = &ir[i].u.eq;

		/*
		 * If both ops are eq's, and they abut, and their
		 * length does not exceed the wordsize, coalesce.
 	 	 * 
		 * (Should we do something useful with overlap?)
		 */
		if (eq1->op == DPF_OP_EQ && eq2->op == DPF_OP_EQ
		&& (eq1->nbits + eq1->offset * 8) == eq2->offset * 8
		&& (eq1->nbits + eq2->nbits) <= DPF_MAXBITS) {
			eq1->mask |= eq2->mask << eq1->nbits;
			eq1->val |= eq2->val << eq1->nbits;
			eq1->nbits += eq2->nbits;
			coalesced++;
			continue;
		} else
			*++p = ir[i];
	}
	irs->irn -= coalesced;
	ir[irs->irn].u.eq.op = DPF_OP_END;
}

/* 
 * Given a shift expression, compute the minimum alignement it guarentees. 
 * This guarentee is used to remove the need to load each word and half 
 * word a byte at a time.  Alignment is monotonically decreasing.  Consider
 * the case where our lower bound is 2 bytes but the actual alignment is
 * 4 --- if we encounter another 2 byte alignment, upgrading the entire
 * alignement would give a bogus guarentee of 4.
 *
 * Recall that a shift represents the expression:
 *	msg + (msg[offset:nbits] & mask) << shift
 * The algorithm:
 *   Alignment starts at DPF_MSG_ALIGNED.
 *	Shifts to the right increase the alignment by 1 << shift.
 *	Masks with lower bits set to zero increase it by the bytes
 *		that are zeroed out.
 *	These alignments are added together.
 *  Addition of alignment and computed alignment is simply min.
 */
int dpf_compute_alignment(struct ir *ir, int alignment) {
	struct shift *s;
	unsigned ca;

	s = &ir->u.shift;
	
	/* (1 << first_bit(mask)) + shift gives the computed alignment */
	if((ca = (s->mask & -s->mask)) == 1)
		ca = 0;		/* correct for mask with low bit set. */

	if(!(ca += s->shift))
		ca = 1;		/* correct again for 0 alignment */

	/* The aligment of ir is minimum of alignment and computed alignment. */
	return ir->u.shift.align =  ca < alignment ? ca : alignment;
}
