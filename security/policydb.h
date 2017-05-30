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
 * A policy database (policydb) specifies the 
 * configuration data for the security policy.
 */

#ifndef _POLICYDB_H_
#define _POLICYDB_H_

#include "symtab.h"
#include "avtab.h"
#include "sidtab.h"
#include "context.h"
#include "constraint.h"

/*
 * A datum type is defined for each kind of symbol 
 * in the configuration data:  individual permissions, 
 * common prefixes for access vectors, classes,
 * users, roles, types, sensitivities, categories, etc.
 */

/* Permission attributes */
typedef struct perm_datum {
	__u32 value;		/* permission bit + 1 */
#ifdef CONFIG_FLASK_MLS
#define MLS_BASE_READ    1	/* MLS base permission `read' */
#define MLS_BASE_WRITE   2	/* MLS base permission `write' */
#define MLS_BASE_READBY  4	/* MLS base permission `readby' */
#define MLS_BASE_WRITEBY 8	/* MLS base permission `writeby' */
	__u32 base_perms;		/* MLS base permission mask */
#endif
} perm_datum_t;

/* Attributes of a common prefix for access vectors */
typedef struct common_datum {
	__u32 value;		/* internal common value */
	symtab_t permissions;	/* common permissions */
} common_datum_t;

/* Class attributes */
typedef struct class_datum {
	__u32 value;		/* class value */
	char *comkey;		/* common name */
	common_datum_t *comdatum;	/* common datum */
	symtab_t permissions;	/* class-specific permission symbol table */
	constraint_node_t *constraints;		/* constraints on class permissions */
#ifdef CONFIG_FLASK_MLS
	mls_perms_t mlsperms;	/* MLS base permission masks */
#endif
} class_datum_t;

/* Role attributes */
typedef struct role_datum {
	__u32 value;		/* internal role value */
	ebitmap_t dominates;	/* set of roles dominated by this role */
	ebitmap_t types;	/* set of authorized types for role */
} role_datum_t;

/* Type attributes */
typedef struct type_datum {
	__u32 value;		/* internal type value */
	unsigned char isalias;	/* is this type an alias for another? */
} type_datum_t;

/* User attributes */
typedef struct user_datum {
	__u32 value;		/* internal user value */
	ebitmap_t roles;	/* set of authorized roles for user */
#ifdef CONFIG_FLASK_MLS
	mls_range_list_t *ranges;	/* list of authorized MLS ranges for user */
#endif
} user_datum_t;


#ifdef CONFIG_FLASK_MLS
/* Sensitivity attributes */
typedef struct level_datum {
	mls_level_t *level;	/* sensitivity and associated categories */
	unsigned char isalias;  /* is this sensitivity an alias for another? */
} level_datum_t;

/* Category attributes */
typedef struct cat_datum {
	__u32 value;		/* internal category bit + 1 */
	unsigned char isalias;  /* is this category an alias for another? */
} cat_datum_t;
#endif


/*
 * The type enforcement configuration specifies default behaviors
 * for permissions that are not explicitly specified in the 
 * type enforcement tables.  This structure stores the default
 * for a single kind of access vector (allow, auditallow, auditdeny,
 * or notify).  The default may be none, all or self.  A set of
 * security classes may be excluded from the default behavior,
 * in which case their permissions will default to none.
 */
typedef struct te_def {
#define TE_DEF_NONE	0	
#define TE_DEF_ALL	1	
#define TE_DEF_SELF	2	
	__u32 value;		/* none, all or self */
	ebitmap_t exclude;	/* set of classes that are excluded */
} te_def_t;


/*
 * The configuration data includes security contexts for 
 * initial SIDs, unlabeled file systems, TCP and UDP port numbers, 
 * network interfaces, and nodes.  This structure stores the
 * relevant data for one such entry.  Entries of the same kind
 * (e.g. all initial SIDs) are linked together into a list.
 */
