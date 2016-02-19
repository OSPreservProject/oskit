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
 * Implementation of the security services.
 */

#include "policydb.h"
#include "services.h"
#include "sidtab.h"
#ifdef CONFIG_FLASK_MLS
#include "mls.h"
#endif

#ifdef __KERNEL__
static spinlock_t ss_lock = SPIN_LOCK_UNLOCKED;
#endif

sidtab_t sidtab;
policydb_t policydb;
int ss_initialized = 0;

/*
 * The largest sequence number that has been used when
 * providing an access decision to the access vector cache.
 * The sequence number only changes when a policy change
 * occurs.
 */
static __u32 latest_granting = 0;


/*
 * Return the boolean value of a constraint expression 
 * when it is applied to the specified source and target 
 * security contexts.
 */
static int constraint_expr_eval(context_struct_t * scontext,
				context_struct_t * tcontext,
				constraint_expr_t * expr)
{
	role_datum_t *srole, *trole;

	switch (expr->expr_type) {
	case CONSTRAINT_EXPR_TYPE_NOT:
		return !constraint_expr_eval(scontext, tcontext, expr->left);
	case CONSTRAINT_EXPR_TYPE_AND:
		return constraint_expr_eval(scontext, tcontext, expr->left) &&
		    constraint_expr_eval(scontext, tcontext, expr->right);
	case CONSTRAINT_EXPR_TYPE_OR:
		return constraint_expr_eval(scontext, tcontext, expr->left) ||
		    constraint_expr_eval(scontext, tcontext, expr->right);
	case CONSTRAINT_EXPR_TYPE_SAMEUSER:
		return (scontext->user == tcontext->user);
	case CONSTRAINT_EXPR_TYPE_TYPE_SOURCE:
		return ebitmap_get_bit(&expr->bitmap, scontext->type - 1);
	case CONSTRAINT_EXPR_TYPE_TYPE_TARGET:
		return ebitmap_get_bit(&expr->bitmap, tcontext->type - 1);
	case CONSTRAINT_EXPR_TYPE_ROLE_SOURCE:
		return ebitmap_get_bit(&expr->bitmap, scontext->role - 1);
	case CONSTRAINT_EXPR_TYPE_ROLE_TARGET:
		return ebitmap_get_bit(&expr->bitmap, tcontext->role - 1);
	case CONSTRAINT_EXPR_TYPE_ROLE_RELATION:
		srole = policydb.role_val_to_struct[scontext->role - 1];
		trole = policydb.role_val_to_struct[tcontext->role - 1];
		switch (expr->relation) {
		case CONSTRAINT_EXPR_VALUE_RELATION_DOM:
			return (scontext->role != tcontext->role) &&
			    ebitmap_get_bit(&srole->dominates,
					    tcontext->role - 1);
		case CONSTRAINT_EXPR_VALUE_RELATION_DOMBY:
			return (scontext->role != tcontext->role) &&
			    ebitmap_get_bit(&trole->dominates,
					    scontext->role - 1);
		case CONSTRAINT_EXPR_VALUE_RELATION_EQ:
			return (scontext->role == tcontext->role);
		case CONSTRAINT_EXPR_VALUE_RELATION_INCOMP:
			return (scontext->role != tcontext->role) &&
			    !ebitmap_get_bit(&srole->dominates,
					     tcontext->role - 1) &&
			    !ebitmap_get_bit(&trole->dominates,
					     scontext->role - 1);
		}
		break;
	}

	return FALSE;
}


/*
 * Set an access vector to the appropriate default.
 */
#define set_te_def(_var,_tclass,_self,_notself) \
	do { \
	        te_def_t *d = &policydb.p_te_def_##_var; \
                *(_var) = 0; \
		if (!ebitmap_get_bit(&d->exclude, _tclass - 1)) { \
			switch (d->value) { \
			case TE_DEF_ALL: \
				*(_var) = 0xffffffff; \
				break; \
			case TE_DEF_SELF: \
				if (scontext->type == tcontext->type) \
					*(_var) = _self; \
				else \
 					*(_var) = _notself; \
				break; \
			} \
		} \
	} while (0)


