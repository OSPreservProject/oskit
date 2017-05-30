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
 * Implementation of the policy database.
 */

#include "policydb.h"
#include "services.h"
#ifdef CONFIG_FLASK_MLS
#include "mls.h"
#endif

/*
 * Initialize a policy database structure.
 */
int policydb_init(policydb_t * p)
{
	int i;

	memset(p, 0, sizeof(policydb_t));

	for (i = 0; i < SYM_NUM; i++) {
		if (symtab_init(&p->symtab[i]))
			return -1;
	}

	if (avtab_init(&p->te_avtab))
		return -1;

	return 0;
}


/*
 * The following *_index functions are used to
 * define the val_to_name and val_to_struct arrays
 * in a policy database structure.  The val_to_name
 * arrays are used when converting security context
 * structures into string representations.  The
 * val_to_struct arrays are used when the attributes
 * of a class, role, or user are needed.
 */

static int common_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	common_datum_t *comdatum;


	comdatum = (common_datum_t *) datum;
	p = (policydb_t *) datap;

	p->p_common_val_to_name[comdatum->value - 1] = (char *) key;

	return 0;
}


static int class_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	class_datum_t *cladatum;


	cladatum = (class_datum_t *) datum;
	p = (policydb_t *) datap;

	p->p_class_val_to_name[cladatum->value - 1] = (char *) key;
	p->class_val_to_struct[cladatum->value - 1] = cladatum;

	return 0;
}


static int role_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	role_datum_t *role;


	role = (role_datum_t *) datum;
	p = (policydb_t *) datap;

	p->p_role_val_to_name[role->value - 1] = (char *) key;
	p->role_val_to_struct[role->value - 1] = role;

	return 0;
}


static int type_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	type_datum_t *typdatum;


	typdatum = (type_datum_t *) datum;
	p = (policydb_t *) datap;

	if (!typdatum->isalias)
		p->p_type_val_to_name[typdatum->value - 1] = (char *) key;

	return 0;
}


#ifdef CONFIG_FLASK_MLS
static int sens_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	level_datum_t *levdatum;


	levdatum = (level_datum_t *) datum;
	p = (policydb_t *) datap;

	if (!levdatum->isalias)
		p->p_sens_val_to_name[levdatum->level->sens - 1] = (char *) key;

	return 0;
}


static int cat_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	cat_datum_t *catdatum;


	catdatum = (cat_datum_t *) datum;
	p = (policydb_t *) datap;


	if (!catdatum->isalias)
		p->p_cat_val_to_name[catdatum->value - 1] = (char *) key;

	return 0;
}
#endif

static int user_index(hashtab_key_t key, hashtab_datum_t datum, void *datap)
{
	policydb_t *p;
	user_datum_t *usrdatum;


	usrdatum = (user_datum_t *) datum;
	p = (policydb_t *) datap;

	p->p_user_val_to_name[usrdatum->value - 1] = (char *) key;
	p->user_val_to_struct[usrdatum->value - 1] = usrdatum;

	return 0;
}

static int (*index_f[SYM_NUM]) (hashtab_key_t key, hashtab_datum_t datum, void *datap) =
{
	common_index,
	    class_index,
	    role_index,
	    type_index,
	    user_index
#ifdef CONFIG_FLASK_MLS
	    ,sens_index,
	    cat_index
#endif
};


/*
 * Define the common val_to_name array and the class
 * val_to_name and val_to_struct arrays in a policy
 * database structure.  
 */
int policydb_index_classes(policydb_t * p)
{
	p->p_common_val_to_name = (char **)
	    malloc(p->p_commons.nprim * sizeof(char *));
	if (!p->p_common_val_to_name)
		return -1;

	if (hashtab_map(p->p_commons.table, common_index, p))
		return -1;

	p->class_val_to_struct = (class_datum_t **)
	    malloc(p->p_classes.nprim * sizeof(class_datum_t *));
	if (!p->class_val_to_struct)
		return -1;

	p->p_class_val_to_name = (char **)
	    malloc(p->p_classes.nprim * sizeof(char *));
	if (!p->p_class_val_to_name)
		return -1;

	if (hashtab_map(p->p_classes.table, class_index, p))
		return -1;
	return 0;
}


/*
 * Define the other val_to_name and val_to_struct arrays
 * in a policy database structure.  
 */
int policydb_index_others(policydb_t * p)
{
	int i;


	printf("security:  %d users, %d roles, %d types",
	       p->p_users.nprim, p->p_roles.nprim, p->p_types.nprim);
#ifdef CONFIG_FLASK_MLS
	printf(", %d levels", p->nlevels);
#endif
	printf("\n");

	printf("security:  %d classes, %d access rules\n",
	       p->p_classes.nprim, p->te_avtab.nel);

	avtab_hash_eval(&p->te_avtab, "security", "access rules");

	p->role_val_to_struct = (role_datum_t **)
	    malloc(p->p_roles.nprim * sizeof(role_datum_t *));
	if (!p->role_val_to_struct)
		return -1;

	p->user_val_to_struct = (user_datum_t **)
	    malloc(p->p_users.nprim * sizeof(user_datum_t *));
	if (!p->user_val_to_struct)
		return -1;

	for (i = SYM_ROLES; i < SYM_NUM; i++) {
		p->sym_val_to_name[i] = (char **)
		    malloc(p->symtab[i].nprim * sizeof(char *));
		if (!p->sym_val_to_name[i])
			return -1;
		if (hashtab_map(p->symtab[i].table, index_f[i], p))
			return -1;
	}

	return 0;
}


