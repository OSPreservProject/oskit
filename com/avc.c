/*
 * Copyright (c) 1999 The University of Utah and the Flux Group.
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

#include <oskit/c/string.h>
#include <oskit/dev/osenv.h>
#include <oskit/dev/osenv_mem.h>
#include <oskit/dev/osenv_log.h>
#include <oskit/flask/avc.h>
#include <oskit/flask/avc_ss.h>
#include <oskit/flask/security.h>
#include <flask/class_to_string.h>
#include <flask/common_perm_to_string.h>
#include <flask/av_inherit.h>
#include <flask/av_perm_to_string.h>

typedef struct avc_callback_node 
{
	struct oskit_avc_callback *callback;
	oskit_u32_t events;
	oskit_security_id_t ssid;
	oskit_security_id_t tsid;
	oskit_security_class_t tclass;
	oskit_access_vector_t perms;
	struct avc_callback_node *next;
} avc_callback_node_t;


typedef struct avc_node {
	struct oskit_avc_entry ae;
	struct avc_node *next;
}               avc_node_t;

#define AVC_CACHE_SLOTS 128
#define AVC_CACHE_MAXNODES 102


typedef struct {
	oskit_avc_t avci;	
	oskit_avc_ss_t avc_ssi;
	unsigned count;
	oskit_osenv_mem_t *mem;
	oskit_osenv_log_t *log;
	avc_callback_node_t *callbacks;
	avc_node_t     *slots[AVC_CACHE_SLOTS];
	avc_node_t     *freelist;
	unsigned int    lru_hint;	/* LRU hint for reclaim scan */
	unsigned int    activeNodes;
	oskit_u32_t	latest_notif;
}               avc_cache_t;


static struct oskit_avc_ops avc_ops;
static struct oskit_avc_ss_ops avc_ss_ops;

#ifndef NULL
#define NULL 0
#endif

#ifndef offsetof
#define	offsetof(type, member)	((oskit_size_t)(&((type *)0)->member))
#endif


oskit_error_t oskit_avc_create(oskit_services_t *osenv,
			       struct oskit_security *security,
			       oskit_avc_t **out_avc)
{	
	oskit_osenv_log_t *log;
	oskit_osenv_mem_t *mem;
	avc_cache_t 	*avc;
	avc_node_t	*new;
	int             i;


	oskit_services_lookup_first(osenv, &oskit_osenv_log_iid, 
				    (void **) &log);
	if (!log)
		return OSKIT_EINVAL;

	oskit_services_lookup_first(osenv, &oskit_osenv_mem_iid, 
				    (void **) &mem);
	if (!mem)
		return OSKIT_EINVAL;

	avc = oskit_osenv_mem_alloc(mem, sizeof(avc_cache_t), 0, 0);
	if (!avc)
		return OSKIT_ENOMEM;

	memset(avc, 0, sizeof(avc_cache_t));
	avc->avci.ops = &avc_ops;
	avc->avci.security = security;
	oskit_security_addref(security);
	avc->avc_ssi.ops = &avc_ss_ops;
	avc->count = 1;

	avc->log = log;
	oskit_osenv_log_addref(log);

	avc->mem = mem;	
	oskit_osenv_mem_addref(mem);

	for (i = 0; i < AVC_CACHE_MAXNODES; i++) {
		new = (avc_node_t *) 
			oskit_osenv_mem_alloc(avc->mem,sizeof(avc_node_t),0,0);
		if (!new) {
		        oskit_osenv_log_log(avc->log, OSENV_LOG_WARNING, 
			    "avc:  only able to allocate %d nodes\n", i);
			break;
		}
		memset(new, 0, sizeof(avc_node_t));
		new->next = avc->freelist;
		avc->freelist = new;
	}

	oskit_osenv_log_log(avc->log, OSENV_LOG_INFO,
		    "AVC:  allocated %d bytes during initialization.\n", 
		    sizeof(avc_cache_t) + i * sizeof(avc_node_t));

	*out_avc = &avc->avci;

	return 0;
}


