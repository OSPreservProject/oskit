/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* HH: Added 4bpp support, and use bytesperpixel. */

#include <stdio.h>
#include "vga.h"
#include "libvga.h"
#ifdef BACKGROUND
#include "vgabg.h"
#endif

static inline void read_write(unsigned long off)
{
    gr_writeb(gr_readb(off) + 1, off);
}

static inline int RGB2BGR(int c)
{
/* a bswap would do the same as the first 3 but in only ONE! cycle. */
/* However bswap is not supported by 386 */

    if (MODEFLAGS & RGB_MISORDERED)
#ifdef __alpha__
	c = ((c >> 0) & 0xff) << 16 |
	    ((c >> 8) & 0xff) << 8 |
	    ((c >> 16) & 0xff) << 0;
#else
	asm("rorw  $8, %0\n"	/* 0RGB -> 0RBG */
	    "rorl $16, %0\n"	/* 0RBG -> BG0R */
	    "rorw  $8, %0\n"	/* BG0R -> BGR0 */
	    "shrl  $8, %0\n"	/* 0BGR -> 0BGR */
      : "=q"(c):"0"(c));
#endif
    return c;
}

#ifdef BACKGROUND

/* Never define ___OK___ */

static int vga_drawpixel_bg(int x, int y)

{
 unsigned long offset;
 int c;
 unsigned char tmp;
  
    if (MODEX) {
	/* select plane */
#if 0
	port_out(0x02, SEQ_I);
	port_out(1 << (x & 3), SEQ_D);
	
	/* write color to pixel */
#endif
	if (1<<(x&3)==1) __svgalib_fast_setpage(0);
	if (1<<(x&3)==2) __svgalib_fast_setpage(1);
	if (1<<(x&3)==4) __svgalib_fast_setpage(2);
	if (1<<(x&3)==8) __svgalib_fast_setpage(3);
	gr_writeb(COL, y * CI.xbytes + (x >> 2));
	vga_setpage(0); /* Page must set back to 0. */
	return 0;
    }
    switch (__svgalib_cur_info.bytesperpixel) {
    case 0:			/* Must be 2 or 16 color mode. */
	/* set up video page */
#if 0
	offset = y * CI.xbytes + (x >> 3);
	vga_setpage(offset >> 16);
	offset &= 0xffff;

	/* select bit */
	port_out(8, GRA_I);
	port_out(0x80 >> (x & 7), GRA_D);

	/* read into latch and write dummy back */
	read_write(offset);
#endif        
        offset = y * CI.xbytes + (x >> 3);
	offset &= 0xffff;
	__svgalib_fast_setpage(0);
	tmp=gr_readb(offset);
	tmp&=(255^(128>>(x&7)));
	tmp|=(((COL&1)<<7)>>(x&7));
	gr_writeb(tmp, offset);
	__svgalib_fast_setpage(1);
	tmp=gr_readb(offset);
	tmp&=(255^(128>>(x&7)));
	tmp|=(((COL&2)<<6)>>(x&7));
	gr_writeb(tmp, offset);
	__svgalib_fast_setpage(2);
	tmp=gr_readb(offset);
	tmp&=(255^(128>>(x&7)));
	tmp|=(((COL&4)<<5)>>(x&7));
	gr_writeb(tmp, offset);
	__svgalib_fast_setpage(3);
	tmp=gr_readb(offset);
	tmp&=(255^(128>>(x&7)));
	tmp|=(((COL&8)<<4)>>(x&7));
	gr_writeb(tmp, offset);
	break;
    case 1:
	offset = y * CI.xbytes + x;

	/* select segment */
	__svgalib_fast_setpage(offset >> 16);

	/* write color to pixel */
	gr_writeb(COL, offset & 0xffff);
	break;
    case 2:
	offset = y * CI.xbytes + x * 2;
	__svgalib_fast_setpage(offset >> 16);
	gr_writew(COL, offset & 0xffff);
	break;
    case 3:
	c = RGB2BGR(COL);
	offset = y * CI.xbytes + x * 3;
	__svgalib_fast_setpage(offset >> 16);
	switch (offset & 0xffff) {
	case 0xfffe:
	    gr_writew(c, 0xfffe);
	    __svgalib_fast_setpage((offset >> 16) + 1);
	    gr_writeb(c >> 16, 0);
	    break;
	case 0xffff:
	    gr_writeb(c, 0xffff);
	    __svgalib_fast_setpage((offset >> 16) + 1);
	    gr_writew(c >> 8, 0);
	    break;
	default:
	    offset &= 0xffff;
	    gr_writew(c, offset);
	    gr_writeb(c >> 16, offset + 2);
	    break;
	}
	break;
    case 4:
	offset = y * __svgalib_cur_info.xbytes + x * 4;
	__svgalib_fast_setpage(offset >> 16);
	gr_writel((MODEFLAGS & RGB_MISORDERED) ? (COL << 8) : COL,
		  offset & 0xffff);
	break;
    }
    return 0;
 
 return(0);
}
#endif