/*
 * The following *_destroy functions are used to
 * free any memory allocated for each kind of
 * symbol data in the policy database.
 */

static int perm_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	if (key)
		free(key);
	free(datum);
	return 0;
}


static int common_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	common_datum_t *comdatum;

	if (key)
		free(key);
	comdatum = (common_datum_t *) datum;
	hashtab_map(comdatum->permissions.table, perm_destroy, 0);
	hashtab_destroy(comdatum->permissions.table);
	free(datum);
	return 0;
}


int constraint_expr_destroy(constraint_expr_t * expr)
{
	expr->count--;
	if (expr->count == 0) {
		ebitmap_destroy(&expr->bitmap);
		if (expr->left)
			constraint_expr_destroy(expr->left);
		if (expr->right)
			constraint_expr_destroy(expr->right);
		free(expr);
	}
	return 0;
}


static int class_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	class_datum_t *cladatum;
	constraint_node_t *constraint, *ctemp;

	if (key)
		free(key);
	cladatum = (class_datum_t *) datum;
	hashtab_map(cladatum->permissions.table, perm_destroy, 0);
	hashtab_destroy(cladatum->permissions.table);
	constraint = cladatum->constraints;
	while (constraint) {
		constraint_expr_destroy(constraint->expr);
		ctemp = constraint;
		constraint = constraint->next;
		free(ctemp);
	}
	if (cladatum->comkey)
		free(cladatum->comkey);
	free(datum);
	return 0;
}

static int role_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	role_datum_t *role;

	if (key)
		free(key);
	role = (role_datum_t *) datum;
	ebitmap_destroy(&role->dominates);
	ebitmap_destroy(&role->types);
	free(datum);
	return 0;
}

static int type_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	if (key)
		free(key);
	free(datum);
	return 0;
}

#ifdef CONFIG_FLASK_MLS
static int sens_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	level_datum_t *levdatum;

	if (key)
		free(key);
	levdatum = (level_datum_t *) datum;
	if (!levdatum->isalias) {
		ebitmap_destroy(&levdatum->level->cat);
		free(levdatum->level);
	}
	free(datum);
	return 0;
}

static int cat_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	if (key)
		free(key);
	free(datum);
	return 0;
}
#endif

static int user_destroy(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	user_datum_t *usrdatum;
#ifdef CONFIG_FLASK_MLS
	mls_range_list_t *rnode, *rtmp;
#endif


	if (key)
		free(key);
	usrdatum = (user_datum_t *) datum;
	ebitmap_destroy(&usrdatum->roles);
#ifdef CONFIG_FLASK_MLS
	rnode = usrdatum->ranges;
	while (rnode) {
		rtmp = rnode;
		rnode = rnode->next;
		ebitmap_destroy(&rtmp->range.level[0].cat);
		ebitmap_destroy(&rtmp->range.level[1].cat);
		free(rtmp);
	}
#endif
	free(datum);
	return 0;
}


static int (*destroy_f[SYM_NUM]) (hashtab_key_t key, hashtab_datum_t datum, void *datap) =
{
	common_destroy,
	    class_destroy,
	    role_destroy,
	    type_destroy,
	    user_destroy
#ifdef CONFIG_FLASK_MLS
	    ,sens_destroy,
	    cat_destroy
#endif
};


/*
 * Free any memory allocated by a policy database structure.
 */
void policydb_destroy(policydb_t * p)
{
	ocontext_t *c, *ctmp;
	int i;

	for (i = 0; i < SYM_NUM; i++) {
		hashtab_map(p->symtab[i].table, destroy_f[i], 0);
		hashtab_destroy(p->symtab[i].table);
	}

	for (i = 0; i < SYM_NUM; i++) {
		if (p->sym_val_to_name[i])
			free(p->sym_val_to_name[i]);
	}

	if (p->class_val_to_struct)
		free(p->class_val_to_struct);
	if (p->role_val_to_struct)
		free(p->role_val_to_struct);
	if (p->user_val_to_struct)
		free(p->user_val_to_struct);

	for (i = 0; i < TED_NUM; i++)
		ebitmap_destroy(&p->te_def[i].exclude);

	avtab_destroy(&p->te_avtab);

	for (i = 0; i < OCON_NUM; i++) {
		c = p->ocontexts[i];
		while (c) {
			ctmp = c;
			c = c->next;
			context_destroy(&ctmp->context[0]);
			context_destroy(&ctmp->context[1]);
			if (i == OCON_ISID || i == OCON_FS || i == OCON_NETIF)
				free(ctmp->u.name);
			free(ctmp);
		}
	}

	return;
}


