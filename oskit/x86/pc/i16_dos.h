/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_X86_PC_I16_DOS_H_
#define _OSKIT_X86_PC_I16_DOS_H_

#include <oskit/compiler.h>
#include <oskit/x86/far_ptr.h>


/* Returns 16-bit DOS version number:
   major version number in high byte, minor in low byte.  */
OSKIT_INLINE unsigned short i16_dos_version(void)
{
	unsigned short dos_version_swapped;
	asm volatile("int $0x21" : "=a" (dos_version_swapped) : "a" (0x3000));
	return (dos_version_swapped >> 8) | (dos_version_swapped << 8);
}

OSKIT_INLINE void i16_dos_putchar(int c)
{
	asm volatile("int $0x21" : : "a" (0x0200), "d" (c));
}

OSKIT_INLINE void i16_dos_exit(int rc)
{
	asm volatile("int $0x21" : : "a" (0x4c00 | (rc & 0xff)));
}

OSKIT_INLINE void i16_dos_get_int_vec(int vecnum, struct far_pointer_16 *out_vec)
{
	asm volatile("
		pushw	%%es
		int	$0x21
		movw	%%es,%0
		popw	%%es
	" : "=r" (out_vec->seg), "=b" (out_vec->ofs)
	  : "a" (0x3500 | vecnum));
}

OSKIT_INLINE void i16_dos_set_int_vec(int vecnum, struct far_pointer_16 *new_vec)
{
	asm volatile("
		pushw	%%ds
		movw	%1,%%ds
		int	$0x21
		popw	%%ds
	" :
	  : "a" (0x2500 | vecnum),
	    "r" (new_vec->seg), "d" (new_vec->ofs));
}

/* Open a DOS file and return the new file handle.
   Returns -1 if an error occurs.  */
OSKIT_INLINE int i16_dos_open(const char *filename, int access)
{
	int fh;
	asm volatile("
		int	$0x21
		jnc	1f
		movl	$-1,%%eax
	1:
	" : "=a" (fh) : "a" (0x3d00 | access), "d" (filename));
	return fh;
}

OSKIT_INLINE void i16_dos_close(int fh)
{
	asm volatile("int $0x21" : : "a" (0x3e00), "b" (fh));
}

OSKIT_INLINE int i16_dos_get_device_info(int fh)
{
	int info_word;
	asm volatile("
		int	$0x21
		jnc	1f
		movl	$-1,%%edx
	1:
	" : "=d" (info_word) : "a" (0x4400), "b" (fh), "d" (0));
	return info_word;
}

OSKIT_INLINE int i16_dos_get_output_status(int fh)
{
	int status;
	asm volatile("
		int	$0x21
		movzbl	%%al,%%eax
		jnc	1f
		movl	$-1,%%eax
	1:
	" : "=a" (status) : "a" (0x4407), "b" (fh));
	return status;
}

OSKIT_INLINE int i16_dos_alloc(unsigned short *inout_paras)
{
	int seg;
	asm volatile("
		int	$0x21
		jnc	1f
		movl	$-1,%%eax
	1:
	" : "=a" (seg), "=b" (*inout_paras)
	  : "a" (0x4800), "b" (*inout_paras));
	return seg;
}

OSKIT_INLINE int i16_dos_free(unsigned short seg)
{
	unsigned short rc;
	asm volatile("
		pushw	%%es
		movw	%2,%%es
		int	$0x21
		popw	%%es
	" : "=a" (rc) : "0" (0x4900), "r" (seg));
	return rc;
}

OSKIT_INLINE unsigned short i16_dos_get_psp_seg(void)
{
	unsigned short psp_seg;
	asm volatile("int $0x21" : "=b" (psp_seg) : "a" (0x6200));
	return psp_seg;
}

#endif /* _OSKIT_X86_PC_I16_DOS_H_ */
