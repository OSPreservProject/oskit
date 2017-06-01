/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
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
 * start_conf_network_eifname.c
 */

#include <assert.h>
#include <stdlib.h>
#include <oskit/startup.h>

#define _OSKIT_FR33BSD_SINTAx_RUL3Z
// #define _OSKIT_LINUX_SINTAx_RUL3Z

/*
 * Pretty names for ethernet interfaces.
 * As far as I can tell, the name must end in an ascii digit,
 * and there is a max of nine interfaces.  FreeBSD names
 * are probably safest (considering that's the default
 * network stack).
 */
static const char *eif_names[] =
{
#ifdef _OSKIT_FR33BSD_SINTAx_RUL3Z
	"de0",
	"de1",
	"de2",
	"de3",
	"de4",
	"de5",
	"de6",
	"de7",
	"de8",
	"de9",
#else
	"eth0",
	"eth1",
	"eth2",
	"eth3",
	"eth4",
	"eth5",
	"eth6",
	"eth7",
	"eth8",
	"eth9",
#endif
};

/*
 * start_conf_network_ifnames.c
 */
const char *
start_conf_network_eifname(int i)
{
	if (i < 0 || i >= sizeof(eif_names)/sizeof(eif_names[0]))
		return NULL;
	return eif_names[i];
}

