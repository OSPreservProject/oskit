/*
 * Copyright (c) 2001 University of Utah and the Flux Group.
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

#ifndef _OSKIT_X86_SPROC_H_
#define _OSKIT_X86_SPROC_H_

#ifdef ASSEMBLER
/* trap number (`int XX' instruction) used for system calls */
#define SYSCALL_INT	0x60

#else /*ASSEMBLER*/

#include <oskit/x86/tss.h>
#include <oskit/x86/seg.h>

/* machine dependent part of struct oskit_sproc */
struct oskit_sproc_machdep {
    struct x86_tss			x86_tss;
    struct x86_desc			x86_tss_desc;
};

#endif /*ASSEMBLER*/
#endif /*_OSKIT_X86_SPROC_H_*/
