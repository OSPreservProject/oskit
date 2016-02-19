/*
 * Copyright (c) 1995-1998 The University of Utah and the Flux Group.
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
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifndef _OSKIT_X86_PIO_H_
#define _OSKIT_X86_PIO_H_

#ifdef	__GNUC__

/* This is a more reliable delay than a few short jmps. */
#define iodelay() \
	asm("pushl %eax; inb $0x80,%al; inb $0x80,%al; popl %eax")

#define inl(port) \
({ unsigned long _tmp__; \
	asm volatile("inl %%dx, %0" : "=a" (_tmp__) : "d" ((unsigned short)(port))); \
	_tmp__; })

#define inl_p(port) ({		\
	unsigned long r;	\
	r = inl(port);		\
	iodelay();		\
	r;			\
})

#define inw(port) \
({ unsigned short _tmp__; \
	asm volatile(".byte 0x66; inl %%dx, %0" : "=a" (_tmp__) : "d" ((unsigned short)(port))); \
	_tmp__; })

#define inw_p(port) ({		\
	unsigned short r;	\
	r = inw(port);		\
	iodelay();		\
	r;			\
})

#define inb(port) \
({ unsigned char _tmp__; \
	asm volatile("inb %%dx, %0" : "=a" (_tmp__) : "d" ((unsigned short)(port))); \
	_tmp__; })

#define inb_p(port) ({		\
	unsigned char r;	\
	r = inb(port);		\
	iodelay();		\
	r;			\
})


#define outl(port, val) \
({ asm volatile("outl %0, %%dx" : : "a" (val) , "d" ((unsigned short)(port))); })

#define outl_p(port, val) ({	\
	outl(port, val);	\
	iodelay();		\
})

#define outw(port, val) \
({asm volatile(".byte 0x66; outl %0, %%dx" : : "a" ((unsigned short)(val)) , "d" ((unsigned short)(port))); })

#define outw_p(port, val) ({	\
	outw(port, val);	\
	iodelay();		\
})

#define outb(port, val) \
({ asm volatile("outb %0, %%dx" : : "a" ((unsigned char)(val)) , "d" ((unsigned short)(port))); })

#define outb_p(port, val) ({	\
	outb(port, val);	\
	iodelay();		\
})


/* Inline code works just as well for 16-bit code as for 32-bit.  */
#define i16_iodelay()		iodelay()
#define i16_inl(port)		inl(port)
#define i16_inl_p(port)		inl_p(port)
#define i16_inw(port)		inw(port)
#define i16_inw_p(port)		inw_p(port)
#define i16_inb(port)		inb(port)
#define i16_inb_p(port)		inb_p(port)
#define i16_outl(port, val)	outl(port, val)
#define i16_outl_p(port, val)	outl_p(port, val)
#define i16_outw(port, val)	outw(port, val)
#define i16_outw_p(port, val)	outw_p(port, val)
#define i16_outb(port, val)	outb(port, val)
#define i16_outb_p(port, val)	outb_p(port, val)

#endif	/* __GNUC__ */

#endif /* _OSKIT_X86_PIO_H_ */