/*
 * Compute access vectors based on a SID pair for
 * the permissions in a particular class.
 */
static int unlocked_security_compute_av(security_id_t ssid,
					security_id_t tsid,
					security_class_t tclass,
					access_vector_t requested,
					access_vector_t * allowed,
					access_vector_t * decided,
#ifdef CONFIG_FLASK_AUDIT
					access_vector_t * auditallow,
					access_vector_t * auditdeny,
#endif
#ifdef CONFIG_FLASK_NOTIFY
					access_vector_t * notify,
#endif
					__u32 *seqno)
{
	context_struct_t *scontext = 0, *tcontext = 0;
	constraint_node_t *constraint;
	avtab_key_t avkey;
	avtab_datum_t *avdatum;
	class_datum_t *tclass_datum;

	*seqno = latest_granting;
	*decided = 0xffffffff;

	if (!ss_initialized) {
		*allowed = requested;
		*decided = requested;
#ifdef CONFIG_FLASK_AUDIT
		*auditallow = 0;
		*auditdeny = 0xffffffff;
#endif
#ifdef CONFIG_FLASK_NOTIFY
		*notify = 0;
#endif
		return 0;
	}
	scontext = sidtab_search(&sidtab, ssid);
	if (!scontext) {
		printf("security_compute_av:  unrecognized SID %d\n", ssid);
		return -EINVAL;
	}
	tcontext = sidtab_search(&sidtab, tsid);
	if (!tcontext) {
		printf("security_compute_av:  unrecognized SID %d\n", tsid);
		return -EINVAL;
	}
	if (!tclass || tclass > policydb.p_classes.nprim) {
		printf("security_compute_av:  unrecognized class %d\n",
		       tclass);
		return -EINVAL;
	}
	tclass_datum = policydb.class_val_to_struct[tclass - 1];

	/* 
	 * Initialize the access vectors to the default values.
	 */
	set_te_def(allowed, tclass, 0xffffffff, 0);
#ifdef CONFIG_FLASK_AUDIT
	set_te_def(auditallow, tclass, 0, 0xffffffff);
	set_te_def(auditdeny, tclass, 0, 0xffffffff);
#endif
#ifdef CONFIG_FLASK_NOTIFY
	set_te_def(notify, tclass, 0, 0xffffffff);
#endif

	/*
	 * If a specific type enforcement rule was defined for
	 * this permission check, then use it.
	 */
	avkey.source_type = scontext->type;
	avkey.target_type = tcontext->type;
	avkey.target_class = tclass;
	avdatum = avtab_search(&policydb.te_avtab, &avkey);
	if (avdatum) {
		if (avdatum->specified & AVTAB_ALLOWED)
			*allowed = avdatum->allowed;
#ifdef CONFIG_FLASK_AUDIT
		if (avdatum->specified & AVTAB_AUDITALLOW)
			*auditallow = avdatum->auditallow;
		if (avdatum->specified & AVTAB_AUDITDENY)
			*auditdeny = avdatum->auditdeny;
#endif
#ifdef CONFIG_FLASK_NOTIFY
		if (avdatum->specified & AVTAB_NOTIFY)
			*notify = avdatum->notify;
#endif
	}
#ifdef CONFIG_FLASK_MLS
	/*
	 * Remove any permissions prohibited by the MLS policy.
	 */
	mls_compute_av(scontext, tcontext, tclass_datum, allowed);
#endif

	/* 
	 * Remove any permissions prohibited by a constraint.
	 */
	constraint = tclass_datum->constraints;
	while (constraint) {
		if ((constraint->permissions & (*allowed)) &&
		    !constraint_expr_eval(scontext, tcontext,
					  constraint->expr)) {
			*allowed = (*allowed) & ~(constraint->permissions);
		}
		constraint = constraint->next;
	}

	return 0;
}


