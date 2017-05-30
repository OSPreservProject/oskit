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


#include "dpf-internal.h"
#include <string.h>

#define IDENTITY_MASK8	0xffUL
#define IDENTITY_MASK16 0xffffUL
#define IDENTITY_MASK32	0xffffffffUL

void dpf_init(struct dpf_ir *ir) {
	memset(ir, 0, sizeof *ir);
}

static inline struct ir *alloc(struct dpf_ir *ir) {
	if(ir->irn >= DPF_MAXELEM)
		fatal(Code size exceeded.  Increase DPF_MAX.);
	return &ir->ir[ir->irn++];
}

static inline void 
mkeq(struct dpf_ir *ir, uint8 nbits, uint16 offset, uint32 mask, uint32 val) {
	struct eq *eq;

	eq = &alloc(ir)->u.eq;
	eq->op = DPF_OP_EQ;
	/* add in relative offset now so don't have to do at demux time. */
	eq->offset = offset + ir->moffset;
	eq->val = val;
	eq->mask = mask;
	eq->nbits = nbits;
}

void dpf_eq8(struct dpf_ir *ir, uint16 offset, uint8 val) 
	{ dpf_meq8(ir, offset, IDENTITY_MASK8, val); }
void dpf_eq16(struct dpf_ir *ir, uint16 offset, uint16 val) 
	{ dpf_meq16(ir, offset, IDENTITY_MASK16, val); }
void dpf_eq32(struct dpf_ir *ir, uint16 offset, uint32 val) 
	{ dpf_meq32(ir, offset, IDENTITY_MASK32, val); }

/* Is the message quantity of nbits at offset equal to val? */
void dpf_meq8(struct dpf_ir *ir, uint16 offset, uint8 mask, uint8 val) {
	if((mask & IDENTITY_MASK8) != mask)
		fatal(Mask exceeds width of type.);
	mkeq(ir, 8, offset, mask, val);
}

void dpf_meq16(struct dpf_ir *ir, uint16 offset, uint16 mask, uint16 val) {
	if((mask & IDENTITY_MASK16) != mask)
		fatal(Mask exceeds width of type.);
	mkeq(ir, 16, offset, mask, val);
}

void dpf_meq32(struct dpf_ir *ir, uint16 offset, uint32 mask, uint32 val) {
	if((mask & IDENTITY_MASK32) != mask)
		fatal(Mask exceeds width of type.);
	mkeq(ir, 32, offset, mask, val);
}

static inline void
mkshift(struct dpf_ir *ir, uint8 nbits, uint16 offset, uint32 mask, uint8 shift) {
	struct shift *s;

	s = &alloc(ir)->u.shift;
	s->op = DPF_OP_SHIFT;
	s->shift = shift;
	s->offset = offset;
	s->mask = mask;
	s->nbits = nbits;
	ir->moffset = 0;	/* relative offset is zero. */
	
}

/* shift the base message pointer by nbits */
void dpf_shift8(struct dpf_ir *ir, uint16 offset, uint8 shift) 
	{ dpf_mshift8(ir, offset, IDENTITY_MASK8, shift); }
void dpf_shift16(struct dpf_ir *ir, uint16 offset, uint8 shift) 
	{ dpf_mshift16(ir, offset, IDENTITY_MASK16, shift); }
void dpf_shift32(struct dpf_ir *ir, uint16 offset, uint8 shift) 
	{ dpf_mshift32(ir, offset, IDENTITY_MASK32, shift); }

void dpf_mshift8(struct dpf_ir *ir, uint16 offset, uint8 mask, uint8 shift) 
	{ mkshift(ir, 8, offset, mask, shift); }
void dpf_mshift16(struct dpf_ir *ir, uint16 offset, uint16 mask, uint8 shift) 
	{ mkshift(ir, 16, offset, mask, shift); }
void dpf_mshift32(struct dpf_ir *ir, uint16 offset, uint32 mask, uint8 shift) 
	{ mkshift(ir, 32, offset, mask, shift); }

void dpf_shifti(struct dpf_ir *ir, uint16 offset) 
	{ ir->moffset += offset; }