/*
 * Load the initial SIDs specified in a policy database
 * structure into a SID table.
 */
int policydb_load_isids(policydb_t *p, sidtab_t *s) 
{
	ocontext_t *head, *c;

	memset(s, 0, sizeof(sidtab));

	head = p->ocontexts[OCON_ISID];
	for (c = head; c; c = c->next) {
		if (!c->context[0].user) {
			printf("security:  SID %s was never defined.\n", 
			       c->u.name);
			return -1;
		}
		if (sidtab_insert(s, c->sid[0], &c->context[0])) {
			printf("security:  unable to load initial SID %s.\n", 
			       c->u.name);
			return -1;
		}
	}

	return 0;
}


/*
 * Return TRUE if the fields in the security context 
 * structure `c' are valid.  Return FALSE otherwise.
 */
int policydb_context_isvalid(policydb_t *p, context_struct_t *c)
{
	role_datum_t *role;
	user_datum_t *usrdatum;


	/*
	 * Role must be authorized for the type.
	 */
	if (!c->role || c->role > p->p_roles.nprim)
		return FALSE;
	role = p->role_val_to_struct[c->role - 1];
	if (!ebitmap_get_bit(&role->types,
			     c->type - 1))
		/* role may not be associated with type */
		return FALSE;

	/*
	 * User must be authorized for the role.
	 */
	if (!c->user || c->user > p->p_users.nprim)
		return FALSE;
	usrdatum = p->user_val_to_struct[c->user - 1];
	if (!usrdatum)
		return FALSE;

	if (!ebitmap_get_bit(&usrdatum->roles,
			     c->role - 1))
		/* user may not be associated with role */
		return FALSE;

#ifdef CONFIG_FLASK_MLS
	if (!mls_context_isvalid(p, c))
		return FALSE;
#endif

	return TRUE;
}


/*
 * Read and validate a security context structure
 * from a policydb binary representation file.
 */
static int context_read_and_validate(context_struct_t * c,
					policydb_t * p,
					FILE * fp)
{
	__u32 buf[32];
	size_t items;

	items = fread(buf, sizeof(__u32), 3, fp);
	if (items != 3) {
		printf("security: context truncated\n");
		return -1;
	}
	c->user = le32_to_cpu(buf[0]);
	c->role = le32_to_cpu(buf[1]);
	c->type = le32_to_cpu(buf[2]);
#ifdef CONFIG_FLASK_MLS
	if (mls_read_range(&c->range, fp)) {
		printf("security: error reading MLS range of context\n");
		return -1;
	}
#endif

	if (!policydb_context_isvalid(p, c)) {
		printf("security:  invalid security context\n");
		context_destroy(c);
		return -1;
	}
	return 0;
}


/*
 * The following *_read functions are used to
 * read the symbol data from a policy database
 * binary representation file.
 */

static int perm_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	perm_datum_t *perdatum;
	__u32 buf[32], len;
	int items, items2;

	perdatum = malloc(sizeof(perm_datum_t));
	if (!perdatum)
		return -1;
	memset(perdatum, 0, sizeof(perm_datum_t));

#ifdef CONFIG_FLASK_MLS
	items = 3;
#else
	items = 2;
#endif
	items2 = fread(buf, sizeof(__u32), items, fp);
	if (items != items2)
		goto bad;

	len = le32_to_cpu(buf[0]);
	perdatum->value = le32_to_cpu(buf[1]);
#ifdef CONFIG_FLASK_MLS
	perdatum->base_perms = le32_to_cpu(buf[2]);
#endif

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (hashtab_insert(h, key, perdatum))
		goto bad;

	return 0;

      bad:
	perm_destroy(key, perdatum, NULL);
	return -1;
}


static int common_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	common_datum_t *comdatum;
	__u32 buf[32], len, nel;
	int items, i;

	comdatum = malloc(sizeof(common_datum_t));
	if (!comdatum)
		return -1;
	memset(comdatum, 0, sizeof(common_datum_t));

	items = fread(buf, sizeof(__u32), 4, fp);
	if (items != 4)
		goto bad;

	len = le32_to_cpu(buf[0]);
	comdatum->value = le32_to_cpu(buf[1]);

	if (symtab_init(&comdatum->permissions))
		goto bad;
	comdatum->permissions.nprim = le32_to_cpu(buf[2]);
	nel = le32_to_cpu(buf[3]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	for (i = 0; i < nel; i++) {
		if (perm_read(p, comdatum->permissions.table, fp))
			goto bad;
	}

	if (hashtab_insert(h, key, comdatum))
		goto bad;

	return 0;

      bad:
	common_destroy(key, comdatum, NULL);
	return -1;
}


