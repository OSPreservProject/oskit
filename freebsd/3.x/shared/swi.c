/*
 * Copyright (c) 1997-1998, 2000 University of Utah and the Flux Group.
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
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $\Id: kern_intr.c,v 1.20 1998/09/26 14:25:31 dfr Exp $
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/errno.h>

#include <machine/ipl.h>

#ifdef __i386__
#include <i386/isa/icu.h>
#include <i386/isa/intr_machdep.h>
#endif

#include <sys/interrupt.h>

#include <oskit/dev/softirq.h>
#include "glue.h"


struct swilist {
	swihand_t	*sl_handler;
	struct swilist	*sl_next;
};

static struct swilist swilists[NSWI];

/*
 * Our softirq vector that osenv gave back to us.
 */
static int swi_vector;

void
swi_init(void)
{
	int		err;
	static void	swi_softintr_handler(void *arg);
	extern void	bsdnet_net_softnet(void);

	err = osenv_softirq_alloc_vector(&swi_vector);
	if (err)
		panic(__FUNCTION__ ": Could not allocate softirq vector\n");

	err = osenv_softirq_alloc(swi_vector, swi_softintr_handler, 0, 0);
	if (err)
		panic(__FUNCTION__ ": Could not allocate softirq handler\n");

	/*
	 * Certain swi's are hardwired into the table in assembly
	 * language. We don't have that of course, so wire down a
	 * couple right here.
	 */
	register_swi(SWI_CLOCK, softclock);
	register_swi(SWI_NET, bsdnet_net_softnet);
	
}

static void
swi_softintr_handler(void *arg)
{
	void swi_check(void);
	
	swi_check();
}

void
swi_schedule(void)
{
	osenv_softirq_schedule(swi_vector);
}

void
register_swi(intr, handler)
	int intr;
	swihand_t *handler;
{
	struct swilist *slp, *slq;
	int s;

	if (intr < NHWI || intr >= NHWI + NSWI)
		panic("register_swi: bad intr %d", intr);
	slp = &swilists[intr - NHWI];
	s = splhigh();

	if (slp->sl_handler == NULL) {
		slp->sl_handler = handler;
	}
	else {
		slq = malloc(sizeof(*slq), M_DEVBUF, M_NOWAIT);
		if (slq == NULL)
                        osenv_panic("register_swi: malloc failed");

		slq->sl_handler = handler;
		slq->sl_next = NULL;
		while (slp->sl_next != NULL)
			slp = slp->sl_next;
		slp->sl_next = slq;
	}
	splx(s);
}

void
swi_dispatcher(intr)
	int intr;
{
	struct swilist *slp = &swilists[intr - NHWI];

	if (slp->sl_handler == NULL)
		return;

	do {
		(*slp->sl_handler)();
		slp = slp->sl_next;
	} while (slp != NULL);
}


void
swi_check(void)
{
	int i;

	if (! oskit_freebsd_ipending)
		return;

	for (i = 0; i < NSWI; i++) {
		if (oskit_freebsd_ipending & (1 << (i + NHWI))) {
			swi_dispatcher(i + NHWI);
			oskit_freebsd_ipending &= ~((1 << (i + NHWI)));
		}
	}
}
