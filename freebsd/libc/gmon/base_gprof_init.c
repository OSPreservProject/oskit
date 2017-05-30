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

#include <stdlib.h>

#ifdef GPROF

void _mcleanup();
void monstartup(unsigned long lowpc, unsigned long highpc);
void gprof_atexit();  /* hook into gprof executable's main */
extern unsigned long _start;
extern unsigned long end;
extern unsigned long etext;

void
base_gprof_init()
{
	/*
	 * Stub this for now. This stuff happens to early (before the
	 * clientos stuff is ready and malloc will work proprely).
	 *
	 * Instead, see start_gprof() in the startup library.
	 */
}
#endif