#define AVC_HASH(ssid,tsid,tclass) \
((ssid ^ (tsid<<2) ^ (tclass<<4)) & (AVC_CACHE_SLOTS - 1))


static OSKIT_COMDECL_V avc_log_stats(oskit_avc_t *a, 
				     int priority,
				     char *tag)
{
#if OSKIT_AVC_KEEP_STATS
	avc_cache_t *avc = (avc_cache_t *) a;

	oskit_osenv_log_log(avc->log, priority,
	    "%s avc:  entry:  %d lookups == %d hits + %d misses (%d discards)\n",
	       tag,
	       a->stats[OSKIT_AVC_ENTRY_LOOKUPS],
	       a->stats[OSKIT_AVC_ENTRY_HITS],
	       a->stats[OSKIT_AVC_ENTRY_MISSES],
	       a->stats[OSKIT_AVC_ENTRY_DISCARDS]);

	oskit_osenv_log_log(avc->log, priority,
	    "%s avc:  cav:  %d lookups == %d hits + %d misses\n",
	       tag,
	       a->stats[OSKIT_AVC_CAV_LOOKUPS],
	       a->stats[OSKIT_AVC_CAV_HITS],
	       a->stats[OSKIT_AVC_CAV_MISSES]);

	oskit_osenv_log_log(avc->log, priority,
	    "%s avc:  cav:  %d/%d probe/hit ratio\n",
	       tag,
	       a->stats[OSKIT_AVC_CAV_PROBES],
	       a->stats[OSKIT_AVC_CAV_HITS]);
#endif
}


static void avc_log_av(
	avc_cache_t *avc,
	int priority,
	oskit_security_class_t tclass,
	oskit_access_vector_t av)
{
	char          **common_pts = 0;
	oskit_access_vector_t common_base = 0;
	int             i, i2, perm;


	if (av == 0) {
		oskit_osenv_log_log(avc->log, priority, " null");
		return;
	}

	for (i = 0; i < AV_INHERIT_SIZE; i++) {
		if (av_inherit[i].tclass == tclass) {
			common_pts = av_inherit[i].common_pts;
			common_base = av_inherit[i].common_base;
			break;
		}
	}

	oskit_osenv_log_log(avc->log, priority, " {");
	i = 0;
	perm = 1;
	while (perm < common_base) {
		if (perm & av)
			oskit_osenv_log_log(avc->log, priority, 
					    " %s", common_pts[i]);
		i++;
		perm <<= 1;
	}

	while (i < sizeof(oskit_access_vector_t) * 8) {
		if (perm & av) {
			for (i2 = 0; i2 < AV_PERM_TO_STRING_SIZE; i2++) {
				if ((av_perm_to_string[i2].tclass == tclass) &&
				    (av_perm_to_string[i2].value == perm))
					break;
			}
			if (i2 < AV_PERM_TO_STRING_SIZE)
				oskit_osenv_log_log(avc->log, priority, 
					    " %s", av_perm_to_string[i2].name);
		}
		i++;
		perm <<= 1;
	}

	oskit_osenv_log_log(avc->log, priority, " }");
}


static void avc_log_query(
	avc_cache_t *avc,
	int priority,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass)	/* IN */
{
	int rc;
	oskit_security_context_t scontext;
	oskit_u32_t scontext_len;
	
 	rc = oskit_security_sid_to_context(avc->avci.security, ssid, 
					   &scontext, &scontext_len);
	if (rc)
		oskit_osenv_log_log(avc->log, priority, 
				    "ssid=%d", ssid);		
	else {
		oskit_osenv_log_log(avc->log, priority, 
				    "scontext=%s", scontext);
		oskit_osenv_mem_free(avc->mem, scontext, OSENV_AUTO_SIZE, 0);
	}
	
	rc = oskit_security_sid_to_context(avc->avci.security, tsid, 
					    &scontext, &scontext_len);
	if (rc)
		oskit_osenv_log_log(avc->log, priority, " tsid=%d", tsid);
	else {
		oskit_osenv_log_log(avc->log, priority, " tcontext=%s", 
				  scontext);

		oskit_osenv_mem_free(avc->mem, scontext, OSENV_AUTO_SIZE, 0);
	}

	oskit_osenv_log_log(avc->log, priority, " tclass=%s", 
				class_to_string[tclass]);
}