static constraint_expr_t *
 constraint_expr_read(FILE * fp)
{
	constraint_expr_t *expr;
	__u32 buf[32];
	int items;

	expr = malloc(sizeof(constraint_expr_t));
	if (!expr)
		return NULL;
	memset(expr, 0, sizeof(constraint_expr_t));

	items = fread(buf, sizeof(__u32), 1, fp);
	if (items != 1)
		goto bad;

	expr->expr_type = le32_to_cpu(buf[0]);
	expr->count = 1;
	items = 0;

	if (expr->expr_type == CONSTRAINT_EXPR_TYPE_ROLE_RELATION) {
		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			goto bad;
		expr->relation = le32_to_cpu(buf[0]);
	}
	switch (expr->expr_type) {
	case CONSTRAINT_EXPR_TYPE_TYPE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_TYPE_TARGET:
	case CONSTRAINT_EXPR_TYPE_ROLE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_ROLE_TARGET:
		if (!ebitmap_read(&expr->bitmap, fp))
			goto bad;
		break;
	case CONSTRAINT_EXPR_TYPE_AND:
	case CONSTRAINT_EXPR_TYPE_OR:
		expr->left = constraint_expr_read(fp);
		if (!expr->left)
			goto bad;
		expr->right = constraint_expr_read(fp);
		if (!expr->right)
			goto bad;
		break;
	case CONSTRAINT_EXPR_TYPE_NOT:
		expr->left = constraint_expr_read(fp);
		if (!expr->left)
			goto bad;
		break;
	}

	return expr;

      bad:
	constraint_expr_destroy(expr);
	return NULL;
}


static int class_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	class_datum_t *cladatum;
	constraint_node_t *c, *l;
	__u32 buf[32], len, len2, ncons, nel;
	int items, i;

	cladatum = (class_datum_t *) malloc(sizeof(class_datum_t));
	if (!cladatum)
		return -1;
	memset(cladatum, 0, sizeof(class_datum_t));

	items = fread(buf, sizeof(__u32), 6, fp);
	if (items != 6)
		goto bad;

	len = le32_to_cpu(buf[0]);
	len2 = le32_to_cpu(buf[1]);
	cladatum->value = le32_to_cpu(buf[2]);

	if (symtab_init(&cladatum->permissions))
		goto bad;
	cladatum->permissions.nprim = le32_to_cpu(buf[3]);
	nel = le32_to_cpu(buf[4]);

	ncons = le32_to_cpu(buf[5]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (len2) {
		cladatum->comkey = malloc(len2 + 1);
		if (!cladatum->comkey)
			goto bad;
		items = fread(cladatum->comkey, 1, len2, fp);
		if (items != len2)
			goto bad;
		cladatum->comkey[len2] = 0;

		cladatum->comdatum = hashtab_search(p->p_commons.table,
						    cladatum->comkey);
		if (!cladatum->comdatum) {
			printf("security:  unknown common %s\n", cladatum->comkey);
			goto bad;
		}
	}
	for (i = 0; i < nel; i++) {
		if (perm_read(p, cladatum->permissions.table, fp))
			goto bad;
	}

	l = NULL;
	for (i = 0; i < ncons; i++) {
		c = malloc(sizeof(constraint_node_t));
		if (!c)
			goto bad;
		memset(c, 0, sizeof(constraint_node_t));
		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			goto bad;
		c->permissions = le32_to_cpu(buf[0]);
		c->expr = constraint_expr_read(fp);
		if (!c->expr)
			goto bad;
		if (l) {
			l->next = c;
		} else {
			cladatum->constraints = c;
		}
		l = c;

	}

#ifdef CONFIG_FLASK_MLS
	if (mls_read_perms(&cladatum->mlsperms, fp))
		goto bad;
#endif

	if (hashtab_insert(h, key, cladatum))
		goto bad;

	return 0;

      bad:
	class_destroy(key, cladatum, NULL);
	return -1;
}


static int role_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	role_datum_t *role;
	__u32 buf[32], len;
	int items;

	role = malloc(sizeof(role_datum_t));
	if (!role)
		return -1;
	memset(role, 0, sizeof(role_datum_t));

	items = fread(buf, sizeof(__u32), 2, fp);
	if (items != 2)
		goto bad;

	len = le32_to_cpu(buf[0]);
	role->value = le32_to_cpu(buf[1]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (!ebitmap_read(&role->dominates, fp))
		goto bad;

	if (!ebitmap_read(&role->types, fp))
		goto bad;

	if (hashtab_insert(h, key, role))
		goto bad;

	return 0;

      bad:
	role_destroy(key, role, NULL);
	return -1;
}


static int type_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	type_datum_t *typdatum;
	__u32 buf[32], len;
	int items;

	typdatum = malloc(sizeof(type_datum_t));
	if (!typdatum)
		return -1;
	memset(typdatum, 0, sizeof(type_datum_t));

	items = fread(buf, sizeof(__u32), 3, fp);
	if (items != 3)
		goto bad;

	len = le32_to_cpu(buf[0]);
	typdatum->value = le32_to_cpu(buf[1]);
	typdatum->isalias = le32_to_cpu(buf[2]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (hashtab_insert(h, key, typdatum))
		goto bad;

	return 0;

      bad:
	type_destroy(key, typdatum, NULL);
	return -1;
}