int security_compute_av(security_id_t ssid,
			security_id_t tsid,
			security_class_t tclass,
			access_vector_t requested,
			access_vector_t * allowed,
			access_vector_t * decided,
#ifdef CONFIG_FLASK_AUDIT
			access_vector_t * auditallow,
			access_vector_t * auditdeny,
#endif
#ifdef CONFIG_FLASK_NOTIFY
			access_vector_t * notify,
#endif
			__u32 *seqno)
{
	int rc = 0;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	rc = unlocked_security_compute_av(ssid, tsid, tclass,
					  requested, allowed, decided,
#ifdef CONFIG_FLASK_AUDIT
					  auditallow, auditdeny,
#endif
#ifdef CONFIG_FLASK_NOTIFY
					  notify,
#endif
					  seqno);

#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif
	return rc;
}


/*
 * Write the security context string representation of 
 * the context structure `context' into a dynamically
 * allocated string of the correct size.  Set `*scontext'
 * to point to this string and set `*scontext_len' to
 * the length of the string.
 */
static int context_struct_to_string(context_struct_t * context,
				    security_context_t * scontext,
				    __u32 *scontext_len)
{
	char *scontextp;

	*scontext = 0;
	*scontext_len = 0;

	/* Compute the size of the context. */
	*scontext_len += strlen(policydb.p_user_val_to_name[context->user - 1]) + 1;
	*scontext_len += strlen(policydb.p_role_val_to_name[context->role - 1]) + 1;
	*scontext_len += strlen(policydb.p_type_val_to_name[context->type - 1]) + 1;
#ifdef CONFIG_FLASK_MLS
	*scontext_len += mls_compute_context_len(context);
#endif

	/* Allocate space for the context; caller must free this space. */
	scontextp = (char *) malloc(*scontext_len);
	if (!scontextp) {
		return -ENOMEM;
	}
	*scontext = (security_context_t) scontextp;

	/*
	 * Copy the user name, role name and type name into the context.
	 */
	sprintf(scontextp, "%s:%s:%s:",
		policydb.p_user_val_to_name[context->user - 1],
		policydb.p_role_val_to_name[context->role - 1],
		policydb.p_type_val_to_name[context->type - 1]);
	scontextp += strlen(policydb.p_user_val_to_name[context->user - 1]) + 1 + strlen(policydb.p_role_val_to_name[context->role - 1]) + 1 + strlen(policydb.p_type_val_to_name[context->type - 1]) + 1;

#ifdef CONFIG_FLASK_MLS
	mls_sid_to_context(context, &scontextp);
#else
	scontextp--;
	*scontextp = 0;
#endif
	return 0;
}


#include <flask/initial_sid_to_string.h>

/*
 * Write the security context string representation of 
 * the context associated with `sid' into a dynamically
 * allocated string of the correct size.  Set `*scontext'
 * to point to this string and set `*scontext_len' to
 * the length of the string.
 */
static int unlocked_security_sid_to_context(security_id_t sid,
					    security_context_t * scontext,
					    __u32 *scontext_len)
{
	context_struct_t *context;

	if (!ss_initialized) {
		if (sid <= SECINITSID_NUM) {
			char *scontextp;

			*scontext_len = strlen(initial_sid_to_string[sid]) + 1;
			scontextp = malloc(*scontext_len);
			strcpy(scontextp, initial_sid_to_string[sid]);
			*scontext = (security_context_t) scontextp;
			return 0;
		}
		printf("security_sid_to_context:  called before initialization on SID %d\n", sid);
		exit(1);
	}
	context = sidtab_search(&sidtab, sid);
	if (!context) {
		printf("security_sid_to_context:  unrecognized SID %d\n", sid);
		return -EINVAL;
	}
	return context_struct_to_string(context, scontext, scontext_len);
}


int security_sid_to_context(security_id_t sid,
			    security_context_t * scontext,
			    __u32 *scontext_len)
{
	int rc = 0;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	rc = unlocked_security_sid_to_context(sid, scontext, scontext_len);

#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif
	return rc;
}


/*
 * Return a SID associated with the security context that
 * has the string representation specified by `scontext'.
 */
