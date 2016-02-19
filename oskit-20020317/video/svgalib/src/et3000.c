/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (c) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/*
   **  Used extended ET3000 registers (EXT+xx) :
   **
   **   00 : CRT (3d4) index 23
   **   01 : CRT (3d4) index 24
   **   02 : CRT (3d4) index 25
   **   03 : SEQ (3c4) index 06
   **   04 : SEQ (3c4) index 07
   **   05 : Segment select (3cd)
   **   06 : ATT (3c0) index 16
   **
 */

#include <stdio.h>
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

#define SEG_SELECT 0x3CD

static int et3000_memory;

static int et3000_init(int, int, int);
static int et3000_interlaced(int mode);
static void et3000_unlock(void);


/* Mode table */
#include "et3000.regs"

static ModeTable et3000_modes[] =
{
/* *INDENT-OFF* */
    OneModeEntry(640x480x256),
    OneModeEntry(800x600x16),
    OneModeEntry(800x600x256),
    OneModeEntry(1024x768x16),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

/*#endif *//* !defined(DYNAMIC) */

/* Fill in chipset specific mode information */

static void et3000_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    switch (modeinfo->colors) {
    case 16:			/* 4-plane 16 color mode */
	modeinfo->maxpixels = 65536 * 8;
	break;
    default:
	if (modeinfo->bytesperpixel > 0)
	    modeinfo->maxpixels = et3000_memory * 1024 /
		modeinfo->bytesperpixel;
	else
	    modeinfo->maxpixels = et3000_memory * 1024;
	break;
    }
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = 0x7ffff;
    modeinfo->haveblit = 0;
    modeinfo->flags |= HAVE_RWPAGE;
    if (et3000_interlaced(mode))
	modeinfo->flags |= IS_INTERLACED;
}


/* Read and store chipset-specific registers */

static int et3000_saveregs(unsigned char regs[])
{
    int i;

    et3000_unlock();
    /* save extended CRT registers */
    for (i = 0; i < 3; i++) {
	port_out(0x23 + i, __svgalib_CRT_I);
	regs[EXT + i] = port_in(__svgalib_CRT_D);
    }

    /* save extended sequencer register */
    for (i = 0; i < 2; ++i) {
	port_out(6 + i, SEQ_I);
	regs[EXT + 3 + i] = port_in(SEQ_D);
    }

    /* save some other ET3000 specific registers */
    regs[EXT + 5] = port_in(0x3cd);

    /* save extended attribute register */
    port_in(__svgalib_IS1_R);		/* reset flip flop */
    port_out(0x16, ATT_IW);
    regs[EXT + 6] = port_in(ATT_R);

    return 7;			/* ET3000 requires 7 additional registers */
}


/* Set chipset-specific registers */

static void et3000_setregs(const unsigned char regs[], int mode)
{
    int i;
    unsigned char save;

    et3000_unlock();
    /* write some ET3000 specific registers */
    port_out(regs[EXT + 5], 0x3cd);

    /* write extended sequencer register */
    for (i = 0; i < 2; ++i) {
	port_out(6 + i, SEQ_I);
	port_out(regs[EXT + 3 + i], SEQ_D);
    }

    /* deprotect CRT register 0x25 */
    port_out(0x11, __svgalib_CRT_I);
    save = port_in(__svgalib_CRT_D);
    port_out(save & 0x7F, __svgalib_CRT_D);

    /* write extended CRT registers */
    for (i = 0; i < 3; i++) {
	port_out(0x23 + i, __svgalib_CRT_I);
	port_out(regs[EXT + i], __svgalib_CRT_D);
    }

    /* set original CRTC 0x11 */
    port_out(0x11, __svgalib_CRT_I);
    port_out(save, __svgalib_CRT_D);

    /* write extended attribute register */
    port_in(__svgalib_IS1_R);		/* reset flip flop */
    port_out(0x16, ATT_IW);
    port_out(regs[EXT + 6], ATT_IW);
}


/* Return non-zero if mode is available */

static int et3000_modeavailable(int mode)
{
    const unsigned char *regs;
    struct info *info;

    regs = LOOKUPMODE(et3000_modes, mode);
    if (regs == NULL || mode == GPLANE16)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE)
	return 0;

    info = &__svgalib_infotable[mode];
    if (et3000_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    return SVGADRV;
}


/* Check if mode is interlaced */

static int et3000_interlaced(int mode)
{
    const unsigned char *regs;

    if (et3000_modeavailable(mode) != SVGADRV)
	return 0;
    regs = LOOKUPMODE(et3000_modes, mode);
    if (regs == NULL || regs == DISABLE_MODE)
	return 0;
    return (regs[EXT + 2] & 0x80) != 0;		/* CRTC 25H */
}


