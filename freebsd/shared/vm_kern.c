/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <oskit/dev/dev.h>

extern char *kmemlimit;

/*
 * We assign distinct bogus values to these
 * just so we can distinguish them below.
 */
vm_map_t kernel_map = (vm_map_t)1;
vm_map_t kmem_map = (vm_map_t)2;

/*
 * Allocate wired-down memory in the kernel's address space.
 * This allocator is expected to manage memory at page granularity.
 */
vm_offset_t
kmem_alloc(vm_map_t map, vm_size_t size)
{
	return (vm_offset_t)osenv_mem_alloc(round_page(size), 0, NBPG);
}

/*
 * Free memory allocated with kmem_alloc() or kmem_malloc().
 */
void
kmem_free(vm_map_t map, vm_offset_t addr, vm_size_t size)
{
	osenv_mem_free((void*)addr, 0, round_page(size));
}

/*
 * Function to allocate a submap in the kernel's address space
 * from which to further allocate kernel memory.  (An old Mach-ism.)
 * This is only called by kmeminit() in kern/kern_malloc.c,
 * but we never actually call kmeminit(), so no problem...
 */
vm_map_t
kmem_suballoc(vm_map_t parent, vm_offset_t *min, vm_offset_t *max,
	      vm_size_t size, boolean_t pageable)
{
	panic("freebsd_glue: kmem_suballoc not implemented");
}

/*
 * Function to allocate memory for higher-level allocators
 * such as the BSD kernel malloc (kern/kern_malloc.c).
 */
vm_offset_t
kmem_malloc(vm_map_t map, vm_size_t size, boolean_t waitflag)
{
	osenv_memflags_t env_flags = waitflag == M_WAITOK ? 0 : OSENV_NONBLOCKING;
	vm_offset_t blk;

	size = round_page(size);

	blk = (vm_offset_t)osenv_mem_alloc(size, env_flags, NBPG);
	if (blk == 0)
		return 0;

	/*
	 * The kernel malloc code assumes that all memory blocks
	 * we return will be between kmembase and kmemlimit
	 * so that their addresses index into the kmemusage array.
	 * Since we can't control what osenv_mem_alloc() gives us,
	 * we instead check each allocated block it gives us,
	 * and if it falls outside the current kmem range,
	 * we dynamically alter BSD's view of reality to include the new range.
	 * This hack fundamentally depends on BSD's malloc() 
	 * not relying on these "constants" to remain constant
	 * during calls to kmem_malloc(), which,
	 * quite miraculously, appears to be true.
	 * It also depends on osenv_mem_alloc() not returning memory blocks
	 * with extremely huge holes between them
	 * (e.g., a few blocks at very high addresses and a few very low);
	 * otherwise we'll be trying to allocate a huge kmemusage array
	 * and will probably run out of memory.
	 */
	retry:
	if (map == kmem_map &&
	    (blk < (vm_offset_t)kmembase || blk >= (vm_offset_t)kmemlimit)) {
		char *newbase, *newlimit;
		struct kmemusage *newusage, *oldusage;
		int newnpg, oldnpg;

		newbase = (char*)(blk & ~(VM_KMEM_SIZE-1));
		newlimit = newbase + VM_KMEM_SIZE;

		if (kmemlimit == 0) {
			/*
			 * This is the very first allocation -
			 * create a kmemusage array for the VM_KMEM_SIZE chunk
			 * around the allocated block.
			 * Hopefully future blocks will also be around here.
			 */
			kmembase = newbase;
			kmemlimit = newlimit;
		} else {
			/*
			 * Reallocate the existing the kmemusage array
			 * to cover the new area in addition to the old.
			 */
			if (newbase > kmembase)
				newbase = kmembase;
			if (newlimit < kmemlimit)
				newlimit = kmemlimit;
		}

		/* Allocate a new kmemusage array */
		newnpg = (newlimit - newbase) / PAGE_SIZE;
		newusage = osenv_mem_alloc(newnpg * sizeof(struct kmemusage),
				          env_flags, 0);
		if (newusage == NULL) {
			osenv_mem_free((void*)blk, env_flags, size);
			return 0;
		}

		/*
		 * Ensure that the kmemusage array hasn't changed
		 * while we were potentially blocked during the allocation.
		 * Actually, it's OK if it's changed,
		 * just as long as it hasn't changed
		 * so as to cover an area outside of our new intended array.
		 */
		if (waitflag == M_WAITOK &&
		    (kmembase < newbase || kmemlimit > newlimit)) {
			osenv_mem_free(newusage, env_flags,
				      newnpg * sizeof(struct kmemusage));
			goto retry;
		}

		/*
		 * Copy the existing array into the new array.
		 * and start officially using the new array.
		 */
		if (kmemusage)
			memcpy(&newusage[(kmembase - newbase) / PAGE_SIZE],
			       &kmemusage[0],
			       sizeof(struct kmemusage) *
			       ((kmemlimit - kmembase) / PAGE_SIZE));
		oldusage = kmemusage;
		oldnpg = (kmemlimit - kmembase) / PAGE_SIZE;
		kmemusage = newusage;
		kmembase = newbase;
		kmemlimit = newlimit;

		/*
		 * Free the old array if there was one.
		 * We can't do this until after the switch is complete
		 * because the free call might block.
		 */
		if (oldusage)
			osenv_mem_free(oldusage, env_flags,
			      oldnpg * sizeof(struct kmemusage));
	}

	return blk;
}

