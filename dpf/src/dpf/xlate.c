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
   Translate old style IR to new style.

   We translate six trees: 
 	eqi 
	 |
	bits(8|16|32)

	eqi
	 |
	andi
	 |
	bits(8|16|32)
 */
#include <dpf-internal.h>
#include <old-dpf.h>

/* 
 * Given a pointer to previous IR, returns a pointer to new-style. Not 
 * reentrant.
 */
struct dpf_ir *dpf_xlate(struct dpf_frozen_ir *ir, int nelems) {
	uint32 val, mask;
	static struct dpf_ir xir;
	int i;

	dpf_init(&xir);

	for(i = 0; i < nelems; i++) {
		if(ir[i].op != DPF_EQI)
			fatal(unexpected op);

		val = ir[i++].imm;
		
		/* check to see if there is an and */
		mask = (ir[i].op == DPF_ANDI) ? ir[i++].imm : 0xffffffff;

		/* load the value */
		switch(ir[i].op) {
		case DPF_BITS8I:
			dpf_meq8(&xir, ir[i].imm, mask, val);
			break;
		case DPF_BITS16I:
			dpf_meq16(&xir, ir[i].imm, mask, val);
			break;
		case DPF_BITS32I:
			dpf_meq32(&xir, ir[i].imm, mask, val);
			break;
		default: fatal(Unexpected op);
		}
	}
	return &xir;
}
