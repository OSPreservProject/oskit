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
 * Simple UNIX-style resource map routines
 */
#include <oskit/amm.h>
#include <errno.h>
#include "rmap.h"

static amm_entry_t *rmap_alloc_entry(amm_t *amm, oskit_addr_t addr,
				     oskit_size_t size, int flags);
static void rmap_free_entry(amm_t *amm, amm_entry_t *entry);

/*
 * XXX need
 *	amm_submap(amm_t *amm, oskit_addr_t start, oskit_size_t size)
 * for common sub-maps.  Would mark the rest of the map unavailable.
 */
void rminit(struct rmap *map, long size, long addr, char *name, int nent)
{
	oskit_addr_t saddr = (oskit_addr_t)addr;
	oskit_addr_t eaddr = saddr + size;

	strncpy(map->name, name, 32);
	map->elts = 0;
	map->maxelts = nent;
	amm_init_gen(&map->amm, RMAP_FREE, 0,
		     rmap_alloc_entry, rmap_free_entry, 0, 0);
	if (AMM_MINADDR < saddr)
		(void)amm_modify(&map->amm, AMM_MINADDR, saddr - AMM_MINADDR,
				 RMAP_UNAVAIL, 0);
	if (eaddr < AMM_MAXADDR)
		(void)amm_modify(&map->amm, eaddr, AMM_MAXADDR - eaddr,
				RMAP_UNAVAIL, 0);
}

/*
 * XXX need:
 *	amm_alloc(amm_t *amm, oskit_addr_t addr, oskit_size_t size, int anywhere)
 * style interface for the common case of allocating free resources.
 */
long rmalloc(struct rmap *map, long size)
{
	amm_entry_t *entry;
	oskit_addr_t addr;
	int rc;

	if (size <= 0)
		panic("rmalloc: bad size");
	addr = AMM_MINADDR;
	entry = amm_find_gen(&map->amm, &addr, (oskit_size_t)size, RMAP_FREE,
			     -1, 0, 0, 0);
	if (entry == 0)
		return 0;
	rc = amm_modify(&map->amm, addr, (oskit_size_t)size, RMAP_INUSE, 0);
	if (rc == ENOMEM) {
		rc = amm_modify(&map->amm, amm_entry_start(entry),
				amm_entry_size(entry), RMAP_INUSE, 0);
		if (rc)
			panic("rmalloc: corrupt map");
		printf("WARNING: %s: resources lost\n", map->name);
	} else if (rc)
		panic("rmalloc: corrupt map");
	return (long)amm_entry_start(entry);
}

/*
 * XXX need:
 *	amm_dealloc(amm_t *amm, oskit_addr_t addr, oskit_size_t size)
 * style interface for the common case of deallocating resources.
 */
void rmfree(struct rmap *map, long size, long uaddr)
{
	amm_entry_t *entry;
	oskit_addr_t addr;
	int rc;

	if (size <= 0)
		panic("rmfree: bad size");
	addr = (oskit_addr_t)uaddr;
	entry = amm_find_gen(&map->amm, &addr, (oskit_size_t)size, RMAP_INUSE,
			     -1, 0, 0, 0);
	if (entry == 0)
		panic("rmfree: corrupt map");
	rc = amm_modify(&map->amm, addr, (oskit_size_t)size, RMAP_FREE, 0);
	if (rc == ENOMEM)
		printf("WARNING: %s: resources lost\n", map->name);
	else if (rc)
		panic("rmfree: corrupt map");
}

void rmdump(struct rmap *map)
{
	printf("MAP `%s' (0x%x):\n", map->name, map);
	amm_dump(&map->amm);
}

static amm_entry_t *
rmap_alloc_entry(amm_t *amm, oskit_addr_t addr, oskit_size_t size, int flags)
{
	struct rmap *map = (struct rmap *)amm;

	if (flags != RMAP_UNAVAIL) {
		if (map->elts == map->maxelts)
			return 0;
		map->elts++;
	}
	return (amm_entry_t *)malloc(sizeof(amm_entry_t));
}

static void
rmap_free_entry(amm_t *amm, amm_entry_t *entry)
{
	struct rmap *map = (struct rmap *)amm;

	if (amm_entry_flags(entry) != RMAP_UNAVAIL)
		map->elts--;
	free((void *)entry);
}

