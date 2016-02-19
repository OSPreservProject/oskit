/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * This header file defines the initial stack provided by the base environment.
 * The stack is BASE_STACK_SIZE bytes by default.
 */
#ifndef _OSKIT_X86_BASE_STACK_H_
#define _OSKIT_X86_BASE_STACK_H_

#define BASE_STACK_SIZE		65536

#ifndef ASSEMBLER

extern char base_stack_start[], base_stack_end[];

/* For the sake of unix mode, where stacks are not always BASE_STACK_SIZE. */
extern int  base_stack_size;

#endif

#endif /* _OSKIT_X86_BASE_STACK_H_ */
