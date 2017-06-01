/* $XConsortium: privates.c /main/5 1996/06/17 10:56:22 mor $ */
/* $XFree86: xc/programs/Xserver/dix/privates.c,v 3.2 1997/01/23 10:57:19 dawes Exp $ */
/*

Copyright (c) 1993  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

#include "X.h"
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#ifndef OSKIT
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "servermd.h"
#include "site.h"
#endif

/*
 *  See the Wrappers and devPrivates section in "Definition of the
 *  Porting Layer for the X v11 Sample Server" (doc/Server/ddx.tbl.ms)
 *  for information on how to use devPrivates.
 */

/*
 *  screen private machinery
 */

int  screenPrivateCount;

void
ResetScreenPrivates()
{
    screenPrivateCount = 0;
}

/* this can be called after some screens have been created,
 * so we have to worry about resizing existing devPrivates
 */
int
AllocateScreenPrivateIndex()
{
    int		index2;
    int		i;
    ScreenPtr	pScreen;
    DevUnion	*nprivs;

    index2 = screenPrivateCount++;
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	nprivs = (DevUnion *)xrealloc(pScreen->devPrivates,
				      screenPrivateCount * sizeof(DevUnion));
	if (!nprivs)
	{
	    screenPrivateCount--;
	    return -1;
	}
	pScreen->devPrivates = nprivs;
    }
    return index2;
}

#ifndef OSKIT
/*
 *  colormap private machinery
 */

int  colormapPrivateCount;

void
ResetColormapPrivates()
{
    colormapPrivateCount = 0;
}


int
AllocateColormapPrivateIndex (initPrivFunc)

InitCmapPrivFunc initPrivFunc;

{
    int		index;
    int		i;
    ColormapPtr	pColormap;
    DevUnion	*privs;

    index = colormapPrivateCount++;

    for (i = 0; i < screenInfo.numScreens; i++)
    {
	/*
	 * AllocateColormapPrivateIndex may be called after the
	 * default colormap has been created on each screen!
	 *
	 * We must resize the devPrivates array for the default
	 * colormap on each screen, making room for this new private.
	 * We also call the initialization function 'initPrivFunc' on
	 * the new private allocated for each default colormap.
	 */

	ScreenPtr pScreen = screenInfo.screens[i];

	pColormap = (ColormapPtr) LookupIDByType (
	    pScreen->defColormap, RT_COLORMAP);

	privs = (DevUnion *) xrealloc (pColormap->devPrivates,
	    colormapPrivateCount * sizeof(DevUnion));

	pColormap->devPrivates = privs;

	if (!privs || !(*initPrivFunc)(pColormap))
	{
	    colormapPrivateCount--;
	    return -1;
	}
    }

    return index;
}
#endif /* !OSKIT */
