/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Simple DMA management.
 */

#ifndef OSKIT
#define OSKIT
#endif

#include <linux/errno.h>
#include <linux/sched.h>
#include <asm/dma.h>
#include "osenv.h"

/*
 * XXX hack, hack
 *
 * On the arm, the DMA functions come from the Linux ISA DMA code in
 * arch/arm/kernel/dma*.
 *
 * On the x86, most of the DMA functions are inlined in asm-i386/dma.h,
 * only request/free are implemented here.
 *
 * In both cases we do not go through an OSKit layer of abstraction,
 * for most of the code so Linux device drivers will not play well with
 * non-Linux DMA-requiring code.
 */

#ifndef OSKIT_ARM32_SHARK
/*
 * Allocate a DMA channel.
 */
int
request_dma(unsigned int drq, const char *name)
{
	struct task_struct *cur = current;
	int chan;

	chan = osenv_isadma_alloc(drq);

	current = cur;
	return chan;
}

/*
 * Free a DMA channel.
 */
void
free_dma(unsigned int drq)
{
	struct task_struct *cur = current;

	osenv_isadma_free(drq);

	current = cur;
}
#endif