#ifndef BACKGROUND
int vga_drawpixel(int x, int y)
#endif
#ifdef BACKGROUND
int __svgalib_fast_drawpixel(int x, int y)
#endif
{
    unsigned long offset;
    int c;

#ifdef BACKGROUND    
    if (!vga_oktowrite())
        {
	 /* Must be in background now. */
	 vga_drawpixel_bg(x,y);
	 return(0);
	}
#endif
    if (MODEX) {
#ifdef BACKGROUND
#if 0
        if (vga_oktowrite)
	    {
#endif
#endif
	     /* select plane */
	     port_out(0x02, SEQ_I);
	     port_out(1 << (x & 3), SEQ_D);
	     /* write color to pixel */
	     gr_writeb(COL, y * CI.xbytes + (x >> 2));
#ifdef BACKGROUND
#if 0
	    }
	  else
	    {
	     if (1<<(x&3)==1) vga_setpage(0);
	     if (1<<(x&3)==2) vga_setpage(1);
	     if (1<<(x&3)==4) vga_setpage(2);
	     if (1<<(x&3)==8) vga_setpage(3);
	     gr_writeb(COL, y * CI.xbytes + (x >> 2));
	     vga_setpage(0);
	    }
#endif
#endif

	return 0;
    }
    switch (__svgalib_cur_info.bytesperpixel) {
    case 0:			/* Must be 2 or 16 color mode. */
	/* set up video page */
	offset = y * CI.xbytes + (x >> 3);
#ifndef BACKGROUND
	vga_setpage(offset >> 16);
#endif
#ifdef BACKGROUND
        __svgalib_fast_setpage(offset >> 16);
#endif
	offset &= 0xffff;

	/* select bit */
	port_out(8, GRA_I);
	port_out(0x80 >> (x & 7), GRA_D);

	/* read into latch and write dummy back */
	read_write(offset);
	break;
    case 1:
	offset = y * CI.xbytes + x;

	/* select segment */
#ifndef BACKGROUND
	vga_setpage(offset >> 16);
#endif
#ifdef  BACKGROUND
        __svgalib_fast_setpage(offset >> 16);        
#endif

	/* write color to pixel */
	gr_writeb(COL, offset & 0xffff);
	break;
    case 2:
	offset = y * CI.xbytes + x * 2;
#ifndef BACKGROUND
	vga_setpage(offset >> 16);
#endif
#ifdef  BACKGROUND
        __svgalib_fast_setpage(offset >> 16);        
#endif	
        gr_writew(COL, offset & 0xffff);
	break;
    case 3:
	c = RGB2BGR(COL);
	offset = y * CI.xbytes + x * 3;
#ifndef BACKGROUND
	vga_setpage(offset >> 16);
#endif
#ifdef  BACKGROUND
        __svgalib_fast_setpage(offset >> 16);        
#endif
	switch (offset & 0xffff) {
	case 0xfffe:
	    gr_writew(c, 0xfffe);
#ifndef BACKGROUND
	    vga_setpage((offset >> 16) + 1);
#endif
#ifdef  BACKGROUND
            __svgalib_fast_setpage((offset >> 16)+1);        
#endif
	    gr_writeb(c >> 16, 0);
	    break;
	case 0xffff:
	    gr_writeb(c, 0xffff);
#ifndef BACKGROUND
	    vga_setpage((offset >> 16) + 1);
#endif
#ifdef  BACKGROUND
            __svgalib_fast_setpage((offset >> 16)+1);        
#endif
	    gr_writew(c >> 8, 0);
	    break;
	default:
	    offset &= 0xffff;
	    gr_writew(c, offset);
	    gr_writeb(c >> 16, offset + 2);
	    break;
	}
	break;
    case 4:
	offset = y * __svgalib_cur_info.xbytes + x * 4;
#ifndef BACKGROUND
	vga_setpage(offset >> 16);
#endif
#ifdef  BACKGROUND
        __svgalib_fast_setpage(offset >> 16);        
#endif
	gr_writel((MODEFLAGS & RGB_MISORDERED) ? (COL << 8) : COL,
		  offset & 0xffff);
	break;
    }
    return 0;
}

#ifdef BACKGROUND
int vga_drawpixel(int x, int y)

{
 __svgalib_dont_switch_vt_yet();
 __svgalib_fast_drawpixel(x,y);
 __svgalib_is_vt_switching_needed();
 return(0); 
}
#endif

