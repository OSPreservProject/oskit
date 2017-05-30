/*
 * Copyright (c) 1996, 1998, 2000 University of Utah and the Flux Group.
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
 * Handle the details of stack guards.
 */
#include <oskit/machine/base_paging.h>
#include <oskit/dev/dev.h>

#include "native.h"

void	pthread_trap_handler(int sig);

/*
 * Init the VM code. In unix mode, just need to install a signal handler.
 */
int
pthread_init_guard(void)
{
	struct sigaction	sa;
	struct	sigaltstack 	ss;

	sa.sa_handler = pthread_trap_handler;
	sa.sa_flags   = 1;

	if (NATIVEOS(sigaction)(SIGBUS, &sa, 0) < 0) 
	{
		errno = NATIVEOS(errno);
		return -1;
	}

	if ((ss.ss_sp = malloc(0x2000)) == NULL)
		panic("pthread_init_guard: No more memory");

	ss.ss_size  = 0x2000;
	ss.ss_flags = 0;
	NATIVEOS(sigfillset)(&sa.sa_mask);
	NATIVEOS(sigdelset)(&sa.sa_mask, SIGINT);
	NATIVEOS(sigdelset)(&sa.sa_mask, SIGTERM);
	NATIVEOS(sigdelset)(&sa.sa_mask, SIGHUP);

	if (NATIVEOS(sigaltstack)(&ss, 0) < 0) {
	    	if (NATIVEOS(errno) == ENOSYS) {
		    	printf("Your system dosen't support sigaltstack,\n");
			printf("turn off STACKGUARD in pthreads and recompile.\n");
		}
		panic("pthread_init_guard: sigaltstack failed");
	}

	return 0;
}

/*
 * Set the memory protection for a region. Used for stack guards, but
 * might be useful for something else later on. The writeable flag
 * indicates whether the memory should be write protected or reset
 * back to writeable.
 */
int
pthread_mprotect(oskit_addr_t addr, size_t length, int prot)
{
	/*printf("pthread_mprotect: 0x%x %d %d\n", addr, length, prot);*/

	if (addr & (PAGE_SIZE - 1)) {
		printf("pthread_mprotect: addr 0x%x not aligned\n", addr);
		return EINVAL;
	}

	if (length & (PAGE_SIZE - 1)) {
		printf("pthread_mprotect: length %d not aligned\n", length);
		return EINVAL;
	}

	switch (prot) {
	case PROT_READ|PROT_WRITE:
	case PROT_READ:
		break;
	default:
		panic("pthread_mprotect: Bad prot: 0x%x", prot);
		break;
	}

	if (NATIVEOS(mprotect)((void *)addr, length, prot) < 0) {
		panic("pthread_mprotect: pdir_map_range at 0x%x\n", addr);
	}

	return 0;
}

/*
 * Trap handler
 */
void
pthread_trap_handler(int sig)
{
	panic("PAGE FAULT: Maybe it's a stack overflow ...");
}
