/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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
 * This simple program tests the SVM (Simple Virtual Memory) module to
 * to ensure that read-only pages are indeed read-only, that stack
 * overflow is properly detected, and that the signal code properly
 * generates the proper signals and that the instruction stream can be
 * restarted.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <oskit/startup.h>
#include <oskit/clientos.h>
#include <oskit/svm.h>
#include <oskit/machine/base_stack.h>

void
segvhandler(int sig, siginfo_t *info, void *scp_addr)
{
	struct sigcontext	*scp = (struct sigcontext *) scp_addr;
		
	printf("SegvHandler: sig:%d code:%d val:%d scp:%p pc:0x%x\n",
	       info->si_signo, info->si_code, info->si_value.sival_int,
	       scp, scp->sc_pc);

	/*
	 * Munge the return address
	 */
#if	defined(OSKIT_X86)
	scp->sc_pc += 6;
#elif   defined(OSKIT_ARM32)
	scp->sc_pc += 4;
#else
#error  "Fix me"
#endif
}

void
bushandler(int sig, siginfo_t *info, void *scp_addr)
{
	struct sigcontext	*scp = (struct sigcontext *) scp_addr;
		
	printf("BusHandler: sig:%d code:%d val:%d scp:%p pc:0x%x\n",
	       info->si_signo, info->si_code, info->si_value.sival_int,
	       scp, scp->sc_pc);

	/*
	 * Stack Overflow. Just exit
	 */
	exit(1);
}

static int
writemem(oskit_addr_t addr)
{
	*((volatile int *) addr) = 69;

	return 0;
}

static int
readmem(oskit_addr_t addr)
{
	return *((volatile int *) addr);
}

int
main(int argc, char **argv)
{
	oskit_addr_t		addr;
	struct sigaction	sa;
	void			oflow(int a);

	/*
	 * Init the OS.
	 */
	oskit_clientos_init();

	start_clock();
	svm_init((oskit_absio_t *) 0);

	/*
	 * Install SIGSEGV handler.
	 */
	sa.sa_sigaction = segvhandler;
	sa.sa_flags     = SA_SIGINFO;
	sa.sa_mask      = 0;

	if (sigaction(SIGSEGV, &sa, 0) < 0) {
		perror("sigaction SIGSEGV1");
		exit(1);
	}

	/*
	 * Install SIGBUS handler.
	 */
	sa.sa_sigaction = bushandler;
	sa.sa_flags     = SA_SIGINFO;
	sa.sa_mask      = 0;

	if (sigaction(SIGBUS, &sa, 0) < 0) {
		perror("sigaction SIGSEGV1");
		exit(1);
	}

	/*
	 * Allocate some memory to play with.
	 */
	addr = 0;
	if (svm_alloc(&addr, 0x8000, SVM_PROT_READ, 0))
		panic("svm_alloc failed");

	printf("svm_alloc returned 0x%x\n", addr);

	/*
	 * Try to read it. Should work.
	 */
	readmem(addr);

	/*
	 * Try to write it. Should fail with a signal.
	 */
	writemem(addr);

	/*
	 * Modify the protection bits and try again.
	 */
	printf("Changing to read/write and trying again ...\n");

	svm_protect(addr, 0x1000, SVM_PROT_READ|SVM_PROT_WRITE);
	writemem(addr);

	/*
	 * Once more and try again.
	 */
	printf("Changing to read only and trying again ...\n");

	svm_protect(addr, 0x1000, SVM_PROT_READ);
	writemem(addr);

	/*
	 * Lets try stack overflow detection. This should generate a SIGBUS.
	 */
	printf("Checking Stack overflow detection code ...\n");

	oflow(69);
	
	exit(0);
}

void
oflow(int a)
{
	void	oflowaux(int);

	oflowaux(a);
}

void
oflowaux(int a)
{
	oflow(a);
}
