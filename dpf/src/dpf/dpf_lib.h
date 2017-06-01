/*
 * Copyright (c) 1997 M.I.T.
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
 *      This product includes software developed by MIT.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND 
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
 */


/* this file now has dpf_lib.h AND demand.h because the test
   file and the library code needs demand.h */

void *mk_udpu(int *sz, unsigned short src_port, unsigned char *src_ip);
void *mk_udpc(int *sz, unsigned short src_port, unsigned char *src_ip, unsigned short dst_port, unsigned char *dst_ip);
struct elem *mk_rarp(int *sz, unsigned char *ethaddr);
struct elem *mk_arp(int *sz, unsigned char *ipaddr);
struct elem *mk_custom(int *sz, unsigned short stream_no);
void mk_arp_string(char *new, unsigned char *ipaddr);
void mk_rarp_string(char *new, unsigned char *ethaddr);
void mk_udpc_string(char *new, unsigned short src_port, unsigned char *src_ip, unsigned short dst_port, unsigned char *dst_ip);
int dpf_one(void);

/* 
 * OSKIT
 * Not sure why this full copy of demand.h is included again here
 * althought I think the comment above is a clue.  
 * See comments in demand.h for why we comment/don't comment some
 * of these things. 
 */
#ifndef __DEMAND__
#define __DEMAND__
#include <stdio.h>

#ifdef NDEBUG
#       define demand(bool, msg)        do { } while(0)
#       define assert(bool)             do { } while(0)
#	define debug_stmt(stmt)		do { } while(0)
#else /* NDEBUG */
#	define debug_stmt(stmt)		do { stmt; } while(0)
#       if defined(__GNUC__) || defined(__ANSI__)
#               define assert(bool) do {                                \
                        if(bool)                                        \
                                (void)0;                                \
                        else                                            \
                                __assert(#bool, __FILE__, __LINE__);    \
                } while(0)

#               define demand(bool, msg) do {                           \
                        if(bool)                                        \
                                (void)0;                                \
                        else                                            \
                                __demand(#bool, #msg, __FILE__, __LINE__);\
                } while(0)


#       else /* __GNUC__ */

#               define assert(bool) do {                                \
                        if(bool)                                        \
                                (void)0;                                \
                        else                                            \
                                __assert("bool", __FILE__, __LINE__);   \
                } while(0)

#               define demand(bool, msg) do {                           \
                        if(bool)                                        \
                                (void)0;                                \
                        else                                            \
                                __demand("bool", "msg", __FILE__, __LINE__);\
                } while(0)

#               define fatal(msg) __fatal(__FILE__, __LINE__, "msg")
#       endif   /* __GNUC__ */

#define __demand(_bool, _msg, _file, line) do {                         \
        printf("file %s, line %d: %s\n",_file,line,_bool);              \
        printf("%s\n",_msg);                                            \
        exit(1);                                                        \
} while(0)

#define __assert(_bool, _file, line) do {                               \
        printf("file %s, line %d: %s\n",_file,line,_bool);              \
        exit(1);                                                        \
} while(0)

#endif /* NDEBUG */

#define __fatal(_file, _line, _msg) do {                                \
        printf("PANIC: %s\n",_msg);                                     \
        printf("(file %s, line %d)\n",_file,_line);                     \
        exit(1);                                                        \
} while(0)

#define fatal(msg) __fatal(__FILE__, __LINE__, #msg)
void exit(int);
#ifndef OSKIT
void panic (char *str);
#endif

        
/* Compile-time assertion used in function. */
#define AssertNow(x) switch(1) { case (x): case 0: }

/* Compile time assertion used at global scope. */
#define _assert_now2(y) _ ##  y
#define _assert_now1(y) _assert_now2(y)
#define AssertNowF(x) \
  static inline void _assert_now1(__LINE__)() { \
                /* used to shut up warnings */                  \
                void (*fp)(int) = _assert_now1(__LINE__);       \
                fp(1);                                          \
                switch(1) case (x): case 0: ;                   \
  }

#endif

