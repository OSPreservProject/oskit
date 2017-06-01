/*
 * Copyright (c) 1997-1999 University of Utah and the Flux Group.
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

#include "X.h"
#ifndef OSKIT
#include "input.h"
#endif
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "osenv.h"

#include <sys/mman.h>

#ifndef OSKIT
typedef char * caddr_t;
#endif

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

#if 0
static struct xf86memMap {
  int offset;
  int memSize;
} xf86memMaps[MAXSCREENS];
#endif

static Bool linearmem = FALSE;


pointer xf86MapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
	pointer base;
	int rc;

	if ((rc = osenv_mem_map_phys(Base, Size, &base, 0)))
		return  -1;

#if 0
	xf86memMaps[ScreenNum].offset = (int) Base;
	xf86memMaps[ScreenNum].memSize = Size;
#endif

	linearmem = TRUE;

	return base;
}


#if 0
void xf86GetVidMemData(ScreenNum, Base, Size)
int ScreenNum;
int *Base;
int *Size;      
{              
   *Base = xf86memMaps[ScreenNum].offset;
   *Size = xf86memMaps[ScreenNum].memSize;
}
#endif
                             
void xf86UnMapVidMem(ScreenNum, Region, Base, Size)
int ScreenNum;
int Region;
pointer Base;
unsigned long Size;
{
#ifdef NOTYET
	osenv_mem_unmap_phys((caddr_t)Base, Size);
#endif
}

Bool xf86LinearVidMem()
{
	return(TRUE);	/* XXX for now we assume that we'll be able to map
			 * physical memory.
			 */
}


/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

static Bool ScreenEnabled[MAXSCREENS];
static Bool InitDone = FALSE;

void
xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	if (!InitDone)
	{
		int i;
		for (i = 0; i < MAXSCREENS; i++)
			ScreenEnabled[i] = FALSE;
		InitDone = TRUE;
	}
	return;
}

void
xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
	return;
}

void
xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{
	ScreenEnabled[ScreenNum] = TRUE;

	return;
}
	
void
xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
	ScreenEnabled[ScreenNum] = FALSE;

	return;
}


void xf86DisableIOPrivs()
{
	return;
}



/***************************************************************************/
/* Interrupt Handling section                                              */
/***************************************************************************/

Bool xf86DisableInterrupts()
{
	osenv_intr_disable();

	return(TRUE);
}

void xf86EnableInterrupts()
{
	osenv_intr_enable();
}