#ifdef CONFIG_FLASK_MLS
static int sens_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	level_datum_t *levdatum;
	__u32 buf[32], len;
	int items;

	levdatum = malloc(sizeof(level_datum_t));
	if (!levdatum)
		return -1;
	memset(levdatum, 0, sizeof(level_datum_t));

	items = fread(buf, sizeof(__u32), 2, fp);
	if (items != 2)
		goto bad;

	len = le32_to_cpu(buf[0]);
	levdatum->isalias = le32_to_cpu(buf[1]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	levdatum->level = mls_read_level(fp);
	if (!levdatum->level)
		goto bad;

	if (hashtab_insert(h, key, levdatum))
		goto bad;

	return 0;

      bad:
	sens_destroy(key, levdatum, NULL);
	return -1;
}


static int cat_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	cat_datum_t *catdatum;
	__u32 buf[32], len;
	int items;

	catdatum = malloc(sizeof(cat_datum_t));
	if (!catdatum)
		return -1;
	memset(catdatum, 0, sizeof(cat_datum_t));

	items = fread(buf, sizeof(__u32), 3, fp);
	if (items != 3)
		goto bad;

	len = le32_to_cpu(buf[0]);
	catdatum->value = le32_to_cpu(buf[1]);
	catdatum->isalias = le32_to_cpu(buf[2]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (hashtab_insert(h, key, catdatum))
		goto bad;

	return 0;

      bad:
	cat_destroy(key, catdatum, NULL);
	return -1;
}
#endif


static int user_read(policydb_t * p, hashtab_t h, FILE * fp)
{
	char *key = 0;
	user_datum_t *usrdatum;
#ifdef CONFIG_FLASK_MLS
	mls_range_list_t *r, *l;
	__u32 nel, i;
#endif
	__u32 buf[32], len;
	int items;


	usrdatum = malloc(sizeof(user_datum_t));
	if (!usrdatum)
		return -1;
	memset(usrdatum, 0, sizeof(user_datum_t));

	items = fread(buf, sizeof(__u32), 2, fp);
	if (items != 2)
		goto bad;

	len = le32_to_cpu(buf[0]);
	usrdatum->value = le32_to_cpu(buf[1]);

	key = malloc(len + 1);
	if (!key)
		goto bad;
	items = fread(key, 1, len, fp);
	if (items != len)
		goto bad;
	key[len] = 0;

	if (!ebitmap_read(&usrdatum->roles, fp))
		goto bad;

#ifdef CONFIG_FLASK_MLS
	items = fread(buf, sizeof(__u32), 1, fp);
	if (items != 1)
		goto bad;
	nel = le32_to_cpu(buf[0]);
	l = NULL;
	for (i = 0; i < nel; i++) {
		r = malloc(sizeof(mls_range_list_t));
		if (!r)
			goto bad;
		memset(r, 0, sizeof(mls_range_list_t));

		if (mls_read_range(&r->range, fp))
			goto bad;

		if (l)
			l->next = r;
		else
			usrdatum->ranges = r;
		l = r;
	}
#endif

	if (hashtab_insert(h, key, usrdatum))
		goto bad;

	return 0;

      bad:
	user_destroy(key, usrdatum, NULL);
	return -1;
}


static int (*read_f[SYM_NUM]) (policydb_t * p, hashtab_t h, FILE * fp) =
{
	common_read,
	    class_read,
	    role_read,
	    type_read,
	    user_read
#ifdef CONFIG_FLASK_MLS
	    ,sens_read,
	    cat_read
#endif
};

#define mls_config(x) \
       ((x) & POLICYDB_CONFIG_MLS) ? "mls" : "no_mls"
#define audit_config(x) \
       ((x) & POLICYDB_CONFIG_AUDIT) ? "audit" : "no_audit"
#define notify_config(x) \
       ((x) & POLICYDB_CONFIG_NOTIFY) ? "notify" : "no_notify"

/*
 * Read the configuration data from a policy database binary
 * representation file into a policy database structure.
 */
