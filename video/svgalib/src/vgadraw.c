/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* 21 January 1995 - added vga_readscanline(), added support for   */
/* non 8-pixel aligned scanlines in 16 color mode. billr@rastergr.com */
#include <stdio.h>
#include "vga.h"
#include "libvga.h"
#ifdef BACKGROUND
#include "vgabg.h"
#endif


/* used to decompose color value into bits (for fast scanline drawing) */
union bits {
    struct {
	unsigned char bit3;
	unsigned char bit2;
	unsigned char bit1;
	unsigned char bit0;
    } b;
    unsigned int i;
};

/* color decompositions */
static union bits color16[16] =
{
    {
	{0, 0, 0, 0}},
    {
	{0, 0, 0, 1}},
    {
	{0, 0, 1, 0}},
    {
	{0, 0, 1, 1}},
    {
	{0, 1, 0, 0}},
    {
	{0, 1, 0, 1}},
    {
	{0, 1, 1, 0}},
    {
	{0, 1, 1, 1}},
    {
	{1, 0, 0, 0}},
    {
	{1, 0, 0, 1}},
    {
	{1, 0, 1, 0}},
    {
	{1, 0, 1, 1}},
    {
	{1, 1, 0, 0}},
    {
	{1, 1, 0, 1}},
    {
	{1, 1, 1, 0}},
    {
	{1, 1, 1, 1}}};

