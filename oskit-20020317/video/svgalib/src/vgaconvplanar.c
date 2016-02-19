/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* These svgalib additions by Harm Hanemaayer. */

#include <stdio.h>
#include "vga.h"
#include "libvga.h"


#if defined(__alpha__) || defined(OSKIT)
/*
 * OSKIT: GCC 2.95 problems.
 */
#undef USE_ASM
#else
#define USE_ASM
#endif


/* This function copies a linear virtual screen in system memory to a */
/* planar (Mode X-like) 256 color mode. */

/* Note that voffset is the video address of the top of the area to be    */
/* copied, that is, you can only use every fourth pixel as a destination. */
/* Similarly, vpitch is the logical video address width of the screen.    */
/* Also, the width must be a multiple of 4.                               */

void vga_copytoplanar256(unsigned char *virtual, int pitch, int voffset,
			 int vpitch, int w, int h)
{
    unsigned char *virtualp;
#ifdef USE_ASM
    unsigned char *voffsetp;
#else
    int voff;
#endif
    int plane, x = 0, y;

    for (plane = 0; plane < 4; plane++) {
	/* Copy pixels that belong in plane. */
	outb(SEQ_I, 0x02);
	outb(SEQ_D, 1 << plane);
	virtualp = virtual;
#ifdef USE_ASM
	voffsetp = __svgalib_graph_mem + voffset;
#else
	voff = voffset;
#endif
	for (y = 0; y < h; y++) {
	    /* Unrolled loop. */
#ifdef USE_ASM
	    __asm__ __volatile__(
				    "xorl %0,%0\n\t"
				    "jmp 2f\n\t"
				    ".align 2,0x90\n\t"

				    "1:\n\t"
				    "movb (%3,%0,4),%%al\n\t"	/* pixel 0 */
				    "movb 4(%3,%0,4),%%ah\n\t"	/* pixel 1 */
				    "movw %%ax,(%2,%0)\n\t"	/* write */
				    "movb 8(%3,%0,4),%%al\n\t"	/* pixel 2 */
				    "movb 12(%3,%0,4),%%ah\n\t"		/* pixel 3 */
				    "movw %%ax,2(%2,%0)\n\t"
				    "movb 16(%3,%0,4),%%al\n\t"		/* pixel 4 */
				    "movb 20(%3,%0,4),%%ah\n\t"		/* pixel 5 */
				    "movw %%ax,4(%2,%0)\n\t"
				    "movb 24(%3,%0,4),%%al\n\t"		/* pixel 6 */
				    "movb 28(%3,%0,4),%%ah\n\t"		/* pixel 7 */
				    "movw %%ax,6(%2,%0)\n\t"
				    "addl $8,%0\n\t"

				    "2:\n\t"
				    "leal 32(,%0,4),%%eax\n\t"	/* x * 4 + 32 */
				    "cmpl %4,%%eax\n\t"		/* < w? */
				    "jl 1b\n\t"

	    /* Do remaining pixels. */
				    "jmp 3f\n\t"
				    ".align 2,0x90\n\t"

				    "4:\n\t"
				    "movb (%3,%0,4),%%al\n\t"
				    "movb %%al,(%2,%0)\n\t"
				    "incl %0\n\t"
				    "3:\n\t"
				    "leal (,%0,4),%%eax\n\t"	/* x * 4 */
				    "cmpl %4,%%eax\n\t"		/* < w? */
				    "jl 4b\n\t"

				    :	/* output */
				    "=r"(x)
				    :	/* input */
				    "0"(x),
				    "r"(voffsetp),
				    "r"(virtualp + plane),
				    "rm"(w), "ax"(0)
				    :	/* modified */
				    "ax", "0"
	    );
#else
	    x = 0;
	    while (x * 4 + 32 < w) {
		gr_writeb(virtualp[x * 4 + plane + 0],
			  voff + x + 0);
		gr_writeb(virtualp[x * 4 + plane + 4],
			  voff + x + 1);
		gr_writeb(virtualp[x * 4 + plane + 8],
			  voff + x + 2);
		gr_writeb(virtualp[x * 4 + plane + 12],
			  voff + x + 3);
		gr_writeb(virtualp[x * 4 + plane + 16],
			  voff + x + 4);
		gr_writeb(virtualp[x * 4 + plane + 20],
			  voff + x + 5);
		gr_writeb(virtualp[x * 4 + plane + 24],
			  voff + x + 6);
		gr_writeb(virtualp[x * 4 + plane + 28],
			  voff + x + 7);
		x += 8;
	    }
	    while (x * 4 < w) {
		gr_writeb(virtualp[x * 4 + plane], voff + x);
		x++;
	    }
#endif
	    virtualp += pitch;	/* Next line. */
#ifdef USE_ASM
	    voffsetp += vpitch;
#else
	    voff += vpitch;
#endif
	}
    }
}


/* This is the equivalent function for planar 16 color modes; pixels are */
/* assumed to be stored in individual bytes ranging from 0 to 15 in the  */
/* virtual screen. It's very slow compared to the Mode X one. */

/* Here width must be a multiple of 8; video addresses are in units of 8 */
/* pixels. */

void vga_copytoplanar16(unsigned char *virtual, int pitch, int voffset,
			int vpitch, int w, int h)
{
    vga_copytoplane(virtual, pitch, voffset, vpitch, w, h, 0);
    vga_copytoplane(virtual, pitch, voffset, vpitch, w, h, 1);
    vga_copytoplane(virtual, pitch, voffset, vpitch, w, h, 2);
    vga_copytoplane(virtual, pitch, voffset, vpitch, w, h, 3);
}


void vga_copytoplane(unsigned char *virtual, int pitch, int voffset,
		     int vpitch, int w, int h, int plane)
{
    int x, y;
    unsigned char planemask;
    unsigned char *virtualp;

    outb(GRA_I, 0x01);		/* Disable set/reset. */
    outb(GRA_D, 0x00);

    outb(GRA_I, 0x08);		/* Write to all bits. */
    outb(GRA_D, 0xff);

    /* Copy pixels that belong in plane. */
    planemask = 1 << plane;
    virtualp = virtual;
    outb(SEQ_I, 0x02);
    outb(SEQ_D, planemask);
    for (y = 0; y < h; y++) {
	x = 0;
	while (x * 8 < w) {
	    unsigned char val;
	    val = 0;
	    if (virtualp[x * 8] & planemask)
		val += 0x80;
	    if (virtualp[x * 8 + 1] & planemask)
		val += 0x40;
	    if (virtualp[x * 8 + 2] & planemask)
		val += 0x20;
	    if (virtualp[x * 8 + 3] & planemask)
		val += 0x10;
	    if (virtualp[x * 8 + 4] & planemask)
		val += 0x08;
	    if (virtualp[x * 8 + 5] & planemask)
		val += 0x04;
	    if (virtualp[x * 8 + 6] & planemask)
		val += 0x02;
	    if (virtualp[x * 8 + 7] & planemask)
		val += 0x01;
	    gr_writeb(val, voffset + x);
	    x++;
	}
	virtualp += pitch;	/* Next line. */
	voffset += vpitch;
    }
}
