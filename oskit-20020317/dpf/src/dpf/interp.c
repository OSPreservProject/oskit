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


#include <dpf-internal.h>
#include <demand.h>
#include <hash.h>

/* 
 * Load everything as 32 bit, and mask off the unneeded stuff.  then
 * the remainder of the problems are aligned, checked, hash.  The cost
 * here is the load of mask, with a possible cache miss.  the benefit
 * is a much smaller interpreter:
 *   orig: sizes x mask x aligned x checked x hash = 4 * 2 * 2 * 2 * 2 = 64 
 *   new :                aligned x checked x hash = 2 * 2 * 2 = 8 
 *
 * (should add all terms, all non-terms?)
 */
#define op(op, aligned, unchecked, hash) \
   ((aligned) << 3| (unchecked)<<2 | (hash)<<1 | (op == DPF_OP_EQ))
#define eq_op(aligned, unchecked, hash) \
	op(DPF_OP_EQ, aligned, unchecked, hash)
#define s_op(aligned, unchecked) \
	op(DPF_OP_SHIFT, aligned, unchecked, 0)

typedef enum {
	/*	                           aligned	unchecked 	hash  */
	EQ 			   = eq_op(0, 		0, 		0),
	EQ_UNCHECKED 		   = eq_op(0, 		1, 		0),
	EQ_HASH                    = eq_op(0, 		0, 		1),
	EQ_UNCHECKED_HASH          = eq_op(0, 		1, 		1),
	EQ_ALIGNED                 = eq_op(1, 		0, 		0),
	EQ_ALIGNED_UNCHECKED       = eq_op(1, 		1, 		0),
	EQ_ALIGNED_HASH            = eq_op(1, 		0, 		1),
	EQ_ALIGNED_UNCHECKED_HASH  = eq_op(1, 		1, 		1),

	/*			       aligned          unchecked */
	SHIFT 			= s_op(0, 		0),
	SHIFT_UNCHECKED 	= s_op(0, 		1),
	SHIFT_ALIGNED 		= s_op(1, 		0),
	SHIFT_ALIGNED_UNCHECKED = s_op(1, 		1),
} state_t;

/* label with the right state. */
void dpf_compile_atom(Atom a) {
	struct ir *ir;
	unsigned unchecked, aligned;

	ir = &a->ir;

	/*
 	 * If we have shifted on this path or the offset surpasses
 	 * the minimal amount of buffer space we are allocated,
 	 * emit a check to see if we have exceeded our memory.
 	 */
	unchecked 	= !ir->shiftp && (ir->u.eq.offset <= DPF_MINMSG);
	aligned      	= ((ir->alignment + ir->u.eq.offset) % 4 == 0);

	a->code[0] = (isshift(ir)) ?
		s_op(aligned, unchecked) :
		eq_op(aligned, unchecked, (a->ht != 0));
}

static inline int 
load_lhs(unsigned *lhs, uint8 *msg, unsigned nbytes, struct ir *ir, int aligned, int unchecked) {
	unsigned off;

	off = ir->u.eq.offset;
	if(unchecked)
		demand(off < nbytes, bogus offset);
	else if(off >= nbytes)
		return 0;

	if(!aligned)
		memcpy(lhs, msg+off, 4);
	else {
		demand((unsigned long)(msg + off) % 4 == 0, bogus alignment);
		*lhs = *(unsigned *)(msg+off);
	}

	*lhs &= ir->u.eq.mask;
	return 1;
}

static inline Atom ht_lookup(Ht ht, uint32 val) {
        Atom hte;

	assert(ht);
        for(hte = ht->ht[hash(ht, val)]; hte; hte = hte->next)
                if(hte->ir.u.eq.val == val)
			return hte;
	return 0;
}

static int fast_interp(uint8 *msg, unsigned nbytes, Atom a, int fail_pid) {
      /* msg += (msg[off]:nbits & mask) << shift */
#     define do_shift(aligned, unchecked) do {				\
	if(!load_lhs(&off, msg, nbytes, ir, aligned, unchecked))	\
		continue;						\
	off <<= ir->u.shift.shift;					\
	if(off >= nbytes)						\
		break;							\
	if((pid = fast_interp(msg+off, nbytes-off,a->kids.lh_first, a->pid)))\
		return pid;						\
      } while(0)

    /* msg[off]:nbits & mask == val */
#   define do_eq(aligned, unchecked) do { 				\
	assert(!a->ht);							\
	if(!load_lhs(&lhs, msg, nbytes, ir, aligned, unchecked))	\
		break;							\
	if(lhs != ir->u.eq.val)						\
		break;							\
	if((pid = fast_interp(msg, nbytes, a->kids.lh_first, a->pid)))	\
		return pid;						\
    } while(0)

