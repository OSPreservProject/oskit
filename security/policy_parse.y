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

%{
#include "policydb.h"
#include "services.h"
#include "queue.h"

policydb_t *policydbp;
queue_t id_queue = 0;

extern char yytext[];
extern int yyerror(char *msg);

static char errormsg[255];

static int insert_separator(int push);
static int insert_id(char *id,int push);
static int define_class(void);
static int define_initial_sid(void);
static int define_common_perms(void);
static int define_av_perms(int inherits);
static int define_sens(void);
static int define_dominance(void);
static int define_category(void);
static int define_level(void);
static int define_common_base(void);
static int define_av_base(void);
static int define_type(void);
static int define_transition(void);
static int define_te_def(int which);
static int define_te_avtab(int which);
static int define_role_types(void);
static role_datum_t *merge_roles_dom(role_datum_t *r1,role_datum_t *r2);
static role_datum_t *define_role_dom(role_datum_t *r);
static int define_constraint(constraint_expr_t *expr);
static constraint_expr_t *define_constraint_expr(constraint_expr_t *left,
						 constraint_expr_t *right,
						 unsigned expr_type);
static int define_user(void);
static int parse_security_context(context_struct_t *c);
static int define_initial_sid_context(void);
static int define_fs_context(int major, int minor);
static int define_port_context(int low, int high);
static int define_netif_context(void);
static int define_node_context(int addr, int mask);
%}

%token COMMON
%token CLASS
%token CONSTRAIN
%token INHERITS
%token SID
%token ROLE
%token ROLES
%token TYPE
%token TYPES
%token ALIAS
%token TYPE_TRANSITION
%token SENSITIVITY
%token DOMINANCE
%token CATEGORY
%token LEVEL
%token RANGES
%token USER
%token DEFAULT
%token EXCLUDE
%token ALLOW
%token AUDITALLOW
%token AUDITDENY
%token NOTIFY
%token SOURCE
%token TARGET
%token SAMEUSER
%token NOT AND OR 
%token IDENTIFIER
%token NUMBER

%left OR
%left AND
%right NOT
%%
policy			: classes initial_sids access_vectors 
                          { if (policydb_index_classes(policydbp)) return -1; }
			  opt_mls te rbac constraints users 
			  {if (policydb_index_others(policydbp)) return -1;} 
			  initial_sid_contexts fs_contexts net_contexts
			;
classes			: class_def 
			| classes class_def
			;
class_def		: CLASS identifier
			{if (define_class()) return -1;}
			;
initial_sids 		: initial_sid_def 
			| initial_sids initial_sid_def
			;
initial_sid_def		: SID identifier
                        {if (define_initial_sid()) return -1;}
			;
access_vectors		: common_perms av_perms
			;
common_perms		: common_perms_def
			| common_perms common_perms_def
			;
common_perms_def	: COMMON identifier '{' identifier_list '}'
			{if (define_common_perms()) return -1;}
			;
av_perms		: av_perms_def
			| av_perms av_perms_def
			;
av_perms_def		: CLASS identifier '{' identifier_list '}'
			{if (define_av_perms(FALSE)) return -1;}
                        | CLASS identifier INHERITS identifier 
			{if (define_av_perms(TRUE)) return -1;}
                        | CLASS identifier INHERITS identifier '{' identifier_list '}'
			{if (define_av_perms(TRUE)) return -1;}
			;
opt_mls			: mls
                        | 
			;
mls			: sensitivities dominance categories levels base_perms
			;
sensitivities	 	: sensitivity_def 
			| sensitivities sensitivity_def
			;
sensitivity_def		: SENSITIVITY identifier alias_def ';'
			{if (define_sens()) return -1;}
			| SENSITIVITY identifier ';'
			{if (define_sens()) return -1;}
	                ;
alias_def		: ALIAS '{' identifier_list '}'
			| ALIAS identifier
			;
dominance		: DOMINANCE identifier 
			{if (define_dominance()) return -1;}
                        | DOMINANCE '{' identifier_list '}' 
			{if (define_dominance()) return -1;}
			;
categories 		: category_def 
			| categories category_def
			;
category_def		: CATEGORY identifier alias_def ';'
			{if (define_category()) return -1;}
			| CATEGORY identifier ';'
			{if (define_category()) return -1;}
			;
levels	 		: level_def 
			| levels level_def
			;
level_def		: LEVEL identifier ':' cat_set ';'
			{if (define_level()) return -1;}
			| LEVEL identifier ';' 
			{if (define_level()) return -1;}
			;
base_perms		: common_base av_base
			;
common_base		: common_base_def
			| common_base common_base_def
			;
common_base_def	        : COMMON identifier '{' perm_base_list '}'
	                {if (define_common_base()) return -1;}
			;
av_base		        : av_base_def
			| av_base av_base_def
			;
av_base_def		: CLASS identifier '{' perm_base_list '}'
	                {if (define_av_base()) return -1;}
                        | CLASS identifier
	                {if (define_av_base()) return -1;}
			;
perm_base_list		: perm_base
			| perm_base_list perm_base
			;
perm_base		: identifier ':' identifier
			{if (insert_separator(0)) return -1;}
                        | identifier ':' '{' identifier_list '}'
			{if (insert_separator(0)) return -1;}
			;
te			: types transitions te_defaults te_avtab
			;
types			: type_def
			| types type_def
			;
type_def		: TYPE identifier alias_def ';'
                        {if (define_type()) return -1;}
	                | TYPE identifier ';'
                        {if (define_type()) return -1;}
    			;
transitions		: transition_def
			| transitions transition_def
			;
transition_def		: TYPE_TRANSITION identifier identifier ':' identifier identifier ';'
                        {if (define_transition()) return -1;}
    			;
te_defaults		: te_default_def |
			| te_defaults te_default_def
			;
te_default_def		: DEFAULT ALLOW te_default_def_suffix
			{if (define_te_def(TED_ALLOWED)) return -1;}
			| DEFAULT AUDITALLOW te_default_def_suffix
			{if (define_te_def(TED_AUDITALLOW)) return -1;}
			| DEFAULT AUDITDENY te_default_def_suffix
			{if (define_te_def(TED_AUDITDENY)) return -1;}
			| DEFAULT NOTIFY te_default_def_suffix
			{if (define_te_def(TED_NOTIFY)) return -1;}
			;
te_default_def_suffix	: identifier ';'
			| identifier EXCLUDE names ';'
			;
te_avtab		: te_avtab_def
                        | te_avtab te_avtab_def
			;
te_avtab_def		: allow_def
			| auditallow_def
			| auditdeny_def
			| notify_def
			;
allow_def		: ALLOW identifier identifier ':' names_sep names  ';'
			{if (define_te_avtab(AVTAB_ALLOWED)) return -1; }
		        ;
auditallow_def		: AUDITALLOW identifier identifier ':' names_sep names ';'
			{if (define_te_avtab(AVTAB_AUDITALLOW)) return -1; }
		        ;
auditdeny_def		: AUDITDENY identifier identifier ':' names_sep names ';'
			{if (define_te_avtab(AVTAB_AUDITDENY)) return -1; }
		        ;
notify_def		: NOTIFY identifier identifier ':' names_sep names ';'
			{if (define_te_avtab(AVTAB_NOTIFY)) return -1; }
		        ;
rbac			: role_types role_dominance
                        ;
role_types		: role_type_def
                        | role_types role_type_def
			;
role_type_def		: ROLE identifier TYPES names ';'
			{if (define_role_types()) return -1;}
                        ;
role_dominance		: DOMINANCE '{' roles '}'
			;
roles			: role_def
			{ $$ = $1; }
			| roles role_def
			{ $$ = (int) merge_roles_dom((role_datum_t*)$1, (role_datum_t*)$2); if ($$ == 0) return -1;}
			;
role_def		: ROLE identifier_push ';'
                        {$$ = (int) define_role_dom(NULL); if ($$ == 0) return -1;}
			| ROLE identifier_push '{' roles '}'
                        {$$ = (int) define_role_dom((role_datum_t*)$4); if ($$ == 0) return -1;}
			;
constraints		: constraint_def
			| constraints constraint_def
			;
