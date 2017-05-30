/*
 * Copyright (c) 1996, 1998, 1999 University of Utah and the Flux Group.
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

/*
 * Generic COM interface registration module.
 * Basically maintains a list of COM interface pointers
 * for a particular interface GUID (IID).
 * Given an IID, you can find all the registered COM interfaces with that IID.
 * Interfaces will be returned in the order in which they were registered.
 * It's harmless to register an interface multiple times;
 * only a single entry in the table will be retained.
 * Currently just does everything with simple lists;
 * if we end up having lots of objects, we may need smarter algorithms.
 */

/*
 * Locking! This is not thread safe, which is *going* to cause problems.
 */

#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/com/mem.h>
#include <oskit/c/stdlib.h>
#include <oskit/c/assert.h>
#include <oskit/c/string.h>

static struct oskit_services_ops services_ops;

/*
 * One of these nodes represents each registered COM interface (object)
 */
struct objnode {
	struct objnode *next;
	oskit_iunknown_t *intf;
};

/*
 * We keep one iidnode for each unique IID we see
 */
struct iidnode {
	struct iidnode *next;
	oskit_guid_t iid;
	struct objnode *objs;
	int objcount;
};

/*
 * This is the interface object. Each services database is parameterized
 * with a memory object.
 */
struct db {
	oskit_services_t	servi;		/* COM interface */
	int			count;		/* Reference count */
	struct iidnode		*iids;		/* DB pointer */
	oskit_mem_t		*memi;		/* Memory object to use */
};