static int unlocked_security_context_to_sid(security_context_t scontext,
					    __u32 scontext_len,
					    security_id_t * sid)
{
	security_context_t scontext2;
	context_struct_t context;
	role_datum_t *role;
	type_datum_t *typdatum;
	user_datum_t *usrdatum;
	char *scontextp, *p, oldc;
	int rc;

	if (!ss_initialized) {
		int i;

		for (i = 1; i < SECINITSID_NUM; i++) {
			if (!strcmp(initial_sid_to_string[i], scontext)) {
				*sid = i;
				return 0;
			}
		}

		printf("context_to_sid: called before initialization\n");
		exit(1);
	}
	*sid = SECSID_NULL;

	/* copy the string so that we can modify the copy as we parse it */
	scontext2 = malloc(scontext_len);
	if (!scontext2) {
		return -ENOMEM;
	}
	memcpy(scontext2, scontext, scontext_len);

	context_init(&context);
	*sid = SECSID_NULL;

	/* Parse the security context. */

	rc = -EINVAL;
	scontextp = (char *) scontext2;
	if (scontextp[scontext_len - 1])
		/* Security context is not null-terminated. */
		goto out;

	/* Extract the user. */
	p = scontextp;
	while (*p && *p != ':')
		p++;

	if (*p == 0)
		goto out;

	*p++ = 0;

	usrdatum = (user_datum_t *) hashtab_search(policydb.p_users.table,
					      (hashtab_key_t) scontextp);
	if (!usrdatum)
		goto out;

	context.user = usrdatum->value;

	/* Extract role. */
	scontextp = p;
	while (*p && *p != ':')
		p++;

	if (*p == 0)
		goto out;

	*p++ = 0;

	role = (role_datum_t *) hashtab_search(policydb.p_roles.table,
					       (hashtab_key_t) scontextp);
	if (!role)
		goto out;
	context.role = role->value;

	/* Extract type. */
	scontextp = p;
	while (*p && *p != ':')
		p++;
	oldc = *p;
	*p++ = 0;

	typdatum = (type_datum_t *) hashtab_search(policydb.p_types.table,
					      (hashtab_key_t) scontextp);

	if (!typdatum)
		goto out;

	context.type = typdatum->value;

#ifdef CONFIG_FLASK_MLS
	if (!oldc)
		goto out;
	rc = mls_context_to_sid(&p, &context);
	if (rc)
		goto out;
#endif

	/* Check the validity of the new context. */
	if (!policydb_context_isvalid(&policydb, &context)) {
		rc = -EINVAL;
		goto out;
	}
	/* Obtain the new sid. */
	if (sidtab_context_to_sid(&sidtab, &context, sid)) {
		rc = -ENOMEM;
		goto out;
	}
	rc = 0;

      out:
	context_destroy(&context);
	free(scontext2);
	return rc;
}


int security_context_to_sid(security_context_t scontext,
			    __u32 scontext_len,
			    security_id_t * sid)
{
	int rc = 0;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	rc = unlocked_security_context_to_sid(scontext, scontext_len, sid);

#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif
	return rc;
}


/*
 * Compute a SID to use for labeling a new object in the 
 * class `tclass' based on a SID pair.  
 */
