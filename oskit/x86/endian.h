/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
 * Copyright (c) 1987, 1991 Regents of the University of California.
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
 *	from: @(#)endian.h	7.8 (Berkeley) 4/3/91
 *	$Id: endian.h,v 1.8 2000/06/05 17:07:13 eeide Exp $
 */

#ifndef _OSKIT_X86_ENDIAN_H_
#define _OSKIT_X86_ENDIAN_H_

#include <oskit/compiler.h>

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define LITTLE_ENDIAN   1234    /* LSB first: i386, vax */
#define BIG_ENDIAN      4321    /* MSB first: 68000, ibm, net */
#define PDP_ENDIAN      3412    /* LSB first in word, MSW first in long */

#define BYTE_ORDER LITTLE_ENDIAN

OSKIT_BEGIN_DECLS

unsigned long	htonl(unsigned long);
unsigned short	htons(unsigned short);
unsigned long	ntohl(unsigned long);
unsigned short	ntohs(unsigned short);

OSKIT_END_DECLS

#define __word_swap_long(x) \
__extension__ ({ register unsigned long __X = (x); \
   __asm ("rorl $16, %1" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })

#if (defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)) && !defined(I386_CPU)

#define __byte_swap_long(x) \
__extension__ ({ register unsigned long __X = (x); \
   __asm ("bswap %0" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })
#else

#define __byte_swap_long(x) \
__extension__ ({ register unsigned long __X = (x); \
   __asm ("xchgb %h1, %b1\n\trorl $16, %1\n\txchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })
#endif

#define __byte_swap_word(x) \
__extension__ ({ register unsigned short __X = (x); \
   __asm ("xchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })

/*
 * Macros for network/external number representation conversion.
 */
#ifdef __GNUC__
#define	ntohl	__byte_swap_long
#define	ntohs	__byte_swap_word
#define	htonl	__byte_swap_long
#define	htons	__byte_swap_word
#endif

#define	NTOHL(x)	((x) = ntohl((unsigned long)(x)))
#define	NTOHS(x)	((x) = ntohs((unsigned short)(x)))
#define	HTONL(x)	((x) = htonl((unsigned long)(x)))
#define	HTONS(x)	((x) = htons((unsigned short)(x)))

#endif /* _OSKIT_X86_ENDIAN_H_ */