#   define do_deq(aligned, unchecked) do {				\
	Atom hte;							\
									\
	assert(a->ht);							\
	if(!load_lhs(&lhs, msg, nbytes, ir, aligned, unchecked))	\
		break;							\
	if(!(hte = ht_lookup(a->ht, lhs)))				\
		break;							\
	if((pid = fast_interp(msg, nbytes, hte->kids.lh_first, hte->pid)))\
		return pid;						\
    } while(0)

	unsigned lhs, off, pid;
	struct ir *ir;

	for(; a; a = a->sibs.le_next) {
		ir = &a->ir;
		switch((state_t)a->code[0]) {
		case EQ:			do_eq(0, 0); break;
		case EQ_UNCHECKED:		do_eq(0, 1); break;
		case EQ_ALIGNED:		do_eq(1, 0); break;
		case EQ_ALIGNED_UNCHECKED:	do_eq(1, 1); break;

		case EQ_HASH:			do_deq(0, 0); break;
		case EQ_ALIGNED_HASH:		do_deq(1, 0); break;
		case EQ_UNCHECKED_HASH:		do_deq(0, 1); break;
		case EQ_ALIGNED_UNCHECKED_HASH: do_deq(1, 1); break;

		case SHIFT:			do_shift(0, 0); break;
		case SHIFT_UNCHECKED: 		do_shift(0, 1); break;
		case SHIFT_ALIGNED: 		do_shift(1, 0); break;
		case SHIFT_ALIGNED_UNCHECKED: 	do_shift(1, 1); break;
		default:			fatal(Bogus op);
		}
	}
	return fail_pid;
}

static int interp(uint8 *msg, unsigned nbytes, Atom a, unsigned fail_pid) {
	struct ir *ir;
	unsigned lhs, off, pid;

	/* need to see if it ends. */
	for(; a; a = a->sibs.le_next) {
		ir = &a->ir;
		if(isshift(ir)) {
			/* msg += (msg[off]:nbits & mask) << shift */
			if(!load_lhs(&off, msg, nbytes, ir, 0, 0))
				continue;
			off <<= ir->u.shift.shift;
			if(off >= nbytes)
				continue;
			else if((pid = interp(msg+off, nbytes-off, a->kids.lh_first, a->pid)))
				return pid;
		} else if(!a->ht) {
			/* msg[off]:nbits & mask == val */
			if(!load_lhs(&lhs, msg, nbytes, ir, 0, 0))
				continue;
			else if(lhs != ir->u.eq.val)
				continue;
			else if((pid = interp(msg, nbytes, a->kids.lh_first, a->pid)))
				return pid;
		} else {
			Atom hte;

			if(!load_lhs(&lhs, msg, nbytes, ir, 0, 0))
				continue;
			if(!(hte = ht_lookup(a->ht, lhs)))
				continue;
			else if((pid = interp(msg, nbytes, hte->kids.lh_first, hte->pid)))
				return pid;
		}
	}
	return fail_pid;
}

static int dpf_interp(uint8 *msg, unsigned nbytes) {

/* 
 * OSKIT
 * Tried changing this to 0 to use fast_interp(); turns out
 * that fast_interp() doesn't work. Gave this error:
 * file ../../../oskit/dpf/src/dpf/interp.c, line 106: 
 *       (unsigned long)(msg + off) % 4 == 0
 *       bogus alignment
 */

#if 1
	return interp(msg, nbytes, dpf_base->kids.lh_first, 0);
#else
	int pid;

	demand((unsigned long)msg % 4 == 0, bogus message alignment);
	pid = fast_interp(msg, nbytes, dpf_base->kids.lh_first, 0);
	assert(pid == interp(msg, nbytes, dpf_base->kids.lh_first, 0));

	return pid;
#endif
}

int (*dpf_compile(Atom trie))() {
	return dpf_interp;
}



/*
 * Compute what special-case optimizations we can do.  Currently
 * optimize for:
 *      1. hash table has no collisions.
 *      2. hash table has only terminals.
 *      3. hash table has only non-terminals.
 */
unsigned ht_state(Ht ht) {
        unsigned state;

        state = 0;

        /* no collisions means we can elide collision checks. */
        if(!ht->coll)
                state |= DPF_NO_COLL;

        /*
         * note: when the hash table is empty, it will trigger ALL_NTERMS,
         * HOWEVER, the hash table will be set to REGEN, which will
         * make ht_regenp return 1.  A bit sleazy.  Weep.
         */
        if(!ht->nterm)
                state |= DPF_ALL_TERMS;
        else if(!ht->term)
                state |= DPF_ALL_NTERMS;

        return state;
}

void dpf_dump(void *ptr) {
	
	if(ptr == dpf_interp) 
		printf("<interpreter routine>\n");
}
