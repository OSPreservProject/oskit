
/* 
 * Generic mode timing module.
 */
#include <stdlib.h>

#include "timing.h"		/* Types. */

#include "driver.h"		/* for __svgalib_monitortype (remove me) */

/* Standard mode timings. */

MonitorModeTiming __svgalib_standard_timings[] =
{
#define S __svgalib_standard_timings
/* 320x200 @ 70 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 400, 409, 411, 450, DOUBLESCAN, S + 1},
/* 320x200 @ 83 Hz, 37.5 kHz hsync */
    {13333, 320, 336, 384, 400, 400, 409, 411, 450, DOUBLESCAN, S + 2},
/* 320x240 @ 60 Hz, 31.5 kHz hsync */
    {12588, 320, 336, 384, 400, 480, 491, 493, 525, DOUBLESCAN, S + 3},
/* 320x240 @ 72 Hz, 36.5 kHz hsync */
    {15750, 320, 336, 384, 400, 480, 488, 491, 521, DOUBLESCAN, S + 4},
/* 640x400 at 70 Hz, 31.5 kHz hsync */
    {25175, 640, 664, 760, 800, 400, 409, 411, 450, 0, S + 5},
/* 640x480 at 60 Hz, 31.5 kHz hsync */
    {25175, 640, 664, 760, 800, 480, 491, 493, 525, 0, S + 6},
/* 640x480 at 72 Hz, 36.5 kHz hsync */
    {31500, 640, 680, 720, 864, 480, 488, 491, 521, 0, S + 7},
/* 800x600 at 56 Hz, 35.15 kHz hsync */
    {36000, 800, 824, 896, 1024, 600, 601, 603, 625, 0, S + 8},
/* 800x600 at 60 Hz, 37.8 kHz hsync */
    {40000, 800, 840, 968, 1056, 600, 601, 605, 628, PHSYNC | PVSYNC, S + 9},
/* 1024x768 at 87 Hz interlaced, 35.5 kHz hsync */
    {44900, 1024, 1048, 1208, 1264, 768, 776, 784, 817, INTERLACED, S + 10},
/* 800x600 at 72 Hz, 48.0 kHz hsync */
    {50000, 800, 856, 976, 1040, 600, 637, 643, 666, PHSYNC | PVSYNC, S + 11},
/* 1024x768 at 87 Hz interlaced, 35.5 kHz hsync */
    {44900, 1024, 1048, 1208, 1264, 768, 776, 784, 817, INTERLACED, S + 12},
/* 1024x768 at 100 Hz, 40.9 kHz hsync */
    {55000, 1024, 1048, 1208, 1264, 768, 776, 784, 817, INTERLACED, S + 13},
/* 1024x768 at 60 Hz, 48.4 kHz hsync */
    {65000, 1024, 1032, 1176, 1344, 768, 771, 777, 806, NHSYNC | NVSYNC, S + 14},
/* 1024x768 at 70 Hz, 56.6 kHz hsync */
    {75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, NHSYNC | NVSYNC, S + 15},
/* 1280x1024 at 87 Hz interlaced, 51 kHz hsync */
    {80000, 1280, 1296, 1512, 1568, 1024, 1025, 1037, 1165, INTERLACED, S + 16},
/* 1024x768 at 76 Hz, 62.5 kHz hsync */
    {85000, 1024, 1032, 1152, 1360, 768, 784, 787, 823, 0, S + 17},
/* 1280x1024 at 60 Hz, 64.3 kHz hsync */
    {110000, 1280, 1328, 1512, 1712, 1024, 1025, 1028, 1054, 0, S + 18},
/* 1280x1024 at 74 Hz, 78.9 kHz hsync */
    {135000, 1280, 1312, 1456, 1712, 1024, 1027, 1030, 1064, 0, NULL}
#undef S
};

#define NUMBER_OF_STANDARD_MODES \
	(sizeof(__svgalib_standard_timings) / sizeof(__svgalib_standard_timings[0]))

static MonitorModeTiming *user_timings = NULL;

/*
 * SYNC_ALLOWANCE is in percent
 * 1% corresponds to a 315 Hz deviation at 31.5 kHz, 1 Hz at 100 Hz
 */
#define SYNC_ALLOWANCE 1

