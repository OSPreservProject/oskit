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
/*
 * Public header file for the Simple Process Library.
 */
#ifndef _OSKIT_SPROC_H_
#define _OSKIT_SPROC_H_

#include <oskit/machine/sproc.h>

#include <oskit/c/sys/types.h>
#include <oskit/c/setjmp.h>
#include <oskit/exec/exec.h>
#include <oskit/threads/pthread.h>
#include <oskit/queue.h>
#include <oskit/uvm.h>

/* Max number of system call arguments */
#define MAXSYSCALLARGS		16

/*

 Process          Process Description   System Call Table
 +-----------+    +----------------+    +--------------------+
 |oskit_sproc|--->|oskit_sproc_desc|--->|oskit_sproc_sysent[]|
 +-----------+    +----------------+    +--------------------+
       |
       |
       |  Thread                  User stack
       |  +------------------+    +-----------------+
       +--|oskit_sproc_thread|--->|oskit_sproc_stack|
       |  +------------------+    +-----------------+
       |
       |  Thread                  User stack
       |  +------------------+    +-----------------+
       +--|oskit_sproc_thread|--->|oskit_sproc_stack|
       |  +------------------+    +-----------------+
       |
       ...

*/

struct oskit_sproc_thread;

/* system call implementation function
 *  
 *  - sth	current executing thread
 *  - arg	pointer passed to the system call
 *  - rval[2]	64bit wide return value
 *
 * should return zero if no error.  non zero value is error code.
 */
typedef int (*oskit_sproc_syscall_func)(struct oskit_sproc_thread *sth,
					void *arg, int rval[2]);

/* system call definition */
struct oskit_sproc_sysent {
    oskit_sproc_syscall_func	entry;
    int				nargs; /* # of args in sizeof(int) */
};

/* process description */
struct oskit_sproc_desc {
    /* maximum number of system call number */
    int				sd_nsyscall;

    /* system call table */
    struct oskit_sproc_sysent	*sd_syscalltab;

    /* trap handler that captures all traps while executing user mode code */
    int				(*sd_handler)(struct oskit_sproc_thread *sth,
					      int signo, int code,
					      struct trap_state *frame);
};

/* process definition */
struct oskit_sproc {
    pthread_mutex_t			sp_lock;
    struct oskit_vmspace		*sp_vm;
    int					sp_flags;
    queue_head_t			sp_thread_head;
    const struct oskit_sproc_desc	*sp_desc;
};

/* thread's stack definition */
struct oskit_sproc_stack {
    oskit_addr_t			st_base;
    oskit_size_t			st_size;
    oskit_size_t			st_redzonesize;
    /* current stack pointer value */
    oskit_addr_t			st_sp;
    struct oskit_sproc			*st_process;
    struct oskit_sproc_thread		*st_thread;
};

/* kernel thread definition */
struct oskit_sproc_thread {
    queue_chain_t			st_thread_chain;
    struct oskit_sproc			*st_process;
    oskit_addr_t			st_entry;
    jmp_buf				st_context;

    struct oskit_sproc_stack		*st_stack;
    /* machine dependent part */
    struct oskit_sproc_machdep		st_machdep;
};


/*
 *  Exported APIs
 */

/* initialize the simple process library */
void		oskit_sproc_init(void);

/* create a process and assign a process description to it */
oskit_error_t	oskit_sproc_create(const struct oskit_sproc_desc *desc,
				   oskit_size_t size,
				   struct oskit_sproc *outproc);

/* destroy a process.  all threads in the process should has exited. */
oskit_error_t	oskit_sproc_destroy(struct oskit_sproc *proc);

/* switch to user mode.  multiple threads can enter into a single process. */
void		oskit_sproc_switch(struct oskit_sproc *proc, oskit_addr_t entry,
				   struct oskit_sproc_stack *stack);

/* allocate a redzone protected stack within a user's address space */
oskit_error_t	oskit_sproc_stack_alloc(struct oskit_sproc *sproc, 
					oskit_addr_t *base,
					oskit_size_t size,
					oskit_size_t redzonesize,
					struct oskit_sproc_stack *out_stack);

/* push some arguments into a stack */
oskit_error_t	oskit_sproc_stack_push(struct oskit_sproc_stack *stack,
				       void *arg, oskit_size_t argsize);

/* map an ELF executable onto the user's address space */
int		oskit_sproc_load_elf(struct oskit_sproc *proc, const char *file, 
				     exec_info_t *info_out);

/*
 * For returning from user mode to kernel.
 * Execution resumes from oskit_sproc_switch().
 */
#define	OSKIT_SPROC_RETURN(sthread,code) \
	longjmp((sthread)->st_context, code)

#endif /*_OSKIT_SPROC_H_*/