static OSKIT_COMDECL_V avc_log_contents(oskit_avc_t *a, 
					int priority,
					char *tag)
{
	avc_cache_t *avc = (avc_cache_t *) a;
	int             i, chain_len, max_chain_len, slots_used;
	avc_node_t     *node;


#if OSKIT_AVC_KEEP_STATS
	avc_log_stats(a, priority, tag);
#endif

	slots_used = 0;
	max_chain_len = 0;
	for (i = 0; i < AVC_CACHE_SLOTS; i++) {
		node = avc->slots[i];
		if (node) {
			oskit_osenv_log_log(avc->log, priority, 
					    "\n%s avc:  slot %d:\n", tag, i);
			slots_used++;
			chain_len = 0;
			while (node) {
				avc_log_query(avc,priority,
					      node->ae.ssid, node->ae.tsid, 
					      node->ae.tclass);
				oskit_osenv_log_log(avc->log, priority, 
						    " allowed");
				avc_log_av(avc,priority,
					   node->ae.tclass, node->ae.allowed);
				oskit_osenv_log_log(avc->log, priority, 
						    "\n");

				chain_len++;
				node = node->next;
			}

			if (chain_len > max_chain_len)
				max_chain_len = chain_len;
		}
	}

	oskit_osenv_log_log(avc->log, priority, 
	    "\n%s avc:  %d nodes and %d slots used, longest chain length %d\n",
	    tag, avc->activeNodes, slots_used, max_chain_len);

	oskit_osenv_log_log(avc->log, priority, 
	    "%s avc:  latest_notif=%d\n", tag, avc->latest_notif);
}


extern inline avc_node_t *avc_reclaim_node(avc_cache_t *avc)
{
	avc_node_t     *prev, *cur;
	int             hvalue, try;


	hvalue = avc->lru_hint;
	for (try = 0; try < 2; try++) {
		do {
			prev = NULL;
			cur = avc->slots[hvalue];
			while (cur) {
				if (!cur->ae.used)
					goto found;

				cur->ae.used = FALSE;

				prev = cur;
				cur = cur->next;
			}
			hvalue = (hvalue + 1) & (AVC_CACHE_SLOTS - 1);
		} while (hvalue != avc->lru_hint);
	}

	oskit_osenv_log_panic(avc->log, "avc_reclaim_node");

found:
	avc->lru_hint = hvalue;

	if (prev == NULL)
		avc->slots[hvalue] = cur->next;
	else
		prev->next = cur->next;

	return cur;
}


extern inline avc_node_t *avc_claim_node(
	avc_cache_t *avc,
	oskit_security_id_t ssid,
	oskit_security_id_t tsid,
	oskit_security_class_t tclass)
{
	avc_node_t     *new;
	int             hvalue;


	hvalue = AVC_HASH(ssid, tsid, tclass);
	if (avc->freelist) {
		new = avc->freelist;
		avc->freelist = avc->freelist->next;
		avc->activeNodes++;
	} else {
		new = avc_reclaim_node(avc);
		if (!new)
			return NULL;
	}

	new->ae.used = TRUE;
	new->ae.ssid = ssid;
	new->ae.tsid = tsid;
	new->ae.tclass = tclass;
	new->next = avc->slots[hvalue];
	avc->slots[hvalue] = new;

	return new;
}


extern inline avc_node_t *avc_search_node(
	avc_cache_t *avc,
	oskit_security_id_t ssid,
	oskit_security_id_t tsid,
	oskit_security_class_t tclass
#if OSKIT_AVC_KEEP_STATS
	,int *probes
#endif
	)
{
	avc_node_t     *cur;
	int             hvalue;
#if OSKIT_AVC_KEEP_STATS
	int             tprobes = 1;
#endif


	hvalue = AVC_HASH(ssid, tsid, tclass);
	cur = avc->slots[hvalue];
	while (cur != NULL &&
	       (ssid != cur->ae.ssid ||
		tclass != cur->ae.tclass ||
		tsid != cur->ae.tsid)) {
#if OSKIT_AVC_KEEP_STATS
		tprobes++;
#endif
		cur = cur->next;
	}

	if (cur == NULL) {
		/* cache miss */
		return NULL;
	}

	/* cache hit */
#if OSKIT_AVC_KEEP_STATS
	if (probes)
		*probes = tprobes;
#endif

	cur->ae.used = TRUE;

	return cur;
}


