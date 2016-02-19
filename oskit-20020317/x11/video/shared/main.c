/***********************************************************

Copyright (c) 1987, 1999  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: main.c /main/82 1996/09/28 17:12:09 rws $ */
/* $XFree86: xc/programs/Xserver/dix/main.c,v 3.10.2.1 1997/06/01 12:33:21 dawes Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#ifndef OSKIT
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "extension.h"
#include "extnsionst.h"
#include "colormap.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "font.h"
#include "opaque.h"
#include "servermd.h"
#include "site.h"
#include "dixfont.h"
#include "dixevents.h"		/* InitEvents() */
#include "dispatch.h"		/* InitProcVectors() */
#endif

#ifdef OSKIT
#include "osenv.h"
#endif

extern CARD32 defaultScreenSaverTime;
extern CARD32 defaultScreenSaverInterval;
extern int defaultScreenSaverBlanking;
extern int defaultScreenSaverAllowExposures;

#ifdef DPMSExtension
#include "dpms.h"
#endif

void ddxGiveUp();

extern int InitClientPrivates(
#if NeedFunctionPrototypes
    ClientPtr /*client*/
#endif
);

extern void Dispatch(
#if NeedFunctionPrototypes
    void
#endif
);

extern char *display;
char *ConnectionInfo;
xConnSetupPrefix connSetupPrefix;

extern int screenPrivateCount;

void FreeScreen(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/
#endif
);

#ifdef OSKIT
ScreenInfo screenInfo;
#endif

/*
 * Dummy entry for EventSwapVector[]
 */
/*ARGSUSED*/
void
NotImplemented(
#if NeedFunctionPrototypes && defined(EVENT_SWAP_PTR)
	xEvent * from,
	xEvent * to
#endif
	)
{
    FatalError("Not implemented");
}

/*
 * Dummy entry for ReplySwapVector[]
 */
/*ARGSUSED*/
void
ReplyNotSwappd(
#if NeedNestedPrototypes
	ClientPtr pClient ,
	int size ,
	void * pbuf
#endif
	)
{
    FatalError("Not implemented");
}


/*
 * This array gives the bytesperPixel value for cases where the number
 * of bits per pixel is a multiple of 8 but not a power of 2.
 */
static int answerBytesPerPixel[ 33 ] = {
	~0, 0, ~0, ~0,	/* 1 bit per pixel */
	0, ~0, ~0, ~0,	/* 4 bits per pixel */
	0, ~0, ~0, ~0,	/* 8 bits per pixel */
	~0,~0, ~0, ~0,
	0, ~0, ~0, ~0,	/* 16 bits per pixel */
	~0,~0, ~0, ~0,
	3, ~0, ~0, ~0,	/* 24 bits per pixel */
	~0,~0, ~0, ~0,
	0		/* 32 bits per pixel */
};


int
#ifdef OSKIT
x11_main()
#else
main(argc, argv)
    int		argc;
    char	*argv[];
#endif
{
    int		i, j, k;

#ifdef OSKIT
    oskit_x11_osenv_init();
#else
    /* Perform any operating system dependent initializations you'd like */
    OsInit();		
#endif

    screenInfo.arraySize = MAXSCREENS;
    screenInfo.numScreens = 0;
    screenInfo.numVideoScreens = -1;
    
    ResetScreenPrivates();

#ifndef OSKIT    
    ResetColormapPrivates();
#endif

    InitOutput(&screenInfo);
    
    if (screenInfo.numScreens < 1)
	    FatalError("no screens found");
    if (screenInfo.numVideoScreens < 0)
	    screenInfo.numVideoScreens = screenInfo.numScreens;


#ifndef OSKIT
    for (i = screenInfo.numScreens - 1; i >= 0; i--)
    {
	    (* screenInfo.screens[i]->CloseScreen)(i, screenInfo.screens[i]);
	    FreeScreen(screenInfo.screens[i]);
	    screenInfo.numScreens = i;
    }	
#endif

    return(0);
}


/*
	grow the array of screenRecs if necessary.
	call the device-supplied initialization procedure 
with its screen number, a pointer to its ScreenRec, argc, and argv.
	return the number of successfully installed screens.

*/

int
#if NeedFunctionPrototypes
AddScreen(
    Bool	(* pfnInit)(
#if NeedNestedPrototypes
	int /*index*/,
	ScreenPtr /*pScreen*/,
	int /*argc*/,
	char ** /*argv*/
#endif
		),
    int argc,
    char **argv)
#else
AddScreen(pfnInit, argc, argv)
    Bool	(* pfnInit)();
    int argc;
    char **argv;
#endif
{

    int i;
    int scanlinepad, format, depth, bitsPerPixel, j, k;
    ScreenPtr pScreen;
#ifdef DEBUG
    void	(**jNI) ();
#endif /* DEBUG */

    i = screenInfo.numScreens;
    if (i == MAXSCREENS)
	return -1;

    pScreen = (ScreenPtr) xalloc(sizeof(ScreenRec));
    if (!pScreen)
	return -1;

    pScreen->devPrivates = (DevUnion *)xalloc(screenPrivateCount *
					      sizeof(DevUnion));
    if (!pScreen->devPrivates && screenPrivateCount)
    {
	xfree(pScreen);
	return -1;
    }
    pScreen->myNum = i;
  
    /* This is where screen specific stuff gets initialized.  Load the
       screen structure, call the hardware, whatever.
       This is also where the default colormap should be allocated and
       also pixel values for blackPixel, whitePixel, and the cursor
       Note that InitScreen is NOT allowed to modify argc, argv, or
       any of the strings pointed to by argv.  They may be passed to
       multiple screens. 
    */ 
    screenInfo.screens[i] = pScreen;
    screenInfo.numScreens++;

    if (!(*pfnInit)(i, pScreen, argc, argv))
    {
	FreeScreen(pScreen);
	screenInfo.numScreens--;
	return -1;
    }
    return i;
}

void
FreeScreen(pScreen)
    ScreenPtr pScreen;
{
    xfree(pScreen->devPrivates);
    xfree(pScreen);
}
