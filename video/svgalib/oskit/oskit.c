/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993, 1999 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */


/* The code is a bit of a mess; also note that the drawing functions */
/* are not speed optimized (the gl functions are much faster). */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "osenv.h"

int initialized = 0;	/* flag: initialize() called ?  */

extern unsigned char text_regs[MAX_REGS]; 	/* VGA registers for saved text mode */

/* saved text mode palette values */
extern unsigned char text_red[256];
extern unsigned char text_green[256];
extern unsigned char text_blue[256];

extern int prv_mode;	/* previous video mode      */


extern unsigned char *font_buf1;	/* saved font data - plane 2 */
extern unsigned char *font_buf2;	/* saved font data - plane 3 */


static void disable_interrupt(void)
{

}

static void enable_interrupt(void)
{

}

int iopl(int level)
{
    return 0;
}

int ioperm(unsigned long from, unsigned long num, int turn_on) 
{
    /* XXX should probably do _some_ parameter checking. */

    return 0;
}

static void __vga_mmap(void)
{
    int rc; 

    /* This assumes pl10+. */
    /* Still seems to waste 64K, so don't bother. */
#ifndef BACKGROUND
#ifdef OSKIT
    rc = osenv_mem_map_phys(GRAPH_BASE, GRAPH_SIZE, &GM, 0);
    /* XXX: do something with rc? */
#else
    GM = (unsigned char *) mmap(
				   (caddr_t) 0,
				   GRAPH_SIZE,
				   PROT_READ | PROT_WRITE,
				   MAP_SHARED,
				   __svgalib_mem_fd,
				   GRAPH_BASE
	);
#endif
#endif

    graph_mem = __svgalib_graph_mem;	/* Exported variable. */
}


void initialize(void)
{
    int i;

    oskit_svgalib_osenv_init();

    disable_interrupt();	/* Is reenabled later by set_texttermio */

    __svgalib_getchipset();		/* make sure a chipset has been selected */
    chipset_unlock();

    /* mmap graphics memory */
    __vga_mmap();

    if ((long) GM < 0) {
	printf("svgalib: mmap error \n");
	exit(1);
    }
    /* disable video */
    vga_screenoff();

    /* Sanity check: (from painful experience) */

    i = __svgalib_saveregs(text_regs);
    if (i > MAX_REGS) {
	puts("svgalib: FATAL internal error:");
	printf("Set MAX_REGS at least to %d in src/driver.h and recompile everything.\n",
	       i);
	exit(1);
    }
    /* This appears to fix the Trident 8900 rebooting problem. */
    if (__svgalib_chipset == TVGA8900) {
	port_out(0x0c, SEQ_I);	/* reg 12 */
	text_regs[EXT + 11] = port_in(SEQ_D);
	port_out(0x1f, __svgalib_CRT_I);
	text_regs[EXT + 12] = port_in(__svgalib_CRT_D);
    }
    /* save text mode palette - first select palette index 0 */
    port_out(0, PEL_IR);

    /* read RGB components - index is autoincremented */
    savepalette(text_red, text_green, text_blue);

    /* shift to color emulation */
    setcoloremulation();

    /* save font data - first select a 16 color graphics mode */
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->savefont) {
         __svgalib_driverspecs->emul->savefont();
    } else if(!__svgalib_novga) {
        __svgalib_driverspecs->setmode(GPLANE16, prv_mode);
    }

    /* Allocate space for textmode font. */
#ifndef BACKGROUND
    font_buf1 = malloc(FONT_SIZE * 2);
#endif
#ifdef BACKGROUND
    font_buf1 = valloc(FONT_SIZE * 2); 
#endif
    font_buf2 = font_buf1 + FONT_SIZE;

    /* save font data in plane 2 */
    port_out(0x04, GRA_I);
    port_out(0x02, GRA_D);
#ifdef __alpha__
    port_out(0x06, GRA_I);
    port_out(0x00, GRA_D);
#endif
#if defined(CONFIG_ALPHA_JENSEN)
    slowcpy_from_sm(font_buf1, SM, FONT_SIZE);
#else
    slowcpy(font_buf1, GM, FONT_SIZE);
#endif

    /* save font data in plane 3 */
    port_out(0x04, GRA_I);
    port_out(0x03, GRA_D);
#if defined(CONFIG_ALPHA_JENSEN)
    slowcpy_from_sm(font_buf2, SM, FONT_SIZE);
#else
    slowcpy(font_buf2, GM, FONT_SIZE);
#endif

    initialized = 1;
}


#ifndef BACKGROUND

void vga_lockvc(void)
{

}

void vga_unlockvc(void)
{

}

#ifndef OSKIT
void vga_runinbackground(int stat, ...)
{
    __svgalib_runinbackground = stat;
}
#endif

#endif