/* mask for end points in plane buffer mode */
static unsigned char mask[8] =
{
    0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
/* display plane buffers (for fast scanline drawing) */
/* 256 bytes -> max 2048 pixel per line (2/16 colors) */
static unsigned char plane0[256];
static unsigned char plane1[256];
static unsigned char plane2[256];
static unsigned char plane3[256];

static inline void shifted_memcpy(void *dest_in, void *source_in, int len)
{
    int *dest = dest_in;
    int *source = source_in;

    len >>= 2;

    while (len--)
	*dest++ = (*source++ << 8);
}

/* RGB_swapped_memcopy returns the amount of bytes unhandled */
static inline int RGB_swapped_memcpy(char *dest, char *source, int len)
{
    int rest, tmp;

    tmp = len / 3;
    rest = len - 3 * tmp;
    len = tmp;

    while (len--) {
	*dest++ = source[2];
	*dest++ = source[1];
	*dest++ = source[0];
	source += 3;
    }

    return rest;
}

int vga_drawscanline(int line, unsigned char *colors)
{
    if ((CI.colors == 2) || (CI.colors > 256))
	return vga_drawscansegment(colors, 0, line, CI.xbytes);
    else
	return vga_drawscansegment(colors, 0, line, CI.xdim);
}


#if defined(CONFIG_ALPHA_JENSEN)

#define vuip    volatile unsigned int *
void a_memcpy(unsigned char *dest, unsigned char *src, long n)
{
    long i;
    if (((unsigned long) src % 4) || ((unsigned long) dest % 4)) {
	if (((unsigned long) src % 2) || ((unsigned long) dest % 2)) {
	    for (i = 0; i < n; i++) {
		*(vuip) (dest + (i << 7)) =
		    (*(unsigned char *) (src + i)) * 0x01010101UL;
	    }
	} else {
	    dest += 0x20UL;
	    for (i = 0; i < n; i += 2) {
		*(vuip) (dest + (i << 7)) =
		    (*(unsigned short *) (src + i)) * 0x00010001UL;
	    }
	    dest -= 0x20UL;
	    for (i = n - (n % 2); i < n; i++) {
		*(vuip) (dest + (i << 7)) =
		    (*(unsigned char *) (src + i)) * 0x01010101UL;
	    }
	}
    } else {
	dest += 0x60UL;
	for (i = 0; i < n; i += 4) {
	    *(vuip) (dest + (i << 7)) = (*(unsigned int *) (src + i));
	}
	dest -= 0x60UL;
	for (i = n - (n % 4); i < n; i++) {
	    *(vuip) (dest + (i << 7)) =
		(*(unsigned char *) (src + i)) * 0x01010101UL;
	}
    }
}

#define MEMCPY  a_memcpy
#define VM      SM

#else

#define MEMCPY  memcpy
#define VM      GM

#endif

#ifdef BACKGROUND

static int _vga_drawscansegment_bg(unsigned char *colors, 
    int x, int y, int length)
{
 /* The easy way */
 int count=0;
 int color;
 
 color=COL;
 while(count<length)
     {
      vga_setcolor(*colors);
      __svgalib_fast_drawpixel(x+count,y);
      colors++;
      count++;
     }
 vga_setcolor(color);
 return(0);
}
#endif


int vga_drawscansegment(unsigned char *colors, int x, int y, int length)
{
    /* both length and x must divide with 8 */
    /*  no longer true (at least for 16 & 256 colors) */

#ifdef BACKGROUND
 __svgalib_dont_switch_vt_yet();
 
 if (!vga_oktowrite())
     {
      /*__svgalib_is_vt_switching_needed(); */
      _vga_drawscansegment_bg(colors,x,y,length);
      __svgalib_is_vt_switching_needed();
      return(0);
     }
#endif

    if (MODEX)
	goto modeX;
    switch (CI.colors) {
    case 16:
	{
	    int i, j, k, first, last, page, l1, l2;
	    int offset, eoffs, soffs, ioffs;
	    union bits bytes;
	    unsigned char *address;

	    k = 0;
	    soffs = ioffs = (x & 0x7);	/* starting offset into first byte */
	    eoffs = (x + length) & 0x7;		/* ending offset into last byte */
	    for (i = 0; i < length;) {
		bytes.i = 0;
		first = i;
		last = i + 8 - ioffs;
		if (last > length)
		    last = length;
		for (j = first; j < last; j++, i++)
		    bytes.i = (bytes.i << 1) | color16[colors[j]].i;
		plane0[k] = bytes.b.bit0;
		plane1[k] = bytes.b.bit1;
		plane2[k] = bytes.b.bit2;
		plane3[k++] = bytes.b.bit3;
		ioffs = 0;
	    }
	    if (eoffs) {
		/* fixup last byte */
		k--;
		bytes.i <<= (8 - eoffs);
		plane0[k] = bytes.b.bit0;
		plane1[k] = bytes.b.bit1;
		plane2[k] = bytes.b.bit2;
		plane3[k++] = bytes.b.bit3;
	    }
	    offset = (y * CI.xdim + x) / 8;
	    vga_setpage((page = offset >> 16));
	    l1 = 0x10000 - (offset &= 0xffff);
	    /* k currently contains number of bytes to write */
	    if (l1 > k)
		l1 = k;
	    l2 = k - l1;
	    /* make k the index of the last byte to write */
	    k--;

#if defined(CONFIG_ALPHA_JENSEN)
	    address = SM + (offset << 7);
#else
	    address = GM + offset;
#endif

	    /* disable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x00, GRA_D);

	    /* write to all bits */
	    port_out(0x08, GRA_I);
	    port_out(0xFF, GRA_D);

	    /* select write map mask register */
	    port_out(0x02, SEQ_I);
	    /* write plane 0 */
	    port_out(0x01, SEQ_D);

	    /* select read map mask register */
	    port_out(0x04, GRA_I);
	    /* read plane 0 */
	    port_out(0x00, GRA_D);
	    if (soffs)
		plane0[0] |= *address & ~mask[soffs];
	    if (eoffs && l2 == 0)
		plane0[k] |= *(address + l1 - 1) & mask[eoffs];
	    MEMCPY(address, plane0, l1);

	    /* write plane 1 */
	    port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
	    port_out(0x02, SEQ_D);
	    /* read plane 1 */
	    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
	    port_out(0x01, GRA_D);
	    if (soffs)
		plane1[0] |= *address & ~mask[soffs];
	    if (eoffs && l2 == 0)
		plane1[k] |= *(address + l1 - 1) & mask[eoffs];
	    MEMCPY(address, plane1, l1);

	    /* write plane 2 */
	    port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
	    port_out(0x04, SEQ_D);
	    /* read plane 2 */
	    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
	    port_out(0x02, GRA_D);
	    if (soffs)
		plane2[0] |= *address & ~mask[soffs];
	    if (eoffs && l2 == 0)
		plane2[k] |= *(address + l1 - 1) & mask[eoffs];
	    MEMCPY(address, plane2, l1);

	    /* write plane 3 */
	    port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
	    port_out(0x08, SEQ_D);
	    /* read plane 3 */
	    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
	    port_out(0x03, GRA_D);
	    if (soffs)
		plane3[0] |= *address & ~mask[soffs];
	    if (eoffs && l2 == 0)
		plane3[k] |= *(address + l1 - 1) & mask[eoffs];
	    MEMCPY(address, plane3, l1);

	    if (l2 > 0) {
		vga_setpage(page + 1);

		/* write plane 0 */
		port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
		port_out(0x01, SEQ_D);
		if (eoffs) {
		    /* read plane 0 */
		    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
		    port_out(0x00, GRA_D);
		    plane0[k] |= *(GM + l2 - 1) & mask[eoffs];
		}
		MEMCPY(GM, &plane0[l1], l2);

		/* write plane 1 */
		port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
		port_out(0x02, SEQ_D);
		if (eoffs) {
		    /* read plane 1 */
		    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
		    port_out(0x01, GRA_D);
		    plane1[k] |= *(GM + l2 - 1) & mask[eoffs];
		}
		MEMCPY(GM, &plane1[l1], l2);

		/* write plane 2 */
		port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
		port_out(0x04, SEQ_D);
		if (eoffs) {
		    /* read plane 2 */
		    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
		    port_out(0x02, GRA_D);
		    plane2[k] |= *(GM + l2 - 1) & mask[eoffs];
		}
		MEMCPY(GM, &plane2[l1], l2);

		/* write plane 3 */
		port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
		port_out(0x08, SEQ_D);
		if (eoffs) {
		    /* read plane 3 */
		    port_out(0x04, GRA_I);	/* ATI needs resetting that one */
		    port_out(0x03, GRA_D);
		    plane3[k] |= *(GM + l2 - 1) & mask[eoffs];
		}
		MEMCPY(GM, &plane3[l1], l2);
	    }
	    /* restore map mask register */
	    port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
	    port_out(0x0F, SEQ_D);

	    /* enable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x0F, GRA_D);
	}
	break;
    case 2:
	{
	    /* disable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x00, GRA_D);

	    /* write to all bits */
	    port_out(0x08, GRA_I);
	    port_out(0xFF, GRA_D);

	    /* write to all planes */
	    port_out(0x02, SEQ_I);
	    port_out(0x0F, SEQ_D);

#if defined(CONFIG_ALPHA_JENSEN)
	    MEMCPY(SM + ((y * CI.xdim + x) << 4), colors, length);
#else
	    MEMCPY(GM + (y * CI.xdim + x) / 8, colors, length);
#endif

	    /* restore map mask register */
	    port_out(0x02, SEQ_I);	/* Yeah, ATI */
	    port_out(0x0F, SEQ_D);

	    /* enable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x0F, GRA_D);
	}
	break;
    case 256:
	{
	    switch (CM) {
	    case G320x200x256:	/* linear addressing - easy and fast */
#if defined(CONFIG_ALPHA_JENSEN)
		MEMCPY(SM + ((y * CI.xdim + x) << 7), colors, length);
#else
		MEMCPY(GM + (y * CI.xdim + x), colors, length);
#endif
#ifdef BACKGROUND
                __svgalib_is_vt_switching_needed();
#endif
		return 0;
	    case G320x240x256:
	    case G320x400x256:
	    case G360x480x256:
	      modeX:
		{
		    int first, last, offset, pixel, plane;


		    for (plane = 0; plane < 4; plane++) {
			/* select plane */
			port_out(0x02, SEQ_I);	/* ATI needs to set it */
			port_out(1 << plane, SEQ_D);

			pixel = plane;
			first = (y * CI.xdim + x) / 4;
			last = (y * CI.xdim + x + length) / 4;
			for (offset = first; offset < last; offset++) {
			    gr_writeb(colors[pixel], offset);
			    pixel += 4;
			}
		    }
		}
#ifdef BACKGROUND
                __svgalib_is_vt_switching_needed();
#endif
		return 0;
	    }
	    {
		unsigned long offset;
		int segment, free;

	      SegmentedCopy:
		offset = y * CI.xbytes + x;
		segment = offset >> 16;
		free = ((segment + 1) << 16) - offset;
		offset &= 0xFFFF;

		if (free < length) {
		    vga_setpage(segment);
#if defined(CONFIG_ALPHA_JENSEN)
		    MEMCPY(SM + (offset << 7), colors, free);
#else
		    MEMCPY(GM + offset, colors, free);
#endif
		    vga_setpage(segment + 1);
		    MEMCPY(VM, colors + free, length - free);
		} else {
		    vga_setpage(segment);
#if defined(CONFIG_ALPHA_JENSEN)
		    MEMCPY(SM + (offset << 7), colors, length);
#else
		    MEMCPY(GM + offset, colors, length);
#endif
		}
	    }
	}
	break;
    case 32768:
    case 65536:
	x *= 2;
	goto SegmentedCopy;
    case 1 << 24:
	if (__svgalib_cur_info.bytesperpixel == 4) {
	    x <<= 2;
	    if (MODEFLAGS & RGB_MISORDERED) {
		unsigned long offset;
		int segment, free;

		offset = y * CI.xbytes + x;
		segment = offset >> 16;
		free = ((segment + 1) << 16) - offset;
		offset &= 0xFFFF;

		if (free < length) {
		    vga_setpage(segment);
		    shifted_memcpy(GM + offset, colors, free);
		    vga_setpage(segment + 1);
		    shifted_memcpy(GM, colors + free, length - free);
		} else {
		    vga_setpage(segment);
		    shifted_memcpy(GM + offset, colors, length);
		}
	    } else {
		goto SegmentedCopy;
	    }
	    break;
	}
	x *= 3;
	if (MODEFLAGS & RGB_MISORDERED) {
	    unsigned long offset;
	    int segment, free;

	    offset = y * CI.xbytes + x;
	    segment = offset >> 16;
	    free = ((segment + 1) << 16) - offset;
	    offset &= 0xFFFF;

	    if (free < length) {
		int i;

		vga_setpage(segment);
		i = RGB_swapped_memcpy(GM + offset, colors, free);
		colors += (free - i);

		switch (i) {
		case 2:
		    gr_writeb(colors[2], 0xfffe);
		    gr_writeb(colors[2], 0xffff);
		    break;
		case 1:
		    gr_writeb(colors[2], 0xffff);
		    break;
		}

		vga_setpage(segment + 1);

		switch (i) {
		case 1:
		    gr_writeb(colors[1], 0);
		    gr_writeb(colors[0], 1);
		    i = 3 - i;
		    free += i;
		    colors += 3;
		    break;
		case 2:
		    gr_writeb(colors[0], 0);
		    i = 3 - i;
		    free += i;
		    colors += 3;
		    break;
		}

		RGB_swapped_memcpy(GM + i, colors, length - free);
	    } else {
		vga_setpage(segment);
		RGB_swapped_memcpy(GM + offset, colors, length);
	    }
	} else {
	    goto SegmentedCopy;
	}
    }

#ifdef BACKGROUND
 __svgalib_is_vt_switching_needed();
#endif
    return 0;
}