#define INRANGE(x,y) \
    ((x) > __svgalib_##y.min * (1.0f - SYNC_ALLOWANCE / 100.0f) && \
     (x) < __svgalib_##y.max * (1.0f + SYNC_ALLOWANCE / 100.0f))

/*
 * Check monitor spec.
 */
static int timing_within_monitor_spec(MonitorModeTiming * mmtp)
{
    float hsf;			/* Horz. sync freq in Hz */
    float vsf;			/* Vert. sync freq in Hz */

    hsf = mmtp->pixelClock * 1000.0f / mmtp->HTotal;
    vsf = hsf / mmtp->VTotal;
    if (mmtp->flags & INTERLACED)
	vsf *= 2.0f;
#ifdef DEBUG
    printf("hsf = %f (in:%d), vsf = %f (in:%d)\n",
	   hsf / 1000, (int) INRANGE(hsf, horizsync),
	   vsf, (int) INRANGE(vsf, vertrefresh));
#endif
    return INRANGE(hsf, horizsync) && INRANGE(vsf, vertrefresh);
}

void __svgalib_addusertiming(MonitorModeTiming * mmtp)
{
    MonitorModeTiming *newmmt;

    if (!(newmmt = malloc(sizeof(*newmmt))))
	return;
    *newmmt = *mmtp;
    newmmt->next = user_timings;
    user_timings = newmmt;
}

/*
 * The __svgalib_getmodetiming function looks up a mode in the standard mode
 * timings, choosing the mode with the highest dot clock that matches
 * the requested svgalib mode, and is supported by the hardware
 * (card limits, and monitor type). cardlimits points to a structure
 * of type CardSpecs that describes the dot clocks the card supports
 * at different depths. Returns non-zero if no mode is found.
 */

/*
 * findclock is an auxilliary function that checks if a close enough
 * pixel clock is provided by the card. Returns clock number if
 * succesful (a special number if a programmable clock must be used), -1
 * otherwise.
 */

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs * cardspecs)
{
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
	int diff;
	diff = cardspecs->clocks[i] - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return i;
    }
    /* Try programmable clocks if available. */
    if (cardspecs->flags & CLOCK_PROGRAMMABLE) {
	int diff;
	diff = cardspecs->matchProgrammableClock(clock) - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return PROGRAMMABLE_CLOCK_MAGIC_NUMBER;
    }
    /* No close enough clock found. */
    return -1;
}

static MonitorModeTiming *search_mode(MonitorModeTiming * timings,
				      int maxclock,
				      ModeInfo * modeinfo,
				      CardSpecs * cardspecs)
{
    int bestclock = 0;
    MonitorModeTiming *besttiming = NULL, *t;

    /*
     * bestclock is the highest pixel clock found for the resolution
     * in the mode timings, within the spec of the card and
     * monitor.
     * besttiming holds a pointer to timing with this clock.
     */

    /* Search the timings for the best matching mode. */
    for (t = timings; t; t = t->next)
	if (t->HDisplay == modeinfo->width
	    && (t->VDisplay == modeinfo->height
		|| ((t->flags & DOUBLESCAN)
		    && t->VDisplay ==
		    modeinfo->height * 2))
	    && timing_within_monitor_spec(t)
	    && t->pixelClock <= maxclock
	    && t->pixelClock > bestclock
	    && cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					    t->pixelClock,
					    t->HTotal)
	    <= cardspecs->maxHorizontalCrtc
	/* Find the clock (possibly scaled by mapClock). */
	    && findclock(cardspecs->mapClock(modeinfo->bitsPerPixel,
					 t->pixelClock), cardspecs) != -1
	    ) {
	    bestclock = t->pixelClock;
	    besttiming = t;
	}
    return besttiming;
}