static OSKIT_COMDECL avc_lookup(
	oskit_avc_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t requested,	/* IN */
	oskit_avc_entry_ref_t *out_aeref)	/* OUT */
{
	avc_cache_t    *avc = (avc_cache_t *) a;
	avc_node_t     *node;
#if OSKIT_AVC_KEEP_STATS
	int             probes;
#endif

#if OSKIT_AVC_KEEP_STATS
	a->stats[OSKIT_AVC_CAV_LOOKUPS]++;
#endif

	node = avc_search_node(avc, ssid, tsid, tclass
#if OSKIT_AVC_KEEP_STATS
			       ,&probes
#endif
		);

	if (node && ((node->ae.decided & requested) == requested)) {
#if OSKIT_AVC_KEEP_STATS
		a->stats[OSKIT_AVC_CAV_HITS]++;
		a->stats[OSKIT_AVC_CAV_PROBES] += probes;
#endif
		out_aeref->ae = &node->ae;
		return 0;
	} 

#if OSKIT_AVC_KEEP_STATS
	a->stats[OSKIT_AVC_CAV_MISSES]++;
#endif

	return OSKIT_ENOENT;
}


static OSKIT_COMDECL avc_insert(
	oskit_avc_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,		/* IN */
	oskit_avc_entry_t *ae,			/* IN */
	oskit_u32_t seqno,			/* IN */
	oskit_avc_entry_ref_t *out_aeref)	/* OUT */
{
	avc_cache_t    *avc = (avc_cache_t *) a;
	avc_node_t     *node;

	if (seqno < avc->latest_notif) {
		oskit_osenv_log_log(avc->log, OSENV_LOG_WARNING, 
				    "avc:  seqno %d < latest_notif %d\n", 
				    seqno,
				    avc->latest_notif);		
		return OSKIT_EAGAIN;
	}

	node = avc_claim_node(avc, ssid, tsid, tclass);
	if (!node) {
		return OSKIT_ENOMEM;
	}
	
	node->ae.allowed = ae->allowed;
	node->ae.decided = ae->decided;
	node->ae.auditallow = ae->auditallow;
	node->ae.auditdeny = ae->auditdeny;
	node->ae.notify = ae->notify;

	out_aeref->ae = &node->ae;
	return 0;
}


static OSKIT_COMDECL_V avc_audit(
	oskit_avc_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,		/* IN */
	oskit_access_vector_t audited,		/* IN */
	oskit_avc_entry_t *ae,			/* IN */
	oskit_u32_t denied)			/* IN */
{
	avc_cache_t *avc = (avc_cache_t *) a;

	oskit_osenv_log_log(avc->log, OSENV_LOG_WARNING, 
			    "\navc:  %s ", denied ? "denied" : "granted");
	avc_log_av(avc,OSENV_LOG_WARNING,tclass,audited);
	oskit_osenv_log_log(avc->log, OSENV_LOG_WARNING, " for ");
	avc_log_query(avc,OSENV_LOG_WARNING,ssid, tsid, tclass);
	oskit_osenv_log_log(avc->log, OSENV_LOG_WARNING, "\n");
}


static OSKIT_COMDECL avc_add_callback(
	oskit_avc_t *a,
	struct oskit_avc_callback *c,
	oskit_u32_t events,
	oskit_security_id_t ssid,
	oskit_security_id_t tsid,
	oskit_security_class_t tclass,
	oskit_access_vector_t perms)
{
	avc_cache_t *avc = (avc_cache_t *) a;
	avc_callback_node_t *node;

	node = (avc_callback_node_t *) 
	      oskit_osenv_mem_alloc(avc->mem, sizeof(avc_callback_node_t),0,0);
	
	if (!node)
		return OSKIT_ENOMEM;

	node->callback = c;
	oskit_avc_callback_addref(c);
	node->events = events;
	node->ssid = ssid;
	node->tsid = tsid;
	node->perms = perms;
	node->next = avc->callbacks;
	avc->callbacks = node;
	return 0;
}


