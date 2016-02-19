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

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *
 *	from: @(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _OSKIT_ARM32_ASM_H_
#define _OSKIT_ARM32_ASM_H_

#include <oskit/config.h>

#if defined(HAVE_P2ALIGN)
# define P2ALIGN(p2)	.p2align p2
#elif defined(__ELF__)
# define P2ALIGN(p2)	.align	(1<<(p2))
#else
# define P2ALIGN(p2)	.align	p2
#endif

#define _BEGIN_ENTRY_NP	.text; .align 0
#define _END_ENTRY_NP

#ifdef GPROF
# define _BEGIN_ENTRY	_BEGIN_ENTRY_NP
# define _END_ENTRY	_END_ENTRY_NP
#else
# define _BEGIN_ENTRY	_BEGIN_ENTRY_NP
# define _END_ENTRY	_END_ENTRY_NP
#endif

#ifndef __ELF__
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#define SEXT(x) "_"#x
#else
#define EXT(x) x
#define LEXT(x) x ## :
#define SEXT(x) x
#endif
#define GLEXT(x)	.globl EXT(x); LEXT(x)
#define _C_FUNC(c)	EXT(c)
#define	_ASM_FUNC(x)	x

#ifdef __STDC__
#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x
#else
#define __CONCAT(x,y)	x/**/y
#endif

/*
 * gas/arm uses @ as a single comment character and thus cannot be used here
 * Instead it recognised the # instead of an @ symbols in .type directives
 * We define a couple of macros so that assembly code will not be dependant
 * on one or the other.
 */
#define _ASM_TYPE_FUNCTION	#function
#define _ASM_TYPE_OBJECT	#object
#define _ENTRY(x)	.globl x; .type x,_ASM_TYPE_FUNCTION; x:

#define	ENTRY(y)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(y)); _END_ENTRY
#define	TWOENTRY(y,z)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(y)); _ENTRY(_C_FUNC(z)); \
			_END_ENTRY
#define	ASENTRY(y)	_BEGIN_ENTRY; _ENTRY(_ASM_FUNC(y)); _END_ENTRY

#define	ENTRY_NP(y)	_BEGIN_ENTRY_NP; _ENTRY(_C_FUNC(y)); _END_ENTRY_NP
#define	ASENTRY_NP(y)	_BEGIN_ENTRY_NP; _ENTRY(_ASM_FUNC(y)); _END_ENTRY_NP

#define	ASMSTR		.asciz

#define RCSID(x)	.text; .asciz x

#ifdef __STDC__
#define	__STRING(x)			#x
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(_ ## sym) ## ,1,0,0,0
#else
#define	__STRING(x)			"x"
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */

#endif /* !_OSKIT_ARM_ASM_H_ */
