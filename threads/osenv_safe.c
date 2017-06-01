/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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
 * The pthread module implements the osenv_ functions.
 * It will also register an instantiation of the osenv_safe interfaces
 * that allows other components to request adaptor to create safe versions
 * of osenv_ based COM interfaces
 */
#include "pthread_internal.h"

#include <oskit/com.h>
#include <oskit/dev/osenv_safe.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/string.h>

/* the COM object */
static osenv_safe_t	safeinterfaces;

static OSKIT_COMDECL
safeinterface_query(osenv_safe_t *intf,
		 const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &osenv_safe_iid, sizeof(*iid)) == 0) {
                *out_ihandle = intf;
                return 0;
        }

        *out_ihandle = NULL;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
safeinterface_addref(osenv_safe_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL_U
safeinterface_release(osenv_safe_t *intf)
{
	/* No reference counting */
	return 1;
}

static OSKIT_COMDECL
safeinterface_stream(osenv_safe_t *intf,
	struct oskit_stream *in_interface,
	struct oskit_stream **out_interface)
{
	return oskit_wrap_stream(in_interface,
		(void (*)(void *))osenv_process_lock,
		(void (*)(void *))osenv_process_unlock,
		0,
		out_interface);
}

static OSKIT_COMDECL
safeinterface_asyncio(osenv_safe_t *intf,
	struct oskit_asyncio *in_interface,
	struct oskit_asyncio **out_interface)
{
	return oskit_wrap_asyncio(in_interface,
		(void (*)(void *))osenv_process_lock,
		(void (*)(void *))osenv_process_unlock,
		0,
		out_interface);
}

static OSKIT_COMDECL
safeinterface_socket(osenv_safe_t *intf,
	struct oskit_socket *in_interface,
	struct oskit_socket **out_interface)
{
	return oskit_wrap_socket(in_interface,
		(void (*)(void *))osenv_process_lock,
		(void (*)(void *))osenv_process_unlock,
		0,
		out_interface);
}

static OSKIT_COMDECL
safeinterface_sockio(osenv_safe_t *intf,
	struct oskit_socket *in_interface,
	struct oskit_socket **out_interface)
{
	return oskit_wrap_sockio(in_interface,
		(void (*)(void *))osenv_process_lock,
		(void (*)(void *))osenv_process_unlock,
		0,
		out_interface);
}

static struct osenv_safe_ops safeinterface_ops = {
	safeinterface_query,
	safeinterface_addref,
	safeinterface_release,
	safeinterface_stream,
	safeinterface_asyncio,
	safeinterface_socket,
	safeinterface_sockio,
};

/*
 * Create and register the pthread interface with COM database.
 */
oskit_error_t
pthread_register_interface(void)
{
	safeinterfaces.ops = &safeinterface_ops;

	return oskit_register(&osenv_safe_iid, (void *) &safeinterfaces);
}
