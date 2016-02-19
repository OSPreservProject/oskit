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


/*
 *  This file contains useful constants.  They can be overridden for 
 *  site-specific uses.
 */
#ifndef __DPF_CONFIG_H__
#define __DPF_CONFIG_H__

/* 
 * If your compiler has inline, removing this definition should improve
 * performance a little bit.
 */
/* #define inline */

/* 
 * Routine to copy user filters into dpf.  Prototype: 
 *	copyin(void *dst, void *src, unsigned nbytes);
 */
#define dpf_copyin memcpy

/* System call annotation.  Mainly meaningless. */
#define SYSCALL

/* Minimum message size (in bytes). */
#define DPF_MINMSG     64

/* Minimum guarenteed message alignment (in bytes). */
#define DPF_MSG_ALIGN 8

/* Maximum number of elements in a DPF filter. */
#define DPF_MAXELEM 256

/* 
 * Maximum number of active DPF filters.  Size + 1 must fit in the data 
 * type defined by fid_t. 
 */
#define DPF_MAXFILT 255
typedef uint8 fid_t; 

/* 
 * Maximum number of instructions required to fab DPF trie.  There is
 * no good reason that clients should have to know this value.
 */
#define DPF_MAXINSTS	2048

/*
 * Percentage of entries in hash table that must be used before dpf will
 * rehash in response to a collision.  Given as an integer.
 */
#define DPF_HT_USAGE 50 /* 50% */

/* Size of initial hash table: larger means less rehashing. */
#define DPF_INITIAL_HTSZ 64

/* Maximum number of instructions per atom. */
#define DPF_MAXCODE 	64

#endif /* __DPF_CONFIG_H__ */
