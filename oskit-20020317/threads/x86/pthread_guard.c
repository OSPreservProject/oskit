/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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
 * Stack Guards. This file is overridden by the unix mode library.
 */

#include <threads/pthread_internal.h>

#ifdef	STACKGUARD
#include <oskit/c/sys/mman.h>
#include <oskit/io/absio.h>
#include <oskit/svm.h>

/*
 * Init the VM code.
 */
void
pthread_init_guard(void)
{
	svm_init((oskit_absio_t *) 0);
}

/*
 * Set the memory protection for a region by calling mprotect. 
 */
int
pthread_mprotect(oskit_addr_t addr, size_t length, int prot)
{
	int		rcode;

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

	if ((rcode = mprotect((void *) addr, length, prot)) < 0)
		printf("pthread_mprotect: mprotect failed for: "
		       "addr:0x%x len:%d prot:0x%x\n", addr, length, prot);

	return rcode;
}
#endif
