/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Linux kernel memory allocation.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/sched.h>
#include <linux/malloc.h>
#include <linux/delay.h>

#include "shared.h"
#include "osenv.h"

/* XXX are all these flags really necessary? */
#define KMALLOC_FLAGS	(OSENV_AUTO_SIZE | OSENV_PHYS_WIRED |	\
			 OSENV_VIRT_EQ_PHYS | OSENV_PHYS_CONTIG)

/*
 * Invoke the OS's memory allocation services,
 * saving the current pointer around the possibly-blocking call.
 */
void *
oskit_linux_mem_alloc(oskit_size_t size, osenv_memflags_t flags, unsigned align)
{
	struct task_struct *cur = current;
	void *mem;
	mem = osenv_mem_alloc(size, flags, align);
	current = cur;
	return mem;
}

void
oskit_linux_mem_free(void *block, osenv_memflags_t flags, oskit_size_t size)
{
	struct task_struct *cur = current;
	osenv_mem_free(block, flags, size);
	current = cur;
}

/*
 * Allocate wired down kernel memory.
 */
void *
kmalloc(unsigned int size, int priority)
{
	int flags;
	void *p;

	flags = KMALLOC_FLAGS;
	if (priority & GFP_ATOMIC)
		flags |= OSENV_NONBLOCKING;
	if (priority & GFP_DMA)
		flags |= OSENV_ISADMA_MEM;
	p = oskit_linux_mem_alloc(size, flags, 0);
	return (p);
}

/*
 * Free kernel memory.
 * XXX this should probably pass the same flags as kmalloc.  It doesn't matter
 *     right now since osenv_mem_free only listens to OSENV_AUTO_FLAGS.
 */
void
kfree(const void *p)
{
	oskit_linux_mem_free((void *) p, KMALLOC_FLAGS, 0);
}

/*
 * Allocate physically contiguous pages.
 */
unsigned long
__get_free_pages(int priority, unsigned long order)
{
	int flags, align;
	void *p;

	flags = OSENV_PHYS_WIRED|OSENV_VIRT_EQ_PHYS|OSENV_PHYS_CONTIG;
	align = PAGE_SIZE;
	if (priority & GFP_ATOMIC)
		flags |= OSENV_NONBLOCKING;
	if (priority & GFP_DMA) {
		flags |= OSENV_ISADMA_MEM;
		/*
		 * DMA allocations need to be naturally aligned.
		 *
		 * XXX this may be a general semantic of Linux memory
		 * allocation, need to check.
		 */
		osenv_assert(order < 256); /* XXX sanity */
		align = (PAGE_SIZE << order);
	}
	p = oskit_linux_mem_alloc(PAGE_SIZE << order, flags, align);
	return ((unsigned long)p);
}

/*
 * Free pages.
 */
void
free_pages(unsigned long addr, unsigned long order)
{
	oskit_linux_mem_free((void *)addr,
			     OSENV_PHYS_WIRED|OSENV_VIRT_EQ_PHYS|OSENV_PHYS_CONTIG,
			     PAGE_SIZE << order);
}

/*
 * Allocate virtually contiguous memory.
 * The underlying physical pages do not need to be contiguous.
 */
void *
vmalloc(unsigned long size)
{
	return oskit_linux_mem_alloc(size,
				     OSENV_AUTO_SIZE|OSENV_PHYS_WIRED,
				     0);
}

void
vfree(void *addr)
{
	oskit_linux_mem_free(addr, OSENV_AUTO_SIZE|OSENV_PHYS_WIRED, 0);
}

/*
 * Slab stuff. No caching, just fake it best we can.
 */
struct slab_stuff {
	char	*name;
	int	size;
};

kmem_cache_t *
kmem_cache_create(const char *name, size_t size, size_t offset,
		  unsigned long flags,
		  void (*ctor)(void*, kmem_cache_t *, unsigned long),
		  void (*dtor)(void*, kmem_cache_t *, unsigned long))
{
	struct slab_stuff	*stuffp;

	if (ctor || dtor || offset)
		osenv_panic("kmem_cache_create: Cannot handle ctor or dtor");

	stuffp = kmalloc(sizeof(struct slab_stuff) + strlen(name) + 1, 0);
	if (!stuffp)
		osenv_panic("kmem_cache_create: No memory left");

	stuffp->name = (char *) (stuffp + 1);
	stuffp->size = size;
	memcpy(stuffp->name, name, strlen(name));

	/* printk("kmem_cache_create: %s %d %p\n", name, size, stuffp); */

	return (kmem_cache_t *) stuffp;
}

void *
kmem_cache_alloc(kmem_cache_t *cachep, int flags)
{
	struct slab_stuff	*stuffp = (struct slab_stuff *) cachep;
	void			*obj;

	obj = kmalloc(stuffp->size, flags);

	/* printk("kmem_cache_alloc: %s %d %p\n",
	          stuffp->name, stuffp->size, obj); */

	return obj;
}

void
kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
#if 0
	struct slab_stuff	*stuffp = (struct slab_stuff *) cachep;

	/* printk("kmem_cache_free: %s %d %p\n",
	          stuffp->name, stuffp->size, objp); */
#endif

	kfree(objp);
}

void
kfree_s(const void *objp, size_t size)
{
	/* printk("kfree_s: %d %p\n", size, objp); */

	kfree(objp);
}
