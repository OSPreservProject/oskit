/*
 * Copyright (c) 1995-1996,1998,2000 University of Utah and the Flux Group.
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
 * This "header file" actually contains assembly language source,
 * and is included by the various crt0 files on the entry path
 * to perform generic initialization common to all environments.
 *
 * This code may trash any and all processor registers.
 * It assumes that esp is set up to point to a suitable stack
 * to make calls on; however, at the end of this code sequence
 * the stack pointer is left where it was at the beginning,
 * and nothing already on the stack (>= esp) is touched.
 */

	.text

	/* Clear the base pointer so that stack backtraces will work.  */
	xorl	%ebp,%ebp

#ifdef __ELF__

/*
 * XXX Currently we're disabling use of the init section
 * because it doesn't work right if -Ttext is used on the linker line
 * to link a kernel (or whatever) at a specific address.
 * The problem is that the init section is before the text segment
 * in the linker script, so it gets placed at 0x080000??
 * even though the text segment has moved elsewhere.
 *
 * The direct result is the linker barfing with an error like:
 * 'Not enough room for program headers (allocated 2, need 3)'
 * This particular error is actually because of a BFD stupidity,
 * in which it blindly guesses that it will need two program segments
 * and then panics when it discovers it really needs three.
 * But even if this BFD bug didn't exist, and it guessed right,
 * the end result would still be wrong,
 * in that the init code would be by itself way out at 0x08000000
 * (which probably won't even load unless the user has >128MB of memory),
 * but we want it to be next to our init segment.
 *
 * So for now, we just don't bother using this segment at all,
 * instead placing the code to call the constructor routines
 * into a function called __oskit_init in the normal .text section,
 * which is called by base_multiboot_main.
 * Fortunately, GCC never directly outputs .init section code;
 * it only outputs entries in the .ctors and .dtors sections,
 * which work fine because they come after the text section.
 */
#if 0
/*
 * Header for the special section '.init',
 * which contains code to be run at initialization time.
 * It is structured as one big function,
 * with the entrypoint defined here
 * and the 'ret' instruction supplied in crtn.o;
 * any object file in the middle can add initialization code
 * simply by inserting more instructions in this section.
 */
	.section .init
	.type EXT(__oskit_init),@function
#else
	/* Fake a __oskit_init function in the normal .text section. */
	.text
	jmp 3f
#endif
GLEXT(__oskit_init)

	/* Call all global constructors.  */
	pushl	%ebx
	pushl	%eax
	lea	ctors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx
#if 1
	/* Remove these two lines if really using the .init section. */
	ret
3:
#endif

/*
 * Similar to the '.init' section,
 * the '.fini' section contains code to run at exit time.
 */
	.section .fini
	.type EXT(__oskit_fini),@function
GLEXT(__oskit_fini)

	/* Call all global destructors.  */
	pushl	%ebx
	pushl	%eax
	lea	dtors,%ebx
1:	movl	(%ebx),%eax
	orl	%eax,%eax
	jz	2f
	call	*%eax
	addl	$4,%ebx
	jmp	1b
2:	popl	%eax
	popl	%ebx

/*
 * When any global variable needing a constructor is declared in C++,
 * it writes a longword into the '.ctors' section
 * which is a pointer to the appropriate constructor routine to call.
 */
	.section .ctors,"a",@progbits
	P2ALIGN(2)
	.long	0
ctors:

/*
 * Similarly, the '.dtors' section is a list of pointers
 * to destructor routines for global C++ objects.
 */
	.section .dtors,"a",@progbits
	P2ALIGN(2)
	.long	0
dtors:

/*
 * Finally, the '.anno' section is our own little invention;
 * it allows generic program annotations of many different kinds
 * to be collected together by the linker into "annotation tables";
 * see oskit/anno.h for more information.
 */
	.section .anno,"aw",@progbits
	P2ALIGN(4)
GLEXT(__ANNO_START__)

/*
 * Now back to our normal programming...
 */
	.text

#endif /* __ELF__ */
