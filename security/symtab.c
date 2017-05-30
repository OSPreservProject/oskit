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
 * Implementation of the symbol table type.
 */

#include "symtab.h"

#define SYMTAB_SIZE 23

static unsigned int symhash(hashtab_key_t key)
{
	char *p, *keyp;
	unsigned int size;
	unsigned int h;


	h = 0;
	keyp = (char *) key;
	size = strlen(keyp);
	for (p = keyp; (p - keyp) < size; p++)
		h = (h << 4) ^ (*p);
	return h % SYMTAB_SIZE;
}


static int symcmp(hashtab_key_t key1, hashtab_key_t key2)
{
	char *keyp1, *keyp2;


	keyp1 = (char *) key1;
	keyp2 = (char *) key2;
	return strcmp(keyp1, keyp2);
}


int symtab_init(symtab_t * s)
{
	s->table = hashtab_create(symhash, symcmp, SYMTAB_SIZE);
	if (!s->table)
		return -1;
	s->nprim = 0;
	return 0;
}


/* FLASK */