static int unlocked_security_transition_sid(security_id_t ssid,
					    security_id_t tsid,
					    security_class_t tclass,
					    security_id_t * out_sid)
{
	context_struct_t *scontext = 0, *tcontext = 0, newcontext;
	avtab_key_t avkey;
	avtab_datum_t *avdatum;
	unsigned int trans = 0;

	if (!ss_initialized) {
		goto default_transition;
	}
	scontext = sidtab_search(&sidtab, ssid);
	if (!scontext) {
		printf("security_transition_sid:  unrecognized SID %d\n", ssid);
		return -EINVAL;
	}
	tcontext = sidtab_search(&sidtab, tsid);
	if (!tcontext) {
		printf("security_transition_sid:  unrecognized SID %d\n", tsid);
		return -EINVAL;
	}
	context_init(&newcontext);

	avkey.source_type = scontext->type;
	avkey.target_type = tcontext->type;
	avkey.target_class = tclass;
	avdatum = avtab_search(&policydb.te_avtab, &avkey);
	trans = (avdatum && (avdatum->specified & AVTAB_TRANSITION));

	switch (tclass) {
	case SECCLASS_PROCESS:
		/* Inherit attribute default values from the process. */
		if (!trans) {
			/* No change in process SID */
			*out_sid = ssid;
			return 0;
		}
		if (context_cpy(&newcontext, scontext)) {
			return -ENOMEM;
		}
		break;
	case SECCLASS_SOCK_FILE:
	case SECCLASS_LNK_FILE:
	case SECCLASS_FILE:
	case SECCLASS_BLK_FILE:
	case SECCLASS_DIR:
	case SECCLASS_CHR_FILE:
	case SECCLASS_FIFO_FILE:
		/*
		 * Inherit most attribute default values from the parent
		 * directory.
		 */
		if (!trans && (scontext->user == tcontext->user)) {
			/* Simply use the parent directory SID. */
			*out_sid = tsid;
			return 0;
		}
		if (context_cpy(&newcontext, tcontext)) {
			return -ENOMEM;
		}
		/* Inherit the user identity from the creating process. */
		newcontext.user = scontext->user;
		break;
	default:
		printf("security_transition_sid:  unrecognized class %d\n", tclass);
		return -EINVAL;
	}

	if (trans) {
		/* Inherit type and role value from the type transition rule. */
		newcontext.type = avdatum->trans_type;
	}
	/* Check the validity of the transition context. */
	if (!policydb_context_isvalid(&policydb, &newcontext)) {
#ifdef CONFIG_FLASK_DEVELOP
		security_context_t s, t, n;
		__u32 slen, tlen, nlen;
#endif

		/*
		 * The transition context is invalid.  Fall back to
		 * the default.
		 */
#ifdef CONFIG_FLASK_DEVELOP
		context_struct_to_string(scontext, &s, &slen);
		context_struct_to_string(tcontext, &t, &tlen);
		context_struct_to_string(&newcontext, &n, &nlen);
		printf("security_transition_sid:  invalid transition context %s", n);
		printf(" for scontext=%s", s);
		printf(" tcontext=%s", t);
		printf(" tclass=%s\n", policydb.p_class_val_to_name[tclass]);
		free(s);
		free(t);
		free(n);
#endif
		context_destroy(&newcontext);
		goto default_transition;
	}
	/* Obtain the sid for the transition context. */
	if (sidtab_context_to_sid(&sidtab, &newcontext, out_sid)) {
		context_destroy(&newcontext);
		return -ENOMEM;
	}
	context_destroy(&newcontext);
	return 0;

      default_transition:
	switch (tclass) {
	case SECCLASS_PROCESS:
		*out_sid = ssid;
		break;
	case SECCLASS_SOCK_FILE:
	case SECCLASS_LNK_FILE:
	case SECCLASS_FILE:
	case SECCLASS_BLK_FILE:
	case SECCLASS_DIR:
	case SECCLASS_CHR_FILE:
	case SECCLASS_FIFO_FILE:
		*out_sid = tsid;
		break;
	default:
		printf("security_transition_sid:  unrecognized class %d\n", tclass);
		return -EINVAL;
	}
	return 0;
}


int security_transition_sid(security_id_t ssid,
			    security_id_t tsid,
			    security_class_t tclass,
			    security_id_t * out_sid)
{
	int rc;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	rc = unlocked_security_transition_sid(ssid, tsid, tclass, out_sid);

#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif
	return rc;
}


/*
 * Compute a SID to use when selecting a member of a 
 * polyinstantiated object of class `tclass' based on 
 * a SID pair.
 */
