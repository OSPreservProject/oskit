/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* Modified by Hartmut Schirmer */

/* TVGA 8900c code taken from tvgalib by Toomas Losin */

/* TVGA 9440 code added by ARK 29-OCT-97 */
/* (root@ark.dyn.ml.org, ark@lhq.com) [nitc?] */
/* updated 9-NOV-97 to support more regs */
/* this should alllow it to work on 9680's as well */

/* Thanks to Albert Erdmann (theone@miami.gdi.net) */
/* for blindly testing files and mailing me results */
/* for the 9680 registers */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

#include "tvga8900.regs"
#include "tvga9440.regs"
#include "tvga9680.regs"

/* static int tvga_chiptype; */
static int tvga8900_memory;	/* amount of video memory in K */
static int tvga8900_nonint;	/* nonzero if non-interlaced jumper set */

static int tvga8900_init(int, int, int);
static int tvga8900_interlaced(int mode);

static int reg_0c = 0xad;	/* value for 256k cards */

static int tvga_model = 8900;	/* set to 8900, 9440, or 9680 based on model */

/* Mode table */
#define ModeEntry94(res) { G##res, g##res##_regs94 }
#define ModeEntry96(res) { G##res, g##res##_regs96 }

static ModeTable tvga_modes_2048_96[] =
{				/* 2M modes for the 9680 */
    ModeEntry96(800x600x16),
    ModeEntry96(1024x768x16),
    ModeEntry96(1280x1024x16),
    ModeEntry96(1600x1200x16),
    ModeEntry96(640x480x256),
    ModeEntry96(800x600x256),
    ModeEntry96(1024x768x256),
    ModeEntry96(1280x1024x256),
    ModeEntry96(1600x1200x256),
    ModeEntry96(320x200x32K),
    ModeEntry96(640x480x32K),
    ModeEntry96(800x600x32K),
    ModeEntry96(1024x768x32K),
    ModeEntry96(320x200x64K),
    ModeEntry96(640x480x64K),
    ModeEntry96(800x600x64K),
    ModeEntry96(1024x768x64K),
    ModeEntry96(320x200x16M),
    ModeEntry96(640x480x16M),
    ModeEntry96(800x600x16M),
    END_OF_MODE_TABLE
};

static ModeTable tvga_modes_1024_96[] =
{				/* 1M modes for the 9680 */
    ModeEntry96(800x600x16),
    ModeEntry96(1024x768x16),
    ModeEntry96(1280x1024x16),
    ModeEntry96(1600x1200x16),
    ModeEntry96(640x480x256),
    ModeEntry96(800x600x256),
    ModeEntry96(1024x768x256),
    ModeEntry96(320x200x32K),
    ModeEntry96(640x480x32K),
    ModeEntry96(800x600x32K),
    ModeEntry96(320x200x64K),
    ModeEntry96(640x480x64K),
    ModeEntry96(800x600x64K),
    ModeEntry96(320x200x16M),
    ModeEntry96(640x480x16M),
    END_OF_MODE_TABLE
};

static ModeTable tvga_modes_512_96[] =
{				/* 512K modes for the 9680 */
    ModeEntry96(800x600x16),
    ModeEntry96(1024x768x16),
    ModeEntry96(640x480x256),
    ModeEntry96(800x600x256),
    ModeEntry96(320x200x32K),
    ModeEntry96(320x200x64K),
    ModeEntry96(320x200x16M),
    END_OF_MODE_TABLE
};

static ModeTable tvga_modes_1024_94[] =
{				/* 1M modes for the 9440 */
    ModeEntry94(800x600x16),
    ModeEntry94(1024x768x16),
    ModeEntry94(1280x1024x16),
    ModeEntry94(1600x1200x16),
    ModeEntry94(640x480x256),
    ModeEntry94(800x600x256),
    ModeEntry94(1024x768x256),
    ModeEntry94(320x200x32K),
    ModeEntry94(640x480x32K),
    ModeEntry94(800x600x32K),
    ModeEntry94(320x200x64K),
    ModeEntry94(640x480x64K),
    ModeEntry94(800x600x64K),
    ModeEntry94(320x200x16M),
    ModeEntry94(640x480x16M),
    END_OF_MODE_TABLE
};

