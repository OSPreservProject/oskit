/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_ARM32_PIO_H_
#define _OSKIT_ARM32_PIO_H_

#include <oskit/types.h>

/*
 * Define inb/outb/etc. routines in terms of where the ISA lands.
 */
extern	oskit_addr_t		isa_iobus_address;
extern	oskit_addr_t		isa_membus_address;

#define inb(port) ({				\
	unsigned int result;			\
        asm volatile ("ldrb %0,[%1]"		\
		     : "=r" (result)		\
		     : "r" ((unsigned int) isa_iobus_address + (port))); \
	result;					\
})
	
#define outb(port, val) ({			\
	asm volatile ("strb %0,[%1]" : :		\
                     "r" (val),			\
                     "r" ((unsigned int) isa_iobus_address + \
			  (unsigned int) (port)));\
})

#define inb_p	inb
#define outb_p	outb

#define writeb(addr, val) ({			\
	asm volatile ("strb %0,[%1]" : :		\
                     "r" (val),			\
                     "r" ((unsigned int) isa_membus_address + \
			  (unsigned int) (addr)));\
})

#define writew(addr, val) ({			\
	asm volatile ("strh %0,[%1]" : :	\
                     "r" (val),			\
                     "r" ((unsigned int) isa_membus_address + \
			  (unsigned int) (addr)));\
})

#define readb(addr) ({				\
	unsigned int result;			\
        asm volatile ("ldrb %0,[%1]"		\
		     : "=r" (result)		\
		     : "r" ((unsigned int) isa_membus_address + \
			    (unsigned int) (addr))); \
	(unsigned char) result;			\
})

#define readw(addr) ({				\
	unsigned int result;			\
        asm volatile ("ldrh %0,[%1]"		\
		     : "=r" (result)		\
		     : "r" ((unsigned int) isa_membus_address + \
			    (unsigned int) (addr))); \
	(unsigned short) result;		\
})

#endif /* _OSKIT_ARM32_PIO_H_ */