constraint_def		: CONSTRAIN names_sep names_pushsep constraint_expr ';'
			{ if (define_constraint((constraint_expr_t*)$4)) return -1; }
			;
constraint_expr		: '(' constraint_expr ')'
			{ $$ = $2; }
			| constraint_expr AND constraint_expr
			{ $$ = (int) define_constraint_expr((constraint_expr_t*)$1, 
					(constraint_expr_t*)$3, 
					CONSTRAINT_EXPR_TYPE_AND); 
			  if ($$ == 0) return -1; }
			| constraint_expr OR constraint_expr
			{ $$ = (int) define_constraint_expr((constraint_expr_t*)$1, 
					(constraint_expr_t*)$3, 
					CONSTRAINT_EXPR_TYPE_OR); 
			  if ($$ == 0) return -1; }
			| NOT constraint_expr
			{ $$ = (int) define_constraint_expr((constraint_expr_t*)$2, 
					NULL, CONSTRAINT_EXPR_TYPE_NOT); 
			  if ($$ == 0) return -1; }
			| constraint_expr_prim
			{ $$ = $1; }
			;
constraint_expr_prim	: SAMEUSER
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_SAMEUSER); 
			  if ($$ == 0) return -1; }
			| SOURCE ROLE names_push
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_ROLE_SOURCE); 
			  if ($$ == 0) return -1; }
			| TARGET ROLE names_push
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_ROLE_TARGET); 
			  if ($$ == 0) return -1; }
			| ROLE identifier_push
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_ROLE_RELATION); 
			  if ($$ == 0) return -1; }
			| SOURCE TYPE names_push
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_TYPE_SOURCE); 
			  if ($$ == 0) return -1; }
			| TARGET TYPE names_push
			{ $$ = (int) define_constraint_expr(NULL,NULL,
						CONSTRAINT_EXPR_TYPE_TYPE_TARGET); 
			  if ($$ == 0) return -1; }
			;
users			: user_def
			| users user_def
			;
user_def		: USER identifier ROLES names_sep opt_user_ranges ';'
	                {if (define_user()) return -1;}
			;
opt_user_ranges		: RANGES user_ranges 
			|
			;
user_ranges		: mls_range_def
			| '{' user_range_def_list '}' 
			;
user_range_def_list	: mls_range_def
			| user_range_def_list mls_range_def
			;
initial_sid_contexts	: initial_sid_context_def
			| initial_sid_contexts initial_sid_context_def
			;
initial_sid_context_def	: SID identifier security_context_def
			{if (define_initial_sid_context()) return -1;}
			;
fs_contexts		: fs_context_def
			| fs_contexts fs_context_def
			;
fs_context_def		: number number security_context_def security_context_def
			{if (define_fs_context($1,$2)) return -1;}
			;
net_contexts		: port_contexts netif_contexts node_contexts
			;
port_contexts		: port_context_def
			| port_contexts port_context_def
			;
port_context_def	: identifier number security_context_def
			{if (define_port_context($2,$2)) return -1;}
			| identifier number '-' number security_context_def
			{if (define_port_context($2,$4)) return -1;}
			;
netif_contexts		: netif_context_def
			| netif_contexts netif_context_def
			;
netif_context_def	: identifier security_context_def security_context_def
			{if (define_netif_context()) return -1;}
			;
node_contexts		: node_context_def
			| node_contexts node_context_def
			;
node_context_def	: ipv4_addr_def ipv4_addr_def security_context_def
			{if (define_node_context($1,$2)) return -1;}
			;
ipv4_addr_def		: number '.' number '.' number '.' number
			{ 
			  unsigned int addr;
	  		  unsigned char *p = ((unsigned char *)&addr);

			  p[0] = $1 & 0xff;				
			  p[1] = $3 & 0xff;
			  p[2] = $5 & 0xff;
			  p[3] = $7 & 0xff;
			  $$ = addr;
			}
    			;
security_context_def	: identifier ':' identifier ':' identifier opt_mls_range_def
	                ;
opt_mls_range_def	: ':' mls_range_def
			|	
			;
mls_range_def		: mls_level_def '-' mls_level_def
			{if (insert_separator(0)) return -1;}
	                | mls_level_def
			{if (insert_separator(0)) return -1;}
	                ;
mls_level_def		: identifier ':' cat_set
			{if (insert_separator(0)) return -1;}
	                | identifier 
			{if (insert_separator(0)) return -1;}
	                ;
cat_set			: identifier 
			| cat_set ',' identifier
			;
names_sep 		: identifier
			{ if (insert_separator(0)) return -1; }
			| '{' identifier_list '}'
			{ if (insert_separator(0)) return -1; }	
			;
names			: identifier
			| '{' identifier_list '}'
			;
names_pushsep		: identifier
			{ if (insert_separator(1)) return -1; }
			| '{' identifier_list '}'
			{ if (insert_separator(1)) return -1; }
			;
names_push		: identifier_push
			| '{' identifier_list_push '}'
			;
identifier_list_push	: identifier_push
			| identifier_list_push identifier_push
			;
identifier_push		: IDENTIFIER
			{ if (insert_id(yytext, 1)) return -1; }
			;
identifier_list		: identifier
			| identifier_list identifier
			;
identifier		: IDENTIFIER
			{ if (insert_id(yytext,0)) return -1; }
			;
number			: NUMBER 
			{ $$ = strtoul(yytext,NULL,0); }
			;
%%
static int insert_separator(int push)
{
	int error;

	if (push)
		error = queue_push(id_queue, 0);
	else
		error = queue_insert(id_queue, 0);

	if (error) {
		yyerror("queue overflow");
		return -1;
	}
	return 0;
}

static int insert_id(char *id, int push)
{
	char *newid = 0;
	int error;

	newid = (char *) malloc(strlen(id) + 1);
	if (!newid) {
		yyerror("out of memory");
		return -1;
	}
	strcpy(newid, id);
	if (push)
		error = queue_push(id_queue, (queue_element_t) newid);
	else
		error = queue_insert(id_queue, (queue_element_t) newid);

	if (error) {
		yyerror("queue overflow");
		free(newid);
		return -1;
	}
	return 0;
}


static int define_class(void)
{
	char *id = 0;
	class_datum_t *datum = 0;
	int ret;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no class name for class definition?");
		return -1;
	}
	datum = (class_datum_t *) malloc(sizeof(class_datum_t));
	if (!datum) {
		yyerror("out of memory");
		goto bad;
	}
	memset(datum, 0, sizeof(class_datum_t));
	datum->value = ++policydbp->p_classes.nprim;

	ret = hashtab_insert(policydbp->p_classes.table,
			     (hashtab_key_t) id, (hashtab_datum_t) datum);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_classes.nprim;
		yyerror("duplicate class definition");
		goto bad;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		goto bad;
	}
	return 0;

      bad:
	if (id)
		free(id);
	if (datum)
		free(datum);
	return -1;
}

static int define_initial_sid(void)
{
	char *id = 0;
	ocontext_t *newc = 0, *c, *head;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no sid name for SID definition?");
		return -1;
	}
	newc = (ocontext_t *) malloc(sizeof(ocontext_t));
	if (!newc) {
		yyerror("out of memory");
		goto bad;
	}
	memset(newc, 0, sizeof(ocontext_t));
	newc->u.name = id;
	context_init(&newc->context[0]);
	head = policydbp->ocontexts[OCON_ISID];

	for (c = head; c; c = c->next) {
		if (!strcmp(newc->u.name, c->u.name)) {
			sprintf(errormsg, "duplicate initial SID %s\n", id);
			yyerror(errormsg);
			goto bad;
		}
	}

	if (head) {
		newc->sid[0] = head->sid[0] + 1;
	} else {
		newc->sid[0] = 1;
	}
	newc->next = head;
	policydbp->ocontexts[OCON_ISID] = newc;

	return 0;

      bad:
	if (id)
		free(id);
	if (newc)
		free(newc);
	return -1;
}

