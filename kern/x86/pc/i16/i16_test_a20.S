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
#include <oskit/config.h>
#ifdef HAVE_CODE16

#include <oskit/x86/asm.h>

	.text
	.code16

/* We really need a 32bit return, but older versions of gas barf if
 * we add in an explicit suffix.
 */
#ifdef HAVE_CODE16GCC
#define RETL	retl
#else
#define RETL	ret
#endif

/*
 * Test the A20 address line; return true if it is enabled.
 */
ENTRY(i16_test_a20)
	xorw	%ax,%ax
	movw	%ax,%fs
	notw	%ax
	movw	%ax,%gs

	/* See if the values in the corresponding locations are the same.  */
	movw	%fs:0,%ax
	cmpw	%gs:16,%ax
	jnz	1f

	/* Yes; try changing one and see if they're still the same.  */
	movw	%ax,%dx
	notw	%ax
	movw	%ax,%fs:0
	cmpw	%gs:16,%ax
	movw	%dx,%fs:0
	jnz	1f

	xorl	%eax,%eax
	RETL
1:
	movl	$1,%eax
	RETL

#endif /* HAVE_CODE16 */
