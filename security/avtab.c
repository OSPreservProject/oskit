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
 * Implementation of the access vector table type.
 */

#include "avtab.h"
#include "policydb.h"

#define AVTAB_HASH(keyp) \
((keyp->target_type + (keyp->source_type << 4) + \
(keyp->target_class << 8)) % AVTAB_SIZE)

#define AVTAB_EQ(key,keyp) \
((key.source_type == keyp->source_type) && \
 (key.target_class == keyp->target_class) && \
 (key.target_type == keyp->target_type))


int avtab_insert(avtab_t * h, avtab_key_t * key, avtab_datum_t * datum)
{
	int hvalue;
	avtab_ptr_t cur, newnode;


	if (!h)
		return -ENOMEM;

	hvalue = AVTAB_HASH(key);
	cur = h->htable[hvalue];
	while (cur != NULL && !AVTAB_EQ(cur->key, key))
		cur = cur->next;

	if (cur != NULL)
		return -EEXIST;

	newnode = (avtab_ptr_t) malloc(sizeof(struct avtab_node));
	if (newnode == NULL)
		return -ENOMEM;
	memset(newnode, 0, sizeof(struct avtab_node));
	newnode->key = *key;
	newnode->datum = *datum;
	newnode->next = h->htable[hvalue];
	h->htable[hvalue] = newnode;

	h->nel++;
	return 0;
}


avtab_datum_t *
 avtab_search(avtab_t * h, avtab_key_t * key)
{
	int hvalue;
	avtab_ptr_t cur;


	if (!h)
		return NULL;

	hvalue = AVTAB_HASH(key);
	cur = h->htable[hvalue];
	while (cur != NULL && !AVTAB_EQ(cur->key, key))
		cur = cur->next;

	if (cur == NULL)
		return NULL;

	return &cur->datum;
}


void avtab_destroy(avtab_t * h)
{
	int i;
	avtab_ptr_t cur, temp;


	if (!h)
		return;

	for (i = 0; i < AVTAB_SIZE; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			temp = cur;
			cur = cur->next;
			free(temp);
		}
		h->htable[i] = NULL;
	}
}