int __svgalib_getmodetiming(ModeTiming * modetiming, ModeInfo * modeinfo,
		  CardSpecs * cardspecs)
{
    int maxclock, desiredclock;
    MonitorModeTiming *besttiming;

    /* Get the maximum pixel clock for the depth of the requested mode. */
    if (modeinfo->bitsPerPixel == 4)
	maxclock = cardspecs->maxPixelClock4bpp;
    else if (modeinfo->bitsPerPixel == 8)
	maxclock = cardspecs->maxPixelClock8bpp;
    else if (modeinfo->bitsPerPixel == 16) {
	if ((cardspecs->flags & NO_RGB16_565)
	    && modeinfo->greenWeight == 6)
	    return 1;		/* No 5-6-5 RGB. */
	maxclock = cardspecs->maxPixelClock16bpp;
    } else if (modeinfo->bitsPerPixel == 24)
	maxclock = cardspecs->maxPixelClock24bpp;
    else if (modeinfo->bitsPerPixel == 32)
	maxclock = cardspecs->maxPixelClock32bpp;
    else
	maxclock = 0;

    /*
     * Check user defined timings first.
     * If there is no match within these, check the standard timings.
     */
    besttiming = search_mode(user_timings, maxclock, modeinfo, cardspecs);
    if (!besttiming) {
	besttiming = search_mode(__svgalib_standard_timings, maxclock, modeinfo, cardspecs);
	if (!besttiming)
	    return 1;
    }
    /*
     * Copy the selected timings into the result, which may
     * be adjusted for the chipset.
     */

    modetiming->flags = besttiming->flags;
    modetiming->pixelClock = besttiming->pixelClock;	/* Formal clock. */

    /*
     * We know a close enough clock is available; the following is the
     * exact clock that fits the mode. This is probably different
     * from the best matching clock that will be programmed.
     */
    desiredclock = cardspecs->mapClock(modeinfo->bitsPerPixel,
				       besttiming->pixelClock);

    /* Fill in the best-matching clock that will be programmed. */
    modetiming->selectedClockNo = findclock(desiredclock, cardspecs);
    if (modetiming->selectedClockNo == PROGRAMMABLE_CLOCK_MAGIC_NUMBER) {
	modetiming->programmedClock =
	    cardspecs->matchProgrammableClock(desiredclock);
	modetiming->flags |= USEPROGRCLOCK;
    } else
	modetiming->programmedClock = cardspecs->clocks[
					    modetiming->selectedClockNo];
    modetiming->HDisplay = besttiming->HDisplay;
    modetiming->HSyncStart = besttiming->HSyncStart;
    modetiming->HSyncEnd = besttiming->HSyncEnd;
    modetiming->HTotal = besttiming->HTotal;
    if (cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
				     modetiming->programmedClock,
				     besttiming->HTotal)
	!= besttiming->HTotal) {
	/* Horizontal CRTC timings are scaled in some way. */
	modetiming->CrtcHDisplay =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HDisplay);
	modetiming->CrtcHSyncStart =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HSyncStart);
	modetiming->CrtcHSyncEnd =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HSyncEnd);
	modetiming->CrtcHTotal =
	    cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel,
					 modetiming->programmedClock,
					 besttiming->HTotal);
	modetiming->flags |= HADJUSTED;
    } else {
	modetiming->CrtcHDisplay = besttiming->HDisplay;
	modetiming->CrtcHSyncStart = besttiming->HSyncStart;
	modetiming->CrtcHSyncEnd = besttiming->HSyncEnd;
	modetiming->CrtcHTotal = besttiming->HTotal;
    }
    modetiming->VDisplay = besttiming->VDisplay;
    modetiming->CrtcVDisplay = besttiming->VDisplay;
    modetiming->VSyncStart = besttiming->VSyncStart;
    modetiming->CrtcVSyncStart = besttiming->VSyncStart;
    modetiming->VSyncEnd = besttiming->VSyncEnd;
    modetiming->CrtcVSyncEnd = besttiming->VSyncEnd;
    modetiming->VTotal = besttiming->VTotal;
    modetiming->CrtcVTotal = besttiming->VTotal;
    if (((modetiming->flags & INTERLACED)
	 && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
	|| (modetiming->VTotal >= 1024
	    && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
	/*
	 * Card requires vertical CRTC timing to be halved for
	 * interlaced modes, or for all modes with vertical
	 * timing >= 1024.
	 */
	modetiming->CrtcVDisplay /= 2;
	modetiming->CrtcVSyncStart /= 2;
	modetiming->CrtcVSyncEnd /= 2;
	modetiming->CrtcVTotal /= 2;
	modetiming->flags |= VADJUSTED;
    }
    return 0;			/* Succesful. */
}
