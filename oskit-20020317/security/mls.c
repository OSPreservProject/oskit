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
 * Implementation of the multi-level security (MLS) policy.
 */

#include "mls.h"

/*
 * Remove any permissions from `allowed' that are
 * denied by the MLS policy.
 */
void mls_compute_av(context_struct_t * scontext,
		    context_struct_t * tcontext,
		    class_datum_t * tclass,
		    access_vector_t * allowed)
{
	unsigned int rel[2];
	int l;

	for (l = 0; l < 2; l++)
		rel[l] = mls_level_relation(scontext->range.level[l],
					    tcontext->range.level[l]);

	if (rel[1] != MLS_RELATION_EQ) {
		if (rel[1] != MLS_RELATION_DOM)
			/* read(s,t) = (s.high >= t.high) = False */
			*allowed = (*allowed) & ~(tclass->mlsperms.read);
		if (rel[1] != MLS_RELATION_DOMBY)
			/* readby(s,t) = read(t,s) = False */
			*allowed = (*allowed) & ~(tclass->mlsperms.readby);
	}
	if ((rel[0] != MLS_RELATION_DOMBY && rel[0] != MLS_RELATION_EQ) ||
	    ((!mls_level_eq(tcontext->range.level[0],
			    tcontext->range.level[1])) &&
	     (rel[1] != MLS_RELATION_DOM && rel[1] != MLS_RELATION_EQ)))
		/*
		 * write(s,t) = ((s.low <= t.low = t.high) or (s.low
		 * <= t.low <= t.high <= s.high)) = False
		 */
		*allowed = (*allowed) & ~(tclass->mlsperms.write);

	if ((rel[0] != MLS_RELATION_DOM && rel[0] != MLS_RELATION_EQ) ||
	    ((!mls_level_eq(scontext->range.level[0],
			    scontext->range.level[1])) &&
	     (rel[1] != MLS_RELATION_DOMBY && rel[1] != MLS_RELATION_EQ)))
		/* writeby(s,t) = write(t,s) = False */
		*allowed = (*allowed) & ~(tclass->mlsperms.writeby);

}


/*
 * Return the length in bytes for the MLS fields of the
 * security context string representation of `context'.
 */
int mls_compute_context_len(context_struct_t * context)
{
	int i, l, len;


	len = 0;
	for (l = 0; l < 2; l++) {
		len += strlen(policydb.p_sens_val_to_name[context->range.level[l].sens - 1]) + 1;

		for (i = 1; i <= ebitmap_length(&context->range.level[l].cat); i++)
			if (ebitmap_get_bit(&context->range.level[l].cat, i - 1))
				len += strlen(policydb.p_cat_val_to_name[i - 1]) + 1;

		if (mls_level_relation(context->range.level[0], context->range.level[1]) == MLS_RELATION_EQ)
			break;
	}

	return len;
}


/*
 * Write the security context string representation of 
 * the MLS fields of `context' into the string `*scontext'.
 * Update `*scontext' to point to the end of the MLS fields.
 */
int mls_sid_to_context(context_struct_t * context,
		       char **scontext)
{
	char *scontextp;
	int i, l;


	scontextp = *scontext;

	for (l = 0; l < 2; l++) {
		strcpy(scontextp,
		       policydb.p_sens_val_to_name[context->range.level[l].sens - 1]);
		scontextp += strlen(policydb.p_sens_val_to_name[context->range.level[l].sens - 1]);
		*scontextp = ':';
		scontextp++;
		for (i = 1; i <= ebitmap_length(&context->range.level[l].cat); i++)
			if (ebitmap_get_bit(&context->range.level[l].cat, i - 1)) {
				strcpy(scontextp, policydb.p_cat_val_to_name[i - 1]);
				scontextp += strlen(policydb.p_cat_val_to_name[i - 1]);
				*scontextp = ',';
				scontextp++;
			}
		if (mls_level_relation(context->range.level[0], context->range.level[1]) != MLS_RELATION_EQ) {
			scontextp--;
			sprintf(scontextp, "-");
			scontextp++;

		} else {
			break;
		}
	}

	scontextp--;
	*scontextp = 0;

	*scontext = scontextp;
	return 0;
}


/*
 * Return TRUE if the MLS fields in the security context 
 * structure `c' are valid.  Return FALSE otherwise.
 */
