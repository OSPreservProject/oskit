/*
 * Copyright (c) 2001 The University of Utah and the Flux Group.
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

#ifndef _OSKIT_SPROC_MACHDEP_H_
#define _OSKIT_SPROC_MACHDEP_H_

#include <oskit/c/stdio.h>
#include <oskit/debug.h>
#include <oskit/dev/dev.h>
#include <oskit/error.h>
#include <oskit/threads/pthread.h>
#include <oskit/threads/pthread.h>
#include <oskit/x86/base_gdt.h>
#include <oskit/x86/base_idt.h>
#include <oskit/x86/base_trap.h>
#include <oskit/x86/base_tss.h>
#include <oskit/x86/base_vm.h>
#include <oskit/x86/gate_init.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/trap.h>

#include "sproc_internal.h"

extern struct gate_init_entry oskit_sproc_svc_trap_inittab[];
extern void oskit_sproc_csw(void);

#define UPDATE_TSS(sthread)					\
do {								\
    base_gdt[BASE_TSS/8] = (sthread)->st_machdep.x86_tss_desc;	\
    set_tr(BASE_TSS);						\
} while (0)

#endif /*_OSKIT_SPROC_MACHDEP_H_*/