static OSKIT_COMDECL avc_remove_callback(
	oskit_avc_t *a,
	struct oskit_avc_callback *c)
{
	avc_cache_t *avc = (avc_cache_t *) a;
	avc_callback_node_t *prev, *cur;

	prev = NULL;
	cur = avc->callbacks;
	while (cur) {
		if (cur->callback == c)
			break;
		prev = cur;
		cur = cur->next;
	}

	if (!cur)
		return OSKIT_ENOENT;

	if (prev)
		prev->next = cur->next;
	else
		avc->callbacks = cur->next;

	oskit_avc_callback_release(cur->callback);	
	oskit_osenv_mem_free(avc->mem, cur, 0, sizeof(avc_callback_node_t));
	return 0;
}


#define AVC_SIDCMP(x,y) \
((x) == (y) || (x) == OSKIT_SECSID_WILD || (y) == OSKIT_SECSID_WILD)


extern inline void avc_update_node(
	oskit_u32_t event,
	avc_node_t *node,
	oskit_access_vector_t perms)
{
	switch (event) { 
	case OSKIT_AVC_CALLBACK_GRANT: 
		node->ae.allowed |= perms; 
		break; 
	case OSKIT_AVC_CALLBACK_TRY_REVOKE: 
	case OSKIT_AVC_CALLBACK_REVOKE: 
		node->ae.allowed &= ~perms; 
		break; 
	case OSKIT_AVC_CALLBACK_AUDITALLOW_ENABLE: 
		node->ae.auditallow |= perms; 
		break; 
	case OSKIT_AVC_CALLBACK_AUDITALLOW_DISABLE: 
		node->ae.auditallow &= ~perms; 
		break; 
	case OSKIT_AVC_CALLBACK_AUDITDENY_ENABLE: 
		node->ae.auditdeny |= perms; 
		break; 
	case OSKIT_AVC_CALLBACK_AUDITDENY_DISABLE: 
		node->ae.auditdeny &= ~perms; 
		break; 
	case OSKIT_AVC_CALLBACK_NOTIFY_ENABLE: 
		node->ae.notify |= perms; 
		break; 
	case OSKIT_AVC_CALLBACK_NOTIFY_DISABLE: 
		node->ae.notify &= ~perms; 
		break; 
	}
}


static int avc_update_cache(
	avc_cache_t *avc,
	oskit_u32_t event, 		/* IN */
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms)		/* IN */
{
	avc_node_t     *node;	
	int i;

	if (ssid == OSKIT_SECSID_WILD || tsid == OSKIT_SECSID_WILD) {
		/* apply to all matching nodes */
		for (i = 0; i < AVC_CACHE_SLOTS; i++) {
			for (node = avc->slots[i]; node; 
			     node = node->next) {
				if (AVC_SIDCMP(ssid, node->ae.ssid) && 
				    AVC_SIDCMP(tsid, node->ae.tsid) &&
				    tclass == node->ae.tclass) {
					avc_update_node(event,node,perms);
				}
			}
		}
	} else {
		/* apply to one node */
		node = avc_search_node(avc, ssid, tsid, tclass
#if OSKIT_AVC_KEEP_STATS
				       ,0
#endif
			);		
		if (node) {
			avc_update_node(event,node,perms);
		}
	}

	return 0;
}