int policydb_read(policydb_t * p, FILE * fp)
{
	ocontext_t *c, *l;
	int i, j;
	__u32 buf[32], len, config, nprim, nel;
	size_t items;

	config = 0;
#ifdef CONFIG_FLASK_MLS
	config |= POLICYDB_CONFIG_MLS;
#endif
#ifdef CONFIG_FLASK_AUDIT
	config |= POLICYDB_CONFIG_AUDIT;
#endif
#ifdef CONFIG_FLASK_NOTIFY
	config |= POLICYDB_CONFIG_NOTIFY;
#endif

	items = fread(buf, sizeof(__u32), 5, fp);
	if (items != 5) {
		return -1;
	}
	for (i = 0; i < 5; i++)
		buf[i] = le32_to_cpu(buf[i]);

	if (buf[0] != POLICYDB_VERSION) {
		printf("security:  policydb version %d does not match my version %d\n", buf[0], POLICYDB_VERSION);
		return -1;
	}
	if (buf[1] != config) {
		printf("security:  policydb configuration (%s,%s,%s) does not match my configuration (%s,%s,%s)\n",
		       mls_config(buf[1]), audit_config(buf[1]), notify_config(buf[1]),
		       mls_config(config), audit_config(config), notify_config(config));
		if ((buf[1] & POLICYDB_CONFIG_MLS) !=
		    (config & POLICYDB_CONFIG_MLS)) {
			printf("security:  policydb MLS configuration differs from mine, unable to proceed\n");
			return -1;
		}
		/*
		 * Audit or notify configuration mismatches are not fatal,
		 * but we do need to remember the policydb configuration
		 * so that we know how to parse the policydb in 
		 * avtab_read.
		 */
		printf("security:  continuing despite configuration mismatch\n");
		config = buf[1];
	}
	if (buf[2] != SYM_NUM || buf[3] != TED_NUM || buf[4] != OCON_NUM) {
		printf("security:  policydb table sizes (%d,%d,%d) do not match mine (%d,%d,%d)\n", buf[2], buf[3], buf[4], SYM_NUM, TED_NUM, OCON_NUM);
		return -1;
	}
	memset(p, 0, sizeof(policydb_t));

#ifdef CONFIG_FLASK_MLS
	items = fread(buf, sizeof(__u32), 1, fp);
	if (items != 1) {
		return -1;
	}
	p->nlevels = le32_to_cpu(buf[0]);
#endif

	for (i = 0; i < SYM_NUM; i++) {
		if (symtab_init(&p->symtab[i]))
			goto bad;
		items = fread(buf, sizeof(__u32), 2, fp);
		if (items != 2)
			goto bad;
		nprim = le32_to_cpu(buf[0]);
		nel = le32_to_cpu(buf[1]);
		for (j = 0; j < nel; j++) {
			if (read_f[i] (p, p->symtab[i].table, fp))
				goto bad;
		}

		p->symtab[i].nprim = nprim;
	}

	for (i = 0; i < TED_NUM; i++) {
		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			goto bad;
		p->te_def[i].value = le32_to_cpu(buf[0]);
		if (!ebitmap_read(&p->te_def[i].exclude, fp)) {
			goto bad;
		}
	}

	if (avtab_read(&p->te_avtab, fp, config))
		goto bad;

	if (policydb_index_classes(p))
		goto bad;

	if (policydb_index_others(p))
		goto bad;

	for (i = 0; i < OCON_NUM; i++) {
		items = fread(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			goto bad;
		nel = le32_to_cpu(buf[0]);
		l = NULL;
		for (j = 0; j < nel; j++) {
			c = malloc(sizeof(ocontext_t));
			if (!c) {
				goto bad;
			}
			memset(c, 0, sizeof(ocontext_t));
			if (l) {
				l->next = c;
			} else {
				p->ocontexts[i] = c;
			}
			l = c;
			switch (i) {
			case OCON_ISID:
				items = fread(buf, sizeof(__u32), 1, fp);
				if (items != 1)
					goto bad;
				c->sid[0] = le32_to_cpu(buf[0]);
				if (context_read_and_validate(&c->context[0], p, fp))
					goto bad;
				break;
			case OCON_FS:
			case OCON_NETIF:
				items = fread(buf, sizeof(__u32), 1, fp);
				if (items != 1)
					goto bad;
				len = le32_to_cpu(buf[0]);
				c->u.name = malloc(len + 1);
				if (!c->u.name) {
					goto bad;
				}
				items = fread(c->u.name, 1, len, fp);
				if (items != len)
					goto bad;
				c->u.name[len] = 0;
				if (context_read_and_validate(&c->context[0], p, fp))
					goto bad;
				if (context_read_and_validate(&c->context[1], p, fp))
					goto bad;
				break;
			case OCON_PORT:
				items = fread(buf, sizeof(__u32), 3, fp);
				if (items != 3)
					goto bad;
				c->u.port.protocol = le32_to_cpu(buf[0]);
				c->u.port.low_port = le32_to_cpu(buf[1]);
				c->u.port.high_port = le32_to_cpu(buf[2]);
				if (context_read_and_validate(&c->context[0], p, fp))
					goto bad;
				break;
			case OCON_NODE:
				items = fread(buf, sizeof(__u32), 2, fp);
				if (items != 2)
					goto bad;
				c->u.node.addr = le32_to_cpu(buf[0]);
				c->u.node.mask = le32_to_cpu(buf[1]);
				if (context_read_and_validate(&c->context[0], p, fp))
					goto bad;
				break;
			}
		}
	}

	return 0;
      bad:
	policydb_destroy(p);
	return -1;
}


#ifndef __KERNEL__

/*
 * Write a security context structure
 * to a policydb binary representation file.
 */
static int context_write(context_struct_t * c, FILE * fp)
{
	__u32 buf[32];
	size_t items, items2;

	items = 0;
	buf[items++] = cpu_to_le32(c->user);
	buf[items++] = cpu_to_le32(c->role);
	buf[items++] = cpu_to_le32(c->type);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items2 != items)
		return -1;
#ifdef CONFIG_FLASK_MLS
	if (mls_write_range(&c->range, fp))
		return -1;
#endif

	return 0;
}


