/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* Converted to especially ugly code and seriously hacked for Mach32: */
/* M. Weller in 1994                                                  */
#include <stdlib.h>

#include "vga.h"
#include "vgabg.h"
#include "libvga.h"
#include "driver.h"

static int current_index = -1;
static int setget = -1;		/* flag var to indicate last lut operation */

/*
 * In grayscale mode, we convert RGB colors to a Y component on the
 * green-channel (the Y component is used in grayscale TV sets for the
 * same purpose and corresponds to the "brightness" of the color as
 * perceived by the human eye.  In order to be able to return to the
 * user the original green-component, we save a backup copy of the
 * green channel in __svgalib_green_backup:
 */
int __svgalib_green_backup[256];


static int set_lut(int index, int red, int green, int blue)
{
    if (__svgalib_novga) return 1;
    
    /* prevents lockups */
    if ((__svgalib_chipset == MACH64)) {
        outb(0x02ec+0x5c00,index);
        outb(0x02ec+0x5c01,red);
        outb(0x02ec+0x5c01,green);
        outb(0x02ec+0x5c01,blue);
        return 0;
    }

    if ((index != current_index) || (setget != 1)) {
	/* select palette register */
	port_out(index, PEL_IW);
	current_index = index + 1;
	setget = 1;
    } else
	current_index++;
    /* write RGB components */
    __svgalib_delay();
    port_out(red, PEL_D);
    __svgalib_delay();
    port_out(green, PEL_D);
    if (SCREENON) {		/* writing the `blue' register will   */
	while (!(inb(0x3da) & 1));	/* load the dac. Waiting for vertical */
	while (inb(0x3da) & 1);	/* or horizontal retrace will load    */
    } else			/* the dac without disturbances       */
	__svgalib_delay();
    port_out(blue, PEL_D);
    return 0;
}


static int get_lut(int index, int *red, int *green, int *blue)
{
    if (__svgalib_novga) return 0;

    /* prevents lockups on mach64 */
    if ((__svgalib_chipset == MACH64)) {
        outb(0x02ec+0x5c00,index);
        *red=inb(0x02ec+0x5c01);
        *green=inb(0x02ec+0x5c01);
        *blue=inb(0x02ec+0x5c01);
        return 0;
    }

    if ((index != current_index) || (setget != 0)) {
	/* select palette register */
	port_out(index, PEL_IR);
	current_index = index + 1;
	setget = 0;
    } else
	current_index++;
    /* read RGB components */
    __svgalib_delay();
    *red = (int) port_in(PEL_D);
    __svgalib_delay();
    *green = (int) port_in(PEL_D);
    __svgalib_delay();
    *blue = (int) port_in(PEL_D);

    return 0;
}

#ifdef BACKGROUND

static void vga_setpalette_bg(int index, int red, int green, int blue)

{
 unsigned char *tmp;
 /* asume max 256 palette places. */
 if (index<256)
     {
      tmp=__svgalib_give_graph_red();
      tmp[index]=red;
      tmp=__svgalib_give_graph_blue();
      tmp[index]=blue;
      tmp=__svgalib_give_graph_green();
      tmp[index]=green;
     } 
 return;
}

#endif


int vga_setpalette(int index, int red, int green, int blue)
{
#ifdef BACKGROUND
 int tmp;
 
 __svgalib_dont_switch_vt_yet();
#endif
    if (__svgalib_grayscale) {
	if ((unsigned) index >= sizeof(__svgalib_green_backup) / sizeof(__svgalib_green_backup[0])) {
	    printf("vga_setpalette: color index %d out of range\n", index);
	}
	__svgalib_green_backup[index] = green;

	green = 0.299 * red + 0.587 * green + 0.114 * blue;
	if (green < 0)
	    green = 0;
	if (green > 255)
	    green = 255;
    }

#ifdef BACKGROUND
    if (!vga_oktowrite()) {
	vga_setpalette_bg(index, red, green, blue);
	__svgalib_is_vt_switching_needed();
	return(0);
    }
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) {
        tmp =__svgalib_driverspecs->emul->setpalette(index, red, green, blue);
    } else {
	tmp = set_lut(index, red, green, blue);
    }
    __svgalib_is_vt_switching_needed();
    return(tmp);
#else
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) {
        return __svgalib_driverspecs->emul->setpalette(index, red, green, blue);
    } else {
	return set_lut(index, red, green, blue);
    }
#endif
}

#ifdef BACKGROUND

static void vga_getpalette_bg(int index, int *red, int *green, int *blue)
{
 unsigned char *tmp;
 /* asume max 256 palette places. */
 if (index<256)
     {
      tmp=__svgalib_give_graph_red();
      *red=tmp[index];
      tmp=__svgalib_give_graph_blue();
      *blue=tmp[index];
      tmp=__svgalib_give_graph_green();
      *green=tmp[index];
     } 
 return;
}

#endif

int vga_getpalette(int index, int *red, int *green, int *blue)
{
#ifdef BACKGROUND
 __svgalib_dont_switch_vt_yet();
#endif
#ifndef BACKGROUND
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->getpalette) 
        __svgalib_driverspecs->emul->getpalette(index, red, green, blue);
    else get_lut(index, red, green, blue);
#endif
#ifdef BACKGROUND
 if (!vga_oktowrite())
     {
      vga_getpalette_bg(index, red, green, blue);
     }
   else {
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->getpalette) 
        __svgalib_driverspecs->emul->getpalette(index, red, green, blue);
    else get_lut(index, red, green, blue);
   }
#endif
    if (__svgalib_grayscale) {
	if ((unsigned) index >= sizeof(__svgalib_green_backup) / sizeof(__svgalib_green_backup[0])) {
	    printf("vga_getpalette: color index %d out of range\n", index);
	}
	*green = __svgalib_green_backup[index];
    }
#ifdef BACKGROUND
 __svgalib_is_vt_switching_needed();
#endif
    return 0;
}


int vga_setpalvec(int start, int num, int *pal)
{
    int i;

    for (i = start; i < start + num; ++i) {
	vga_setpalette(i, pal[0], pal[1], pal[2]);
	pal += 3;
    }
    return num;
}


int vga_getpalvec(int start, int num, int *pal)
{
    int i;

    for (i = start; i < start + num; ++i) {
	vga_getpalette(i, pal + 0, pal + 1, pal + 2);
	pal += 3;
    }
    return num;
}
