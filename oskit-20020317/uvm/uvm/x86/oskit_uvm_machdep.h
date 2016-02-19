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

#ifndef _OSKIT_UVM_INTERNAL_MACHDEP_H_
#define _OSKIT_UVM_INTERNAL_MACHDEP_H_

#include <oskit/x86/base_trap.h>

/* this is for providing pcb for in-kernel threads */
extern struct user	oskit_uvm_user;
#define DEFAULT_PCB	(&oskit_uvm_user.u_pcb)

struct oskit_uvm_per_thread {
    struct oskit_uvm_per_thread		*pt_next;
    pthread_t				pt_tid;

    oskit_vmspace_t			pt_vm;
    struct user				pt_user;
    struct lwp				pt_lwp;
};

extern int	oskit_uvm_pfault_handler(struct trap_state *ts);
extern void	oskit_uvm_redzone_init(void);

#endif /*_OSKIT_UVM_INTERNAL_MACHDEP_H_*/
