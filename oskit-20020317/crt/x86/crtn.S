/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
 * This file provides terminators for various "special" sections.
 * See the corresponding starters in crti.h for more information.
 */

#ifdef __ELF__

/* Disabled because of binutils stupidity - see crti.h for details.  */
#if 0
	.section .init
	ret
#endif

	.section .fini
	ret

	.section .ctors,"a",@progbits
	.long	0

	.section .dtors,"a",@progbits
	.long	0

	.section .anno,"aw",@progbits
	.globl	__ANNO_END__
__ANNO_END__:

#endif /* __ELF__ */

