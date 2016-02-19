/*
 * Copyright (c) 1996-2000 University of Utah and the Flux Group.
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
#include <stdio.h>
#include <oskit/startup.h>

#ifdef GPROF
/*
 * These are defined in the C libraries.
 */
void _mcleanup();
void monstartup(unsigned long lowpc, unsigned long highpc);
void moncontrol(int mode);

/*
 * This is the hook into gprof executable's main.
 * Defined in the gprof library.
 */
void gprof_atexit();

/*
 * Special symbols.
 */
extern unsigned long _start;
extern unsigned long end;
extern unsigned long etext;

void
start_gprof(void)
{
	extern int enable_gprof; /* XXX Kern library */

	if (! enable_gprof) {
		printf("start_gprof: GPROF is not enabled in kern library!");
		return;
	}
	
	startup_atexit(gprof_atexit, NULL);
	startup_atexit(_mcleanup, NULL);
	monstartup((unsigned long)&_start, (unsigned long)&etext);
}

void
pause_gprof(int onoff)
{
	moncontrol(onoff);
}

/*
 * Override this to prevent it from happening out of multiboot_main.
 */
void
base_gprof_init()
{
}
#endif