/*
 * The following *_write functions are used to
 * write the symbol data to a policy database
 * binary representation file.
 */

static int perm_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	perm_datum_t *perdatum;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;

	perdatum = (perm_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(perdatum->value);
#ifdef CONFIG_FLASK_MLS
	buf[items++] = cpu_to_le32(perdatum->base_perms);
#endif
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	return 0;
}


static int common_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	common_datum_t *comdatum;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;

	comdatum = (common_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(comdatum->value);
	buf[items++] = cpu_to_le32(comdatum->permissions.nprim);
	buf[items++] = cpu_to_le32(comdatum->permissions.table->nel);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	if (hashtab_map(comdatum->permissions.table, perm_write, fp))
		return -1;

	return 0;
}


static int constraint_expr_write(constraint_expr_t * expr, FILE * fp)
{
	__u32 buf[32];
	int items, items2;

	items = 0;
	buf[items++] = cpu_to_le32(expr->expr_type);
	if (expr->expr_type == CONSTRAINT_EXPR_TYPE_ROLE_RELATION)
		buf[items++] = cpu_to_le32(expr->relation);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	switch (expr->expr_type) {
	case CONSTRAINT_EXPR_TYPE_TYPE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_TYPE_TARGET:
	case CONSTRAINT_EXPR_TYPE_ROLE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_ROLE_TARGET:
		if (!ebitmap_write(&expr->bitmap, fp))
			return -1;
		break;
	default:
		break;
	}

	if (expr->left) {
		if (constraint_expr_write(expr->left, fp))
			return -1;
	}
	if (expr->right) {
		if (constraint_expr_write(expr->right, fp))
			return -1;
	}
	return 0;
}


static int class_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	class_datum_t *cladatum;
	constraint_node_t *c;
	__u32 buf[32], len, len2, ncons;
	int items, items2;
	FILE *fp = p;

	cladatum = (class_datum_t *) datum;

	len = strlen(key);
	if (cladatum->comkey)
		len2 = strlen(cladatum->comkey);
	else
		len2 = 0;

	ncons = 0;
	for (c = cladatum->constraints; c; c = c->next) {
		ncons++;
	}

	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(len2);
	buf[items++] = cpu_to_le32(cladatum->value);
	buf[items++] = cpu_to_le32(cladatum->permissions.nprim);
	buf[items++] = cpu_to_le32(cladatum->permissions.table->nel);
	buf[items++] = cpu_to_le32(ncons);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	if (cladatum->comkey) {
		items = fwrite(cladatum->comkey, 1, len2, fp);
		if (items != len2)
			return -1;
	}
	if (hashtab_map(cladatum->permissions.table, perm_write, fp))
		return -1;

	for (c = cladatum->constraints; c; c = c->next) {
		buf[0] = cpu_to_le32(c->permissions);
		items = fwrite(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			return -1;
		if (constraint_expr_write(c->expr, fp))
			return -1;
	}

#ifdef CONFIG_FLASK_MLS
	if (mls_write_perms(&cladatum->mlsperms, fp))
		return -1;
#endif

	return 0;
}

static int role_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	role_datum_t *role;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;

	role = (role_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(role->value);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	if (!ebitmap_write(&role->dominates, fp))
		return -1;

	if (!ebitmap_write(&role->types, fp))
		return -1;

	return 0;
}

static int type_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	type_datum_t *typdatum;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;

	typdatum = (type_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(typdatum->value);
	buf[items++] = cpu_to_le32(typdatum->isalias);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	return 0;
}

#ifdef CONFIG_FLASK_MLS
static int sens_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	level_datum_t *levdatum;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;

	levdatum = (level_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(levdatum->isalias);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	if (mls_write_level(levdatum->level, fp))
		return -1;

	return 0;
}

static int cat_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	cat_datum_t *catdatum;
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;


	catdatum = (cat_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(catdatum->value);
	buf[items++] = cpu_to_le32(catdatum->isalias);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	return 0;
}
#endif