int avtab_map(avtab_t * h,
	      int (*apply) (avtab_key_t * k,
			    avtab_datum_t * d,
			    void *args),
	      void *args)
{
	int i, ret;
	avtab_ptr_t cur;


	if (!h)
		return 0;

	for (i = 0; i < AVTAB_SIZE; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			ret = apply(&cur->key, &cur->datum, args);
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return 0;
}


int avtab_init(avtab_t * h)
{
	int i;


	for (i = 0; i < AVTAB_SIZE; i++)
		h->htable[i] = (avtab_ptr_t) NULL;
	h->nel = 0;
	return 0;
}


void avtab_hash_eval(avtab_t * h, char *progname, char *table)
{
	int i, chain_len, slots_used, max_chain_len;
	avtab_ptr_t cur;


	slots_used = 0;
	max_chain_len = 0;
	for (i = 0; i < AVTAB_SIZE; i++) {
		cur = h->htable[i];
		if (cur) {
			slots_used++;
			chain_len = 0;
			while (cur) {
				chain_len++;
				cur = cur->next;
			}

			if (chain_len > max_chain_len)
				max_chain_len = chain_len;
		}
	}

	printf("%s:  %s:  %d nodes and %d slots used, longest chain length %d\n",
	       progname, table, h->nel, slots_used, max_chain_len);
}


int avtab_read(avtab_t * a, FILE * fp, __u32 config)
{
	int i, rc;
	avtab_key_t avkey;
	avtab_datum_t avdatum;
	__u32 buf[32];
	__u32 nel;
	size_t items, items2;


	if (avtab_init(a)) {
		printf("security: avtab: out of memory\n");
		return -1;
	}
	items = fread(&nel, sizeof(__u32), 1, fp);
	if (items != 1) {
		printf("security: avtab: truncated table\n");
		goto bad;
	}
	nel = le32_to_cpu(nel);
	if (!nel) {
		printf("security: avtab: table is empty\n");
		goto bad;
	}
	for (i = 0; i < nel; i++) {
		memset(&avkey, 0, sizeof(avtab_key_t));
		memset(&avdatum, 0, sizeof(avtab_datum_t));

		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1) {
			printf("security: avtab: truncated entry\n");
			goto bad;
		}
		items2 = le32_to_cpu(buf[0]);
		if (items2 > (sizeof(buf) / sizeof(__u32))) {
			printf("security: avtab: entry too large\n");
			goto bad;
		}
		items = fread(buf, sizeof(__u32), items2, fp);
		if (items != items2) {
			printf("security: avtab: truncated entry\n");
			goto bad;
		}
		items = 0;
		avkey.source_type = le32_to_cpu(buf[items++]);
		avkey.target_type = le32_to_cpu(buf[items++]);
		avkey.target_class = le32_to_cpu(buf[items++]);
		avdatum.specified = le32_to_cpu(buf[items++]);
		if (avdatum.specified & AVTAB_ALLOWED)
			avdatum.allowed = le32_to_cpu(buf[items++]);
		if (avdatum.specified & AVTAB_TRANSITION)
			avdatum.trans_type = le32_to_cpu(buf[items++]);
		if (config & POLICYDB_CONFIG_AUDIT) {
			if (avdatum.specified & AVTAB_AUDITALLOW) {
#ifdef CONFIG_FLASK_AUDIT
				avdatum.auditallow = le32_to_cpu(buf[items++]);
#else
				items++;
#endif
			}
			if (avdatum.specified & AVTAB_AUDITDENY) {
#ifdef CONFIG_FLASK_AUDIT
				avdatum.auditdeny = le32_to_cpu(buf[items++]);
#else
				items++;
#endif
			}
		}
		if (config & POLICYDB_CONFIG_NOTIFY) {
			if (avdatum.specified & AVTAB_NOTIFY) {
#ifdef CONFIG_FLASK_AUDIT
				avdatum.notify = le32_to_cpu(buf[items++]);
#else
				items++;
#endif
			}
		}
		if (items != items2) {
			printf("security: avtab: entry only had %d items, expected %d\n", items2, items);
			goto bad;
		}
		rc = avtab_insert(a, &avkey, &avdatum);
		if (rc) {
			if (rc == -ENOMEM)
				printf("security: avtab: out of memory\n");
			if (rc == -EEXIST)
				printf("security: avtab: duplicate entry\n");
			goto bad;
		}
	}

	return 0;

      bad:
	avtab_destroy(a);
	return -1;
}


#ifndef __KERNEL__
int avtab_write(avtab_t * a, FILE * fp)
{
	int i;
	avtab_ptr_t cur;
	__u32 buf[32];
	__u32 nel;
	size_t items, items2;


	nel = cpu_to_le32(a->nel);
	items = fwrite(&nel, sizeof(__u32), 1, fp);
	if (items != 1)
		return -1;

	for (i = 0; i < AVTAB_SIZE; i++) {
		for (cur = a->htable[i]; cur; cur = cur->next) {
			items = 1;	/* item 0 is used for the item count */
			buf[items++] = cpu_to_le32(cur->key.source_type);
			buf[items++] = cpu_to_le32(cur->key.target_type);
			buf[items++] = cpu_to_le32(cur->key.target_class);
			buf[items++] = cpu_to_le32(cur->datum.specified);
			if (cur->datum.specified & AVTAB_ALLOWED)
				buf[items++] = cpu_to_le32(cur->datum.allowed);
			if (cur->datum.specified & AVTAB_TRANSITION)
				buf[items++] = cpu_to_le32(cur->datum.trans_type);
#ifdef CONFIG_FLASK_AUDIT
			if (cur->datum.specified & AVTAB_AUDITALLOW)
				buf[items++] = cpu_to_le32(cur->datum.auditallow);
			if (cur->datum.specified & AVTAB_AUDITDENY)
				buf[items++] = cpu_to_le32(cur->datum.auditdeny);
#endif
#ifdef CONFIG_FLASK_NOTIFY
			if (cur->datum.specified & AVTAB_NOTIFY)
				buf[items++] = cpu_to_le32(cur->datum.notify);
#endif
			buf[0] = cpu_to_le32(items - 1);

			items2 = fwrite(buf, sizeof(__u32), items, fp);
			if (items != items2)
				return -1;
		}
	}

	return 0;
}
#endif
