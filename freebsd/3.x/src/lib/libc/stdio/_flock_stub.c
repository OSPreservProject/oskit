/*
 * Copyright (c) 1998 John Birrell <jb@cimlogic.com.au>.
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
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
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
 * $Id: _flock_stub.c,v 1.2 1998/05/05 21:56:42 jb Exp $
 *
 */

#include <stdio.h>

/* Don't build this in libc_r, just libc: */
#ifndef	_THREAD_SAFE
/*
 * Declare weak references in case the application is not linked
 * with libpthread.
 */
#ifdef __ELF__
#if defined(__arm___) || defined(OSKIT_ARM32)
void flockfile () __attribute__ ((weak, alias ("_flockfile_stub")));
void _flockfile_debug () __attribute__ ((weak, alias ("flockfile_debug_stub")));
int ftrylockfile () __attribute__ ((weak, alias ("_ftrylockfile_stub")));
void funlockfile () __attribute__ ((weak, alias ("_funlockfile_stub")));
#else
#pragma weak flockfile=_flockfile_stub
#pragma weak _flockfile_debug=_flockfile_debug_stub
#pragma weak ftrylockfile=_ftrylockfile_stub
#pragma weak funlockfile=_funlockfile_stub
#endif
#else
#define NO_WEAK_SYMBOLS
#endif

/*
 * This function is a stub for the _flockfile function in libpthread.
 */
void
#ifdef NO_WEAK_SYMBOLS
flockfile(FILE *fp)
#else
_flockfile_stub(FILE *fp)
#endif
{
}

/*
 * This function is a stub for the _flockfile_debug function in libpthread.
 */
void
#ifdef NO_WEAK_SYMBOLS
_flockfile_debug(FILE *fp, char *fname, int lineno)
#else
_flockfile_debug_stub(FILE *fp, char *fname, int lineno)
#endif
{
}

/*
 * This function is a stub for the _ftrylockfile function in libpthread.
 */
int
#ifdef NO_WEAK_SYMBOLS
ftrylockfile(FILE *fp)
#else
_ftrylockfile_stub(FILE *fp)
#endif
{
	return(0);
}

/*
 * This function is a stub for the _funlockfile function in libpthread.
 */
void
#ifdef NO_WEAK_SYMBOLS
funlockfile(FILE *fp)
#else
_funlockfile_stub(FILE *fp)
#endif
{
}
#endif