typedef struct ocontext {
	union {
		char *name;	/* name of initial SID, fs or netif */
		struct {
			__u8 protocol;
			__u16 low_port;
			__u16 high_port;
		} port;		/* TCP or UDP port information */
		struct {
			__u32 addr;
			__u32 mask;
		} node;		/* node information */
	} u;
	context_struct_t context[2];	/* security context(s) */
	security_id_t sid[2];	/* SID(s) */
	struct ocontext *next;
} ocontext_t;


/* symbol table array indices */
#define SYM_COMMONS 0
#define SYM_CLASSES 1
#define SYM_ROLES   2
#define SYM_TYPES   3
#define SYM_USERS   4
#ifdef CONFIG_FLASK_MLS
#define SYM_LEVELS  5
#define SYM_CATS    6
#define SYM_NUM     7
#else
#define SYM_NUM     5
#endif

/* type enforcement default array indices */
#define TED_ALLOWED 	0
#define TED_AUDITALLOW	1
#define TED_AUDITDENY 	2
#define TED_NOTIFY 	3
#define TED_NUM    	4

/* object context array indices */
#define OCON_ISID  0	/* initial SIDs */
#define OCON_FS    1	/* unlabeled file systems */
#define OCON_PORT  2	/* TCP and UDP port numbers */
#define OCON_NETIF 3	/* network interfaces */
#define OCON_NODE  4	/* nodes */
#define OCON_NUM   5

/* The policy database */
typedef struct policydb {
	/* symbol tables */
	symtab_t symtab[SYM_NUM];
#define p_commons symtab[SYM_COMMONS]
#define p_classes symtab[SYM_CLASSES]
#define p_roles symtab[SYM_ROLES]
#define p_types symtab[SYM_TYPES]
#define p_users symtab[SYM_USERS]
#define p_levels symtab[SYM_LEVELS]
#define p_cats symtab[SYM_CATS]

	/* symbol names indexed by (value - 1) */
	char **sym_val_to_name[SYM_NUM];
#define p_common_val_to_name sym_val_to_name[SYM_COMMONS]
#define p_class_val_to_name sym_val_to_name[SYM_CLASSES]
#define p_role_val_to_name sym_val_to_name[SYM_ROLES]
#define p_type_val_to_name sym_val_to_name[SYM_TYPES]
#define p_user_val_to_name sym_val_to_name[SYM_USERS]
#define p_sens_val_to_name sym_val_to_name[SYM_LEVELS]
#define p_cat_val_to_name sym_val_to_name[SYM_CATS]

	/* class, role, and user attributes indexed by (value - 1) */
	class_datum_t **class_val_to_struct;
	role_datum_t **role_val_to_struct;
	user_datum_t **user_val_to_struct;

	/* type enforcement defaults */
	te_def_t te_def[TED_NUM];
#define p_te_def_allowed te_def[TED_ALLOWED]
#define p_te_def_auditallow te_def[TED_AUDITALLOW]
#define p_te_def_auditdeny te_def[TED_AUDITDENY]
#define p_te_def_notify te_def[TED_NOTIFY]

	/* type enforcement access vectors and transitions */
	avtab_t te_avtab;

	/* security contexts of initial SIDs, unlabeled file systems,
	   TCP or UDP port numbers, network interfaces, and nodes */
	ocontext_t *ocontexts[OCON_NUM];

#ifdef CONFIG_FLASK_MLS
	/* number of legitimate MLS levels */
	__u32 nlevels;
#endif
} policydb_t;

extern int policydb_init(policydb_t * p);

extern int policydb_index_classes(policydb_t * p);

extern int policydb_index_others(policydb_t * p);

extern int constraint_expr_destroy(constraint_expr_t * expr);

extern void policydb_destroy(policydb_t * p);

extern int policydb_load_isids(policydb_t *p, sidtab_t *s);

extern int policydb_context_isvalid(policydb_t *p, context_struct_t *c);

extern int policydb_read(policydb_t * p, FILE * fp);

#ifndef __KERNEL__
int policydb_write(policydb_t * p, FILE * fp);
#endif

#define POLICYDB_VERSION 1
#define POLICYDB_CONFIG_MLS    1
#define POLICYDB_CONFIG_AUDIT  2
#define POLICYDB_CONFIG_NOTIFY 4

#endif	/* _POLICYDB_H_ */

/* FLASK */
