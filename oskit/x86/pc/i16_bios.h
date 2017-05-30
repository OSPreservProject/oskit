/*
 * Copyright (c) 1994-1998 University of Utah and the Flux Group.
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
 * Inline functions for common calls to the PC's 16-bit BIOS
 * from 16-bit real or v86 mode.
 */
#ifndef _OSKIT_X86_PC_I16_BIOS_H_
#define _OSKIT_X86_PC_I16_BIOS_H_

#include <oskit/compiler.h>


OSKIT_INLINE void i16_bios_putchar(int c)
{
	asm volatile("int $0x10" : : "a" (0x0e00 | (c & 0xff)), "b" (0x07));
}

OSKIT_INLINE int i16_bios_getchar()
{
	int c;
	asm volatile("int $0x16" : "=a" (c) : "a" (0x0000));
	c &= 0xff;
	return c;
}

OSKIT_INLINE void i16_bios_warm_boot(void)
{
	asm volatile("
		cli
		movw	$0x40,%ax
		movw	%ax,%ds
		movw	$0x1234,0x72
		ljmp	$0xffff,$0x0000
	");
}

OSKIT_INLINE void i16_bios_cold_boot(void)
{
	asm volatile("
		cli
		movw	$0x40,%ax
		movw	%ax,%ds
		movw	$0x0000,0x72
		ljmp	$0xffff,$0x0000
	");
}

OSKIT_INLINE unsigned char i16_bios_copy_ext_mem(
	unsigned src_la, unsigned dest_la, unsigned short word_count)
{
	char buf[48];
	unsigned short i, rc;

	/* Initialize the descriptor structure.  */
	for (i = 0; i < sizeof(buf); i++)
		buf[i] = 0;
	*((unsigned short*)(buf+0x10)) = 0xffff; /* source limit */
	*((unsigned long*)(buf+0x12)) = src_la; /* source linear address */
	*((unsigned char*)(buf+0x15)) = 0x93; /* source access rights */
	*((unsigned short*)(buf+0x18)) = 0xffff; /* dest limit */
	*((unsigned long*)(buf+0x1a)) = dest_la; /* dest linear address */
	*((unsigned char*)(buf+0x1d)) = 0x93; /* dest access rights */

#if 0
	i16_puts("buf:");
	for (i = 0; i < sizeof(buf); i++)
		i16_writehexb(buf[i]);
	i16_puts("");
#endif

	/* Make the BIOS call to perform the copy.  */
	asm volatile("
		int	$0x15
	" : "=a" (rc)
	  : "a" ((unsigned short)0x8700),
	    "c" (word_count),
	    "S" ((unsigned short)(unsigned)buf));

	return rc >> 8;
}

#endif /* _OSKIT_X86_PC_I16_BIOS_H_ */
