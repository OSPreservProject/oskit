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
 * One-time initialization for AMM (simple version)
 */
#include "amm.h"

/*
 * Initialize the given amm structure with the specified range.
 * The indicated range is marked as AMM_FREE and the remaining range as
 * AMM_RESERVED.
 */
void
amm_init(struct amm *amm, oskit_addr_t lo, oskit_addr_t hi)
{
	amm_init_gen(amm, AMM_FREE, 0, 0, 0, 0, 0);

	if (AMM_MINADDR < lo) {
		if (amm_modify(amm, AMM_MINADDR, lo - AMM_MINADDR,
			       AMM_RESERVED, 0))
			panic("amm_init: no memory");
	}
	if (hi < AMM_MAXADDR) {
		if (amm_modify(amm, hi, AMM_MAXADDR - hi, AMM_RESERVED, 0))
			panic("amm_init: no memory");
	}
}
