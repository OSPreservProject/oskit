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

#include <oskit/principal.h>
#include <oskit/comsid.h>
#include <flask/flask.h>
#include <stdlib.h>
#include <string.h>

#ifndef offsetof
#define	offsetof(type, member)	((oskit_size_t)(&((type *)0)->member))
#endif

struct principal
{
    oskit_principal_t prini;	/* principal interface */
    oskit_comsid_t    sidi;     /* sid interface */
    unsigned count;		/* reference count */
    oskit_identity_t id;	/* identity */
    oskit_security_id_t sid;	/* security identifier */
};

static struct oskit_principal_ops prin_ops;
static struct oskit_comsid_ops sid_ops;


oskit_error_t oskit_principal_create(oskit_identity_t *id,
				   oskit_principal_t **out_prin)
{
    struct principal *prin;
    int index;

    if (!id)
	    return OSKIT_EINVAL;

    prin = malloc(sizeof(struct principal));
    if (!prin)
	    return OSKIT_ENOMEM;

    prin->prini.ops = &prin_ops;
    prin->sidi.ops = &sid_ops;
    prin->count = 1;
    prin->id.uid = id->uid;
    prin->id.gid = id->gid;
    prin->id.ngroups = id->ngroups;
    prin->id.groups = malloc(id->ngroups * sizeof(unsigned long));
    if (!prin->id.groups && prin->id.ngroups)
    {
	free(prin);
	return OSKIT_ENOMEM;
    }

    for (index = 0; index < id->ngroups; index++)
	    prin->id.groups[index] = id->groups[index];

    prin->sid = OSKIT_SECINITSID_UNLABELED;

    *out_prin = &prin->prini;

    return 0;
}


static OSKIT_COMDECL prin_query(oskit_principal_t *p,
			       const struct oskit_guid *iid,
			       void **out_ihandle)
{
    struct principal *prin;

    prin = (struct principal*)p;
    if (prin == NULL)
	    panic("%s:%d: null principal", __FILE__, __LINE__);
    if (prin->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
	memcmp(iid, &oskit_principal_iid, sizeof(*iid)) == 0) {
	*out_ihandle = &prin->prini;
	++prin->count;
	return 0;
    }

    if (memcmp(iid, &oskit_comsid_iid, sizeof(*iid)) == 0) {
	    *out_ihandle = &prin->sidi;
	    ++prin->count;
	    return 0;
    }

    *out_ihandle = NULL;
    return OSKIT_E_NOINTERFACE;
}


static OSKIT_COMDECL_U prin_addref(oskit_principal_t *p)
{
    struct principal *prin;

    prin = (struct principal*)p;
    if (prin == NULL)
	    panic("%s:%d: null principal", __FILE__, __LINE__);
    if (prin->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    return ++prin->count;
}


static OSKIT_COMDECL_U prin_release(oskit_principal_t *p)
{
    struct principal *prin;
    unsigned newcount;

    prin = (struct principal*)p;
    if (prin == NULL)
	    panic("%s:%d: null principal", __FILE__, __LINE__);
    if (prin->count == 0)
	    panic("%s:%d: bad count", __FILE__, __LINE__);

    newcount = --prin->count;
    if (newcount == 0)
    {
	free(prin->id.groups);
	free(prin);
    }

    return newcount;
}


static OSKIT_COMDECL prin_getid(oskit_principal_t *p,
			       oskit_identity_t *out_id)
{
    struct principal *prin;
    int index;

    prin = (struct principal *)p;
    if (!prin || !prin->count)
	    return OSKIT_EINVAL;

    out_id->uid = prin->id.uid;
    out_id->gid = prin->id.gid;
    out_id->ngroups = prin->id.ngroups;
    out_id->groups = malloc(prin->id.ngroups * sizeof(unsigned long));
    if (!out_id->groups && out_id->ngroups)
	    return OSKIT_ENOMEM;

    for (index = 0; index < out_id->ngroups; index++)
	    out_id->groups[index] = prin->id.groups[index];
    return 0;
}


static struct oskit_principal_ops prin_ops = {
    prin_query,
    prin_addref,
    prin_release,
    prin_getid
};



static OSKIT_COMDECL
sid_query(oskit_comsid_t * s,
	  const struct oskit_guid * iid,
	  void **out_ihandle)
{
        struct principal *prin;

	prin = (struct principal *) ((char *) s -
				     offsetof(struct principal, sidi));

	return oskit_principal_query(&prin->prini, iid, out_ihandle);
}


static OSKIT_COMDECL_U
sid_addref(oskit_comsid_t * s)
{
        struct principal *prin;

	prin = (struct principal *) ((char *) s -
				     offsetof(struct principal, sidi));

	return oskit_principal_addref(&prin->prini);
}


static OSKIT_COMDECL_U
sid_release(oskit_comsid_t * s)
{
        struct principal *prin;

	prin = (struct principal *) ((char *) s -
				     offsetof(struct principal, sidi));

	return oskit_principal_release(&prin->prini);
}


static OSKIT_COMDECL
sid_get(oskit_comsid_t * s,
	oskit_security_class_t * outclass,
	oskit_security_id_t * outsid)
{
        struct principal *prin;

	prin = (struct principal *) ((char *) s -
				     offsetof(struct principal, sidi));

	*outclass = OSKIT_SECCLASS_PROCESS;
	*outsid = prin->sid;
	return 0;
}


static OSKIT_COMDECL
sid_set(oskit_comsid_t * s,
	oskit_security_id_t newsid)
{
        struct principal *prin;

	prin = (struct principal *) ((char *) s -
				     offsetof(struct principal, sidi));

	prin->sid = newsid;

	return 0;
}


static struct oskit_comsid_ops sid_ops = {
	sid_query,
	sid_addref,
	sid_release,
	sid_get,
	sid_set
};
