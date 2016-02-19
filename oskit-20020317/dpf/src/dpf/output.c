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

static void dump(Atom p, int n);

static void indent(int n) {
        while(n-- > 0)
                putchar(' ');
}

/* Print out the tree. */
void dpf_output(void) {
	if(!dpf_base->kids.lh_first) {
		printf("Nil trie\n");
		return;
	}
	printf("\nDumping ir.\n");
	printf("base: \n");
	dpf_dump(dpf_iptr);
	printf("rest: \n");
	dump(dpf_base->kids.lh_first, 3);
}

static void dump_eqnode(Atom p, int n, int bits) {
	struct ir *ir;

	/* have to handle hash table as well. */
	ir = &p->ir;

	if(!p->ht) {
		indent(n);
		printf("msg[%d:%d] & 0x%x == 0x%x, refcnt = %d, level = %d, align = %d", 
			ir->u.eq.offset, bits, ir->u.eq.mask, ir->u.eq.val, p->refcnt, ir->level, ir->alignment);
		if(p->pid)
			printf(", pid = %d\n", p->pid);
		printf("\n");
		dpf_dump(p->code);
		dump(p->kids.lh_first, n + 5);
	} else {
		Atom hte, h;
		int i, sz;
		Ht ht;

		printf("ht: \n");
		dpf_dump(p->code);
		for(ht = p->ht, i = 0, sz = ht->htsz; i < sz; i++) {
			if(!(hte = ht->ht[i]))
				continue;

			indent(n);

			printf("{ val = 0x%x, ref = %d", hte->ir.u.eq.val, hte->refcnt);
			if(hte->pid)
				printf(", pid = %d", hte->pid);
			printf(" }\n");
			dump(hte->kids.lh_first, n + 5);
			dpf_dump(hte->code);

			for(h = hte->next; h; h = h->next) {
				indent(n);
				printf("-> { val = 0x%x, ref = %d", h->ir.u.eq.val, h->refcnt);
				if(h->pid)
					printf(", pid = %d", h->pid);
				printf("}\n");
				dump(h->kids.lh_first, n + 5);
				dpf_dump(h->code);
			}
		}
		indent(n);
		printf(" ==  msg[%d:%d] & 0x%x,  refcnt = %d, level = %d\n", 
			ir->u.eq.offset, bits, ir->u.eq.mask, p->refcnt,
ir->level);
	}

}

static void dump_shiftnode(Atom p, int n, int bits) {
	struct ir *ir;

	indent(n);
	ir = &p->ir;
	printf("msg + msg[%d:%d] & 0x%x << %d, align = %d, refcnt = %d, level = %d", 
		ir->u.shift.offset, bits, ir->u.shift.mask, ir->u.shift.shift, ir->u.shift.align, p->refcnt, ir->level);
	if(p->pid)
		printf(", pid = %d\n", p->pid);
	printf("\n");
	dpf_dump(p->code);
	dump(p->kids.lh_first, n + 5);
}

static void dump(Atom p, int n) {
	if(!p) return;

	for(;; p = p->sibs.le_next) {

		if(iseq(&p->ir))
			dump_eqnode(p, n, p->ir.u.eq.nbits);
		else  {
			demand(isshift(&p->ir), bogus op);
			dump_shiftnode(p, n, p->ir.u.shift.nbits);
		}

		if(!p->sibs.le_next) 
			return;

		indent(n - 2);
		printf("OR\n");
	}
}