static int define_common_perms(void)
{
	char *id = 0, *perm = 0;
	common_datum_t *comdatum = 0;
	perm_datum_t *perdatum = 0;
	int ret;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no common name for common perm definition?");
		return -1;
	}
	comdatum = (common_datum_t *) malloc(sizeof(common_datum_t));
	if (!comdatum) {
		yyerror("out of memory");
		goto bad;
	}
	memset(comdatum, 0, sizeof(common_datum_t));
	comdatum->value = ++policydbp->p_commons.nprim;
	ret = hashtab_insert(policydbp->p_commons.table,
			 (hashtab_key_t) id, (hashtab_datum_t) comdatum);

	if (ret == HASHTAB_PRESENT) {
		yyerror("duplicate common definition");
		goto bad;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		goto bad;
	}
	if (symtab_init(&comdatum->permissions)) {
		yyerror("out of memory");
		goto bad;
	}
	while ((perm = queue_remove(id_queue))) {
		perdatum = (perm_datum_t *) malloc(sizeof(perm_datum_t));
		if (!perdatum) {
			yyerror("out of memory");
			goto bad_perm;
		}
		memset(perdatum, 0, sizeof(perm_datum_t));
		perdatum->value = ++comdatum->permissions.nprim;

#ifdef CONFIG_FLASK_MLS
		/*
		 * By default, we set all four base permissions on this
		 * permission. This means that if base_permissions is not
		 * explicitly defined for this permission, then this
		 * permission will only be granted in the equivalent case.
		 */
		perdatum->base_perms = MLS_BASE_READ | MLS_BASE_WRITE |
		    MLS_BASE_READBY | MLS_BASE_WRITEBY;
#endif

		if (perdatum->value >= (sizeof(access_vector_t) * 8)) {
			yyerror("too many permissions to fit in an access vector");
			goto bad_perm;
		}
		ret = hashtab_insert(comdatum->permissions.table,
				     (hashtab_key_t) perm,
				     (hashtab_datum_t) perdatum);

		if (ret == HASHTAB_PRESENT) {
			sprintf(errormsg, "duplicate permission %s in common %s",
				perm, id);
			yyerror(errormsg);
			goto bad_perm;
		}
		if (ret == HASHTAB_OVERFLOW) {
			yyerror("hash table overflow");
			goto bad_perm;
		}
	}

	return 0;

      bad:
	if (id)
		free(id);
	if (comdatum)
		free(comdatum);
	return -1;

      bad_perm:
	if (perm)
		free(perm);
	if (perdatum)
		free(perdatum);
	return -1;
}


static int define_av_perms(int inherits)
{
	char *id;
	class_datum_t *cladatum;
	common_datum_t *comdatum;
	perm_datum_t *perdatum = 0, *perdatum2 = 0;
	int ret;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no tclass name for av perm definition?");
		return -1;
	}
	cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table,
						    (hashtab_key_t) id);
	if (!cladatum) {
		sprintf(errormsg, "class %s is not defined", id);
		yyerror(errormsg);
		goto bad;
	}
	free(id);

	if (cladatum->comdatum || cladatum->permissions.nprim) {
		yyerror("duplicate access vector definition");
		return -1;
	}
	if (symtab_init(&cladatum->permissions)) {
		yyerror("out of memory");
		return -1;
	}
	if (inherits) {
		id = (char *) queue_remove(id_queue);
		if (!id) {
			yyerror("no inherits name for access vector definition?");
			return -1;
		}
		comdatum = (common_datum_t *) hashtab_search(policydbp->p_commons.table,
						     (hashtab_key_t) id);

		if (!comdatum) {
			sprintf(errormsg, "common %s is not defined", id);
			yyerror(errormsg);
			goto bad;
		}
		cladatum->comkey = id;
		cladatum->comdatum = comdatum;

		/*
		 * Class-specific permissions start with values 
		 * after the last common permission.
		 */
		cladatum->permissions.nprim += comdatum->permissions.nprim;
	}
	while ((id = queue_remove(id_queue))) {
		perdatum = (perm_datum_t *) malloc(sizeof(perm_datum_t));
		if (!perdatum) {
			yyerror("out of memory");
			goto bad;
		}
		memset(perdatum, 0, sizeof(perm_datum_t));
		perdatum->value = ++cladatum->permissions.nprim;

#ifdef CONFIG_FLASK_MLS
		/*
		 * By default, we set all four base permissions on this
		 * permission. This means that if base_permissions is not
		 * explicitly defined for this permission, then this
		 * permission will only be granted in the equivalent case.
		 */
		perdatum->base_perms = MLS_BASE_READ | MLS_BASE_WRITE |
		    MLS_BASE_READBY | MLS_BASE_WRITEBY;
		/* actual value set in define_av_base */
#endif

		if (perdatum->value >= (sizeof(access_vector_t) * 8)) {
			yyerror("too many permissions to fit in an access vector");
			goto bad;
		}
		if (inherits) {
			/*
			 * Class-specific permissions and 
			 * common permissions exist in the same
			 * name space.
			 */
			perdatum2 = (perm_datum_t *) hashtab_search(cladatum->comdatum->permissions.table,
						     (hashtab_key_t) id);
			if (perdatum2) {
				sprintf(errormsg, "permission %s conflicts with an inherited permission", id);
				yyerror(errormsg);
				goto bad;
			}
		}
		ret = hashtab_insert(cladatum->permissions.table,
				     (hashtab_key_t) id,
				     (hashtab_datum_t) perdatum);

		if (ret == HASHTAB_PRESENT) {
			sprintf(errormsg, "duplicate permission %s", id);
			yyerror(errormsg);
			goto bad;
		}
		if (ret == HASHTAB_OVERFLOW) {
			yyerror("hash table overflow");
			goto bad;
		}
	}

	return 0;

      bad:
	if (id)
		free(id);
	if (perdatum)
		free(perdatum);
	return -1;
}


static int define_sens(void)
{
#ifdef CONFIG_FLASK_MLS
	char *id;
	mls_level_t *level = 0;
	level_datum_t *datum = 0, *aliasdatum = 0;
	int ret;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no sensitivity name for sensitivity definition?");
		return -1;
	}
	level = (mls_level_t *) malloc(sizeof(mls_level_t));
	if (!level) {
		yyerror("out of memory");
		goto bad;
	}
	memset(level, 0, sizeof(mls_level_t));
	level->sens = 0;	/* actual value set in define_dominance */
	++policydbp->p_levels.nprim;
	ebitmap_init(&level->cat);	/* actual value set in define_level */

	datum = (level_datum_t *) malloc(sizeof(level_datum_t));
	if (!datum) {
		yyerror("out of memory");
		goto bad;
	}
	memset(datum, 0, sizeof(level_datum_t));
	datum->isalias = FALSE;
	datum->level = level;

	ret = hashtab_insert(policydbp->p_levels.table,
			     (hashtab_key_t) id, (hashtab_datum_t) datum);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_levels.nprim;
		sprintf(errormsg, "duplicate definition for sensitivity %s", id);
		yyerror(errormsg);
		goto bad;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		goto bad;
	}

	while ((id = queue_remove(id_queue))) {
		aliasdatum = (level_datum_t *) malloc(sizeof(level_datum_t));
		if (!aliasdatum) {
			yyerror("out of memory");
			goto bad_alias;
		}
		memset(aliasdatum, 0, sizeof(level_datum_t));
		aliasdatum->isalias = TRUE;
		aliasdatum->level = level;

		ret = hashtab_insert(policydbp->p_levels.table,
		       (hashtab_key_t) id, (hashtab_datum_t) aliasdatum);

		if (ret == HASHTAB_PRESENT) {
			sprintf(errormsg, "duplicate definition for level %s", id);
			yyerror(errormsg);
			goto bad_alias;
		}
		if (ret == HASHTAB_OVERFLOW) {
			yyerror("hash table overflow");
			goto bad_alias;
		}
	}

	return 0;

      bad:
	if (id)
		free(id);
	if (level)
		free(level);
	if (datum)
		free(datum);
	return -1;

      bad_alias:
	if (id)
		free(id);
	if (aliasdatum)
		free(aliasdatum);
	return -1;
#else
	yyerror("sensitivity definition in non-MLS configuration");
	return -1;
#endif
}

