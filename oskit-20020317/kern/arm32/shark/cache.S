/*
 * Copyright (c) 1999, 2001 University of Utah and the Flux Group.
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

/*
 * arm8 support code Copyright (c) 1997 ARM Limited
 * arm8 support code Copyright (c) 1997 Causality Limited
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 */
 
#include <oskit/arm32/asm.h>
#include <oskit/arm32/base_trap.h>
#include <oskit/arm32/proc_reg.h>

/*
 * Clean the entire cache. For the SA110, you clean the cache by reading
 * from a special area of memory. 
 */
ENTRY(arm32_cache_clean)
	mrs	r3, cpsr_all
	orr	r2, r3, #(PSR_I | PSR_F)
	msr	cpsr_all, r2

	ldr	r0, Larm32_cache_clean_addr
	ldr	r1, Larm32_cache_clean_size
	ldr	r0, [r0, #0]
	ldr	r1, [r1, #0]

cleanloop:
	ldr	r2, [r0], #32
	subs	r1, r1, #32
	bne	cleanloop

	msr	cpsr_all, r3
	mov	pc, lr

Larm32_cache_clean_addr:
	.word	EXT(arm32_cache_clean_addr)
Larm32_cache_clean_size:
	.word	EXT(arm32_cache_clean_size)


/*
 * Clean an address range in the D cache.
 * Data is written back but not invalidated (flushed).
 *
 * r0 = start address.
 * r1 = length in bytes.
 */
ENTRY(clean_cache_range_D)
	/*
	 * Align start address and size to a cache line.
	 */
	and	r2, r0, #0x1f
	add	r1, r1, r2
	bic	r0, r0, #0x1f
	add	r1, r0, r1
	
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #32
	cmp	r0, r1
	blt	1b
	mov	r0, #0
	mov	pc, lr

/*
 * Clean and Flush an address range from the D cache.
 *
 * r0 = start address.
 * r1 = length in bytes.
 */
ENTRY(flush_cache_range_D)
	/*
	 * Align start address and size to a cache line.
	 */
	and	r2, r0, #0x1f
	add	r1, r1, r2
	bic	r0, r0, #0x1f
	add	r1, r0, r1
	
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c6, 1		@ flush D entry
	add	r0, r0, #32
	cmp	r0, r1
	blt	1b
	mov	r0, #0
	mov	pc, lr


	.bss
	/*
	 * These are initialized elsewhere.
	 */
GLEXT(arm32_cache_clean_addr)
	.word	4
GLEXT(arm32_cache_clean_size)
	.space	4