static OSKIT_COMDECL
services_query(oskit_services_t *si,
	       const oskit_iid_t *iid, void **out_ihandle)
{
	struct db	*s = (struct db *) si;

        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_services_iid, sizeof(*iid)) == 0) {
                *out_ihandle = &s->servi;
		s->count++;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
services_addref(oskit_services_t *si)
{
	struct db	*s = (struct db *) si;

	assert(s->count);
	return ++s->count;
}

static OSKIT_COMDECL_U
services_release(oskit_services_t *si)
{
	struct db	*s = (struct db *) si;

	assert(s->count);

	if (--s->count == 0) {
		oskit_mem_t	*memi = s->memi;
		struct iidnode  *in, *nin;
		struct objnode  *on, *non;

		/*
		 * Must free up all the registered interface objects.
		 */
		in = s->iids;
		while (in) {
			on = in->objs;
			while (on) {
				oskit_iunknown_release(on->intf);
				non = on->next;
				oskit_mem_free(memi,
					       (void *) on, sizeof(*on), 0);
				on  = non;
			}

			nin = in->next;
			oskit_mem_free(memi, (void *) in, sizeof(*in), 0);
			in  = nin;
		}

		oskit_mem_free(memi, (void *)s, sizeof(*s), 0);
		oskit_mem_release(memi);
		return 0;
	}
	return s->count;
}

/*
 * Register an interface in the services database.
 * More than one interface can be registered for a particular IID.
 */
OSKIT_COMDECL
services_addservice(oskit_services_t *si,
		    const struct oskit_guid *iid, void *interface)
{
	struct db	 *s = (struct db *) si;
	oskit_iunknown_t *iu = (oskit_iunknown_t*)interface;
	struct iidnode   *in;
	struct objnode   *on, **onp;

	/* Find or create the appropriate iidnode */
	for (in = s->iids; ; in = in->next) {
		if (in == NULL) {
			in = oskit_mem_alloc(s->memi, sizeof(*in), 0);
			if (in == NULL)
				return OSKIT_E_OUTOFMEMORY;
			in->iid = *iid;
			in->objs = NULL;
			in->objcount = 0;
			in->next = s->iids;
			s->iids = in;
			break;
		}
		if (memcmp(&in->iid, iid, sizeof(*iid)) == 0)
			break;
	}

	/* Make sure this interface isn't already registered */
	for (onp = &in->objs; *onp; onp = &(*onp)->next) {
		if ((*onp)->intf == interface)
			return 0;
	}

	/* Create a new objnode for this interface */
	on = oskit_mem_alloc(s->memi, sizeof(*on), 0);
	if (on == NULL)
		return OSKIT_E_OUTOFMEMORY;
	on->next = NULL;
	on->intf = iu;	oskit_iunknown_addref(iu);
	*onp = on;
	in->objcount++;

	return 0;
}

/*
 * Unregister a previously registered interface.
 */
OSKIT_COMDECL
services_remservice(oskit_services_t *si,
		    const struct oskit_guid *iid, void *interface)
{
	struct db	 *s = (struct db *) si;
	struct iidnode   *in;
	struct objnode   *on, **onp;

	/* Find the appropriate iidnode */
	for (in = s->iids; ; in = in->next) {
		if (in == NULL)
			return OSKIT_E_INVALIDARG;
		if (memcmp(&in->iid, iid, sizeof(*iid)) == 0)
			break;
	}

	/* Find and remove the objnode */
	for (onp = &in->objs; ; onp = &on->next) {
		on = *onp;
		if (on == NULL)
			return OSKIT_E_INVALIDARG;
		if (on->intf == interface)
			break;
	}
	*onp = on->next;
	oskit_iunknown_release(on->intf);
	oskit_mem_free(s->memi, (void *) on, sizeof(*on), 0);
	in->objcount--;

	return 0;
}

/*
 * Obtain a list of all the registered interfaces with a specified IID.
 */
OSKIT_COMDECL
services_lookup(oskit_services_t *si,
		const oskit_guid_t *iid, void ***out_interface_array)
{
	struct db	 *s = (struct db *) si;
	struct iidnode *in;
	struct objnode *on;
	void **arr;
	int i;

	/* Find the appropriate iidnode */
	for (in = s->iids; ; in = in->next) {
		if (in == NULL) {
			*out_interface_array = NULL;
			return 0;
		}
		if (memcmp(&in->iid, iid, sizeof(*iid)) == 0)
			break;
	}
	if (in->objcount == 0) {
		*out_interface_array = NULL;
		return 0;
	}

	/*
	 * Allocate an array to hold the interface pointers to return
	 * Note that we *do* use malloc here, since the caller is responsible
	 * for freeing up the array.
	 */
	arr = malloc(sizeof(*arr)*in->objcount);
	if (arr == NULL)
		return OSKIT_E_OUTOFMEMORY;

	/* Fill it in */
	for (i = 0, on = in->objs; i < in->objcount; i++, on = on->next) {
		assert(on != NULL);
		arr[i] = on->intf;
		oskit_iunknown_addref(on->intf);
	}
	assert(on == NULL);

	*out_interface_array = arr;
	return in->objcount;
}

/*
 * Lookup the first interface registered for a given IID.
 * This is typically used to look up "the" instance of a service
 */
OSKIT_COMDECL
services_lookup_first(oskit_services_t *si,
		     const oskit_guid_t *iid, void **out_intf)
{
	struct db	 *s = (struct db *) si;
	struct iidnode *in;
	oskit_iunknown_t *intf;

	/* Find the appropriate iidnode */
	for (in = s->iids; ; in = in->next) {
		if (in == NULL) {
			*out_intf = NULL;
			return 0;
		}
		if (memcmp(&in->iid, iid, sizeof(*iid)) == 0)
			break;
	}
	if (in->objcount == 0) {
		*out_intf = NULL;
		return 0;
	}

	*out_intf = intf = in->objs->intf;
	oskit_iunknown_addref(intf);
	return 0;
}

OSKIT_COMDECL
services_clone(oskit_services_t *si, oskit_services_t **out_intf)
{
	struct db	*s = (struct db *) si;
	struct db	*ns;
	struct iidnode  *in;
	struct objnode  *on;
	oskit_error_t	rc;

	ns = oskit_mem_alloc(s->memi, sizeof(*ns), 0);
	if (ns == NULL)
		return OSKIT_E_OUTOFMEMORY;

	ns->count     = 1;
	ns->memi      = s->memi;
	ns->servi.ops = &services_ops;
	ns->iids      = 0;
	oskit_mem_addref(ns->memi);

	in = s->iids;
	while (in) {
		on = in->objs;
		while (on) {
			if ((rc = services_addservice(&ns->servi,
						      &in->iid, on->intf))
			    != NULL) {
				panic("services_clone");
			}
			on = on->next;
		}
		in = in->next;
	}

	*out_intf = &ns->servi;
	return 0;
}

static struct oskit_services_ops services_ops = {
	services_query,
	services_addref,
	services_release,
	services_addservice,
	services_remservice,
	services_lookup,
	services_lookup_first,
	services_clone,
};

/*
 * The memory object interface is optional, except when creating the
 * initial global registry. Subsequent to the creation of the global
 * registry, we can look there for a default memory object is one is
 * not provided.
 */
oskit_error_t
oskit_services_create(oskit_mem_t *memi, oskit_services_t **out_intf)
{
	struct db	*s;

	/*
	 * If a memory object was not provided, look in the global registry.
	 */
	if (!memi) {
		oskit_lookup_first(&oskit_mem_iid, (void *) &memi);

		if (!memi)
			panic("oskit_services_create: Null memory object!");
	}

	s = oskit_mem_alloc(memi, sizeof(*s), 0);
	if (s == NULL)
		return OSKIT_E_OUTOFMEMORY;

	s->count     = 1;
	s->memi      = memi;
	s->servi.ops = &services_ops;
	s->iids      = 0;
	oskit_mem_addref(memi);

	*out_intf = &s->servi;

	return 0;
}
