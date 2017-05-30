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

/*
 * An example for simple process library.  This kernel loads an ELF binary
 * into a user space and execute it in user mode.
 * Several quite simple system calls are implemented.
 */

#include <oskit/c/termios.h>
#include <oskit/c/unistd.h>
#include <oskit/clientos.h>
#include <oskit/debug.h>
#include <oskit/exec/exec.h>
#include <oskit/gdb.h>
#include <oskit/machine/pc/phys_lmm.h>
#include <oskit/sproc.h>
#include <oskit/startup.h>
#include <oskit/threads/pthread.h>
#include <oskit/version.h>
#include <oskit/x86/proc_reg.h>
#include <oskit/x86/trap.h>
#include <oskit/page.h>
#include <oskit/c/malloc.h>
#include <stdio.h>

#include "proc.h"
#include "syscallno.h"

/* # of iteration */
#define NITER		1
/* # of processes run in parallel */
#define NPROCESS	4
/* # of threads in a single process */
#define NTHREAD		4

#undef MALLOC_REGRESSION_TEST

#define USER_STACK_SIZE	(16*1024)

/* for debugging sproc */
extern int	oskit_sproc_debug;

extern int	syscall_init(void);
static void	execute_process(void *);
static int	handler(struct oskit_sproc_thread *sth, int signo, int code, 
			struct trap_state *frame);
#ifdef MALLOC_REGRESSION_TEST
static void	malloc_regression_thread(void);
#endif

extern struct oskit_sproc_sysent my_syscall_tab[];

static struct oskit_sproc_desc process_desc = {
    NSYS,			/* # of system calls implemented */
    my_syscall_tab,		/* system call table */
    handler			/* trap handler */
};

extern int
main()
{
    int i, j;

#ifndef KNIT
    oskit_clientos_init_pthreads();
#endif

    printf("oskit_clientos_init_pthreads done\n");

#if 0
    {
	extern struct termios base_raw_termios, base_cooked_termios;
	printf("setting serial port for gdb\n");
	base_raw_termios.c_ispeed = B38400;
	base_raw_termios.c_ospeed = B38400;
	base_cooked_termios.c_ispeed = B38400;
	base_cooked_termios.c_ospeed = B38400;
	gdb_pc_com_init(2, 0);
	
	gdb_trap_mask = (1 <<T_PAGE_FAULT) | (1 << T_NO_FPU);
	printf("break!\n");
	gdb_breakpoint();
	printf("go!\n");
    }
#endif

    start_fs_bmod();

#if 0
    {
	extern int __isthreaded;
	__isthreaded = 1;		/* for freebsd libc */
    }
#endif

    start_clock();
    pthread_init(1);

    printf(">> Initializing UVM\n");
    oskit_uvm_init();
    oskit_uvm_swap_init();
    printf(">> Swap On\n");
    if (swapon("/swapfile")) {
	extern int errno;
	panic("swapon failed (errno %d)\n", errno);
    }
    printf(">> Starting the page daemon\n");
    oskit_uvm_start_pagedaemon();

    printf(">> Initializing Simple Process Library\n");
    oskit_sproc_init();

    printf(">> We are ready\n");
    
#ifdef  GPROF
    start_gprof();
#endif

    /* Initialize my system calls */
    syscall_init();

    /* For regression test only */
#ifdef MALLOC_REGRESSION_TEST
    {
	pthread_t thread;

	printf(">> Creating a kernel malloc regression test thread\n");
	if (pthread_create(&thread, NULL, 
			   (void*(*)(void*))malloc_regression_thread, 0)) {
	    panic("pthread_create failed\n");
	}
    }
#endif

    for (j = 0 ; j < NITER ; j++) {
	pthread_t th[NTHREAD];
	int rc;

	printf("********  Create processes (%d) ********\n", j);
	for (i = 0 ; i < NTHREAD ; i++) {
	    rc = pthread_create(&th[i], NULL,
				(void *(*)(void*))execute_process,
			(i % 2 == 0 ? "/usermain_hello"
			 : "/usermain_malloc"));
	    assert(rc == 0);
	}
	
	printf("********  Waiting (%d) ********\n", j);
	
	for (i = 0 ; i < NTHREAD ; i++) {
	    pthread_join(th[i], NULL);
	}
    }
    printf("******** Terminated ********\n");
    return 0;
}


