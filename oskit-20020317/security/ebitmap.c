/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
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
 * Implementation of the extensible bitmap type.
 */

#include "ebitmap.h"

int ebitmap_or(ebitmap_t * dst, ebitmap_t * e1, ebitmap_t * e2)
{
	ebitmap_node_t *n1, *n2, *new, *prev;


	ebitmap_init(dst);

	n1 = e1->node;
	n2 = e2->node;
	prev = 0;
	while (n1 || n2) {
		new = (ebitmap_node_t *) malloc(sizeof(ebitmap_node_t));
		if (!new) {
			ebitmap_destroy(dst);
			return FALSE;
		}
		memset(new, 0, sizeof(ebitmap_node_t));
		if (n1 && n2 && n1->startbit == n2->startbit) {
			new->startbit = n1->startbit;
			new->map = n1->map | n2->map;
			n1 = n1->next;
			n2 = n2->next;
		} else if (!n2 || (n1 && n1->startbit < n2->startbit)) {
			new->startbit = n1->startbit;
			new->map = n1->map;
			n1 = n1->next;
		} else {
			new->startbit = n2->startbit;
			new->map = n2->map;
			n2 = n2->next;
		}

		new->next = 0;
		if (prev)
			prev->next = new;
		else
			dst->node = new;
		prev = new;
	}

	dst->highbit = (e1->highbit > e2->highbit) ? e1->highbit : e2->highbit;
	return TRUE;
}


int ebitmap_cmp(ebitmap_t * e1, ebitmap_t * e2)
{
	ebitmap_node_t *n1, *n2;


	if (e1->highbit != e2->highbit)
		return FALSE;

	n1 = e1->node;
	n2 = e2->node;
	while (n1 && n2 &&
	       (n1->startbit == n2->startbit) &&
	       (n1->map == n2->map)) {
		n1 = n1->next;
		n2 = n2->next;
	}

	if (n1 || n2)
		return FALSE;

	return TRUE;
}


int ebitmap_cpy(ebitmap_t * dst, ebitmap_t * src)
{
	ebitmap_node_t *n, *new, *prev;


	ebitmap_init(dst);
	n = src->node;
	prev = 0;
	while (n) {
		new = (ebitmap_node_t *) malloc(sizeof(ebitmap_node_t));
		if (!new) {
			ebitmap_destroy(dst);
			return FALSE;
		}
		memset(new, 0, sizeof(ebitmap_node_t));
		new->startbit = n->startbit;
		new->map = n->map;
		new->next = 0;
		if (prev)
			prev->next = new;
		else
			dst->node = new;
		prev = new;
		n = n->next;
	}

	dst->highbit = src->highbit;
	return TRUE;
}


int ebitmap_contains(ebitmap_t * e1, ebitmap_t * e2)
{
	ebitmap_node_t *n1, *n2;


	if (e1->highbit < e2->highbit)
		return FALSE;

	n1 = e1->node;
	n2 = e2->node;
	while (n1 && n2 && (n1->startbit <= n2->startbit)) {
		if (n1->startbit < n2->startbit) {
			n1 = n1->next;
			continue;
		}
		if ((n1->map & n2->map) != n2->map)
			return FALSE;

		n1 = n1->next;
		n2 = n2->next;
	}

	if (n2)
		return FALSE;

	return TRUE;
}


int ebitmap_get_bit(ebitmap_t * e, unsigned long bit)
{
	ebitmap_node_t *n;


	if (e->highbit < bit)
		return FALSE;

	n = e->node;
	while (n && (n->startbit <= bit)) {
		if ((n->startbit + MAPSIZE) > bit) {
			if (n->map & (MAPBIT << (bit - n->startbit)))
				return TRUE;
			else
				return FALSE;
		}
		n = n->next;
	}

	return FALSE;
}


int ebitmap_set_bit(ebitmap_t * e, unsigned long bit, int value)
{
	ebitmap_node_t *n, *prev, *new;


	prev = 0;
	n = e->node;
	while (n && n->startbit <= bit) {
		if ((n->startbit + MAPSIZE) > bit) {
			if (value) {
				n->map |= (MAPBIT << (bit - n->startbit));
			} else {
				n->map &= ~(MAPBIT << (bit - n->startbit));
				if (!n->map) {
					/* drop this node from the bitmap */

					if (!n->next) {
						/*
						 * this was the highest map
						 * within the bitmap
						 */
						if (prev)
							e->highbit = prev->startbit + MAPSIZE;
						else
							e->highbit = 0;
					}
					if (prev)
						prev->next = n->next;
					else
						e->node = n->next;

					free(n);
				}
			}
			return TRUE;
		}
		prev = n;
		n = n->next;
	}

	if (!value)
		return TRUE;

	new = (ebitmap_node_t *) malloc(sizeof(ebitmap_node_t));
	if (!new)
		return FALSE;
	memset(new, 0, sizeof(ebitmap_node_t));

	new->startbit = bit & ~(MAPSIZE - 1);
	new->map = (MAPBIT << (bit - new->startbit));

	if (!n)
		/* this node will be the highest map within the bitmap */
		e->highbit = new->startbit + MAPSIZE;

	if (prev) {
		new->next = prev->next;
		prev->next = new;
	} else {
		new->next = e->node;
		e->node = new;
	}

	return TRUE;
}


