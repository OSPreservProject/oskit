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
 *      $\Id: crt1.c,v 1.2 1998/09/07 23:31:59 jdp Exp $
 */

#ifndef __GNUC__
#error "GCC is needed to compile this file"
#endif

#include <stdlib.h>

#include "native.h"

typedef void (*fptr)(void);

extern void __oskit_init(void);
extern void __oskit_fini(void);
extern int main(int, char **, char **);

#ifdef GCRT
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int eprol;
extern int etext;
#endif

extern int _DYNAMIC;
#pragma weak _DYNAMIC

#ifdef __i386__
#define get_rtld_cleanup()				\
    ({ fptr __value;					\
       __asm__("movl %%edx,%0" : "=rm"(__value));	\
       __value; })
#elif defined __arm__
#define get_rtld_cleanup()				\
    ({ fptr __value;					\
       __asm__("mov %0, a1" : "=r"(__value));	\
       __value; })
#else
#error "This file only supports the i386 and arm architectures"
#endif

#ifdef OSKIT
extern char base_stack_end[];
int oskit_usermode_simulation;
extern void (*oskit_libc_exit)(int);
extern void oskitunix_mem_init(void);
extern void oskitunix_stdio_init(void);
extern void oskitunix_stdio_exit(void);

#include <oskit/version.h>
/* static char **version_string = &oskit_version_string; */

static void
sys_exit(int v)
{
	oskitunix_stdio_exit();
	native__exit(v);
}
#endif

char **environ;
char *__progname;

void
_start(char *arguments, ...)
{
    fptr rtld_cleanup;
    int argc;
    char **argv;
    char **env;

    /* Must be first, expects magic in %edx */
    rtld_cleanup = get_rtld_cleanup();

#ifdef OSKIT
    oskitunix_mem_init();
#endif

    argv = &arguments;
    argc = * (int *) (argv - 1);
    env = argv + argc + 1;
    environ = env;
    if(argc > 0)
	__progname = argv[0];
    else
        __progname = "";

    if(&_DYNAMIC != 0)
	atexit(rtld_cleanup);

#ifdef GCRT
    atexit(_mcleanup);
#endif
#ifndef __arm__			/* XXX */
    atexit(__oskit_fini);
#endif
#ifdef GCRT
    monstartup(&eprol, &etext);
#endif
#ifndef __arm__			/* XXX */
    __oskit_init();
#endif
#ifdef OSKIT
	oskit_usermode_simulation = 1;
	oskit_libc_exit = sys_exit;
	oskitunix_stdio_init();
	/*
	 * Switch over to the oskit stack. This allows a mainthread
	 * stackguard to work properly in usermode, and ensures that
	 * errant programs will be caught in user mode.
	 */
#ifdef __i386__
	asm ("movl $base_stack_end, %esp");
#elif __arm__
	asm ("mov sp, %0" : : "r" (&base_stack_end));
#else
# error what processor?
#endif
#endif /* OSKIT */
    exit( main(argc, argv, env) );
}

#ifdef GCRT
__asm__(".text");
__asm__("eprol:");
__asm__(".previous");
#endif