int mls_context_isvalid(policydb_t *p, context_struct_t * c)
{
	unsigned int relation;
	level_datum_t *levdatum;
	user_datum_t *usrdatum;
	mls_range_list_t *rnode;
	int i, l;

	/*  
	 * MLS range validity checks: high must dominate low, low level must 
	 * be valid (category set <-> sensitivity check), and high level must 
	 * be valid (category set <-> sensitivity check)
	 */
	relation = mls_level_relation(c->range.level[1],
				      c->range.level[0]);
	if (!(relation & (MLS_RELATION_DOM | MLS_RELATION_EQ)))
		/* High does not dominate low. */
		return FALSE;

	for (l = 0; l < 2; l++) {
		if (!c->range.level[l].sens || c->range.level[l].sens > p->p_levels.nprim)
			return FALSE;
		levdatum = (level_datum_t *) hashtab_search(p->p_levels.table,
		p->p_sens_val_to_name[c->range.level[l].sens - 1]);
		if (!levdatum)
			return FALSE;

		for (i = 1; i <= ebitmap_length(&c->range.level[l].cat); i++) {
			if (ebitmap_get_bit(&c->range.level[l].cat, i - 1)) {
				if (i > p->p_cats.nprim)
					return FALSE;
				if (!ebitmap_get_bit(&levdatum->level->cat, i - 1))
					/*
					 * Category may not be associated with
					 * sensitivity in low level.
					 */
					return FALSE;
			}
		}
	}

	/*
	 * User must be authorized for the MLS range.
	 */
	if (!c->user || c->user > p->p_users.nprim)
		return FALSE;
	usrdatum = p->user_val_to_struct[c->user - 1];
	for (rnode = usrdatum->ranges; rnode; rnode = rnode->next) {
		if (mls_range_contains(rnode->range, c->range))
			break;
	}
	if (!rnode)
		/* user may not be associated with range */
		return FALSE;

	return TRUE;
}


/*
 * Set the MLS fields in the security context structure
 * `context' based on the string representation in
 * the string `*scontext'.  Update `*scontext' to
 * point to the end of the string representation of
 * the MLS fields.  
 *
 * This function modifies the string in place, inserting
 * NULL characters to terminate the MLS fields. 
 */
int mls_context_to_sid(char **scontext,
		       context_struct_t * context)
{

	char delim;
	char *scontextp, *p;
	level_datum_t *levdatum;
	cat_datum_t *catdatum;
	int l;

	/* Extract low sensitivity. */
	scontextp = p = *scontext;
	while (*p && *p != ':' && *p != '-')
		p++;

	delim = *p;
	if (delim != 0)
		*p++ = 0;

	for (l = 0; l < 2; l++) {
		levdatum = (level_datum_t *) hashtab_search(policydb.p_levels.table,
					      (hashtab_key_t) scontextp);

		if (!levdatum)
			return -EINVAL;

		context->range.level[l].sens = levdatum->level->sens;

		if (delim == ':') {
			/* Extract low category set. */
			while (1) {
				scontextp = p;
				while (*p && *p != ',' && *p != '-')
					p++;
				delim = *p;
				if (delim != 0)
					*p++ = 0;

				catdatum = (cat_datum_t *) hashtab_search(policydb.p_cats.table,
					      (hashtab_key_t) scontextp);

				if (!catdatum)
					return -EINVAL;

				if (!ebitmap_set_bit(&context->range.level[l].cat,
					      catdatum->value - 1, TRUE))
					return -ENOMEM;
				if (delim != ',')
					break;
			}
		}
		if (delim == '-') {
			/* Extract high sensitivity. */
			scontextp = p;
			while (*p && *p != ':')
				p++;

			delim = *p;
			if (delim != 0)
				*p++ = 0;
		} else
			break;
	}

	if (l == 0) {
		context->range.level[1].sens = context->range.level[0].sens;
		ebitmap_cpy(&context->range.level[1].cat, &context->range.level[0].cat);
	}
	*scontext = p;
	return 0;
}


/* 
 * Copies the MLS range from `scontext' into `newcontext'.
 */
int mls_member_sid(context_struct_t * newcontext,
		   context_struct_t * scontext)
{
	int l;

	/* Copy the MLS range from the source context */
	for (l = 0; l < 2; l++) {
		newcontext->range.level[l].sens = scontext->range.level[l].sens;
		if (!ebitmap_cpy(&newcontext->range.level[l].cat, &scontext->range.level[l].cat))
			return -ENOMEM;
	}

	return 0;
}


/* 
 * Convert the MLS fields in the security context
 * structure `c' from the values specified in the
 * policy `oldp' to the values specified in the policy `newp'.
 */
int mls_convert_context(policydb_t * oldp,
			policydb_t * newp,
			context_struct_t * c)
{
	level_datum_t *levdatum;
	cat_datum_t *catdatum;
	ebitmap_t bitmap;
	int l, i;

	for (l = 0; l < 2; l++) {
		levdatum = (level_datum_t *) hashtab_search(
						    newp->p_levels.table,
		   oldp->p_sens_val_to_name[c->range.level[l].sens - 1]);

		if (!levdatum)
			return -EINVAL;
		c->range.level[l].sens = levdatum->level->sens;

		ebitmap_init(&bitmap);
		for (i = 1; i <= ebitmap_length(&c->range.level[l].cat); i++) {
			if (ebitmap_get_bit(&c->range.level[l].cat, i - 1)) {
				catdatum = (cat_datum_t *) hashtab_search(newp->p_cats.table,
					 oldp->p_cat_val_to_name[i - 1]);
				if (!catdatum)
					return -EINVAL;
				if (!ebitmap_set_bit(&bitmap, catdatum->value - 1, TRUE))
					return -ENOMEM;
			}
		}
		ebitmap_destroy(&c->range.level[l].cat);
		c->range.level[l].cat = bitmap;
	}

	return 0;
}


