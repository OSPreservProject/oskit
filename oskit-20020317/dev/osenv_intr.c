/*
 * Copyright (c) 1996, 1998-2001 University of Utah and the Flux Group.
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
 * Default interrupt control object.
 */
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_intr.h>
#include <stdlib.h>
#include <string.h>

/*
 * There is one and only one interrupt interface in this implementation.
 */
static struct oskit_osenv_intr_ops	osenv_intr_ops;
#ifdef KNIT
       struct oskit_osenv_intr		osenv_intr_object = {&osenv_intr_ops};
#else
static struct oskit_osenv_intr		osenv_intr_object = {&osenv_intr_ops};
#endif

static OSKIT_COMDECL
intr_query(oskit_osenv_intr_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_intr_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
intr_addref(oskit_osenv_intr_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
intr_release(oskit_osenv_intr_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_V
intr_enable(oskit_osenv_intr_t *s)
{
	osenv_intr_enable();
}

static OSKIT_COMDECL_V
intr_disable(oskit_osenv_intr_t *s)
{
	osenv_intr_disable();
}

static OSKIT_COMDECL_U
intr_enabled(oskit_osenv_intr_t *s)
{
	return osenv_intr_enabled();
}

static OSKIT_COMDECL_U
intr_savedisable(oskit_osenv_intr_t *s)
{
	return osenv_intr_save_disable();
}

static struct oskit_osenv_intr_ops osenv_intr_ops = {
	intr_query,
	intr_addref,
	intr_release,
	intr_enable,
	intr_disable,
	intr_enabled,
	intr_savedisable,
};

#ifndef KNIT
/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_intr_t *
oskit_create_osenv_intr(void)
{
	return &osenv_intr_object;
}
#endif