static int avc_control(
	avc_cache_t *avc,
	oskit_u32_t event, 		/* IN */
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno,		/* IN */
	oskit_access_vector_t *out_retained)	/* OUT */
{

	avc_callback_node_t *c;
	oskit_access_vector_t tretained = 0, cretained = 0;
	int rc;

	if (event != OSKIT_AVC_CALLBACK_TRY_REVOKE)
		avc_update_cache(avc,event,ssid,tsid,tclass,perms);

	for (c = avc->callbacks; c; c = c->next)
	{
		if ((c->events & event) &&
		    AVC_SIDCMP(c->ssid, ssid) && 
		    AVC_SIDCMP(c->tsid, tsid) &&
		    c->tclass == tclass &&
		    (c->perms & perms)) {
			cretained = 0;
			rc = oskit_avc_callback_invoke(c->callback,
						 event, ssid, tsid, tclass,
						 (c->perms & perms),	
						 &cretained);
			if (rc)
				return rc;
			tretained |= cretained;			
		}
	}

	if (event == OSKIT_AVC_CALLBACK_TRY_REVOKE) {
		/* revoke any unretained permissions */
		perms &= ~tretained;
		avc_update_cache(avc,event,ssid,tsid,tclass,perms);
		*out_retained = tretained;
	}

	if (seqno > avc->latest_notif)
		avc->latest_notif = seqno;
	
	return 0;
}


static OSKIT_COMDECL avc_ss_grant(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno)		/* IN */
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_control(avc, OSKIT_AVC_CALLBACK_GRANT,
			   ssid, tsid, tclass, perms, seqno, 0);
}


static OSKIT_COMDECL avc_ss_try_revoke(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno,		/* IN */
	oskit_access_vector_t *out_retained)	/* OUT */
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_control(avc, OSKIT_AVC_CALLBACK_TRY_REVOKE,
			   ssid, tsid, tclass, perms, seqno, out_retained);
}


static OSKIT_COMDECL avc_ss_revoke(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno)		/* IN */
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_control(avc, OSKIT_AVC_CALLBACK_REVOKE,
			   ssid, tsid, tclass, perms, seqno, 0);
}


static OSKIT_COMDECL avc_ss_reset(oskit_avc_ss_t *a,
				  oskit_u32_t seqno)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	avc_callback_node_t *c;
	int rc;
	avc_node_t     *node, *tmp;
	int             i;


	for (i = 0; i < AVC_CACHE_SLOTS; i++) {
		node = avc->slots[i];
		while (node) {
			tmp = node;
			node = node->next;
			tmp->ae.ssid = tmp->ae.tsid = OSKIT_SECSID_NULL;
			tmp->ae.tclass = OSKIT_SECCLASS_NULL;
			tmp->ae.allowed = tmp->ae.decided = 0;
			tmp->ae.auditallow = tmp->ae.auditdeny = 0;
			tmp->ae.notify = 0;
			tmp->ae.used = FALSE;
			tmp->next = avc->freelist;
			avc->freelist = tmp;
			avc->activeNodes--;
		}
		avc->slots[i] = 0;
	}
	avc->lru_hint = 0;

	for (c = avc->callbacks; c; c = c->next) {
		if (c->events & OSKIT_AVC_CALLBACK_RESET) {
			rc = oskit_avc_callback_invoke(c->callback,
						 OSKIT_AVC_CALLBACK_RESET, 
						 0, 0, 0, 0, 0);
			if (rc)
				return rc;
		}
	}

	if (seqno > avc->latest_notif)
		avc->latest_notif = seqno;

	return 0;
}


