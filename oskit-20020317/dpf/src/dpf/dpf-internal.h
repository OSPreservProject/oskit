
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


#ifndef __DPF_INTERNAL_H__
#define __DPF_INTERNAL_H__
/*
 *  DPF internals.   For the moment, we emphasize simplicity by wasting
 *  space. It requires no brains to eliminate this, only attention to detail.
 *
 * SHould have both in-place and regen options.  E.g., after getting
 * to a certain size, keep the code associated with each atom rather
 * than copying.  (Hopefully, this is not too much of a big deal, since
 * will mostly be dealing with the same protocol families.)
 *
 * Make the hash table be uniform.
 * Add deq opcode.
 * Make pptr a ptr to a ptr.
 */
#include <dpf.h>
#include <queue.h>
#include <dpf-config.h>
#include <demand.h>

/* Note, we make eq's into meq's and shifts into mshifts. */
enum {
	DPF_OP_EQ = 1,
#	define iseq(x) ((x)->u.eq.op == DPF_OP_EQ)
	DPF_OP_SHIFT,
#	define isshift(x) ((x)->u.eq.op == DPF_OP_SHIFT)

#	define islegalop(x) (((unsigned)(op)) < DPF_OP_END)
#	define isend(x) ((x)->u.eq.op == DPF_OP_END)
	DPF_OP_END			/* mark end */
};

/* Used by hash table optimizer */
enum {
	DPF_REGEN = 0,
	DPF_NO_COLL =  (1 << 0),
	DPF_ALL_NTERMS = (1 << 1),
	DPF_ALL_TERMS = (1 << 2),
};

/* Hash table.  Should track terms/non-terms so we can optimize. */
typedef struct ht {
	uint16 htsz;	/* size of the hash table. */
	uint8 log2_sz;  /* log2(htsz) */
	uint8 nbits;	/* number of bits key is on. */
	uint32 ent;	/* number of unique buckets. */
	int coll;	/* number of collisions. */
	fid_t term;	/* number of unique terminals. */
	fid_t nterm;  /* number of unique non-terminals. */
	unsigned state;  /* last optimization state. */
	struct atom *ht[DPF_INITIAL_HTSZ]; 	/* struct hack */
} *Ht;

/* 
 * Could squeeze a lot of space by aggregating straight-line atoms or
 * by using segments for the pointers.  Size of node:
 *	1. with 64 bit ptrs = 5 * 8 + 4 + 2 + 2 + 3 * 4 + 4 = 64 bytes.
 *	2. with 32 bit ptrs = 5 * 4 + 4 + 2 + 2 + 3 * 4 + 4 = 44 bytes.
 * Uch.
 */
typedef struct atom {
	/**
	 * The three pointers that situate us in the filter trie. 
	 */

	/* Parent of this node (if any). */
	struct atom *parent;	

	/* List of kids (if any): can either be or-kids or entries in ht. */
	LIST_HEAD(alist_head, atom) kids;

	/* Siblings (if any). */
	LIST_ENTRY(atom) sibs;	

	/**
	 * Hash table support. 
	 */

	/* Pointer to hash table (non-nil if we are a disjunctive node). */
	struct ht *ht;
	/* Pointer to next bucket. */
	struct atom *next;
	/* Used by hash table to perform indirect jump to child (if any). */
	void *label;

	/**
	 * Code generation information.  Computed by DPF (can't trust app).
	 */

	/* 
	 * Number of filters dependent on this node.  NOTE: this field 
	 * can be removed by using the insight that the last atom
	 * in the filter with a refcnt > 1 must either be (1) a hash table, 
	 * (2) be marked with a pid, or (3) be an or list.  Deletion can
	 * be done by deleting all atoms upto the first atom satisfying
	 * one of these conditions.
	 */
	fid_t	refcnt;		

	/* If you want to do more than 64k filters, redefine these. */
	fid_t	pid;		/* process id of owner. */

	/* The atom at this level. */
	struct ir ir;		
#ifdef VCODE
	unsigned code[DPF_MAXCODE];
#else
	const void **code;	/* label to jump to. */
#endif
} *Atom;

extern Atom dpf_active[DPF_MAXFILT+1];

struct dpf_frozen_ir;
struct dpf_ir *dpf_xlate(struct dpf_frozen_ir *ir, int nelems);

extern Atom dpf_base;

/* dump trie. */
void dpf_output(void);

/* check internal consistency */
void dpf_check(void);

/* Coalesce adjacent atoms. */
void dpf_coalesce(struct dpf_ir *irs);

/* Compute alignement given by ir + alignment. */
int dpf_compute_alignment(struct ir *ir, int alignement);

int (*dpf_compile(Atom trie))();

void dpf_compile_atom(Atom a);

extern unsigned log2(unsigned);

#define hashmult 1737350767	/* magic prime used for hashing. */

int dpf_allocpid(void);
void dpf_freepid(int pid);

unsigned ht_state(Ht ht);
int orlist_p(Atom a);

void dpf_dump(void *ptr);
void dpf_verbose(int v);


#endif /* __DPF_INTERNAL_H__ */