static int unlocked_security_member_sid(security_id_t ssid,
					security_id_t tsid,
					security_class_t tclass,
					security_id_t * out_sid)
{
#ifndef CONFIG_FLASK_MLS
	*out_sid = tsid;
	return 0;
#else
	context_struct_t *scontext = 0, *tcontext = 0, newcontext;

	if (!ss_initialized) {
		*out_sid = tsid;
		return 0;
	}
	scontext = sidtab_search(&sidtab, ssid);
	if (!scontext) {
		printf("security_member_sid:  unrecognized SID %d\n", ssid);
		return -EINVAL;
	}
	tcontext = sidtab_search(&sidtab, tsid);
	if (!tcontext) {
		printf("security_member_sid:  unrecognized SID %d\n", tsid);
		return -EINVAL;
	}
	/*
	 * Define a new security context, with the MLS range inherited from
	 * the source SID, but all other attributes inherited from the target
	 * SID.
	 */
	context_init(&newcontext);

	if (mls_member_sid(&newcontext, scontext))
		return -EINVAL;

	newcontext.role = tcontext->role;
	newcontext.type = tcontext->type;
	newcontext.user = tcontext->user;

	/* Check the validity of the new context. */
	if (!policydb_context_isvalid(&policydb, &newcontext)) {
		context_destroy(&newcontext);
		return -EINVAL;
	}
	/* Obtain the sid for the new context. */
	if (sidtab_context_to_sid(&sidtab, &newcontext, out_sid)) {
		context_destroy(&newcontext);
		return -ENOMEM;
	}
	context_destroy(&newcontext);

	return 0;
#endif
}


int security_member_sid(security_id_t ssid,
			security_id_t tsid,
			security_class_t tclass,
			security_id_t * out_sid)
{
	int rc;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	rc = unlocked_security_member_sid(ssid, tsid, tclass, out_sid);

#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif
	return rc;
}


/*
 * Verify that each permission that is defined under the
 * existing policy is still defined with the same value
 * in the new policy.
 */
static int validate_perm(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	hashtab_t h;
	perm_datum_t *perdatum, *perdatum2;


	h = (hashtab_t) p;
	perdatum = (perm_datum_t *) datum;

	perdatum2 = (perm_datum_t *) hashtab_search(h, key);
	if (!perdatum2) {
		printf("security:  permission %s disappeared", key);
		return -1;
	}
	if (perdatum->value != perdatum2->value) {
		printf("security:  the value of permission %s changed", key);
		return -1;
	}
	return 0;
}


/*
 * Verify that each class that is defined under the
 * existing policy is still defined with the same 
 * attributes in the new policy.
 */
static int validate_class(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	policydb_t *newp;
	class_datum_t *cladatum, *cladatum2;

	newp = (policydb_t *) p;
	cladatum = (class_datum_t *) datum;

	cladatum2 = (class_datum_t *) hashtab_search(newp->p_classes.table, key);
	if (!cladatum2) {
		printf("security:  class %s disappeared\n", key);
		return -1;
	}
	if (cladatum->value != cladatum2->value) {
		printf("security:  the value of class %s changed\n", key);
		return -1;
	}
	if ((cladatum->comdatum && !cladatum2->comdatum) ||
	    (!cladatum->comdatum && cladatum2->comdatum)) {
		printf("security:  the inherits clause for the access vector definition for class %s changed\n", key);
		return -1;
	}
	if (cladatum->comdatum) {
		if (hashtab_map(cladatum->comdatum->permissions.table, validate_perm,
				cladatum2->comdatum->permissions.table)) {
			printf(" in the access vector definition for class %s\n", key);
			return -1;
		}
	}
	if (hashtab_map(cladatum->permissions.table, validate_perm,
			cladatum2->permissions.table)) {
		printf(" in access vector definition for class %s\n", key);
		return -1;
	}
	return 0;
}


typedef struct {
	policydb_t *oldp;
	policydb_t *newp;
} convert_context_args_t;

/*
 * Convert the values in the security context
 * structure `c' from the values specified
 * in the policy `p->oldp' to the values specified
 * in the policy `p->newp'.  Verify that the
 * context is valid under the new policy.
 */
