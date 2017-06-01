/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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

#include <X.h>

#include <xf86.h>
#include <xf86Priv.h>

#include "s3.h"
#include "s3linear.h"

extern Bool xf86ScreensOpen;

extern void FreeScreen(
    ScreenPtr /*pScreen*/
);


DisplayModePtr
oskit_s3_init_mode(int x) 
{
	DisplayModePtr p;

	p = xf86Screens[0]->modes;
	
	if (x < 0)
		return;

	while (x && p->next) {
		p = p->next;
		x--;
	}

        if (!p || x) {
		printf("%s: No mode %d!\n", __FUNCTION__, x);
		return;
	}
		     
	s3Init(p);

	/* XXX needed? */
	s3EnableLinear();

	return p;
}

void
oskit_s3_cleanup(void) {
	int i;

	s3DisableLinear();

	if (xf86ScreensOpen)
		for ( i=0; i < xf86MaxScreens && xf86Screens[i] && 
			xf86Screens[i]->configured; i++ ) {
			(xf86Screens[i]->EnterLeaveVT)(LEAVE, i);
		}

        for (i = screenInfo.numScreens - 1; i >= 0; i--)
        {
            (* screenInfo.screens[i]->CloseScreen)(i, screenInfo.screens[i]);
            FreeScreen(screenInfo.screens[i]);
            screenInfo.numScreens = i;
        }
}