static int define_dominance(void)
{
#ifdef CONFIG_FLASK_MLS
	level_datum_t *datum;
	int order;
	char *id;


	order = 0;
	while ((id = (char *) queue_remove(id_queue))) {
		datum = (level_datum_t *) hashtab_search(policydbp->p_levels.table,
						     (hashtab_key_t) id);
		if (!datum) {
			sprintf(errormsg, "unknown sensitivity %s used in dominance definition", id);
			yyerror(errormsg);
			free(id);
			continue;
		}
		if (datum->level->sens != 0) {
			sprintf(errormsg, "sensitivity %s occurs multiply in dominance definition", id);
			yyerror(errormsg);
			free(id);
			return -1;
		}
		datum->level->sens = ++order;

		/* no need to keep sensitivity name */
		free(id);
	}

	if (order != policydbp->p_levels.nprim) {
		yyerror("all sensitivities must be specified in dominance definition");
		return -1;
	}
	return 0;
#else
	yyerror("dominance definition in non-MLS configuration");
	return -1;
#endif
}

static int define_category(void)
{
#ifdef CONFIG_FLASK_MLS
	char *id;
	cat_datum_t *datum = 0, *aliasdatum = 0;
	int ret;

	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no category name for category definition?");
		return -1;
	}
	datum = (cat_datum_t *) malloc(sizeof(cat_datum_t));
	if (!datum) {
		yyerror("out of memory");
		goto bad;
	}
	memset(datum, 0, sizeof(cat_datum_t));
	datum->isalias = FALSE;
	datum->value = ++policydbp->p_cats.nprim;

	ret = hashtab_insert(policydbp->p_cats.table,
			     (hashtab_key_t) id, (hashtab_datum_t) datum);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_cats.nprim;
		sprintf(errormsg, "duplicate definition for category %s", id);
		yyerror(errormsg);
		goto bad;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		goto bad;
	}

	while ((id = queue_remove(id_queue))) {
		aliasdatum = (cat_datum_t *) malloc(sizeof(cat_datum_t));
		if (!aliasdatum) {
			yyerror("out of memory");
			goto bad_alias;
		}
		memset(aliasdatum, 0, sizeof(cat_datum_t));
		aliasdatum->isalias = TRUE;
		aliasdatum->value = datum->value;

		ret = hashtab_insert(policydbp->p_cats.table,
		       (hashtab_key_t) id, (hashtab_datum_t) aliasdatum);

		if (ret == HASHTAB_PRESENT) {
			sprintf(errormsg, "duplicate definition for category %s", id);
			yyerror(errormsg);
			goto bad_alias;
		}
		if (ret == HASHTAB_OVERFLOW) {
			yyerror("hash table overflow");
			goto bad_alias;
		}
	}

	return 0;

      bad:
	if (id)
		free(id);
	if (datum)
		free(datum);
	return -1;

      bad_alias:
	if (id)
		free(id);
	if (aliasdatum)
		free(aliasdatum);
	return -1;
#else
	yyerror("category definition in non-MLS configuration");
	return -1;
#endif
}


