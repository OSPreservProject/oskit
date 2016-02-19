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
 * Simple UNIX-style address map routines
 */
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include "amap.h"

static struct amap *themap;
extern int errno;

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

/*
 * XXX for illustration purposes, we maintain a fixed list of map entries
 * so we can use our own alloc/free/split/join routines.
 */
#define NUMME	100
static struct amap_entry *mapfree;

static struct amap_entry *
alloc_map_entry(struct amap *map)
{
	struct amap_entry *me;

#ifdef DEBUG
	printf("\t0x%x: map_alloc", map);
#endif

	if (mapfree == 0) {
#ifdef DEBUG
		printf("failed\n");
#endif
		return 0;
	}
	me = mapfree;
	mapfree = mapfree->next;
	me->map = map;
	me->next = 0;

#ifdef DEBUG
	printf("->0x%x\n", me);
#endif
	return me;
}

static void
free_map_entry(struct amap_entry *me)
{
#ifdef DEBUG
	printf("\t0x%x: map_free 0x%x\n", me->map, me);
#endif
	me->map = 0;
	me->next = mapfree;
	mapfree = me;
}

static amm_entry_t *
alloc_amm_entry(amm_t *amm, oskit_addr_t addr, oskit_size_t size, int flags)
{
	amm_entry_t *entry;
 
#ifdef DEBUG
	printf("0x%x: alloc: addr=0x%x, size=0x%x, flags=0x%x: ",
	       amm, addr, size, flags);
#endif
	/*
	 * Don't ue full-sized entries for free or reserved memory
	 */
	if (flags == AMAP_FREE || flags == AMAP_RESERVED) {
		entry = (amm_entry_t *)smalloc(sizeof(amm_entry_t));
#ifdef DEBUG
		printf("smalloc->0x%x\n", entry);
#endif
	} else
		entry = (amm_entry_t *)alloc_map_entry((struct amap *)amm);

	return entry;
}

static void
free_amm_entry(amm_t *amm, amm_entry_t *entry)
{
	if (amm_entry_flags(entry) == AMAP_FREE ||
	    amm_entry_flags(entry) == AMAP_RESERVED) {
#ifdef DEBUG
		printf("0x%x: free: 0x%x\n", amm, entry);
#endif
		sfree(entry, sizeof *entry);
	} else
		free_map_entry((struct amap_entry *)entry);
}

static int
split_amm_entry(amm_t *amm, amm_entry_t *entry, oskit_addr_t addr,
	  amm_entry_t **head, amm_entry_t **tail)
{
	struct amap_entry *me = (struct amap_entry *)entry;
	amm_entry_t *nentry;

#ifdef DEBUG
	printf("0x%x: split: entry=0x%x, addr=0x%x, flags=0x%x",
	       amm, entry, addr, amm_entry_flags(entry));
#endif

	nentry = alloc_amm_entry(amm, addr, amm_entry_end(entry) - addr,
				 amm_entry_flags(entry));
	if (nentry == 0) {
#ifdef DEBUG
		printf("failed\n");
#endif
		return ENOMEM;
	}

	*head = entry;
	*tail = nentry;

#ifdef DEBUG
	printf("->head=0x%x, tail=0x%x\n", *head, *tail);
#endif
	return 0;
}

static int
join_amm_entry(amm_t *amm, amm_entry_t *head, amm_entry_t *tail,
	 amm_entry_t **new)
{
	struct amap_entry *hme = (struct amap_entry *)head;
	struct amap_entry *tme = (struct amap_entry *)tail;

	assert(hme->map == tme->map);

#ifdef DEBUG
	printf("0x%x: join: head=0x%x, tail=0x%x", amm, head, tail);
#endif

	*new = alloc_amm_entry(amm, amm_entry_start(head),
			       amm_entry_end(tail) - amm_entry_start(head),
			       amm_entry_flags(head));
	if (*new == 0) {
#ifdef DEBUG
		printf("failed\n");
#endif
		return ENOMEM;
	}

#ifdef DEBUG
	printf("->new=0x%x\n", *new);
#endif
	return 0;
}

void
minit(struct amap *map)
{
	struct amap_entry *me;
	int i;

	me = (struct amap_entry *)smalloc(sizeof(struct amap) * NUMME);
	mapfree = me;
	for (i = 0; i < NUMME-1; i++) {
		me->next = me + 1;
		me++;
	}
	me->next = 0;
	amm_init_gen(&map->amm, AMAP_FREE, 0,
		     alloc_amm_entry, free_amm_entry,
		     split_amm_entry, join_amm_entry);

	/*
	 * First page and upper half of the AS is unusable
	 */
	i = amm_modify(&map->amm, AMM_MINADDR, PAGE_SIZE - AMM_MINADDR,
		       AMAP_RESERVED, 0);
	if (i)
		panic("minit: cannot set lo range");
	i = amm_modify(&map->amm, 0x80000000, AMM_MAXADDR - 0x80000000,
		       AMAP_RESERVED, 0);
	if (i)
		panic("minit: cannot set hi range");

	themap = map;
}