static ModeTable tvga_modes_512_94[] =
{				/* 512K modes for the 9440 */
    ModeEntry94(800x600x16),
    ModeEntry94(1024x768x16),
    ModeEntry94(640x480x256),
    ModeEntry94(800x600x256),
    ModeEntry94(320x200x32K),
    ModeEntry94(320x200x64K),
    ModeEntry94(320x200x16M),
    END_OF_MODE_TABLE
};

static ModeTable tvga_modes_1024[] =
{				/* 1Mb, non-interlace jumper set */
/* *INDENT-OFF* */
    OneModeEntry(640x480x256),
    OneModeEntry(800x600x256),
    OneModeEntry(1024x768x256),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

#define INTERL(res,i) { G##res, g##res##i##_regs }

static ModeTable tvga_modes_1024i[] =
{				/* 1Mb, jumper set to interlaced */
/* *INDENT-OFF* */
    INTERL(640x480x256, i),
    INTERL(800x600x256, i),
    INTERL(1024x768x256, i),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

static ModeTable tvga_modes_512[] =
{				/* 512K */
/* *INDENT-OFF* */
    INTERL(640x480x256, i),
    INTERL(800x600x256, i1),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

static ModeTable *tvga_modes = NULL;

static void nothing(void)
{
}

/* Fill in chipset specific mode information */

static void tvga8900_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    if (modeinfo->bytesperpixel > 0)
	modeinfo->maxpixels = tvga8900_memory * 1024 /
	    modeinfo->bytesperpixel;
    else
	modeinfo->maxpixels = tvga8900_memory * 1024;
    modeinfo->maxlogicalwidth = 2040;
    modeinfo->startaddressrange = 0xfffff;
    if (mode == G320x200x256) {
	/* Special case: bank boundary may not fall within display. */
	modeinfo->startaddressrange = 0xf0000;
	/* Hack: disable page flipping capability for the moment. */
	modeinfo->startaddressrange = 0xffff;
	modeinfo->maxpixels = 65536;
    }
    modeinfo->haveblit = 0;

    if (tvga8900_interlaced(mode))
	modeinfo->flags |= IS_INTERLACED;
    modeinfo->flags &= ~HAVE_RWPAGE;
}


/* select the correct register table */
static void setup_registers(void)
{
    if (tvga_modes == NULL) {
	if (tvga_model == 9440) {
	    if (tvga8900_memory < 1024)
		tvga_modes = tvga_modes_512_94;
	    else
		tvga_modes = tvga_modes_1024_94;
	}
	else if (tvga_model == 9680) {
	    if (tvga8900_memory < 1024)
		tvga_modes = tvga_modes_512_96;
	    else if (tvga8900_memory < 2048)
		tvga_modes = tvga_modes_1024_96;
	    else
		tvga_modes = tvga_modes_2048_96;
	}
	else {
	    if (tvga8900_memory < 1024)
		tvga_modes = tvga_modes_512;
	    if (tvga8900_nonint)
		tvga_modes = tvga_modes_1024;
	    else
		tvga_modes = tvga_modes_1024i;
	}
    }
}


/* Read and store chipset-specific registers */

static int tvga8900_saveregs(unsigned char regs[])
{
    int i;

    /* I know goto is bad, but i didnt want to indent all the old code.. */
    /* the 9680 uses the same regs, just completely different values     */
    if (tvga_model == 9440 || tvga_model == 9680) goto tvga9440_saveregs;

    /* save extended CRT registers */
    for (i = 0; i < 7; i++) {
	port_out(0x18 + i, __svgalib_CRT_I);
	regs[EXT + i] = port_in(__svgalib_CRT_D);
    }

    /* now do the sequencer mode regs */
    port_out(0x0b, SEQ_I);	/* force old mode regs */
    port_out(port_in(SEQ_D), SEQ_D);	/* by writing */

    /* outw(0x3C4, 0x820E); *//* unlock conf. reg */
    /* port_out(0x0c, SEQ_I); *//* save conf. reg */
    /* regs[EXT + 11] = port_in(SEQ_D); */

    port_out(0x0d, SEQ_I);	/* old reg 13 */
    regs[EXT + 7] = port_in(SEQ_D);
    port_out(0x0e, SEQ_I);	/* old reg 14 */
    regs[EXT + 8] = port_in(SEQ_D);

    port_out(0x0b, SEQ_I);	/* now use new regs */
    port_in(SEQ_D);
    port_out(0x0d, SEQ_I);	/* new reg 13 */
    regs[EXT + 9] = port_in(SEQ_D);
    port_out(0x0e, SEQ_I);	/* new reg 14 */
    regs[EXT + 10] = port_in(SEQ_D) ^ 0x02;

    /* we do the ^ 0x02 so that when the regs are restored */
    /* later we don't have a special case; see trident.doc */

    return 12;			/* tridents requires 12 additional registers */


    /* 9440 code added by ARK */
    tvga9440_saveregs:

    /* unprotect some trident regs */
    port_out(0x0E, SEQ_I);
    port_out(port_in(SEQ_D) | 0x80, SEQ_D);

    /* save sequencer regs */
    port_out(0x0B, SEQ_I);
    regs[EXT + 0] = port_in(SEQ_D);
    port_out(0x0D, SEQ_I);
    regs[EXT + 1] = port_in(SEQ_D);
    port_out(0x0E, SEQ_I);
    regs[EXT + 2] = port_in(SEQ_D);
    port_out(0x0F, SEQ_I);
    regs[EXT + 3] = port_in(SEQ_D);

    /* save extended CRT registers */
    port_out(0x19, __svgalib_CRT_I);
    regs[EXT + 4] = port_in(__svgalib_CRT_D);
    port_out(0x1E, __svgalib_CRT_I);
    regs[EXT + 5] = port_in(__svgalib_CRT_D);
    port_out(0x1F, __svgalib_CRT_I);
    regs[EXT + 6] = port_in(__svgalib_CRT_D);
    port_out(0x21, __svgalib_CRT_I);
    regs[EXT + 7] = port_in(__svgalib_CRT_D);
    port_out(0x25, __svgalib_CRT_I);
    regs[EXT + 8] = port_in(__svgalib_CRT_D);
    port_out(0x27, __svgalib_CRT_I);
    regs[EXT + 9] = port_in(__svgalib_CRT_D);
    port_out(0x29, __svgalib_CRT_I);
    regs[EXT + 10] = port_in(__svgalib_CRT_D);
    /* Extended regs 11/12 are clobbered by vga.c */
    port_out(0x2A, __svgalib_CRT_I);
    regs[EXT + 13] = port_in(__svgalib_CRT_D);
    port_out(0x2F, __svgalib_CRT_I);
    regs[EXT + 14] = port_in(__svgalib_CRT_D);
    port_out(0x30, __svgalib_CRT_I);
    regs[EXT + 15] = port_in(__svgalib_CRT_D);
    port_out(0x36, __svgalib_CRT_I);
    regs[EXT + 16] = port_in(__svgalib_CRT_D);
    port_out(0x38, __svgalib_CRT_I);
    regs[EXT + 17] = port_in(__svgalib_CRT_D);
    port_out(0x50, __svgalib_CRT_I);
    regs[EXT + 18] = port_in(__svgalib_CRT_D);

    /* grfx controller */
    port_out(0x0F, GRA_I);
    regs[EXT + 19] = port_in(GRA_D);
    port_out(0x2F, GRA_I);
    regs[EXT + 20] = port_in(GRA_D);

    /* trident specific ports */
    regs[EXT + 21] = port_in(0x43C8);
    regs[EXT + 22] = port_in(0x43C9);
    regs[EXT + 23] = port_in(0x83C6);
    regs[EXT + 24] = port_in(0x83C8);
    for(i=0;i<5;i++)
	port_in(PEL_MSK);
    regs[EXT + 25] = port_in(PEL_MSK);

    /* reprotect the regs to avoid conflicts */
    port_out(0x0E, SEQ_I);
    port_out(port_in(SEQ_D) & 0x7F, SEQ_D);

    return 26;               /* The 9440 requires 26 additional registers */
}


/* Set chipset-specific registers */

static void tvga8900_setregs(const unsigned char regs[], int mode)
{
    int i;
    int crtc31 = 0;

    /* 7 extended CRT registers */
    /* 4 extended Sequencer registers (old and new) */
    /* CRTC reg 0x1f is apparently dependent */
    /* on the amount of memory installed. */

    switch (tvga8900_memory >> 8) {
    case 1:
	crtc31 = 0x94;
	reg_0c = 0xad;
	break;			/* 256K */
    case 2:
    case 3:
	crtc31 = 0x98;
	reg_0c = 0xcd;
	break;			/* 512/768K */
    case 4:			/* 1024K */
	crtc31 = 0x18;
	reg_0c = 0xcd;
	if (mode == G1024x768x256) {
	    reg_0c = 0xed;
	    crtc31 = 0x98;
	} else if (mode == G640x480x256 || mode == G800x600x256)
	    reg_0c = 0xed;
	break;
    }

    if (tvga_model == 9440 || tvga_model == 9680) goto tvga9440_setregs;

    if (mode == TEXT) {
	reg_0c = regs[EXT + 11];
	crtc31 = regs[EXT + 12];
    }
#ifdef REG_DEBUG
    printf("Setting extended registers\n");
#endif

    /* write extended CRT registers */
    for (i = 0; i < 7; i++) {
	port_out(0x18 + i, __svgalib_CRT_I);
	port_out(regs[EXT + i], __svgalib_CRT_D);
    }

    /* update sequencer mode regs */
    port_out(0x0b, SEQ_I);	/* select old regs */
    port_out(port_in(SEQ_D), SEQ_D);
    port_out(0x0d, SEQ_I);	/* old reg 13 */
    port_out(regs[EXT + 7], SEQ_D);
    port_out(0x0e, SEQ_I);	/* old reg 14 */
#if 0
    port_out(regs[EXT + 8], SEQ_D);
#endif
    port_out(((port_in(SEQ_D) & 0x08) | (regs[EXT + 8] & 0xf7)), SEQ_D);


    port_out(0x0b, SEQ_I);
    port_in(SEQ_D);		/* select new regs */

    if (tvga8900_memory > 512) {
	port_out(0x0e, SEQ_I);	/* set bit 7 of reg 14  */
	port_out(0x80, SEQ_D);	/* to enable writing to */
	port_out(0x0c, SEQ_I);	/* reg 12               */
	port_out(reg_0c, SEQ_D);
    }
    /*      outw(0x3c4, 0x820e); *//* unlock conf. reg */
    /*      port_out(0x0c, SEQ_I); *//* reg 12 */

    port_out(0x0d, SEQ_I);	/* new reg 13 */
    port_out(regs[EXT + 9], SEQ_D);
    port_out(0x0e, SEQ_I);	/* new reg 14 */
    port_out(regs[EXT + 10], SEQ_D);

#ifdef REG_DEBUG
    printf("Now setting last two extended registers.\n");
#endif

    /* update CRTC reg 1f */
    port_out(0x1f, __svgalib_CRT_I);
    port_out((port_in(__svgalib_CRT_D) & 0x03) | crtc31, __svgalib_CRT_D);

    return;


    tvga9440_setregs:

    /* update sequencer mode regs */
    port_out(0x0D, SEQ_I);
    port_out(regs[EXT + 1], SEQ_D);
    port_out(0x0E, SEQ_I);
    port_out(regs[EXT + 2], SEQ_D);
    port_out(0x0F, SEQ_I);
    port_out(regs[EXT + 3], SEQ_D);
#if 0
// you cant write to this anyways... it messes things up...
//    port_out(0x0B, SEQ_I);
//    port_out(regs[EXT + 0], SEQ_D);
#endif
    /* write extended CRT registers */
    port_out(0x19, __svgalib_CRT_I);
    port_out(regs[EXT + 4], __svgalib_CRT_D);
    port_out(0x1E, __svgalib_CRT_I);
    port_out(regs[EXT + 5], __svgalib_CRT_D);
    port_out(0x1F, __svgalib_CRT_I);
    port_out(regs[EXT + 6], __svgalib_CRT_D);
    port_out(0x21, __svgalib_CRT_I);
    port_out(regs[EXT + 7], __svgalib_CRT_D);
    port_out(0x25, __svgalib_CRT_I);
    port_out(regs[EXT + 8], __svgalib_CRT_D);
    port_out(0x27, __svgalib_CRT_I);
    port_out(regs[EXT + 9], __svgalib_CRT_D);
    port_out(0x29, __svgalib_CRT_I);
    port_out(regs[EXT + 10], __svgalib_CRT_D);
    /* Extended regs 11/12 are clobbered by vga.c */
    port_out(0x2A, __svgalib_CRT_I);
    port_out(regs[EXT + 13], __svgalib_CRT_D);
    port_out(0x2F, __svgalib_CRT_I);
    port_out(regs[EXT + 14], __svgalib_CRT_D);
    port_out(0x30, __svgalib_CRT_I);
    port_out(regs[EXT + 15], __svgalib_CRT_D);
    port_out(0x36, __svgalib_CRT_I);
    port_out(regs[EXT + 16], __svgalib_CRT_D);
    port_out(0x38, __svgalib_CRT_I);
    port_out(regs[EXT + 17], __svgalib_CRT_D);
    port_out(0x50, __svgalib_CRT_I);
    port_out(regs[EXT + 18], __svgalib_CRT_D);

    /* grfx controller */
    port_out(0x0F, GRA_I);
    port_out(regs[EXT + 19], GRA_D);
    port_out(0x2F, GRA_I);
    port_out(regs[EXT + 20], GRA_D);

    /* unprotect 3DB */
    port_out(0x0F, GRA_I);
    port_out(port_in(GRA_D) | 0x04, GRA_D);
    /* allow user-defined clock rates */
    port_out((port_in(0x3DB) & 0xFC) | 0x02, 0x3DB);

    /* trident specific ports */
    port_out(regs[EXT + 21], 0x43C8);
    port_out(regs[EXT + 22], 0x43C9);
    port_out(regs[EXT + 23], 0x83C6);
    port_out(regs[EXT + 24], 0x83C8);
    for(i=0;i<5;i++)
	port_in(PEL_MSK);
    port_out(regs[EXT + 25], PEL_MSK);
    port_out(0xFF, PEL_MSK);
}


/* Return nonzero if mode is available */

static int tvga8900_modeavailable(int mode)
{
    const unsigned char *regs;
    struct info *info;

    regs = LOOKUPMODE(tvga_modes, mode);
    if (regs == NULL || mode == GPLANE16)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE)
	return 0;

    info = &__svgalib_infotable[mode];
    if (tvga8900_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    return SVGADRV;
}


/* Check if mode is interlaced */

static int tvga8900_interlaced(int mode)
{
    const unsigned char *regs;

    setup_registers();
    regs = LOOKUPMODE(tvga_modes, mode);
    if (regs == NULL || regs == DISABLE_MODE)
	return 0;
    return tvga8900_nonint == 0;
}


/* Set a mode */

static int tvga8900_setmode(int mode, int prv_mode)
{
    const unsigned char *regs;

    regs = LOOKUPMODE(tvga_modes, mode);
    if (regs == NULL)
	return (int) (__svgalib_vga_driverspecs.setmode(mode, prv_mode));
    if (!tvga8900_modeavailable(mode))
	return 1;
    __svgalib_setregs(regs);
    tvga8900_setregs(regs, mode);
    return 0;
}


/* Indentify chipset; return non-zero if detected */

static int tvga8900_test(void)
{
    int origVal, newVal;
    int save0b;
    /* 
     * Check first that we have a Trident card.
     */
    outb(0x3c4, 0x0b);
    save0b = inb(0x3c5);
    outw(0x3C4, 0x000B);	/* Switch to Old Mode */
    inb(0x3C5);			/* Now to New Mode */
    outb(0x3C4, 0x0E);
    origVal = inb(0x3C5);
    outb(0x3C5, 0x00);
    newVal = inb(0x3C5) & 0x0F;
    outb(0x3C5, (origVal ^ 0x02));

    if (newVal != 2) {
	outb(0x3c5, origVal);
	outb(0x3c4, 0x0b);
	outb(0x3c5, save0b);
	return 0;
    }

    /* The version check that was here was moved to the init function by ARK */
    /* in order to tell the 8900 from the 9440 model.. */

    tvga8900_init(0, 0, 0);
    return 1;
}


/* Bank switching function - set 64K bank number */

static void tvga8900_setpage(int page)
{
    port_out(0x0b, SEQ_I);
    port_out(port_in(SEQ_D), SEQ_D);
    port_in(SEQ_D);		/* select new mode regs */

    port_out(0x0e, SEQ_I);
    port_out(page ^ 0x02, SEQ_D);	/* select the page */
}


/* Set display start address (not for 16 color modes) */
/* Trident supports any address in video memory (up to 1Mb) */

static void tvga8900_setdisplaystart(int address)
{
    if (__svgalib_cur_mode == G320x200x256) {
	outw(0x3d4, 0x0d + (address & 0x00ff) * 256);
	outw(0x3d4, 0x0c + (address & 0xff00));
	address <<= 2;		/* Adjust address so that extended bits */
	/* are correctly set later (too allow for */
	/* multi-page flipping in 320x200). */
	goto setextendedbits;
    }
    if (tvga8900_memory == 1024) {
	outw(0x3d4, 0x0d + ((address >> 3) & 0x00ff) * 256);	/* sa2-sa9 */
	outw(0x3d4, 0x0c + ((address >> 3) & 0xff00));	/* sa10-sa17 */
    } else {
	outw(0x3d4, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
	outw(0x3d4, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    }
    if (__svgalib_cur_mode != G320x200x256) {
	inb(0x3da);		/* set ATC to addressing mode */
	outb(0x3c0, 0x13 + 0x20);	/* select ATC reg 0x13 */
	if (tvga8900_memory == 1024) {
	    outb(0x3c0, (inb(0x3c1) & 0xf0) | (address & 7));
	    /* write sa0-2 to bits 0-2 */
	    address >>= 1;
	} else
	    outb(0x3c0, (inb(0x3c1) & 0xf0) | ((address & 3) << 1));
	/* write sa0-1 to bits 1-2 */
    }
  setextendedbits:
    outb(0x3d4, 0x1e);
    outb(0x3d5, (inb(0x3d5) & 0x5f) | 0x80	/* set bit 7 */
	 | ((address & 0x40000) >> 13));	/* sa18: write to bit 5 */
    outb(0x3c4, 0x0b);
    outb(0x3c5, 0);		/* select 'old mode' */
    outb(0x3c4, 0x0e);
    outb(0x3c5, (inb(0x3c5) & 0xfe)
	 | ((address & 0x80000) >> 19));	/* sa19: write to bit 0 */
    outb(0x3c4, 0x0b);
    inb(0x3c5);			/* return to 'new mode' */
}


/* Set logical scanline length (usually multiple of 8) */
/* Trident supports multiples of 8 to 2040 */

static void tvga8900_setlogicalwidth(int width)
{
    outw(0x3d4, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
}


/* Function table */

DriverSpecs __svgalib_tvga8900_driverspecs =
{
    tvga8900_saveregs,
    tvga8900_setregs,
    nothing,			/* unlock */
    nothing,			/* lock */
    tvga8900_test,
    tvga8900_init,
    tvga8900_setpage,
    (void (*)(int)) nothing,	/* __svgalib_setrdpage */
    (void (*)(int)) nothing,	/* __svgalib_setwrpage */
    tvga8900_setmode,
    tvga8900_modeavailable,
    tvga8900_setdisplaystart,
    tvga8900_setlogicalwidth,
    tvga8900_getmodeinfo,
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

static int tvga8900_init(int force, int par1, int par2)
{
    if (force) {
#ifdef DEBUG
	printf("Forcing memory to %dK\n", par1);
#endif
	tvga8900_memory = par1;
	tvga8900_nonint = par2 & 1;
/*
par2 specs
bit 31-3:	reserved
bit 2-1:	00 - force 8900
		01 - force 9440
		10 - force 9680
		11 - reserved
bit 0:		force noninterlaced if set
		only affects the 8900
*/
	switch (par2 & 6)
	{
	    case 0: tvga_model=8900;
	    break;
	    case 2: tvga_model=9440;
	    break;
	    case 4: tvga_model=9680;
	    break;
	    case 6: tvga_model=0;
	    break;
	}
    } else {
	port_out(0x1f, __svgalib_CRT_I);
	/* this should detect up to 2M memory now */
	tvga8900_memory = (port_in(__svgalib_CRT_D) & 0x07) * 256 + 256;

	/* Now is the card running in interlace mode? */
	port_out(0x0f, SEQ_I);
	tvga8900_nonint = port_in(SEQ_D) & 0x04;

	/* check version */
	outw(0x3c4, 0x000b);
	switch (inb(0x3c5)) {
	case 0x02:			/* 8800cs */
	case 0x03:			/* 8900b */
	case 0x04:			/* 8900c */
	case 0x13:
	case 0x33:			/* 8900cl */
	case 0x23:			/* 9000 */
	    tvga_model = 8900;
	    break;
	case 0xD3:
/* I know 0xD3 is a 9680, but if your 9680 is detected as a */
/* 9440, EMAIL ME! (ark) and I will change this...          */
	    tvga_model = 9680;
	    break;
	default:
/* Whatever the original 8900 driver thought was */
/* an invalid version, we will use as a 9440.    */
	    tvga_model = 9440;
	}
    }

    /* The 9440 uses ports 43C8 and 43C9... we need more privileges */
    if (tvga_model == 9440 || tvga_model == 9680)
	if (getenv("IOPERM") == NULL)
            if (iopl(3) < 0) {
		printf("tvga%d: Cannot get I/O permissions\n",tvga_model);
    }


    if (__svgalib_driver_report) {
	if(tvga_model == 9440)
	    printf("Using Trident 9440 driver (%dK)\n",
		tvga8900_memory);
	else if(tvga_model == 9680)
	    printf("Using Trident 9680 driver (%dK)\n",
		tvga8900_memory);
	else
	    printf("Using Trident 8900/9000 driver (%dK, %sinterlaced).\n",
		tvga8900_memory, (tvga8900_nonint) ? "non-" : "");
    }
    __svgalib_driverspecs = &__svgalib_tvga8900_driverspecs;
    setup_registers();
    return 0;
}