/* Set a mode */

static int et3000_setmode(int mode, int prv_mode)
{
    const unsigned char *regs;

    switch (et3000_modeavailable(mode)) {
    case STDVGADRV:
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    case SVGADRV:
	regs = LOOKUPMODE(et3000_modes, mode);
	if (regs != NULL)
	    break;
    default:
	return 1;		/* mode not available */
    }

    et3000_unlock();
    __svgalib_setregs(regs);
    et3000_setregs(regs, mode);
    return 0;
}

/* Unlock chipset-specific registers */

static void et3000_unlock(void)
{
    /* get access to extended registers */
    int base;

    base = (port_in(0x3cc) & 1 ? 0x3d0 : 0x3b0);
    port_out(3, 0x3bf);
    port_out(0xa0, base + 8);
    port_out(0x24, base + 4);
    port_out(port_in(base + 5) & 0xDF, base + 5);
}


/* Relock chipset-specific registers */

static void et3000_lock(void)
{
}


/* Indentify chipset; return non-zero if detected */

static int et3000_test(void)
{
    unsigned char old, val;
    int base;

    /* test for Tseng clues */
    old = port_in(0x3cd);
    port_out(old ^ 0x3f, 0x3cd);
    val = port_in(0x3cd);
    port_out(old, 0x3cd);

    /* return false if not Tseng */
    if (val != (old ^ 0x3f))
	return 0;

    /* test for ET3000 clues */
    if (port_in(0x3cc) & 1)
	base = 0x3d4;
    else
	base = 0x3b4;
    port_out(0x1b, base);
    old = port_in(base + 1);
    port_out(old ^ 0xff, base + 1);
    val = port_in(base + 1);
    port_out(old, base + 1);

    /* return false if not ET3000 */
    if (val != (old ^ 0xff))
	return 0;

    /* Found ET3000 */
    et3000_init(0, 0, 0);
    return 1;
}



static unsigned char last_page = 0x40;

/* Bank switching function - set 64K bank number */
static void et3000_setpage(int page)
{
    last_page = page | (page << 3) | 0x40;
    port_out(last_page, SEG_SELECT);
}


/* Bank switching function - set 64K read bank number */
static void et3000_setrdpage(int page)
{
    last_page &= 0xC7;
    last_page |= (page << 3);
    port_out(last_page, SEG_SELECT);
}

/* Bank switching function - set 64K write bank number */
static void et3000_setwrpage(int page)
{
    last_page &= 0xF8;
    last_page |= page;
    port_out(last_page, SEG_SELECT);
}


/* Set display start address (not for 16 color modes) */
/* ET4000 supports any address in video memory (up to 1Mb) */

static void et3000_setdisplaystart(int address)
{
    outw(0x3d4, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
    outw(0x3d4, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    inb(0x3da);			/* set ATC to addressing mode */
    outb(0x3c0, 0x13 + 0x20);	/* select ATC reg 0x13 */
    outb(0x3c0, (inb(0x3c1) & 0xf0) | ((address & 3) << 1));
    /* write sa0-1 to bits 1-2 */
    outb(0x3d4, 0x23);
    outb(0x3d5, (inb(0x3d5) & 0xfd)
	 | ((address & 0x40000) >> 17));	/* write sa18 to bit 1 */
}


/* Set logical scanline length (usually multiple of 8) */

static void et3000_setlogicalwidth(int width)
{
    outw(0x3d4, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
}


/* Function table (exported) */

DriverSpecs __svgalib_et3000_driverspecs =
{
    et3000_saveregs,
    et3000_setregs,
    et3000_unlock,
    et3000_lock,
    et3000_test,
    et3000_init,
    et3000_setpage,
    et3000_setrdpage,
    et3000_setwrpage,
    et3000_setmode,
    et3000_modeavailable,
    et3000_setdisplaystart,
    et3000_setlogicalwidth,
    et3000_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    0,				/* extset */
    0,
    0,				/* linear */
    NULL,                       /* Accelspecs */
    NULL,                       /* Emulation */
};


/* Initialize chipset (called after detection) */

static int et3000_init(int force, int par1, int par2)
{
    if (force)
	et3000_memory = par1;
    else {
	et3000_memory = 512;
	/* Any way to check this ?? */
    }

    if (__svgalib_driver_report)
	printf("Using Tseng ET3000 driver (%d).\n", et3000_memory);
    __svgalib_driverspecs = &__svgalib_et3000_driverspecs;

    return 0;
}