static OSKIT_COMDECL avc_ss_set_auditallow(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno,		/* IN */
	oskit_bool_t enable)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	if (enable)
		return avc_control(avc, OSKIT_AVC_CALLBACK_AUDITALLOW_ENABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
	else
		return avc_control(avc, OSKIT_AVC_CALLBACK_AUDITALLOW_DISABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
}


static OSKIT_COMDECL avc_ss_set_auditdeny(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno,		/* IN */
	oskit_bool_t enable)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	if (enable)
		return avc_control(avc, OSKIT_AVC_CALLBACK_AUDITDENY_ENABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
	else
		return avc_control(avc, OSKIT_AVC_CALLBACK_AUDITDENY_DISABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
}


static OSKIT_COMDECL avc_ss_set_notify(
	oskit_avc_ss_t *a,
	oskit_security_id_t ssid,		/* IN */
	oskit_security_id_t tsid,		/* IN */
	oskit_security_class_t tclass,	/* IN */
	oskit_access_vector_t perms,		/* IN */
	oskit_u32_t seqno,		/* IN */
	oskit_bool_t enable)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	if (enable)
		return avc_control(avc, OSKIT_AVC_CALLBACK_NOTIFY_ENABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
	else
		return avc_control(avc, OSKIT_AVC_CALLBACK_NOTIFY_DISABLE,
				   ssid, tsid, tclass, perms, seqno, 0);
}


static OSKIT_COMDECL avc_query(oskit_avc_t *a,
			       const struct oskit_guid *iid,
			       void **out_ihandle)
{
	avc_cache_t *avc = (avc_cache_t *)a;
	
	if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	    memcmp(iid, &oskit_avc_iid, sizeof(*iid)) == 0) {
		*out_ihandle = &avc->avci;
		++avc->count;
		return 0;
	}

	if (memcmp(iid, &oskit_avc_ss_iid, sizeof(*iid)) == 0) 
	{
		*out_ihandle = &avc->avc_ssi;
		++avc->count;
		return 0;
	}

	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;    
}


static OSKIT_COMDECL_U avc_addref(oskit_avc_t *a)
{
	avc_cache_t *avc = (avc_cache_t *) a;
	return ++avc->count;
}


static OSKIT_COMDECL_U avc_release(oskit_avc_t *a)
{
	avc_cache_t *avc = (avc_cache_t *) a;
	oskit_osenv_mem_t *mem;
	avc_node_t *node, *ntmp;
	avc_callback_node_t *c, *ctmp;		
	unsigned newcount;
	int i;
    
	newcount = --avc->count;
	if (newcount == 0)
	{
		oskit_security_release(avc->avci.security);
		oskit_osenv_log_release(avc->log);
		for (i = 0; i < AVC_CACHE_SLOTS; i++) {
			node = avc->slots[i];
			while (node) {
				ntmp = node;
				node = node->next;
				oskit_osenv_mem_free(avc->mem, ntmp, 0, 
						     sizeof(avc_node_t));
			}
		}
		node = avc->freelist;
		while (node) {
			ntmp = node;
			node = node->next;
			oskit_osenv_mem_free(avc->mem, ntmp, 0, 
					     sizeof(avc_node_t));
		}
		c = avc->callbacks;
		while (c) {
			ctmp = c;
			c = c->next;
			oskit_avc_callback_release(ctmp->callback);
			oskit_osenv_mem_free(avc->mem, ctmp, 0, 
					     sizeof(avc_callback_node_t));
		}
		mem = avc->mem;
		oskit_osenv_mem_free(mem, avc, 0, sizeof(avc_cache_t));
		oskit_osenv_mem_release(mem);
	}

	return newcount;
}


static OSKIT_COMDECL avc_ss_query(oskit_avc_ss_t *a,
				  const struct oskit_guid *iid,
				  void **out_ihandle)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_query(&avc->avci, iid, out_ihandle);
}


static OSKIT_COMDECL_U avc_ss_addref(oskit_avc_ss_t *a)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_addref(&avc->avci);
}


static OSKIT_COMDECL_U avc_ss_release(oskit_avc_ss_t *a)
{
	avc_cache_t *avc = (avc_cache_t *) ((char *) a - offsetof(avc_cache_t, avc_ssi));
	return avc_release(&avc->avci);
}


static struct oskit_avc_ops avc_ops = {
    avc_query,
    avc_addref,
    avc_release,
    avc_lookup,
    avc_insert,
    avc_audit,
    avc_add_callback,
    avc_remove_callback,
    avc_log_contents,
    avc_log_stats
};

static struct oskit_avc_ss_ops avc_ss_ops = {
    avc_ss_query,
    avc_ss_addref,
    avc_ss_release,
    avc_ss_grant,
    avc_ss_try_revoke,
    avc_ss_revoke,
    avc_ss_reset,
    avc_ss_set_auditallow,
    avc_ss_set_auditdeny,
    avc_ss_set_notify
};