/*
 * Read a MLS level structure from a policydb binary 
 * representation file.
 */
mls_level_t *mls_read_level(FILE * fp)
{
	mls_level_t *l;
	__u32 sens;
	int items;

	l = malloc(sizeof(mls_level_t));
	if (!l) {
		printf("security: mls: out of memory\n");
		return NULL;
	}
	memset(l, 0, sizeof(mls_level_t));

	items = fread(&sens, sizeof(__u32), 1, fp);
	if (items != 1) {
		printf("security: mls: truncated level\n");
		goto bad;
	}
	l->sens = cpu_to_le32(sens);

	if (!ebitmap_read(&l->cat, fp)) {
		printf("security: mls:  error reading level categories\n");
		goto bad;
	}
	return l;

      bad:
	free(l);
	return NULL;
}


/*
 * Read a MLS range structure from a policydb binary 
 * representation file.
 */
int mls_read_range(mls_range_t * r,
		   FILE * fp)
{
	__u32 buf[3];
	int items, items2;

	items = fread(buf, sizeof(__u32), 1, fp);
	if (items != 1)
		return -1;

	items2 = le32_to_cpu(buf[0]);
	if (items2 > (sizeof(buf) / sizeof(__u32))) {
		printf("security: mls:  range too large\n");
		return -1;
	}
	items = fread(buf, sizeof(__u32), items2, fp);
	if (items != items2) {
		printf("security: mls:  truncated range\n");
		return -1;
	}
	r->level[0].sens = le32_to_cpu(buf[0]);
	if (items > 1) {
		r->level[1].sens = le32_to_cpu(buf[1]);
	} else {
		r->level[1].sens = r->level[0].sens;
	}

	if (!ebitmap_read(&r->level[0].cat, fp)) {
		printf("security: mls:  error reading low categories\n");
		return -1;
	}
	if (items > 1) {
		if (!ebitmap_read(&r->level[1].cat, fp)) {
			printf("security: mls:  error reading high categories\n");
			goto bad_high;
		}
	} else {
		if (!ebitmap_cpy(&r->level[1].cat, &r->level[0].cat)) {
			printf("security: mls:  out of memory\n");
			goto bad_high;
		}
	}

	return 0;

      bad_high:
	ebitmap_destroy(&r->level[0].cat);
	return -1;
}


/*
 * Read a MLS perms structure from a policydb binary 
 * representation file.
 */
int mls_read_perms(mls_perms_t * p,
		   FILE * fp)
{
	__u32 buf[32];
	int items;

	items = fread(buf, sizeof(__u32), 4, fp);
	if (items != 4) {
		printf("security: mls:  truncated mls permissions\n");
		return -1;
	}
	p->read = le32_to_cpu(buf[0]);
	p->readby = le32_to_cpu(buf[1]);
	p->write = le32_to_cpu(buf[2]);
	p->writeby = le32_to_cpu(buf[3]);
	return 0;
}


#ifndef __KERNEL__
/*
 * Write a MLS level structure to a policydb binary 
 * representation file.
 */
int mls_write_level(mls_level_t * l,
		    FILE * fp)
{
	__u32 sens;
	int items;

	sens = cpu_to_le32(l->sens);
	items = fwrite(&sens, sizeof(__u32), 1, fp);
	if (items != 1)
		return -1;

	if (!ebitmap_write(&l->cat, fp))
		return -1;

	return 0;
}


/*
 * Write a MLS range structure to a policydb binary 
 * representation file.
 */
int mls_write_range(mls_range_t * r,
		    FILE * fp)
{
	__u32 buf[3];
	int items, items2;
	int rel;

	rel = mls_level_relation(r->level[1], r->level[0]);

	items = 1;		/* item 0 is used for the item count */
	buf[items++] = cpu_to_le32(r->level[0].sens);
	if (rel != MLS_RELATION_EQ)
		buf[items++] = cpu_to_le32(r->level[1].sens);
	buf[0] = cpu_to_le32(items - 1);

	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items2 != items)
		return -1;

	if (!ebitmap_write(&r->level[0].cat, fp))
		return -1;
	if (rel != MLS_RELATION_EQ)
		if (!ebitmap_write(&r->level[1].cat, fp))
			return -1;

	return 0;
}


/*
 * Write a MLS perms structure to a policydb binary 
 * representation file.
 */
int mls_write_perms(mls_perms_t * p,
		    FILE * fp)
{
	__u32 buf[32];
	int items, items2;

	items = 0;
	buf[items++] = cpu_to_le32(p->read);
	buf[items++] = cpu_to_le32(p->readby);
	buf[items++] = cpu_to_le32(p->write);
	buf[items++] = cpu_to_le32(p->writeby);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items2 != items)
		return -1;

	return 0;
}

#endif

/* FLASK */
