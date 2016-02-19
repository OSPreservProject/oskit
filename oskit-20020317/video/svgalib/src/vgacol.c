/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

#include <stdlib.h>

#include "vga.h"
#include "libvga.h"

int vga_setrgbcolor(int r, int g, int b)
{
    switch (CI.colors) {
    case 32768:
	COL =
	    (b >> 3) +
	    ((g >> 3) << 5) +
	    ((r >> 3) << 10);
	break;
    case 65536:
	COL =
	    (b >> 3) +
	    ((g >> 2) << 5) +
	    ((r >> 3) << 11);
	break;
    case 1 << 24:
	COL = b + (g << 8) + (r << 16);
	break;
    default:
	return 0;
    }
    return COL;
}

static const unsigned char ega_red[16] =
{0, 0, 0, 0, 168, 168, 168, 168, 84, 84, 84, 84, 255, 255, 255, 255};
static const unsigned char ega_green[16] =
{0, 0, 168, 168, 0, 0, 84, 168, 84, 84, 255, 255, 84, 84, 255, 255};
static const unsigned char ega_blue[16] =
{0, 168, 0, 168, 0, 168, 0, 168, 84, 255, 84, 255, 84, 255, 84, 255};

int vga_setegacolor(int c)
{
    if (c < 0)
	c = 0;
    else if (c > 15)
	c = 15;
    switch (CI.colors) {
    case 1 << 15:
    case 1 << 16:
    case 1 << 24:
	return vga_setrgbcolor(ega_red[c], ega_green[c], ega_blue[c]);
    }
    vga_setcolor(c);
    return c;
}
