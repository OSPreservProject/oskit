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
 * Public header file for the UVM.
 */

#ifndef _OSKIT_UVM_H_
#define _OSKIT_UVM_H_

#include <oskit/com.h>
#include <oskit/c/sys/mman.h>
#include <oskit/machine/uvm.h>
#include <oskit/machine/base_trap.h>

struct oskit_vmspace;
typedef struct oskit_vmspace *oskit_vmspace_t;

/* User supplied error handler function */
typedef	void	(*oskit_uvm_handler)(struct oskit_vmspace *,
				     int signo, struct trap_state *frame);

/*
 *  Exported APIs
 */

/* initialize uvm */
void		oskit_uvm_init(void);
/* start pagedaemon */
void		oskit_uvm_start_pagedaemon(void);
/* create a vmspace */
oskit_error_t	oskit_uvm_create(oskit_size_t size, oskit_vmspace_t *out_vm);
/* destroy a vmspace */
oskit_error_t	oskit_uvm_destroy(oskit_vmspace_t vm);
/* print debugging info */
void		oskit_uvm_vmspace_dump(oskit_vmspace_t vm);

/* initialize swapping stuff */
void		oskit_uvm_swap_init(void);
/* enable swapping to specified oskit_{blkio,absio}_t stream */
oskit_error_t	oskit_uvm_swap_on(oskit_iunknown_t *iunknown);
/* disable swapping to specified oskit_{blkio,absio}_t stream. (broken) */
oskit_error_t	oskit_uvm_swap_off(oskit_iunknown_t *iunknown);

/* mmap and munmap oskit_{blkio,absio}_t */
oskit_error_t	oskit_uvm_mmap(oskit_vmspace_t vm, oskit_addr_t *addr,
			       oskit_size_t size, int prot, int flags,
			       oskit_iunknown_t *iunknown, oskit_off_t foff);
oskit_error_t	oskit_uvm_munmap(oskit_vmspace_t vm, oskit_addr_t addr,
				 oskit_size_t len);
/* mprotect */
oskit_error_t	oskit_uvm_mprotect(oskit_vmspace_t vm, oskit_addr_t addr,
				   oskit_size_t len, int prot);
/* madvise */
oskit_error_t	oskit_uvm_madvise(struct oskit_vmspace *vm, oskit_addr_t addr,
				  oskit_size_t len, int behav);

/* assign a vmspace to current executing thread and returns previous assigned
 * vmspace. */
oskit_vmspace_t	oskit_uvm_vmspace_set(oskit_vmspace_t vm);

/* returns the vmspace of current executing thread */
oskit_vmspace_t	oskit_uvm_vmspace_get(void);


/*
 *  Copyin/copyout.   Make sure to call oskit_uvm_vmspace_set()
 *  before calling these functions.  This is to set curpcb variable
 *  for the NetBSD code.
 */

/* copyin and copyout */
oskit_error_t	copyin(const void *from, void *to, oskit_size_t len);
oskit_error_t	copyout(const void *from, void *to, oskit_size_t len);
oskit_error_t	copyinstr(const void *from, void *to, oskit_size_t len, 
			  oskit_size_t *lencopied);
oskit_error_t	copyoutstr(const void *from, void *to, oskit_size_t len, 
			   oskit_size_t *lencopied);

/* store to user space */
int		subyte(void *, int);
int		susword(void *, short);
int		suswintr(void *, short);
int		suword(void *, long);
int		suiword(void *, long);

/* fetch from user space */
int		fubyte(const void *);
int		fusword(const void *);
int		fuswintr(const void *);
long		fuword(const void *);

/* map specified physical address range into virtual address space
 * this function is for osenv_mem_map_phys() */
int		oskit_uvm_mem_map_phys(oskit_addr_t pa, oskit_size_t size,
				       void **addr, int flags);

void		oskit_uvm_handler_set(oskit_vmspace_t vm,
				      oskit_uvm_handler handler);
oskit_uvm_handler	oskit_uvm_handler_get(oskit_vmspace_t vm);

/* for simple process library */
void		oskit_uvm_csw_hook_set(void (*hook)(void));


/*
 *  Global Variables
 */

/* kernel vmspace */
extern struct oskit_vmspace oskit_uvm_kvmspace;

/* for debugging */
extern int oskit_uvm_debug;

#endif /*_OSKIT_UVM_H_*/