static int user_write(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	user_datum_t *usrdatum;
#ifdef CONFIG_FLASK_MLS
	mls_range_list_t *r;
	__u32 nel;
#endif
	__u32 buf[32], len;
	int items, items2;
	FILE *fp = p;


	usrdatum = (user_datum_t *) datum;

	len = strlen(key);
	items = 0;
	buf[items++] = cpu_to_le32(len);
	buf[items++] = cpu_to_le32(usrdatum->value);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

	items = fwrite(key, 1, len, fp);
	if (items != len)
		return -1;

	if (!ebitmap_write(&usrdatum->roles, fp))
		return -1;

#ifdef CONFIG_FLASK_MLS
	nel = 0;
	for (r = usrdatum->ranges; r; r = r->next)
		nel++;
	buf[0] = cpu_to_le32(nel);
	items = fwrite(buf, sizeof(__u32), 1, fp);
	if (items != 1)
		return -1;
	for (r = usrdatum->ranges; r; r = r->next) {
		if (mls_write_range(&r->range, fp))
			return -1;
	}
#endif

	return 0;
}


static int (*write_f[SYM_NUM]) (hashtab_key_t key, hashtab_datum_t datum, void *datap) =
{
	common_write,
	    class_write,
	    role_write,
	    type_write,
	    user_write
#ifdef CONFIG_FLASK_MLS
	    ,sens_write,
	    cat_write
#endif
};


/*
 * Write the configuration data in a policy database
 * structure to a policy database binary representation
 * file.
 */
int policydb_write(policydb_t * p, FILE * fp)
{
	ocontext_t *c;
	int i, j;
	__u32 buf[32], len, config, nel;
	size_t items, items2;

	config = 0;
#ifdef CONFIG_FLASK_MLS
	config |= POLICYDB_CONFIG_MLS;
#endif
#ifdef CONFIG_FLASK_AUDIT
	config |= POLICYDB_CONFIG_AUDIT;
#endif
#ifdef CONFIG_FLASK_NOTIFY
	config |= POLICYDB_CONFIG_NOTIFY;
#endif

	items = 0;
	buf[items++] = cpu_to_le32(POLICYDB_VERSION);
	buf[items++] = cpu_to_le32(config);
	buf[items++] = cpu_to_le32(SYM_NUM);
	buf[items++] = cpu_to_le32(TED_NUM);
	buf[items++] = cpu_to_le32(OCON_NUM);
	items2 = fwrite(buf, sizeof(__u32), items, fp);
	if (items != items2)
		return -1;

#ifdef CONFIG_FLASK_MLS
	buf[0] = cpu_to_le32(p->nlevels);
	items = fwrite(buf, sizeof(__u32), 1, fp);
	if (items != 1)
		return -1;
#endif

	for (i = 0; i < SYM_NUM; i++) {
		buf[0] = cpu_to_le32(p->symtab[i].nprim);
		buf[1] = cpu_to_le32(p->symtab[i].table->nel);
		items = fwrite(buf, sizeof(__u32), 2, fp);
		if (items != 2)
			return -1;
		if (hashtab_map(p->symtab[i].table, write_f[i], fp))
			return -1;
	}

	for (i = 0; i < TED_NUM; i++) {
		buf[0] = cpu_to_le32(p->te_def[i].value);
		items = fwrite(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			return -1;
		if (!ebitmap_write(&p->te_def[i].exclude, fp))
			return -1;
	}

	if (avtab_write(&p->te_avtab, fp))
		return -1;

	for (i = 0; i < OCON_NUM; i++) {
		nel = 0;
		for (c = p->ocontexts[i]; c; c = c->next)
			nel++;
		buf[0] = cpu_to_le32(nel);
		items = fwrite(buf, sizeof(__u32), 1, fp);
		if (items != 1)
			return -1;
		for (c = p->ocontexts[i]; c; c = c->next) {
			switch (i) {
			case OCON_ISID:
				buf[0] = cpu_to_le32(c->sid[0]);
				items = fwrite(buf, sizeof(__u32), 1, fp);
				if (items != 1)
					return -1;
				if (context_write(&c->context[0], fp))
					return -1;
				break;
			case OCON_FS:
			case OCON_NETIF:
				len = strlen(c->u.name);
				buf[0] = cpu_to_le32(len);
				items = fwrite(buf, sizeof(__u32), 1, fp);
				if (items != 1)
					return -1;
				items = fwrite(c->u.name, 1, len, fp);
				if (items != len)
					return -1;
				if (context_write(&c->context[0], fp))
					return -1;
				if (context_write(&c->context[1], fp))
					return -1;
				break;
			case OCON_PORT:
				buf[0] = c->u.port.protocol;
				buf[1] = c->u.port.low_port;
				buf[2] = c->u.port.high_port;
				for (j = 0; j < 3; j++) {
					buf[j] = cpu_to_le32(buf[j]);
				}
				items = fwrite(buf, sizeof(__u32), 3, fp);
				if (items != 3)
					return -1;
				if (context_write(&c->context[0], fp))
					return -1;
				break;
			case OCON_NODE:
				buf[0] = cpu_to_le32(c->u.node.addr);
				buf[1] = cpu_to_le32(c->u.node.mask);
				items = fwrite(buf, sizeof(__u32), 2, fp);
				if (items != 2)
					return -1;
				if (context_write(&c->context[0], fp))
					return -1;
				break;
			}
		}
	}

	return 0;
}

#endif
