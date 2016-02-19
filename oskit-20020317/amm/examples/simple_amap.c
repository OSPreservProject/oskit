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
/*
 * Simple(st) UNIX-style address map routines
 */
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include "simple_amap.h"

static struct simple_amap *themap;
extern int errno;

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

void
simple_minit(struct simple_amap *map)
{
	amm_init(&map->amm, PAGE_SIZE, 0x80000000);

	themap = map;
}

void
simple_mdestroy(struct simple_amap *map)
{
	amm_destroy(&map->amm);
}

char *
mmap(char *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	oskit_addr_t aaddr;
	struct simple_amap *map = themap;	/* XXX */
	int rc;

	if (fd != -1 || offset != 0) {
		errno = EINVAL;
		return (char *)-1;
	}
	/* XXX check page alignment, etc. */
	prot &= AMAP_PROTMASK;

	aaddr = (oskit_addr_t)addr;
	rc = amm_allocate(&map->amm, &aaddr, (oskit_size_t)len, prot);
	if (rc || ((flags & MAP_FIXED) && aaddr != addr) {
		if (rc == 0)
			(void)amm_deallocate(&map->amm, aaddr, (oskit_size_t)len);
		errno = ENOMEM;
		return (char *)-1;
	}

	return (char *)aaddr;
}

int
munmap(char *addr, size_t len)
{
	struct simple_amap *map = themap;	/* XXX */

	return amm_deallocate(&map->amm, (oskit_addr_t)addr, (oskit_size_t)len);
}

int
mprotect(char *addr, size_t len, int prot)
{
	struct simple_amap *map = themap;	/* XXX */

	prot &= AMAP_PROTMASK;
	/* XXX should fail if some portion of the AS is not allocated */
	return amm_protect(&map->amm, (oskit_addr_t)addr, (oskit_size_t)len, prot);
}

/*
 * XXX amm_protect can only be used to set absolute protection values.
 * For wiring we want to and/or bits into the existing values.
 */
int
mlock(char *addr, size_t len)
{
	struct simple_amap *map = themap;	/* XXX */

	/* XXX should fail if some portion of the AS is not allocated */
	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       AMAP_WIRED, AMAP_WIRED);
}

int
munlock(char *addr, size_t len)
{
	struct simple_amap *map = themap;	/* XXX */

	/* XXX should fail if some portion of the AS is not allocated/locked */
	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       0, AMAP_WIRED);
}

/*
 * Modify the attributes of all allocated entries in the given range
 */
int
amap_prot_range(struct simple_amap *map, oskit_addr_t addr, oskit_size_t size,
		int prot, int protmask)
{
	oskit_addr_t saddr, eaddr;
	oskit_size_t size;
	amm_entry_t *entry;
	int flags, rc;

	saddr = addr;
	eaddr = saddr + size;
	while (saddr < eaddr) {
		/*
		 * Find the next allocated entry and modify its protection
		 * if necessary.
		 */
		entry = amm_find_addr(amm, saddr);
		flags = amm_entry_flags(entry);
		if ((flags & AMM_ALLOCATED) == AMM_ALLOCATED) {
			if (eaddr < amm_entry_end(entry))
				size = eaddr - saddr;
			else
				size = amm_entry_end(entry) - saddr;
			if ((flags & protmask) != prot) {
				prot = (flags & ~protmask) | prot;
				rc = amm_modify(amm, saddr, size, prot, 0);
				if (rc)
					return rc;
			}
		}
		saddr = amm_entry_end(entry);
	}
	return 0;
}

int
mdumpfunc(amm_t *amm, amm_entry_t *entry, void *arg)
{
	struct simple_amap_entry *me;
	oskit_addr_t saddr, eaddr;
	int flags;

	flags = amm_entry_flags(entry);
	if (flags == AMAP_RESERVED || flags == AMAP_FREE)
		return 0;

	saddr = amm_entry_start(entry);
	eaddr = amm_entry_end(entry);
	me = (struct simple_amap_entry *)entry;

	printf("\t0x%x: [0x%x-0x%x]: %c%c%c,%c, map=0x%x\n",
	       me, saddr, eaddr,
	       (flags & 4) ? 'r' : '-',
	       (flags & 2) ? 'w' : '-',
	       (flags & 1) ? 'x' : '-',
	       (flags & AMAP_WIRED) ? 'W' : '-',
	       me->map);
	return 0;
}

void
mdump(struct simple_amap *map)
{
	printf("MAP 0x%x:\n", map);
	(void)amm_iterate(&map->amm, mdumpfunc, 0);
}