static int define_level(void)
{
#ifdef CONFIG_FLASK_MLS
	int n;
	char *id, *levid;
	level_datum_t *levdatum;
	cat_datum_t *catdatum;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no level name for level definition?");
		return -1;
	}
	levdatum = (level_datum_t *) hashtab_search(policydbp->p_levels.table,
						    (hashtab_key_t) id);
	if (!levdatum) {
		sprintf(errormsg, "unknown sensitivity %s used in level definition", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	if (ebitmap_length(&levdatum->level->cat)) {
		sprintf(errormsg, "sensitivity %s used in multiple level definitions", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	levid = id;
	n = 1;
	while ((id = queue_remove(id_queue))) {
		catdatum = (cat_datum_t *) hashtab_search(policydbp->p_cats.table,
						     (hashtab_key_t) id);
		if (!catdatum) {
			sprintf(errormsg, "unknown category %s used in level definition", id);
			yyerror(errormsg);
			free(id);
			continue;
		}
		if (!ebitmap_set_bit(&levdatum->level->cat, catdatum->value - 1, TRUE)) {
			yyerror("out of memory");
			free(id);
			free(levid);
			return -1;
		}
		/* no need to keep category name */
		free(id);

		n = n * 2;
	}

	free(levid);

	policydbp->nlevels += n;

	return 0;
#else
	yyerror("level definition in non-MLS configuration");
	return -1;
#endif
}


static int define_common_base(void)
{
#ifdef CONFIG_FLASK_MLS
	char *id, *perm, *base;
	common_datum_t *comdatum;
	perm_datum_t *perdatum;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no common name for common base definition?");
		return -1;
	}
	comdatum = (common_datum_t *) hashtab_search(policydbp->p_commons.table,
						     (hashtab_key_t) id);
	if (!comdatum) {
		sprintf(errormsg, "common %s is not defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	while ((perm = queue_remove(id_queue))) {
		perdatum = (perm_datum_t *) hashtab_search(comdatum->permissions.table,
						   (hashtab_key_t) perm);
		if (!perdatum) {
			sprintf(errormsg, "permission %s is not defined for common %s", perm, id);
			yyerror(errormsg);
			free(id);
			free(perm);
			return -1;
		}

		/*
		 * An explicit definition of base_permissions for this
		 * permission.  Reset the value to zero.
		 */
		perdatum->base_perms = 0;

		while ((base = queue_remove(id_queue))) {
			if (!strcmp(base, "read"))
				perdatum->base_perms |= MLS_BASE_READ;
			else if (!strcmp(base, "write"))
				perdatum->base_perms |= MLS_BASE_WRITE;
			else if (!strcmp(base, "readby"))
				perdatum->base_perms |= MLS_BASE_READBY;
			else if (!strcmp(base, "writeby"))
				perdatum->base_perms |= MLS_BASE_WRITEBY;
			else if (strcmp(base, "none")) {
				sprintf(errormsg, "base permission %s is not defined", base);
				yyerror(errormsg);
				free(base);
				return -1;
			}
			free(base);
		}

		free(perm);
	}

	free(id);

	return 0;
#else
	yyerror("MLS base permission definition in non-MLS configuration");
	return -1;
#endif
}


#ifdef CONFIG_FLASK_MLS
static int common_base_set(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	perm_datum_t *perdatum;
	class_datum_t *cladatum;

	perdatum = (perm_datum_t *) datum;
	cladatum = (class_datum_t *) p;

	if (perdatum->base_perms & MLS_BASE_READ)
		cladatum->mlsperms.read |= (1 << (perdatum->value - 1));

	if (perdatum->base_perms & MLS_BASE_WRITE)
		cladatum->mlsperms.write |= (1 << (perdatum->value - 1));

	if (perdatum->base_perms & MLS_BASE_READBY)
		cladatum->mlsperms.readby |= (1 << (perdatum->value - 1));

	if (perdatum->base_perms & MLS_BASE_WRITEBY)
		cladatum->mlsperms.writeby |= (1 << (perdatum->value - 1));

	return 0;
}
#endif

static int define_av_base(void)
{
#ifdef CONFIG_FLASK_MLS
	char *id, *base;
	class_datum_t *cladatum;
	perm_datum_t *perdatum;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no tclass name for av base definition?");
		return -1;
	}
	cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table,
						    (hashtab_key_t) id);
	if (!cladatum) {
		sprintf(errormsg, "class %s is not defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	free(id);

	/*
	 * Determine which common permissions should be included in each MLS
	 * vector for this access vector definition.
	 */
	if (cladatum->comdatum)
		hashtab_map(cladatum->comdatum->permissions.table, common_base_set, cladatum);

	while ((id = queue_remove(id_queue))) {
		perdatum = (perm_datum_t *) hashtab_search(cladatum->permissions.table,
						     (hashtab_key_t) id);
		if (!perdatum) {
			sprintf(errormsg, "permission %s is not defined", id);
			yyerror(errormsg);
			free(id);
			return -1;
		}
		/*
		 * An explicit definition of base_permissions for this
		 * permission.  Reset the value to zero.
		 */
		perdatum->base_perms = 0;

		while ((base = queue_remove(id_queue))) {
			if (!strcmp(base, "read")) {
				perdatum->base_perms |= MLS_BASE_READ;
				cladatum->mlsperms.read |= (1 << (perdatum->value - 1));
			} else if (!strcmp(base, "write")) {
				perdatum->base_perms |= MLS_BASE_WRITE;
				cladatum->mlsperms.write |= (1 << (perdatum->value - 1));
			} else if (!strcmp(base, "readby")) {
				perdatum->base_perms |= MLS_BASE_READBY;
				cladatum->mlsperms.readby |= (1 << (perdatum->value - 1));
			} else if (!strcmp(base, "writeby")) {
				perdatum->base_perms |= MLS_BASE_WRITEBY;
				cladatum->mlsperms.writeby |= (1 << (perdatum->value - 1));
			} else if (strcmp(base, "none")) {
				sprintf(errormsg, "base permission %s is not defined", base);
				yyerror(errormsg);
				free(base);
				continue;
			}
			free(base);
		}

		free(id);
	}

	return 0;
#else
	yyerror("MLS base permission definition in non-MLS configuration");
	return -1;
#endif
}


static int define_type(void)
{
	char *id;
	type_datum_t *datum, *aliasdatum;
	int ret;


	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no type name for type definition?");
		return -1;
	}
	datum = (type_datum_t *) malloc(sizeof(type_datum_t));
	if (!datum) {
		yyerror("out of memory");
		free(id);
		return -1;
	}
	memset(datum, 0, sizeof(type_datum_t));
	datum->isalias = FALSE;
	datum->value = ++policydbp->p_types.nprim;

	ret = hashtab_insert(policydbp->p_types.table,
			     (hashtab_key_t) id, (hashtab_datum_t) datum);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_types.nprim;
		free(datum);
		sprintf(errormsg, "duplicate definition for type %s", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		free(datum);
		free(id);
		return -1;
	}
	while ((id = queue_remove(id_queue))) {
		aliasdatum = (type_datum_t *) malloc(sizeof(type_datum_t));
		if (!aliasdatum) {
			yyerror("out of memory");
			return -1;
		}
		memset(aliasdatum, 0, sizeof(type_datum_t));
		aliasdatum->isalias = TRUE;
		aliasdatum->value = datum->value;

		ret = hashtab_insert(policydbp->p_types.table,
		       (hashtab_key_t) id, (hashtab_datum_t) aliasdatum);

		if (ret == HASHTAB_PRESENT) {
			sprintf(errormsg, "duplicate definition for type %s", id);
			yyerror(errormsg);
			free(aliasdatum);
			free(id);
			return -1;
		}
		if (ret == HASHTAB_OVERFLOW) {
			yyerror("hash table overflow");
			free(aliasdatum);
			free(id);
			return -1;
		}
	}

	return 0;
}

static int define_transition(void)
{
	char *stype = 0, *ttype = 0, *tclass = 0, *newtype = 0;
	avtab_key_t avkey;
	avtab_datum_t avdatum, *avdatump;
	type_datum_t *datum;
	class_datum_t *cladatum;
	int ret;


	stype = (char *) queue_remove(id_queue);
	if (!stype) {
		yyerror("no oldtype in transition definition?");
		return -1;
	}
	datum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						(hashtab_key_t) stype);
	if (!datum) {
		sprintf(errormsg, "unknown type %s used in transition definition", stype);
		yyerror(errormsg);
		goto bad;
	}
	avkey.source_type = datum->value;

	ttype = (char *) queue_remove(id_queue);
	if (!ttype) {
		yyerror("no type in transition definition?");
		goto bad;
	}
	datum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						(hashtab_key_t) ttype);
	if (!datum) {
		sprintf(errormsg, "unknown type %s used in transition definition", ttype);
		yyerror(errormsg);
		goto bad;
	}
	avkey.target_type = datum->value;

	tclass = (char *) queue_remove(id_queue);
	if (!tclass) {
		yyerror("no class in transition definition?");
		goto bad;
	}
	cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table,
						 (hashtab_key_t) tclass);
	if (!cladatum) {
		sprintf(errormsg, "unknown class %s used in transition definition", tclass);
		yyerror(errormsg);
		goto bad;
	}
	avkey.target_class = cladatum->value;

	newtype = (char *) queue_remove(id_queue);
	if (!newtype) {
		yyerror("no newtype in transition definition?");
		goto bad;
	}
	datum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						(hashtab_key_t) newtype);
	if (!datum) {
		sprintf(errormsg, "unknown type %s used in transition definition", newtype);
		yyerror(errormsg);
		goto bad;
	}
	avdatump = avtab_search(&policydbp->te_avtab, &avkey);
	if (avdatump) {
		if (avdatump->specified & AVTAB_TRANSITION) {
			sprintf(errormsg, "duplicate rule for (%s,%s:%s)",
				stype, ttype, tclass);
			yyerror(errormsg);
			goto bad;
		}
		avdatump->specified |= AVTAB_TRANSITION;
		avdatump->trans_type = datum->value;
	} else {
		memset(&avdatum, 0, sizeof avdatum);
		avdatum.specified |= AVTAB_TRANSITION;
		avdatum.trans_type = datum->value;
		ret = avtab_insert(&policydbp->te_avtab, &avkey, &avdatum);
		if (ret) {
			yyerror("hash table overflow");
			goto bad;
		}
	}

	free(stype);
	free(ttype);
	free(tclass);
	free(newtype);

	return 0;

      bad:
	if (stype)
		free(stype);
	if (ttype)
		free(ttype);
	if (tclass)
		free(tclass);
	if (newtype)
		free(newtype);

	return -1;
}


static int define_te_def(int which)
{
	class_datum_t *cladatum;
	te_def_t *ted;
	int value;
	char *id;

	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no default name?");
		return -1;
	}
	if (!strcmp(id, "none"))
		value = TE_DEF_NONE;
	else if (!strcmp(id, "all"))
		value = TE_DEF_ALL;
	else if (!strcmp(id, "self"))
		value = TE_DEF_SELF;
	else {
		free(id);
		sprintf(errormsg, "unrecognized default %s", id);
		yyerror(errormsg);
		return -1;
	}

	free(id);

	ted = &policydbp->te_def[which];
	ted->value = value;
	ebitmap_init(&ted->exclude);

	while ((id = queue_remove(id_queue))) {
		cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table, id);
		if (!cladatum) {
			sprintf(errormsg, "unrecognized class %s", id);
			yyerror(errormsg);
			free(id);
			return -1;
		}
		free(id);
		if (!ebitmap_set_bit(&ted->exclude, cladatum->value - 1,
				     TRUE)) {
			yyerror("out of memory");
			return -1;
		}
	}

	return 0;
}


