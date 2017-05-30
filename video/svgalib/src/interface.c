
#include <stdlib.h>
#include <string.h>

#include "timing.h"
#include "libvga.h"		/* for __svgalib_infotable and ramdac inlines */
#include "ramdac/ramdac.h"
#include "accel.h"


/*
 * This is a temporary function that allocates and fills in a ModeInfo
 * structure based on a svgalib mode number.
 */

ModeInfo *
 __svgalib_createModeInfoStructureForSvgalibMode(int mode)
{
    ModeInfo *modeinfo;
    /* Create the new ModeInfo structure. */
    modeinfo = malloc(sizeof(ModeInfo));
    modeinfo->width = __svgalib_infotable[mode].xdim;
    modeinfo->height = __svgalib_infotable[mode].ydim;
    modeinfo->bytesPerPixel = __svgalib_infotable[mode].bytesperpixel;
    switch (__svgalib_infotable[mode].colors) {
    case 16:
	modeinfo->colorBits = 4;
	break;
    case 256:
	modeinfo->colorBits = 8;
	break;
    case 32768:
	modeinfo->colorBits = 15;
	modeinfo->blueOffset = 0;
	modeinfo->greenOffset = 5;
	modeinfo->redOffset = 10;
	modeinfo->blueWeight = 5;
	modeinfo->greenWeight = 5;
	modeinfo->redWeight = 5;
	break;
    case 65536:
	modeinfo->colorBits = 16;
	modeinfo->blueOffset = 0;
	modeinfo->greenOffset = 5;
	modeinfo->redOffset = 11;
	modeinfo->blueWeight = 5;
	modeinfo->greenWeight = 6;
	modeinfo->redWeight = 5;
	break;
    case 256 * 65536:
	modeinfo->colorBits = 24;
	modeinfo->blueOffset = 0;
	modeinfo->greenOffset = 8;
	modeinfo->redOffset = 16;
	modeinfo->blueWeight = 8;
	modeinfo->greenWeight = 8;
	modeinfo->redWeight = 8;
	break;
    }
    modeinfo->bitsPerPixel = modeinfo->bytesPerPixel * 8;
    if (__svgalib_infotable[mode].colors == 16)
	modeinfo->bitsPerPixel = 4;
    modeinfo->lineWidth = __svgalib_infotable[mode].xbytes;
    return modeinfo;
}


/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */

int __svgalib_colorbits_to_colormode(int bpp, int colorbits)
{
    if (colorbits == 8)
	return CLUT8_6;
    if (colorbits == 15)
	return RGB16_555;
    if (colorbits == 16)
	return RGB16_565;
    if (colorbits == 24) {
	if (bpp == 24)
	    return RGB24_888_B;
	else
	    return RGB32_888_B;
    }
    return CLUT8_6;
}


/*
 * Clear the accelspecs structure (disable acceleration).
 */

void __svgalib_clear_accelspecs(AccelSpecs * accelspecs)
{
    memset(accelspecs, 0, sizeof(AccelSpecs));
}