void
mdestroy(struct amap *map)
{
	amm_destroy(&map->amm);
}

char *
mmap(char *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	int attr = AMM_FORWARD|AMM_FIRSTFIT;
	int aflags;
	oskit_addr_t aaddr;
	struct amap *map = themap;	/* XXX */
	amm_entry_t *entry;
	int rc;

	if (fd != -1 || offset != 0) {
		errno = EINVAL;
		return (char *)-1;
	}
	/* XXX check page alignment, etc. */
	prot &= AMAP_PROTMASK;

	if (flags & MAP_FIXED)
		attr |= AMM_EXACT_ADDR;

	aaddr = (oskit_addr_t)addr;
	entry = amm_find_gen(&map->amm, &aaddr, (oskit_size_t)len,
			     AMAP_FREE, -1, 0, 0, attr);
	if (entry == 0) {
		errno = ENOMEM;
		return (char *)-1;
	}

	errno = amm_modify(&map->amm, aaddr, (oskit_size_t)len,
			   AMAP_ALLOCATED|prot,
			   (amm_entry_t *)alloc_map_entry(map));
	if (errno)
		return (char *)-1;

	return (char *)aaddr;
}

int
munmap(char *addr, size_t len)
{
	struct amap *map = themap;	/* XXX */

	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       AMAP_FREE, -1);
}

int
mprotect(char *addr, size_t len, int prot)
{
	struct amap *map = themap;	/* XXX */

	prot &= AMAP_PROTMASK;
	/* XXX should fail if some portion of the AS is not allocated */
	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       prot, AMAP_PROTMASK);
}

int
mlock(char *addr, size_t len)
{
	struct amap *map = themap;	/* XXX */

	/* XXX should fail if some portion of the AS is not allocated */
	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       AMAP_WIRED, AMAP_WIRED);
}

int
munlock(char *addr, size_t len)
{
	struct amap *map = themap;	/* XXX */

	/* XXX should fail if some portion of the AS is not allocated/locked */
	return amap_prot_range(map, (oskit_addr_t)addr, (oskit_size_t)len,
			       0, AMAP_WIRED);
}

/*
 * Modify the attributes of all allocated entries in the given range
 */
int
amap_prot_range(struct amap *map, oskit_addr_t addr, oskit_size_t size,
		int prot, int protmask)
{
	oskit_addr_t saddr, eaddr;
	amm_entry_t *entry;
	int rc;

	/*
	 * Loop over individual entries, skipping free and reserved ones
	 */
	saddr = addr;
	eaddr = saddr + size;
	while (saddr < eaddr) {
		entry = amm_find_gen(&map->amm, &saddr, PAGE_SIZE,
				     AMAP_ALLOCATED, AMAP_ALLOCATED, 0, 0, 0);
		/*
		 * No more allocated memory in the range, all done.
		 */
		if (entry == 0)
			break;
		/*
		 * Protection is already correct, nothing to do.
		 */
		if ((entry->flags & protmask) == prot) {
			saddr = amm_entry_end(entry);
			continue;
		}
		/*
		 * Set the new protection
		 */
		prot = (entry->flags & ~protmask) | prot;

		/*
		 * Entry is completely contained in the range,
		 * just modify the existing entry.
		 */
		assert(saddr >= amm_entry_start(entry));
		if (amm_entry_start(entry) == saddr &&
		    amm_entry_end(entry) <= eaddr) {
			rc = amm_modify(&map->amm,
					saddr, amm_entry_size(entry), prot,
					prot == AMAP_FREE ? 0 : entry);
			if (rc)
				return EINVAL;
			saddr = amm_entry_end(entry);
			continue;
		}
		/*
		 * Otherwise allocate an entry for the new range
		 */
		rc = amm_modify(&map->amm, saddr, eaddr - saddr, prot,
				prot == AMAP_FREE ? 0 :
				(amm_entry_t *)alloc_map_entry(map));
		if (rc)
			return EINVAL;
		saddr = eaddr;
	}
	return 0;
}

int
mdumpfunc(amm_t *amm, amm_entry_t *entry, void *arg)
{
	struct amap_entry *me;
	oskit_addr_t saddr, eaddr;
	int flags;

	flags = amm_entry_flags(entry);
	if (flags == AMAP_RESERVED || flags == AMAP_FREE)
		return 0;

	saddr = amm_entry_start(entry);
	eaddr = amm_entry_end(entry);
	me = (struct amap_entry *)entry;

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
mdump(struct amap *map)
{
	printf("MAP 0x%x:\n", map);
	(void)amm_iterate(&map->amm, mdumpfunc, 0);
}