static int convert_context(security_id_t key,
			   context_struct_t * c,
			   void *p)
{
	convert_context_args_t *args;
	context_struct_t oldc;
	role_datum_t *role;
	type_datum_t *typdatum;
	user_datum_t *usrdatum;
	security_context_t s;
	__u32 len;
	int rc = -EINVAL;
       
	args = (convert_context_args_t *) p;

	if (context_cpy(&oldc, c))
		return -ENOMEM;

	/* Convert the user. */
	usrdatum = (user_datum_t *) hashtab_search(args->newp->p_users.table,
			    args->oldp->p_user_val_to_name[c->user - 1]);

	if (!usrdatum) {
		goto bad;
	}
	c->user = usrdatum->value;

	/* Convert the role. */
	role = (role_datum_t *) hashtab_search(args->newp->p_roles.table,
			    args->oldp->p_role_val_to_name[c->role - 1]);
	c->role = role->value;

	/* Convert the type. */
	typdatum = (type_datum_t *)
	    hashtab_search(args->newp->p_types.table,
			   args->oldp->p_type_val_to_name[c->type - 1]);
	if (!typdatum) {
		goto bad;
	}
	c->type = typdatum->value;

#ifdef CONFIG_FLASK_MLS
	rc = mls_convert_context(args->oldp, args->newp, c);
	if (rc)
		goto bad;
#endif

	/* Check the validity of the new context. */
	if (!policydb_context_isvalid(args->newp, c)) {
		rc = -EINVAL;
		goto bad;
	}

	return 0;

      bad:
	context_struct_to_string(&oldc, &s, &len);
	printf("security:  invalidating context %s\n", s);
	free(s);
	return rc;
}


/*
 * Read a new set of configuration data from 
 * a policy database binary representation file.
 *
 * Verify that each class that is defined under the
 * existing policy is still defined with the same 
 * attributes in the new policy.  
 *
 * Convert the context structures in the SID table to the
 * new representation and verify that all entries
 * in the SID table are valid under the new policy. 
 *
 * Change the active policy database to use the new 
 * configuration data.  
 *
 * Reset the access vector cache.
 */
int security_load_policy(FILE * fp)
{
	policydb_t newpolicydb;
	convert_context_args_t args;
	__u32 seqno;
	int rc;
#ifdef __KERNEL__
	unsigned long flags;
#endif

#ifdef __KERNEL__
	spin_lock_irqsave(&ss_lock, flags);
#endif

	if (policydb_read(&newpolicydb, fp)) {
		rc = -EINVAL;
		goto out;
	}
	/* Verify that the existing classes did not change. */
	if (hashtab_map(policydb.p_classes.table, validate_class, &newpolicydb)) {
		printf("security:  the definition of an existing class changed\n");
		policydb_destroy(&newpolicydb);
		rc = -EINVAL;
		goto out;
	}
	/* Convert the internal representations of contexts. */
	args.oldp = &policydb;
	args.newp = &newpolicydb;
	sidtab_map_remove_on_error(&sidtab, convert_context, &args);

	/* Copy the new policy into place. */
	policydb_destroy(&policydb);
	memcpy(&policydb, &newpolicydb, sizeof(struct policydb));

	seqno = ++latest_granting;

#ifdef __KERNEL__
	printf("security:  resetting access vector caches\n");
	spin_unlock_irqrestore(&ss_lock, flags);
	avc_ss_reset(seqno);
	return 0;
#else
	rc = 0;
#endif

      out:
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif

	return rc;
}


#ifdef CONFIG_FLASK_NOTIFY
/*
 * Notify the security server that an operation
 * associated with a previously granted permission 
 * has successfully completed.
 */
int security_notify_perm(security_id_t ssid,
			 security_id_t tsid,
			 security_class_t tclass,
			 access_vector_t requested)
{
	return 0;
}
#endif


/*
 * Return the SIDs to use for an unlabeled file system
 * that is being mounted from the device with the
 * the kdevname `name'.  The `fs_sid' SID is returned for 
 * the file system and the `file_sid' SID is returned
 * for all files within that file system.
 */