static int define_te_avtab(int which)
{
	char *id, *tclassid, *stype, *ttype;
	avtab_key_t avkey;
	avtab_datum_t avdatum, *avdatump;
	class_datum_t *cladatum;
	type_datum_t *typdatum;
	perm_datum_t *perdatum;
	ebitmap_t classmap;
	int ret, i;


	stype = (char *) queue_remove(id_queue);
	if (!stype) {
		yyerror("no source type name for permission definition?");
		return -1;
	}
	typdatum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						   (hashtab_key_t) stype);
	if (!typdatum) {
		sprintf(errormsg, "unknown type %s used in permission definition", stype);
		yyerror(errormsg);
		free(stype);
		return -1;
	}
	avkey.source_type = typdatum->value;

	ttype = (char *) queue_remove(id_queue);
	if (!ttype) {
		yyerror("no target type name for permission definition?");
		free(stype);
		return -1;
	}
	typdatum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						   (hashtab_key_t) ttype);
	if (!typdatum) {
		sprintf(errormsg, "unknown type %s used in permission definition", ttype);
		yyerror(errormsg);
		free(ttype);
		free(stype);
		return -1;
	}
	avkey.target_type = typdatum->value;

	ebitmap_init(&classmap);
	while ((tclassid = queue_remove(id_queue))) {
		cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table,
					       (hashtab_key_t) tclassid);
		if (!cladatum) {
			sprintf(errormsg, "unknown class %s used in permission definition", tclassid);
			yyerror(errormsg);
			ebitmap_destroy(&classmap);
			free(tclassid);
			free(ttype);
			free(stype);
			return -1;
		}
		if (!ebitmap_set_bit(&classmap, cladatum->value - 1, TRUE)) {
			yyerror("out of memory");
			ebitmap_destroy(&classmap);
			free(tclassid);
			free(ttype);
			free(stype);
			return -1;
		}
		avkey.target_class = cladatum->value;

		avdatump = avtab_search(&policydbp->te_avtab, &avkey);
		if (avdatump) {
			if (avdatump->specified & which) {
				sprintf(errormsg, "duplicate rule for (%s,%s:%s)",
					stype, ttype, tclassid);
				ebitmap_destroy(&classmap);
				yyerror(errormsg);
				free(tclassid);
				free(ttype);
				free(stype);
				return -1;
			}
			avdatump->specified |= which;
		} else {
			memset(&avdatum, 0, sizeof avdatum);
			avdatum.specified |= which;
			ret = avtab_insert(&policydbp->te_avtab, &avkey, &avdatum);
			if (ret) {
				ebitmap_destroy(&classmap);
				yyerror("hash table overflow");
				free(tclassid);
				free(ttype);
				free(stype);
				return -1;
			}
		}

		free(tclassid);
	}

	free(ttype);
	free(stype);

	while ((id = queue_remove(id_queue))) {
		for (i = 0; i < ebitmap_length(&classmap); i++) {
			if (ebitmap_get_bit(&classmap, i)) {
				cladatum = policydbp->class_val_to_struct[i];
				avkey.target_class = i + 1;
				avdatump = avtab_search(&policydbp->te_avtab, &avkey);

				perdatum = (perm_datum_t *) hashtab_search(cladatum->permissions.table,
						     (hashtab_key_t) id);
				if (!perdatum) {
					if (cladatum->comdatum) {
						perdatum = (perm_datum_t *) hashtab_search(cladatum->comdatum->permissions.table,
						     (hashtab_key_t) id);
					}
					if (!perdatum) {
						sprintf(errormsg, "permission %s is not defined for class", id);
						yyerror(errormsg);
						free(id);
						continue;
					}
				}
				switch (which) {
				case AVTAB_ALLOWED:
					avdatump->allowed |= (1 << (perdatum->value - 1));
					break;
#ifdef CONFIG_FLASK_AUDIT
				case AVTAB_AUDITALLOW:
					avdatump->auditallow |= (1 << (perdatum->value - 1));
					break;
				case AVTAB_AUDITDENY:
					avdatump->auditdeny |= (1 << (perdatum->value - 1));
					break;
#endif
#ifdef CONFIG_FLASK_NOTIFY
				case AVTAB_NOTIFY:
					avdatump->notify |= (1 << (perdatum->value - 1));
					break;
#endif
				}
			}
		}

		free(id);
	}

	ebitmap_destroy(&classmap);

	return 0;
}


static int define_role_types(void)
{
	role_datum_t *role;
	char *role_id, *id;
	type_datum_t *typdatum;
	int ret;

	role_id = queue_remove(id_queue);

	role = (role_datum_t *) malloc(sizeof(role_datum_t));
	if (!role) {
		yyerror("out of memory");
		free(role_id);
		return -1;
	}
	memset(role, 0, sizeof(role_datum_t));
	ebitmap_init(&role->dominates);
	ebitmap_init(&role->types);
	role->value = ++policydbp->p_roles.nprim;

	while ((id = queue_remove(id_queue))) {
		typdatum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						     (hashtab_key_t) id);
		if (!typdatum) {
			sprintf(errormsg, "unknown type %s used in role types definition", id);
			yyerror(errormsg);
			free(id);
			continue;
		}
		if (!ebitmap_set_bit(&role->types, typdatum->value - 1, TRUE)) {
			yyerror("out of memory");
			free(id);
			free(role_id);
			return -1;
		}
		/* no need to keep type name */
		free(id);
	}

	ret = hashtab_insert(policydbp->p_roles.table,
			(hashtab_key_t) role_id, (hashtab_datum_t) role);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_roles.nprim;
		free(role);
		sprintf(errormsg, "duplicate definition for role %s", role_id);
		yyerror(errormsg);
		free(role_id);
		return -1;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		free(role);
		free(id);
		free(role_id);
		return -1;
	}
	return 0;
}


static role_datum_t *
 merge_roles_dom(role_datum_t * r1, role_datum_t * r2)
{
	role_datum_t *new;

	new = malloc(sizeof(role_datum_t));
	if (!new) {
		yyerror("out of memory");
		return NULL;
	}
	memset(new, 0, sizeof(role_datum_t));
	new->value = 0;		/* temporary role */
	if (!ebitmap_or(&new->dominates, &r1->dominates, &r2->dominates)) {
		free(new);
		yyerror("out of memory");
		new = NULL;
	}
	if (!r1->value) {
		/* free intermediate result */
		ebitmap_destroy(&r1->dominates);
		free(r1);
	}
	if (!r2->value) {
		/* free intermediate result */
		yyerror("right hand role is temporary?");
		ebitmap_destroy(&r2->dominates);
		free(r2);
	}
	return new;
}


static role_datum_t *
 define_role_dom(role_datum_t * r)
{
	role_datum_t *role;
	char *role_id;


	role_id = queue_remove(id_queue);
	role = (role_datum_t *) hashtab_search(policydbp->p_roles.table,
					       role_id);
	if (!role) {
		sprintf(errormsg, "no types definition for role %s", role_id);
		yyerror(errormsg);
		free(role_id);
		return NULL;
	}
	if (r) {
		if (!ebitmap_cpy(&role->dominates, &r->dominates)) {
			yyerror("out of memory");
			free(role_id);
			return NULL;
		}
		if (!r->value) {
			/* free intermediate result */
			ebitmap_destroy(&r->dominates);
			free(r);
		}
	}
	if (!ebitmap_set_bit(&role->dominates, role->value - 1, TRUE)) {
		yyerror("out of memory");
		free(role_id);
		return NULL;
	}
	return role;
}


static int define_constraint(constraint_expr_t * expr)
{
	struct constraint_node *node;
	char *id;
	class_datum_t *cladatum;
	perm_datum_t *perdatum;
	ebitmap_t classmap;
	int i;


	ebitmap_init(&classmap);
	while ((id = queue_remove(id_queue))) {
		cladatum = (class_datum_t *) hashtab_search(policydbp->p_classes.table,
						     (hashtab_key_t) id);
		if (!cladatum) {
			sprintf(errormsg, "class %s is not defined", id);
			ebitmap_destroy(&classmap);
			yyerror(errormsg);
			free(id);
			constraint_expr_destroy(expr);
			return -1;
		}
		if (!ebitmap_set_bit(&classmap, cladatum->value - 1, TRUE)) {
			yyerror("out of memory");
			ebitmap_destroy(&classmap);
			free(id);
			constraint_expr_destroy(expr);
			return -1;
		}
		node = malloc(sizeof(struct constraint_node));
		if (!node) {
			yyerror("out of memory");
			constraint_expr_destroy(expr);
			return -1;
		}
		memset(node, 0, sizeof(constraint_node_t));
		node->expr = expr;
		expr->count++;
		node->permissions = 0;

		node->next = cladatum->constraints;
		cladatum->constraints = node;

		free(id);
	}

	expr->count--;

	while ((id = queue_remove(id_queue))) {
		for (i = 0; i < ebitmap_length(&classmap); i++) {
			if (ebitmap_get_bit(&classmap, i)) {
				cladatum = policydbp->class_val_to_struct[i];
				node = cladatum->constraints;

				perdatum = (perm_datum_t *) hashtab_search(cladatum->permissions.table,
						     (hashtab_key_t) id);
				if (!perdatum) {
					if (cladatum->comdatum) {
						perdatum = (perm_datum_t *) hashtab_search(cladatum->comdatum->permissions.table,
						     (hashtab_key_t) id);
					}
					if (!perdatum) {
						sprintf(errormsg, "permission %s is not defined", id);
						yyerror(errormsg);
						free(id);
						ebitmap_destroy(&classmap);
						return -1;
					}
				}
				node->permissions |= (1 << (perdatum->value - 1));
			}
		}
		free(id);
	}

	ebitmap_destroy(&classmap);

	return 0;
}