#ifdef MALLOC_REGRESSION_TEST
static void
malloc_regression_thread()
{
    void *x;
    while (1) {
	x = malloc(4096);
	assert(x);
	free(x);
    }
}
#endif

static void
load_process(struct oskit_sproc *sproc, exec_info_t *exec_info, const char *elf)
{
    oskit_addr_t heap;
    oskit_error_t error;

    printf("**** creating a process [%s] (pid %p, thread %d) ****\n",
	   elf, sproc, (int)pthread_self());

    if (oskit_sproc_create(&process_desc, PROCESS_SIZE, sproc)) {
	panic("oskit_sproc_create failed\n");
    }

    error = oskit_sproc_load_elf(sproc, elf, exec_info);
    if (error) {
	panic("oskit_sproc_load_elf failed (0x%x)\n", error);
    }
#if 0
    printf("Execute user code: entry %x\n", exec_info->entry);
#endif

    /* map heap area */
    heap = HEAP_START_ADDR;
    error = oskit_uvm_mmap(sproc->sp_vm, &heap,
			  HEAP_SIZE, PROT_READ|PROT_WRITE,
			  MAP_PRIVATE|MAP_ANON|MAP_FIXED, 0, 0);
    if (error) {
	panic("oskit_uvm_mmap failed (heap)\n");
    }
    assert(heap == HEAP_START_ADDR);
}


/****************************************************************
 *
 *	Multiple threads in a Process
 *
 ****************************************************************/

struct arg {
    struct oskit_sproc	*sproc;
    oskit_addr_t	entry;
};

static void
lwp(struct arg *arg)
{
    oskit_addr_t stkaddr;
    struct oskit_sproc_stack stk;
    int userarg[] = {100, 200, 300};
    stkaddr = 0;

#if 0
    /* just for testing fubyte() */
    {
	int i;
	oskit_uvm_vmspace_set(arg->sproc->sp_vm);
	printf("Reading first 16bytes\n");
	for (i = 0 ; i < 16 ; i++) {
	    printf("%02x ", fubyte((void*)(OSKIT_UVM_MINUSER_ADDRESS + i)));
	}
	puts("\n");
	oskit_uvm_vmspace_set(&oskit_uvm_kvmspace);
    }
#endif

    if (oskit_sproc_stack_alloc(arg->sproc, &stkaddr, USER_STACK_SIZE, 
				PAGE_SIZE, &stk)) {
	panic("oskit_sproc_alloc_stack failed\n");
    }

    if (oskit_sproc_stack_push(&stk, &userarg, sizeof(userarg))) {
	panic("oskit_sproc_alloc_push failed\n");
    }

    printf("thread %d: process %p, user stack [%x..%x]\n", (int)pthread_self(), 
	   arg->sproc, stkaddr + PAGE_SIZE,
	   stkaddr + USER_STACK_SIZE + PAGE_SIZE);
    oskit_sproc_switch(arg->sproc, arg->entry, &stk);
}

static void
execute_process(void *arg)
{
    struct oskit_sproc sproc;
    exec_info_t exec_info;
    int i;
    const char *filename = (char *)arg;
    pthread_t th[NTHREAD];

    load_process(&sproc, &exec_info, filename);

    for (i = 0 ; i < NTHREAD ; i++) {
	struct arg arg;
	arg.sproc = &sproc;
	arg.entry = exec_info.entry;
	
	pthread_create(&th[i], NULL, 
		       (void*(*)(void*))lwp, &arg);
    }

    for (i = 0 ; i < NTHREAD ; i++) {
	pthread_join(th[i], NULL);
    }

    printf("**** destroying process (thread %d) ****\n", (int)pthread_self());
    oskit_sproc_destroy(&sproc);
}


/*
 *  Trap handler for the simple process library
 */
static int
handler(struct oskit_sproc_thread *sthread, int signo, int code, 
	struct trap_state *ts)
{
    printf("ERR!: thread = %d, process = %p, signo = %d, "
	   "code = %d, frame = %p\n", 
	   (int)pthread_self(), sthread->st_process, signo, code, ts);
    return 1;			/* abort execution */
#if 0
    return 0;			/* continue execution */
#endif
}

/*
 *  For debugging
 */
void
dump_kvmspace()
{
    oskit_uvm_vmspace_dump(&oskit_uvm_kvmspace);
}