#ifdef BACKGROUND

static int _vga_getscansegment_bg(unsigned char *colors, 
    int x, int y, int length)
{
 /* The easy way */
 int count=0;
 
 while(count<length)
     {
      vga_setcolor(*colors);
      *colors=__svgalib_fast_getpixel(x+count,y);
      colors++;
      count++;
     }
 return(0);
}
#endif

int vga_getscansegment(unsigned char *colors, int x, int y, int length)
{
#ifdef BACKGROUND
 __svgalib_dont_switch_vt_yet();
 
 if (!vga_oktowrite())
     {
      /*__svgalib_is_vt_switching_needed(); */
      _vga_getscansegment_bg(colors,x,y,length);
      __svgalib_is_vt_switching_needed();
      return(0);
     }
#endif

    if (MODEX)
	goto modeX2;
    switch (CI.colors) {
    case 16:
	{
	    int i, k, page, l1, l2;
	    int offset, eoffs, soffs, nbytes, bit;
	    unsigned char *address;
	    unsigned char color;
	    k = 0;
	    soffs = (x & 0x7);	/* starting offset into first byte */
	    eoffs = (x + length) & 0x7;		/* ending offset into last byte */
	    offset = (y * CI.xdim + x) / 8;
	    vga_setpage((page = offset >> 16));
	    l1 = 0x10000 - (offset &= 0xffff);
	    if (soffs)
		nbytes = (length - (8 - soffs)) / 8 + 1;
	    else
		nbytes = length / 8;
	    if (eoffs)
		nbytes++;
	    if (l1 > nbytes)
		l1 = nbytes;
	    l2 = nbytes - l1;
	    address = GM + offset;
	    /* disable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x00, GRA_D);
	    /* read plane 0 */
	    port_out(0x04, GRA_I);
	    port_out(0x00, GRA_D);
	    memcpy(plane0, address, l1);
	    /* read plane 1 */
	    port_out(0x04, GRA_I);
	    port_out(0x01, GRA_D);
	    memcpy(plane1, address, l1);
	    /* read plane 2 */
	    port_out(0x04, GRA_I);
	    port_out(0x02, GRA_D);
	    memcpy(plane2, address, l1);
	    /* read plane 3 */
	    port_out(0x04, GRA_I);
	    port_out(0x03, GRA_D);
	    memcpy(plane3, address, l1);
	    if (l2 > 0) {
		vga_setpage(page + 1);
		/* read plane 0 */
		port_out(0x04, GRA_I);
		port_out(0x00, GRA_D);
		memcpy(&plane0[l1], GM, l2);
		/* read plane 1 */
		port_out(0x04, GRA_I);
		port_out(0x01, GRA_D);
		memcpy(&plane1[l1], GM, l2);
		/* read plane 2 */
		port_out(0x04, GRA_I);
		port_out(0x02, GRA_D);
		memcpy(&plane2[l1], GM, l2);
		/* read plane 3 */
		port_out(0x04, GRA_I);
		port_out(0x03, GRA_D);
		memcpy(&plane3[l1], GM, l2);
	    }
	    /* enable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x0F, GRA_D);
	    k = 0;
	    for (i = 0; i < length;) {
		for (bit = 7 - soffs; bit >= 0 && i < length; bit--, i++) {
		    color = (plane0[k] & (1 << bit) ? 1 : 0);
		    color |= (plane1[k] & (1 << bit) ? 1 : 0) << 1;
		    color |= (plane2[k] & (1 << bit) ? 1 : 0) << 2;
		    color |= (plane3[k] & (1 << bit) ? 1 : 0) << 3;
		    colors[i] = color;
		}
		k++;
		soffs = 0;
	    }
	}
	break;
    case 2:
	{
	    /* disable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x00, GRA_D);
	    /* read from plane 0 */
	    port_out(0x04, SEQ_I);
	    port_out(0x00, SEQ_D);
	    memcpy(colors, GM + (y * CI.xdim + x) / 8, length);
	    /* enable Set/Reset Register */
	    port_out(0x01, GRA_I);
	    port_out(0x0F, GRA_D);
	}
	break;
    case 256:
	{
	    switch (CM) {
	    case G320x200x256:	/* linear addressing - easy and fast */
		memcpy(colors, GM + y * CI.xdim + x, length);
#ifdef BACKGROUND
                __svgalib_is_vt_switching_needed();
#endif
		return 0;
	    case G320x240x256:
	    case G320x400x256:
	    case G360x480x256:
	      modeX2:
		{
		    int first, last, offset, pixel, plane;
		    for (plane = 0; plane < 4; plane++) {
			/* select plane */
			port_out(0x02, SEQ_I);	/* ATI needs resetting that one */
			port_out(1 << plane, SEQ_D);
			pixel = plane;
			first = (y * CI.xdim + x) / 4;
			last = (y * CI.xdim + x + length) / 4;
			for (offset = first; offset < last; offset++) {
			    colors[pixel] = gr_readb(offset);
			    pixel += 4;
			}
		    }
		}
#ifdef BACKGROUND
                __svgalib_is_vt_switching_needed();
#endif
		return 0;
	    }
	    {
		unsigned long offset;
		int segment, free;
	      SegmentedCopy2:
		offset = y * CI.xbytes + x;
		segment = offset >> 16;
		free = ((segment + 1) << 16) - offset;
		offset &= 0xFFFF;
		if (free < length) {
		    vga_setpage(segment);
		    memcpy(colors, GM + offset, free);
		    vga_setpage(segment + 1);
		    memcpy(colors + free, GM, length - free);
		} else {
		    vga_setpage(segment);
		    memcpy(colors, GM + offset, length);
		}
	    }
	}
	break;
    case 32768:
    case 65536:
	x *= 2;
	goto SegmentedCopy2;
    case 1 << 24:
#ifdef BACKGROUND
        __svgalib_is_vt_switching_needed();
#endif
	return -1;		/* not supported */
    }
#ifdef BACKGROUND
    __svgalib_is_vt_switching_needed();
#endif
    return 0;
}
