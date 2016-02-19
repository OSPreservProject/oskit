/*
 * Copyright (c) 1995,1997,1998,1999 The University of Utah and the Flux Group.
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
 * Definitions for producing 16-bit Intel-architecture code.
 */
#ifndef _OSKIT_X86_I16_H_
#define _OSKIT_X86_I16_H_

#include <oskit/machine/types.h>
#include <oskit/compiler.h>

#ifdef HAVE_CODE16GCC
#define CODE16_STRING ".code16gcc"
#else
#define CODE16_STRING ".code16"
#endif


/* Switch GAS into 16-bit mode.  */
#define CODE16 asm(CODE16_STRING);

/* Switch back to 32-bit mode.  */
#define CODE32 asm(".code32");


/*
 * Macros to switch between 16-bit and 32-bit code
 * in the middle of a C function.
 * Be careful with these!
 * If you accidentally leave the assembler in the wrong mode
 * at the end of a function, following functions will be assembled wrong
 * and you'll get very, very strange results...
 * It's safer to use the higher-level macros below when possible.
 */
#define i16_switch_to_32bit(cs32) asm volatile("
		ljmp	%0,$1f
		.code32
	1:
	" : : "i" (cs32));
#define switch_to_16bit(cs16) asm volatile("
		ljmp	%0,$1f
		"CODE16_STRING"
	1:
	" : : "i" (cs16));


/*
 * From within one type of code, execute 'stmt' in the other.
 * These are safer and harder to screw up with than the above macros.
 */
#define i16_do_32bit(cs32, cs16, stmt) \
	({	i16_switch_to_32bit(cs32); \
		{ stmt; } \
		switch_to_16bit(cs16); })
#define do_16bit(cs32, cs16, stmt) \
	({	switch_to_16bit(cs16); \
		{ stmt; } \
		i16_switch_to_32bit(cs32); })


OSKIT_BEGIN_DECLS

/*
 * The OSKit minimal C library provides 16-bit versions
 * of a few of the most popular C library functions...
 */
void *i16_memcpy(void *to, const void *from, unsigned int n);
void *i16_memset(void *to, int ch, unsigned int n);
void i16_bcopy(const void *from, void *to, unsigned int n);
void i16_bzero(void *to, unsigned int n);
int i16_putchar(int c);
int i16_console_putbytes(const char *s, int len);
int i16_puts(const char *str);
int i16_printf(const char *format, ...);
int i16_vprintf(const char *format, oskit_va_list vl);
void i16_exit(int status) OSKIT_NORETURN;
void i16_panic(const char *format, ...) OSKIT_NORETURN;

OSKIT_END_DECLS

#endif /* _OSKIT_X86_I16_H_ */
