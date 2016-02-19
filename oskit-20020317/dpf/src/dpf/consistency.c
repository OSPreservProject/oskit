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


/* check internal consistency of dpf_base */
#include <dpf-internal.h>
#include <string.h>

#ifdef OSKIT
void dpf_check(void);
#endif

static int ck_pid_table[DPF_MAXFILT];

static int seen_pid(int pid) {
        if(pid) {
                if(!ck_pid_table[pid])
                        ck_pid_table[pid] = 1;
                else {
                        printf("***\npid %d has been seen twice\n\n", pid);
                        dpf_output();
                        return 1;
                }
        }
        return 0;
}

static void cktrie(Atom parent, Atom a, int refcnt) {
        int n, htn;
        Ht ht;
        Atom or, prev;

	if(!a)
		return;

        for(prev = 0, n = 0; a; prev = a, a = a->sibs.le_next) {
                if(a->parent != parent) {
			printf("\n********\nbogus parent!\n\n");
			dpf_output();
 			fatal(bogus parent);
		}
                n += a->refcnt;

                demand(!seen_pid(a->pid), already seen pid);

                if(!(ht = a->ht)) {
                        cktrie(a, a->kids.lh_first, a->refcnt - !!a->pid);
                        continue;
                }
		demand(!a->pid, ht should not have a pid);

		for(htn = 0, or = a->kids.lh_first; or; or = or->sibs.le_next) {
                	demand(or->parent == a, bogus parent);
                	htn += or->refcnt;
                	cktrie(or, or->kids.lh_first, or->refcnt - !!or->pid);
		}
	
                if(htn != a->refcnt) {
                        printf("****\na->refcnt == %d, htn = %d\n\n",
                                a->refcnt, htn);
                        dpf_output();
                        fatal(bogus refcnt);
                }
        }

        if(n == refcnt)
                return;

        printf("****\nrefcnt == %d, n = %d\n\n", refcnt, n);
        dpf_output();
        fatal(bogus refcnt);
}

void dpf_check(void) {
#ifndef NDEBUG
        int i, refcnt;
	Atom base;

        if(!(base = dpf_base->kids.lh_first))
                return;

        memset(ck_pid_table, 0, sizeof ck_pid_table);

        for(refcnt = i = 0; i < DPF_MAXFILT; i++)
                if(dpf_active[i])
                        refcnt++;

        cktrie(dpf_base, base, refcnt);
#endif
}