static constraint_expr_t *
 define_constraint_expr(constraint_expr_t * left,
			constraint_expr_t * right,
			unsigned expr_type)
{
	struct constraint_expr *expr;
	type_datum_t *typdatum;
	role_datum_t *role;
	char *id;

	expr = malloc(sizeof(struct constraint_expr));
	if (!expr) {
		yyerror("out of memory");
		return NULL;
	}
	memset(expr, 0, sizeof(constraint_expr_t));
	expr->left = left;
	expr->right = right;
	expr->expr_type = expr_type;
	expr->relation = 0;
	expr->count = 1;
	ebitmap_init(&expr->bitmap);

	switch (expr_type) {
	case CONSTRAINT_EXPR_TYPE_ROLE_RELATION:
		id = (char *) queue_remove(id_queue);
		if (!id) {
			yyerror("no id for relation?");
			free(expr);
			return NULL;
		}
		if (!strcmp(id, "dom"))
			expr->relation = CONSTRAINT_EXPR_VALUE_RELATION_DOM;
		else if (!strcmp(id, "domby"))
			expr->relation = CONSTRAINT_EXPR_VALUE_RELATION_DOMBY;
		else if (!strcmp(id, "eq"))
			expr->relation = CONSTRAINT_EXPR_VALUE_RELATION_EQ;
		else if (!strcmp(id, "incomp"))
			expr->relation = CONSTRAINT_EXPR_VALUE_RELATION_INCOMP;
		else {
			sprintf(errormsg, "unknown relation %s", id);
			yyerror(errormsg);
			free(expr);
			return NULL;
		}
		free(id);
		break;
	case CONSTRAINT_EXPR_TYPE_TYPE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_TYPE_TARGET:
		while ((id = (char *) queue_remove(id_queue))) {
			typdatum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						     (hashtab_key_t) id);
			if (!typdatum) {
				sprintf(errormsg, "unknown type %s", id);
				yyerror(errormsg);
				ebitmap_destroy(&expr->bitmap);
				free(expr);
				return NULL;
			}
			if (!ebitmap_set_bit(&expr->bitmap, typdatum->value - 1, TRUE)) {
				yyerror("out of memory");
				ebitmap_destroy(&expr->bitmap);
				free(expr);
				return NULL;
			}
			free(id);
		}
		break;
	case CONSTRAINT_EXPR_TYPE_ROLE_SOURCE:
	case CONSTRAINT_EXPR_TYPE_ROLE_TARGET:
		while ((id = (char *) queue_remove(id_queue))) {
			role = (role_datum_t *) hashtab_search(policydbp->p_roles.table,
						     (hashtab_key_t) id);
			if (!role) {
				sprintf(errormsg, "unknown role %s", id);
				yyerror(errormsg);
				ebitmap_destroy(&expr->bitmap);
				free(expr);
				return NULL;
			}
			if (!ebitmap_set_bit(&expr->bitmap, role->value - 1, TRUE)) {
				yyerror("out of memory");
				ebitmap_destroy(&expr->bitmap);
				free(expr);
				return NULL;
			}
			free(id);
		}
		break;
	}

	return expr;
}


static int define_user(void)
{
	char *id;
	user_datum_t *usrdatum;
	role_datum_t *role;
	int ret;
#ifdef CONFIG_FLASK_MLS
	mls_range_list_t *rnode;
	level_datum_t *levdatum;
	cat_datum_t *catdatum;
	int relation, l;
	char *levid;
#endif

	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no user name for user definition?");
		return -1;
	}
	usrdatum = (user_datum_t *) malloc(sizeof(user_datum_t));
	if (!usrdatum) {
		yyerror("out of memory");
		free(id);
		return -1;
	}
	memset(usrdatum, 0, sizeof(user_datum_t));
	usrdatum->value = ++policydbp->p_users.nprim;
	ebitmap_init(&usrdatum->roles);
	ret = hashtab_insert(policydbp->p_users.table,
			 (hashtab_key_t) id, (hashtab_datum_t) usrdatum);

	if (ret == HASHTAB_PRESENT) {
		--policydbp->p_users.nprim;
		free(usrdatum);
		free(id);
		yyerror("duplicate user definition");
		return -1;
	}
	if (ret == HASHTAB_OVERFLOW) {
		yyerror("hash table overflow");
		free(usrdatum);
		free(id);
		return -1;
	}
	while ((id = queue_remove(id_queue))) {
		role = (role_datum_t *) hashtab_search(policydbp->p_roles.table,
						     (hashtab_key_t) id);
		if (!role) {
			sprintf(errormsg, "unknown role %s used in user role definition", id
			    );
			yyerror(errormsg);
			free(id);
			continue;
		}
		if (!ebitmap_set_bit(&usrdatum->roles, role->value - 1, TRUE)) {
			yyerror("out of memory");
			free(id);
			return -1;
		}
		/* no need to keep role name */
		free(id);
	}

#ifdef CONFIG_FLASK_MLS
	while ((id = queue_remove(id_queue))) {
		rnode = (mls_range_list_t *) malloc(sizeof(mls_range_list_t));
		if (!rnode) {
			yyerror("out of memory");
			free(id);
			return -1;
		}
		memset(rnode, 0, sizeof(mls_range_list_t));

		for (l = 0; l < 2; l++) {
			levdatum = (level_datum_t *) hashtab_search(policydbp->p_levels.table,
						     (hashtab_key_t) id);
			if (!levdatum) {
				sprintf(errormsg, "unknown sensitivity %s used in user range definition", id);
				yyerror(errormsg);
				free(rnode);
				free(id);
				continue;
			}
			rnode->range.level[l].sens = levdatum->level->sens;
			ebitmap_init(&rnode->range.level[l].cat);

			levid = id;

			while ((id = queue_remove(id_queue))) {
				catdatum = (cat_datum_t *) hashtab_search(policydbp->p_cats.table,
						     (hashtab_key_t) id);
				if (!catdatum) {
					sprintf(errormsg, "unknown category %s used in user range definition", id);
					yyerror(errormsg);
					free(id);
					continue;
				}
				if (!(ebitmap_get_bit(&levdatum->level->cat, catdatum->value - 1))) {
					sprintf(errormsg, "category %s cannot be associated with level %s", id, levid);
					yyerror(errormsg);
					free(id);
					continue;
				}
				if (!ebitmap_set_bit(&rnode->range.level[l].cat, catdatum->value - 1, TRUE)) {
					yyerror("out of memory");
					free(id);
					free(levid);
					ebitmap_destroy(&rnode->range.level[l].cat);
					free(rnode);
					return -1;
				}

				/*
				 * no need to keep category name
				 */
				free(id);
			}

			/*
			 * no need to keep sensitivity name
			 */
			free(levid);

			id = queue_remove(id_queue);
			if (!id)
				break;
		}

		if (l == 0) {
			rnode->range.level[1].sens = rnode->range.level[0].sens;
			if (!ebitmap_cpy(&rnode->range.level[1].cat, &rnode->range.level[0].cat)) {
				yyerror("out of memory");
				free(id);
				ebitmap_destroy(&rnode->range.level[0].cat);
				free(rnode);
				return -1;
			}
		}
		relation = mls_level_relation(rnode->range.level[1], rnode->range.level[0]);
		if (!(relation & (MLS_RELATION_DOM | MLS_RELATION_EQ))) {
			/* high does not dominate low */
			yyerror("high does not dominate low");
			ebitmap_destroy(&rnode->range.level[0].cat);
			ebitmap_destroy(&rnode->range.level[1].cat);
			free(rnode);
			return -1;
		}
		rnode->next = usrdatum->ranges;
		usrdatum->ranges = rnode;
	}
#endif

	return 0;
}