int security_fs_sid(char *name,
		    security_id_t * fs_sid,
		    security_id_t * file_sid)
{
	int rc = 0;
	ocontext_t *c;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	c = policydb.ocontexts[OCON_FS];
	while (c) {
		if (strcmp(c->u.name, name) == 0)
			break;
		c = c->next;
	}

	if (c) {
		if (!c->sid[0] || !c->sid[1]) {
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[0],
						  &c->sid[0])) {
				rc = -ENOMEM;
				goto out;
			}
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[1],
						  &c->sid[1])) {
				rc = -ENOMEM;
				goto out;
			}
		}
		*fs_sid = c->sid[0];
		*file_sid = c->sid[1];
	} else {
		*fs_sid = SECINITSID_FS;
		*file_sid = SECINITSID_FILE;
	}

      out:
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif

	return rc;
}


/*
 * Return the SID of the port specified by
 * `domain', `type', `protocol', and `port'.
 */
int security_port_sid(__u16 domain,
		      __u16 type,
		      __u8 protocol,
		      __u16 port,
		      security_id_t * out_sid)
{
	ocontext_t *c;
	int rc = 0;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	c = policydb.ocontexts[OCON_PORT];
	while (c) {
		if (c->u.port.protocol == protocol &&
		    c->u.port.low_port <= port &&
		    c->u.port.high_port >= port)
			break;
		c = c->next;
	}

	if (c) {
		if (!c->sid[0]) {
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[0],
						  &c->sid[0])) {
				rc = -ENOMEM;
				goto out;
			}
		}
		*out_sid = c->sid[0];
	} else {
		*out_sid = SECINITSID_PORT;
	}

      out:
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif

	return rc;
}


/*
 * Return the SIDs to use for a network interface
 * with the name `name'.  The `if_sid' SID is returned for 
 * the interface and the `msg_sid' SID is returned as 
 * the default SID for messages received on the
 * interface.
 */
int security_netif_sid(char *name,
		       security_id_t * if_sid,
		       security_id_t * msg_sid)
{
	int rc = 0;
	ocontext_t *c;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	c = policydb.ocontexts[OCON_NETIF];
	while (c) {
		if (strcmp(name, c->u.name) == 0)
			break;
		c = c->next;
	}

	if (c) {
		if (!c->sid[0] || !c->sid[1]) {
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[0],
						  &c->sid[0])) {
				rc = -ENOMEM;
				goto out;
			}
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[1],
						  &c->sid[1])) {
				rc = -ENOMEM;
				goto out;
			}
		}
		*if_sid = c->sid[0];
		*msg_sid = c->sid[1];
	} else {
		*if_sid = SECINITSID_NETIF;
		*msg_sid = SECINITSID_NETMSG;
	}

      out:
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif

	return rc;
}


/*
 * Return the SID of the node specified by the address
 * `addrp' where `addrlen' is the length of the address
 * in bytes and `domain' is the communications domain or
 * address family in which the address should be interpreted.
 */
int security_node_sid(__u16 domain,
		      void *addrp,
		      __u32 addrlen,
		      security_id_t *out_sid)
{
	int rc = 0;
	__u32 addr;
	ocontext_t *c;
#ifdef __KERNEL__
	unsigned long flags;

	spin_lock_irqsave(&ss_lock, flags);
#endif

	if (domain != AF_INET || addrlen != sizeof(__u32)) {
		*out_sid = SECINITSID_NODE;
		goto out;
	}
	addr = *((__u32 *)addrp);

	c = policydb.ocontexts[OCON_NODE];
	while (c) {
		if (c->u.node.addr == (addr & c->u.node.mask))
			break;
		c = c->next;
	}

	if (c) {
		if (!c->sid[0]) {
			if (sidtab_context_to_sid(&sidtab,
						  &c->context[0],
						  &c->sid[0])) {
				rc = -ENOMEM;
				goto out;
			}
		}
		*out_sid = c->sid[0];
	} else {
		*out_sid = SECINITSID_NODE;
	}

      out:
#ifdef __KERNEL__
	spin_unlock_irqrestore(&ss_lock, flags);
#endif

	return rc;
}

/* FLASK */