#ifndef BACKGROUND
int vga_getpixel(int x, int y)
#endif
#ifdef BACKGROUND
int __svgalib_fast_getpixel(int x, int y)
#endif
{
    unsigned long offset;
    unsigned char mask;
    int pix = 0;
#ifdef BACKGROUND
    int tmp;
#endif 

    if (MODEX) {
#ifdef BACKGROUND
	if (vga_oktowrite())
	    {
#endif
	     /* select plane */
	     port_out(0x02, SEQ_I);
	     port_out(1 << (x & 3), SEQ_D);	    
#ifdef BACKGROUND
             pix=gr_readb(y*CI.xbytes+(x>>2));
	    }
	  else
	    {
	     if (1<<(x&3)==1) vga_setpage(0);
	     if (1<<(x&3)==2) vga_setpage(1);
	     if (1<<(x&3)==4) vga_setpage(2);
	     if (1<<(x&3)==8) vga_setpage(3);
	     pix=gr_readb(y * CI.xbytes + (x >> 2));
	     vga_setpage(0);
	    }
	return (pix);
#endif
#ifndef BACKGROUND
	return gr_readb(y * CI.xbytes + (x >> 2));
#endif
    }
    switch (__svgalib_cur_info.bytesperpixel) {
    case 0:			/* Must be 2 or 16 color mode. */
#ifdef BACKGROUND
        if (vga_oktowrite())
	    {
#endif
	     /* set up video page */
	     offset = y * CI.xbytes + (x >> 3);
	     vga_setpage(offset >> 16);
	     offset &= 0xffff;

	     /* select bit */
	     mask = 0x80 >> (x & 7);
	     pix = 0;
	     port_out(4, GRA_I);
	     port_out(0, GRA_D);	/* Select plane 0. */
	     if (gr_readb(offset) & mask)
	        pix |= 0x01;
	     port_out(4, GRA_I);
	     port_out(1, GRA_D);	/* Select plane 1. */
	     if (gr_readb(offset) & mask)
	        pix |= 0x02;
	     port_out(4, GRA_I);
	     port_out(2, GRA_D);	/* Select plane 2. */
	     if (gr_readb(offset) & mask)
	        pix |= 0x04;
	     port_out(4, GRA_I);
	     port_out(3, GRA_D);	/* Select plane 3. */
	     if (gr_readb(offset) & mask)
	        pix |= 0x08;
#ifdef BACKGROUND
            }
	  else
	    {
	     offset = y * CI.xbytes + (x >> 3);
	     offset &= 0xffff;
	     vga_setpage(0);
	     tmp=gr_readb(offset);
	     if ((tmp&(128>>(x&7)))) pix=1;
	     vga_setpage(1);
	     tmp=gr_readb(offset);
	     if ((tmp&(128>>(x&7)))) pix|=2;
	     vga_setpage(2);
	     tmp=gr_readb(offset);
	     if ((tmp&(128>>(x&7)))) pix|=4;
	     vga_setpage(3);
	     tmp=gr_readb(offset);
	     if (tmp&(128>>(x&7))) pix|=8;
	    }
	break;
#endif
#ifndef BACKGROUND
	return pix;
#endif
    case 1:
	offset = y * CI.xbytes + x;

	/* select segment */
	vga_setpage(offset >> 16);
#ifdef BACKGROUND
        pix=gr_readb(offset & 0xffff);
	break;
#endif
#ifndef BACKGROUND
	return gr_readb(offset & 0xffff);
#endif
    case 2:
	offset = y * CI.xbytes + x * 2;
	vga_setpage(offset >> 16);
#ifdef BACKGROUND
        pix=gr_readw(offset & 0xffff);
	break;
#endif
#ifndef BACKGROUND	
	return gr_readw(offset & 0xffff);
#endif
    case 3:
	offset = y * CI.xbytes + x * 3;
	vga_setpage(offset >> 16);
	switch (offset & 0xffff) {
	case 0xfffe:
	    pix = gr_readw(0xfffe);
	    vga_setpage((offset >> 16) + 1);
#ifdef BACKGROUND
            pix=RGB2BGR(pix+(gr_readb(0)<<16));
	    break;
#endif
#ifndef BACKGROUND	
	    return RGB2BGR(pix + (gr_readb(0) << 16));
#endif
	case 0xffff:
	    pix = gr_readb(0xffff);
	    vga_setpage((offset >> 16) + 1);
#ifdef BACKGROUND
            pix=RGB2BGR(pix+(gr_readw(0)<<8));
	    break;
#endif
#ifndef BACKGROUND	
	    return RGB2BGR(pix + (gr_readw(0) << 8));
#endif
	default:
	    offset &= 0xffff;
#ifdef BACKGROUND
            pix=RGB2BGR(gr_readw(offset)+(gr_readb(offset+2)<<26));
	    break;
#endif
#ifndef BACKGROUND	
	    return RGB2BGR(gr_readw(offset)
			   + (gr_readb(offset + 2) << 26));
#endif
	}
	break; 
    case 4:
	offset = y * __svgalib_cur_info.xbytes + x * 4;
	vga_setpage(offset >> 16);
#ifdef BACKGROUND
        pix=gr_readl(offset & 0xffff);
	break;
#endif
#ifndef BACKGROUND	
	return gr_readl(offset & 0xffff);
#endif
    }
#ifdef BACKGROUND
    return(pix);
#endif
#ifndef BACKGROUND
    return 0;
#endif
}

#ifdef BACKGROUND
int vga_getpixel(int x, int y)

{
 int pixel;
 
 __svgalib_dont_switch_vt_yet();
 pixel=__svgalib_fast_getpixel(x,y);
 __svgalib_is_vt_switching_needed();
 return(pixel); 
}
#endif