static int parse_security_context(context_struct_t * c)
{
	char *id;
	role_datum_t *role;
	type_datum_t *typdatum;
	user_datum_t *usrdatum;
#ifdef CONFIG_FLASK_MLS
	char *levid;
	level_datum_t *levdatum;
	cat_datum_t *catdatum;
	int l;
#endif

	context_init(c);

	/* extract the user */
	id = queue_remove(id_queue);
	if (!id) {
		yyerror("no effective user?");
		goto bad;
	}
	usrdatum = (user_datum_t *) hashtab_search(policydbp->p_users.table,
						   (hashtab_key_t) id);
	if (!usrdatum) {
		sprintf(errormsg, "user %s is not defined", id);
		yyerror(errormsg);
		free(id);
		goto bad;
	}
	c->user = usrdatum->value;

	/* no need to keep the user name */
	free(id);

	/* extract the role */
	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no role name for sid context definition?");
		return -1;
	}
	role = (role_datum_t *) hashtab_search(policydbp->p_roles.table,
					       (hashtab_key_t) id);
	if (!role) {
		sprintf(errormsg, "role %s is not defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	c->role = role->value;

	/* no need to keep the role name */
	free(id);


	/* extract the type */
	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no type name for sid context definition?");
		return -1;
	}
	typdatum = (type_datum_t *) hashtab_search(policydbp->p_types.table,
						   (hashtab_key_t) id);
	if (!typdatum) {
		sprintf(errormsg, "type %s is not defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	c->type = typdatum->value;

	/* no need to keep the type name */
	free(id);

#ifdef CONFIG_FLASK_MLS
	/* extract the low sensitivity */
	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no sensitivity name for sid context definition?");
		return -1;
	}
	for (l = 0; l < 2; l++) {
		levdatum = (level_datum_t *) hashtab_search(policydbp->p_levels.table,
						     (hashtab_key_t) id);
		if (!levdatum) {
			sprintf(errormsg, "Sensitivity %s is not defined", id);
			yyerror(errormsg);
			free(id);
			return -1;
		}
		c->range.level[l].sens = levdatum->level->sens;

		/* extract low category set */
		levid = id;
		while ((id = queue_remove(id_queue))) {
			catdatum = (cat_datum_t *) hashtab_search(policydbp->p_cats.table,
						     (hashtab_key_t) id);
			if (!catdatum) {
				sprintf(errormsg, "unknown category %s used in initial sid context", id);
				yyerror(errormsg);
				free(levid);
				free(id);
				goto bad;
			}
			if (!ebitmap_set_bit(&c->range.level[l].cat,
					     catdatum->value - 1, TRUE)) {
				yyerror("out of memory");
				free(levid);
				free(id);
				goto bad;
			}
			/* no need to keep category name */
			free(id);
		}

		/* no need to keep the sensitivity name */
		free(levid);

		/* extract high sensitivity */
		id = (char *) queue_remove(id_queue);
		if (!id)
			break;
	}

	if (l == 0) {
		c->range.level[1].sens = c->range.level[0].sens;
		if (!ebitmap_cpy(&c->range.level[1].cat, &c->range.level[0].cat)) {

			yyerror("out of memory");
			goto bad;
		}
	}
#endif

	if (!policydb_context_isvalid(policydbp, c)) {
		yyerror("invalid security context");
		goto bad;
	}
	return 0;

      bad:
	context_destroy(c);

	return -1;
}


static int define_initial_sid_context(void)
{
	char *id;
	ocontext_t *c, *head;

	id = (char *) queue_remove(id_queue);
	if (!id) {
		yyerror("no sid name for SID context definition?");
		return -1;
	}
	head = policydbp->ocontexts[OCON_ISID];
	for (c = head; c; c = c->next) {
		if (!strcmp(id, c->u.name))
			break;
	}

	if (!c) {
		sprintf(errormsg, "SID %s is not defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	if (c->context[0].user) {
		sprintf(errormsg, "The context for SID %s is multiply defined", id);
		yyerror(errormsg);
		free(id);
		return -1;
	}
	/* no need to keep the sid name */
	free(id);

	if (parse_security_context(&c->context[0]))
		return -1;

	return 0;
}

static int define_fs_context(int major, int minor)
{
	ocontext_t *newc, *c, *head;

	newc = (ocontext_t *) malloc(sizeof(ocontext_t));
	if (!newc) {
		yyerror("out of memory");
		return -1;
	}
	memset(newc, 0, sizeof(ocontext_t));

	newc->u.name = (char *) malloc(6);
	if (!newc->u.name) {
		yyerror("out of memory");
		free(newc);
		return -1;
	}
	sprintf(newc->u.name, "%02x:%02x", major, minor);

	if (parse_security_context(&newc->context[0])) {
		free(newc->u.name);
		free(newc);
		return -1;
	}
	if (parse_security_context(&newc->context[1])) {
		context_destroy(&newc->context[0]);
		free(newc->u.name);
		free(newc);
		return -1;
	}
	head = policydbp->ocontexts[OCON_FS];

	for (c = head; c; c = c->next) {
		if (!strcmp(newc->u.name, c->u.name)) {
			sprintf(errormsg, "duplicate entry for file system %s\n", newc->u.name);
			yyerror(errormsg);
			context_destroy(&newc->context[0]);
			context_destroy(&newc->context[1]);
			free(newc->u.name);
			free(newc);
			return -1;
		}
	}

	newc->next = head;
	policydbp->ocontexts[OCON_FS] = newc;

	return 0;
}

static int define_port_context(int low, int high)
{
	ocontext_t *newc;
	char *id;

	newc = malloc(sizeof(ocontext_t));
	if (!newc) {
		yyerror("out of memory");
		return -1;
	}
	memset(newc, 0, sizeof(ocontext_t));

	id = (char *) queue_remove(id_queue);
	if (!id) {
		free(newc);
		return -1;
	}
	if ((strcmp(id, "tcp") == 0) || (strcmp(id, "TCP") == 0)) {
		newc->u.port.protocol = IPPROTO_TCP;
	} else if ((strcmp(id, "udp") == 0) || (strcmp(id, "UDP") == 0)) {
		newc->u.port.protocol = IPPROTO_UDP;
	} else {
		sprintf(errormsg, "unrecognized protocol %s", id);
		yyerror(errormsg);
		free(newc);
		return -1;
	}

	newc->u.port.low_port = low;
	newc->u.port.high_port = high;

	if (parse_security_context(&newc->context[0])) {
		free(newc);
		return -1;
	}
	newc->next = policydbp->ocontexts[OCON_PORT];
	policydbp->ocontexts[OCON_PORT] = newc;
	return 0;
}

static int define_netif_context(void)
{
	ocontext_t *newc, *c, *head;

	newc = (ocontext_t *) malloc(sizeof(ocontext_t));
	if (!newc) {
		yyerror("out of memory");
		return -1;
	}
	memset(newc, 0, sizeof(ocontext_t));

	newc->u.name = (char *) queue_remove(id_queue);
	if (!newc->u.name) {
		free(newc);
		return -1;
	}
	if (parse_security_context(&newc->context[0])) {
		free(newc->u.name);
		free(newc);
		return -1;
	}
	if (parse_security_context(&newc->context[1])) {
		context_destroy(&newc->context[0]);
		free(newc->u.name);
		free(newc);
		return -1;
	}
	head = policydbp->ocontexts[OCON_NETIF];

	for (c = head; c; c = c->next) {
		if (!strcmp(newc->u.name, c->u.name)) {
			sprintf(errormsg, "duplicate entry for network interface %s\n", newc->u.name);
			yyerror(errormsg);
			context_destroy(&newc->context[0]);
			context_destroy(&newc->context[1]);
			free(newc->u.name);
			free(newc);
			return -1;
		}
	}

	newc->next = head;
	policydbp->ocontexts[OCON_NETIF] = newc;
	return 0;
}

static int define_node_context(int addr, int mask)
{
	ocontext_t *newc, *c, *l, *head;

	newc = malloc(sizeof(ocontext_t));
	if (!newc) {
		yyerror("out of memory");
		return -1;
	}
	memset(newc, 0, sizeof(ocontext_t));

	newc->u.node.addr = addr;
	newc->u.node.mask = mask;

	if (parse_security_context(&newc->context[0])) {
		free(newc);
		return -1;
	}
	/* Place this at the end of the list, to retain
	   the matching order specified in the configuration. */
	head = policydbp->ocontexts[OCON_NODE];
	for (l = NULL, c = head; c; l = c, c = c->next);

	if (l)
		l->next = newc;
	else
		policydbp->ocontexts[OCON_NODE] = newc;

	return 0;
}

/* FLASK */

