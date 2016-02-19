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

#include <threads/pthread_internal.h>
#include <oskit/com/wrapper.h>
#include <oskit/c/string.h>

/***************************************************************************/
/*
 * methods of socket_factories.
 */
#include <oskit/com.h>
#include <oskit/com/services.h>
#include <oskit/net/socket.h>

#ifdef KNIT
extern struct oskit_socket_factory *oskit_unsafe_socket_factory;
#define pthread_old_sf oskit_unsafe_socket_factory
#else
static struct oskit_socket_factory *pthread_old_sf;
#endif

static OSKIT_COMDECL
socket_factory_query(oskit_socket_factory_t *b,
	const struct oskit_guid *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_socket_factory_iid, sizeof(*iid)) == 0) {
		*out_ihandle = b;
		return 0;
	}
	*out_ihandle = NULL;
	return OSKIT_E_NOINTERFACE;
}

static OSKIT_COMDECL_U
socket_factory_addref(oskit_socket_factory_t *b)
{
	return 1;
}

static OSKIT_COMDECL_U
socket_factory_release(oskit_socket_factory_t *b)
{
	return 1;
}

/*
 * function to create a wrapped socket.
 */
static OSKIT_COMDECL
socket_factory_create(
	oskit_socket_factory_t *factory,
	oskit_u32_t domain,
	oskit_u32_t type, oskit_u32_t protocol, oskit_socket_t **aso)
{
	oskit_error_t		rc;
	struct oskit_socket	*sock;

	/*
	 * Use the original socket factory to create a socket, then
	 * wrap it up with a thread safe wrapper.
	 */
	osenv_process_lock();
	rc = oskit_socket_factory_create(pthread_old_sf,
					 domain, type, protocol, &sock);
	osenv_process_unlock();

	if (rc)
		return rc;

	rc = oskit_wrap_sockio(sock,
			       (void (*)(void *))osenv_process_lock,
			       (void (*)(void *))osenv_process_unlock,
			       0, aso);

	oskit_socket_release(sock);

	return rc;
}

static struct oskit_socket_factory_ops sf_ops = {
	socket_factory_query,
	socket_factory_addref,
	socket_factory_release,
	socket_factory_create
};

#ifdef KNIT
static struct oskit_socket_factory pthread_new_sf = { &sf_ops };
struct oskit_socket_factory *oskit_safe_socket_factory = &pthread_new_sf;
#else
static struct oskit_socket_factory pthread_new_sf = { &sf_ops };

/*
 * Stash the unsafe socket factory, and install a safe version built on it
 * in the registry.
 */
void
pthread_init_socketfactory(oskit_socket_factory_t *sf)
{
	pthread_old_sf = sf;

	/*
	 * And register the new socket factory.
	 */
	if (oskit_register(&oskit_socket_factory_iid,
			   (void *) &pthread_new_sf))
		panic("pthread_init_socketfactory: Register failed");
}
#endif /* !KNIT */