void ebitmap_destroy(ebitmap_t * e)
{
	ebitmap_node_t *n, *temp;


	if (!e)
		return;

	n = e->node;
	while (n) {
		temp = n;
		n = n->next;
		free(temp);
	}

	e->highbit = 0;
	e->node = 0;
	return;
}


int ebitmap_read(ebitmap_t * e, FILE * fp)
{
	ebitmap_node_t *n, *l;
	__u32 buf[32], mapsize, count, i;
	__u64 map;
	size_t items;


	ebitmap_init(e);

	items = fread(buf, sizeof(__u32), 3, fp);
	if (items != 3)
		return FALSE;
	mapsize = le32_to_cpu(buf[0]);
	e->highbit = le32_to_cpu(buf[1]);
	count = le32_to_cpu(buf[2]);

	if (mapsize != MAPSIZE) {
		printf("security: ebitmap: map size %d does not match my size %d (high bit was %d)\n", mapsize, MAPSIZE, e->highbit);
		return FALSE;
	}
	if (!e->highbit) {
		e->node = NULL;
		return TRUE;
	}
	if (e->highbit & (MAPSIZE - 1)) {
		printf("security: ebitmap: high bit (%d) is not a multiple of the map size (%d)\n", e->highbit, MAPSIZE);
		goto bad;
	}
	l = NULL;
	for (i = 0; i < count; i++) {
		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1) {
			printf("security: ebitmap: truncated map\n");
			goto bad;
		}
		n = (ebitmap_node_t *) malloc(sizeof(ebitmap_node_t));
		if (!n) {
			printf("security: ebitmap: out of memory\n");
			goto bad;
		}
		memset(n, 0, sizeof(ebitmap_node_t));

		n->startbit = le32_to_cpu(buf[0]);

		if (n->startbit & (MAPSIZE - 1)) {
			printf("security: ebitmap start bit (%d) is not a multiple of the map size (%d)\n", n->startbit, MAPSIZE);
			goto bad_free;
		}
		if (n->startbit > (e->highbit - MAPSIZE)) {
			printf("security: ebitmap start bit (%d) is beyond the end of the bitmap (%d)\n", n->startbit, (e->highbit - MAPSIZE));
			goto bad_free;
		}
		items = fread(&map, sizeof(__u64), 1, fp);
		if (items != 1) {
			printf("security: ebitmap: truncated map\n");
			goto bad_free;
		}
		n->map = le64_to_cpu(map);

		if (!n->map) {
			printf("security: ebitmap: null map in ebitmap (startbit %d)\n", n->startbit);
			goto bad_free;
		}
		if (l) {
			if (n->startbit <= l->startbit) {
				printf("security: ebitmap: start bit %d comes after start bit %d\n", n->startbit, l->startbit);
				goto bad_free;
			}
			l->next = n;
		} else
			e->node = n;

		l = n;
	}

	return TRUE;

      bad_free:
	free(n);
      bad:
	ebitmap_destroy(e);
	return FALSE;
}


#ifndef __KERNEL__
int ebitmap_write(ebitmap_t * e, FILE * fp)
{
	ebitmap_node_t *n;
	__u32 buf[32], bit, count;
	__u64 map;
	size_t items;

	buf[0] = cpu_to_le32(MAPSIZE);
	buf[1] = cpu_to_le32(e->highbit);

	count = 0;
	for (n = e->node; n; n = n->next)
		count++;
	buf[2] = cpu_to_le32(count);

	items = fwrite(buf, sizeof(__u32), 3, fp);
	if (items != 3)
		return FALSE;

	for (n = e->node; n; n = n->next) {
		bit = cpu_to_le32(n->startbit);
		items = fwrite(&bit, sizeof(__u32), 1, fp);
		if (items != 1)
			return FALSE;
		map = cpu_to_le64(n->map);
		items = fwrite(&map, sizeof(__u64), 1, fp);
		if (items != 1)
			return FALSE;

	}

	return TRUE;
}
#endif

/* FLASK */

