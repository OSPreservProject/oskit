/*-
 * Copyright 1996-1998 John D. Polstra.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *      $\Id: rtld_start.S,v 1.1 1999/02/22 13:34:33 stoller Exp stoller $
 */

/*
 * Binder entry point.  Control is transferred to here by code in the PLT.
 * On entry, there are two arguments on the stack.  In ascending address
 * order, they are (1) "obj", a pointer to the calling object's Obj_Entry,
 * and (2) "reloff", the byte offset of the appropriate relocation entry
 * in the PLT relocation table.
 *
 * We are careful to preserve all registers, even the the caller-save
 * registers.  That is because this code may be invoked by low-level
 * assembly-language code that is not ABI-compliant.
 */
	.align	4
	.globl	_rtld_bind_start
	.type	_rtld_bind_start,@function
_rtld_bind_start:
	pushf				# Save eflags
	pushl	%eax			# Save %eax
	pushl	%edx			# Save %edx
	pushl	%ecx			# Save %ecx
	pushl	20(%esp)		# Copy reloff argument
	pushl	20(%esp)		# Copy obj argument

	call	_rtld_bind@PLT		# Transfer control to the binder
	/* Now %eax contains the entry point of the function being called. */

	addl	$8,%esp			# Discard binder arguments
	movl	%eax,20(%esp)		# Store target over obj argument
	popl	%ecx			# Restore %ecx
	popl	%edx			# Restore %edx
	popl	%eax			# Restore %eax
	popf				# Restore eflags
	leal	4(%esp),%esp		# Discard reloff, do not change eflags
	ret				# "Return" to target address
