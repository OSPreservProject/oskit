/*
 * Copyright (c) 1996, 1998-1999, 2001 University of Utah and the Flux Group.
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
 * Default IRQ control object.
 */
#include <oskit/com.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/osenv_irq.h>
#include <stdlib.h>
#include <string.h>

/*
 * There is one and only one interrupt interface in this implementation.
 */
static struct oskit_osenv_irq_ops	osenv_irq_ops;
#ifdef KNIT
       struct oskit_osenv_irq		osenv_irq_object = {&osenv_irq_ops};
#else
static struct oskit_osenv_irq		osenv_irq_object = {&osenv_irq_ops};
#endif

static OSKIT_COMDECL
irq_query(oskit_osenv_irq_t *s, const oskit_iid_t *iid, void **out_ihandle)
{
        if (memcmp(iid, &oskit_iunknown_iid, sizeof(*iid)) == 0 ||
            memcmp(iid, &oskit_osenv_irq_iid, sizeof(*iid)) == 0) {
                *out_ihandle = s;
                return 0;
        }

        *out_ihandle = 0;
        return OSKIT_E_NOINTERFACE;
};

static OSKIT_COMDECL_U
irq_addref(oskit_osenv_irq_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
irq_release(oskit_osenv_irq_t *s)
{
	/* Only one object */
	return 1;
}

static OSKIT_COMDECL_U
irq_alloc(oskit_osenv_irq_t *o, int irq,
	  void (*handler)(void *), void *data, int flags)
{
	return osenv_irq_alloc(irq, handler, data, flags);
}

static OSKIT_COMDECL_V
irq_free(oskit_osenv_irq_t *o, int irq,
	 void (*handler)(void *), void *data)
{
	osenv_irq_free(irq, handler, data);
}

static OSKIT_COMDECL_V
irq_disable(oskit_osenv_irq_t *o, int irq)
{
	osenv_irq_disable(irq);
}

static OSKIT_COMDECL_V
irq_enable(oskit_osenv_irq_t *o, int irq)
{
	osenv_irq_enable(irq);
}

static OSKIT_COMDECL_U
irq_pending(oskit_osenv_irq_t *o, int irq)
{
	return osenv_irq_pending(irq);
}

static struct oskit_osenv_irq_ops osenv_irq_ops = {
	irq_query,
	irq_addref,
	irq_release,
	irq_alloc,
	irq_free,
	irq_disable,
	irq_enable,
	irq_pending,
};

#ifndef KNIT
/*
 * Return a reference to the one and only interrupt object.
 */
oskit_osenv_irq_t *
oskit_create_osenv_irq(void)
{
	return &osenv_irq_object;
}
#endif
