/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen

   This library is free software; you can redistribute it and/or
   modify it without any restrictions. This library is distributed
   in the hope that it will be useful, but without any warranty.

   Multi-chipset support Copyright 1993 Harm Hanemaayer
   partially copyrighted (C) 1993 by Hartmut Schirmer

   OTI-067/077/087 support driver release 1.2 (12 September 1994)
   Rewritten/fleshed out by Christopher Wiles (a0017097@wsuaix.csc.wsu.edu)

   Features in this release: 
   * _real_ 640x480x256 mode
   * 800x600x16, 800x600x256 modes
   * 1024x768x16, 1024x768x256 modes
   * 640x480x32K, 800x600x32K modes
   * 1280x1024x16 mode (untested)
   * OTI-087 chipset support, including 2M variants


   Still left to do:
   * linear addressing
   The stubs are in the code, and can be activated by setting
   OTI87_LINEAR_OK to 1 (I've set it to 0).  The '87 goes into and out
   of linear mode just fine; I'm having a hell of a time getting SVGALIB
   to use the linear buffer instead of the bank switched buffer at A0000.
   In any event, if someone feels adventurous, the bones are present.

   * 64K/16M modes
   64K/16M modes appear to be broken.  I couldn't even switch them in
   from Oak's own DOS test utility ...

   * 1280x1024 modes
   I've included a definition for the 16 color mode.  I couldn't test
   it, as my monitor's maximum resolution is 1024x768.  According to
   the documentation, a 2 meg board should be able to do an interlaced
   1280x1024x256.  I couldn't force Oak's BIOS to enter that mode.
   Could be because I have only 1 meg on board.
 */

#include <stdio.h>
#include <string.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include <sys/mman.h>
#include <sys/types.h>
#include "oak.regs"

#ifdef OSKIT
#include "osenv.h"
#endif

#define OTI_MISC_DATA EXT+01
#define OTI_BCOMPAT_DATA EXT+02
#define OTI_SEGMENT_DATA EXT+03
#define OTI_CONFIG_DATA	EXT+04
#define OTI_OVERFLOW_DATA EXT+05
#define OTI_HSYNC2_DATA EXT+06
#define OTI_OVERFLOW2_DATA EXT+07

#define CLOCK_DATA EXT + 08
#define OVERFLOW_DATA EXT + 09
#define OTI87_HSYNC2_DATA EXT+10
#define OTI87_XCRTC_DATA EXT+11
#define OTI87_COLORS_DATA EXT+12
#define OTI87_FIFO_DATA EXT+13
#define OTI87_MODESELECT_DATA EXT+14
#define OTI87_FEATURE_DATA EXT+15
#define OTI87_XREADSEGMENT_DATA EXT+16
#define OTI87_XWRITESEGMENT_DATA EXT+17
#define OTI87_RAMDAC_DATA EXT+18

#define OTI_INDEX 0x3DE		/* Oak extended index register */
#define OTI_R_W 0x3DF		/* Oak extended r/w register */
#define OTI_CRT_CNTL 0xC	/* Oak CRT COntrol Register */
#define OTI_MISC  0xD		/* Oak Misc register */
#define OTI_BCOMPAT  0xE	/* Oak Back compat register */
#define OTI_SEGMENT  0x11	/* Oak segment register */
#define OTI_CONFIG  0x12	/* Oak config register */
#define OTI_OVERFLOW  0x14	/* Oak overflow register */
#define OTI_OVERFLOW2  0x16	/* Oak overflow2 register */

#define OTI87_PRODUCT 0x00	/* Oak product register */
#define OTI87_STATUS 0x02	/* Oak status register */
#define OTI87_VIDMAP 0x05	/* Oak video memory register */
#define OTI87_CLOCK 0x06	/* Oak clock select register */
#define OTI87_OVERFLOW 0x14	/* Oak overflow register */
#define OTI87_HSYNC2 0x15	/* Oak hsynch/2 register */
#define OTI87_XCRTC 0x17	/* Oak extended CRTC register */
#define OTI87_COLORS 0x19	/* Oak color range register */
#define OTI87_FIFO 0x20		/* Oak FIFO depth register */
#define OTI87_MODESELECT 0x21	/* Oak mode select register */
#define OTI87_FEATURE 0x22	/* Oak feature select register */
#define OTI87_XREADSEGMENT 0x23	/* Oak extended read segment reg. */
#define OTI87_XWRITESEGMENT 0x24	/* Oak extended write seg. reg. */
#define OTI87_XCOMMONSEGMENT 0x25	/* Oak extended common register */

#define OTI87_OK_LINEAR 0	/* Set to 1 for linear addressing */
#define DAC_SC11487 0x3c6	/* Sierra DAC command register */
#define DAC_REVERT 0x3c8	/* Clears DAC from command mode */

static int oak_chiptype;
static int oak_memory;
static int oak_lockreg_save;
static unsigned char last_page = 0;
static void *linearframebuffer;
static unsigned char *VGAMemory_Save;

static int oak_init(int, int, int);
static int oak_interlaced(int mode);
static int oak_memorydetect(void);
static int oak_chipdetect(void);
static int oak_inlinearmode(void);
static int OAKGetByte(int);

static void oak_unlock(void);
static void oak_setlinearmode(void);
static void oak_clearlinearmode(void);
static void oak_setpage(int page);
static void OAK_WriteDAC(int function);
static void OAKSetByte(int, int);

/* Mode table */

static ModeTable oak_modes[] =
{
/* *INDENT-OFF* */
    OneModeEntry(640x480x256),
    OneModeEntry(640x480x32K),
    OneModeEntry(800x600x16),
    OneModeEntry(1024x768x16),
    OneModeEntry(800x600x256),
    OneModeEntry(800x600x32K),
    OneModeEntry(1024x768x256),
    OneModeEntry(1280x1024x16),
    END_OF_MODE_TABLE
/* *INDENT-ON* */













};

/* oak_getmodeinfo( int mode, vga_modeinfo *modeinfo)   Tell SVGALIB stuff

 * Return mode information (blityes/no, start address, interlace, linear
 */

static void oak_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    if (modeinfo->bytesperpixel > 0) {
	modeinfo->maxpixels = oak_memory * 1024 / modeinfo->bytesperpixel;
    } else {
	modeinfo->maxpixels = oak_memory * 1024;	/* any val */
    }

    modeinfo->maxlogicalwidth = 2040;
    modeinfo->startaddressrange = 0xfffff;
    modeinfo->haveblit = 0;
    modeinfo->flags |= HAVE_RWPAGE;

    if (oak_interlaced(mode)) {
	modeinfo->flags |= IS_INTERLACED;
    }
    if ((oak_chiptype == 87) & (OTI87_OK_LINEAR == 1)) {
	modeinfo->flags |= CAPABLE_LINEAR;
	modeinfo->flags |= EXT_INFO_AVAILABLE;
	modeinfo->chiptype = 87;
	modeinfo->aperture_size = oak_memorydetect();
	modeinfo->set_aperture_page = oak_setpage;
	if (oak_inlinearmode()) {
	    modeinfo->flags |= IS_LINEAR;
	    /* The following line was missing? */
	    modeinfo->linear_aperture = linearframebuffer;
	}
    }
}

/* oak_saveregs( unsigned char regs[] )                 Save extended regs

 * Save extended registers into regs[].
 * OTI-067/077 have eight registers
 * OTI-087 has nineteen registers (only 11 used for native mode)
 *
 */

static int oak_saveregs(unsigned char regs[])
{
    oak_unlock();
    regs[OTI_SEGMENT_DATA] = OAKGetByte(OTI_SEGMENT);
    regs[OTI_MISC_DATA] = OAKGetByte(OTI_MISC);
    regs[OTI_BCOMPAT_DATA] = OAKGetByte(OTI_BCOMPAT);
    regs[OTI_CONFIG_DATA] = OAKGetByte(OTI_CONFIG);
    regs[OTI_OVERFLOW_DATA] = OAKGetByte(OTI_OVERFLOW);
    regs[OTI_HSYNC2_DATA] = OAKGetByte(OTI87_HSYNC2);
    regs[OTI_OVERFLOW2_DATA] = OAKGetByte(OTI_OVERFLOW2);
    if (oak_chiptype == 87) {
	regs[EXT + 8] = OAKGetByte(OTI87_CLOCK);
	regs[EXT + 9] = OAKGetByte(OTI87_OVERFLOW);
	regs[OTI87_HSYNC2_DATA] = OAKGetByte(OTI87_HSYNC2);
	regs[OTI87_XCRTC_DATA] = OAKGetByte(OTI87_XCRTC);
	regs[OTI87_COLORS_DATA] = OAKGetByte(OTI87_COLORS);
	regs[OTI87_FIFO_DATA] = OAKGetByte(OTI87_FIFO_DATA);
	regs[OTI87_MODESELECT_DATA] = OAKGetByte(OTI87_MODESELECT);
	regs[OTI87_FEATURE_DATA] = OAKGetByte(OTI87_FEATURE_DATA);
	regs[OTI87_XREADSEGMENT_DATA] = OAKGetByte(OTI87_XREADSEGMENT);
	regs[OTI87_XWRITESEGMENT_DATA] = OAKGetByte(OTI87_XWRITESEGMENT);
    }
    return 19;			/*19 additional registers */
}

/* oak_setregs( const unsigned char regs[], int mode )  Set Oak registers

 * Set extended registers for specified graphics mode
 * regs [EXT+1] through [EXT+7] used for OTI-067/77
 * regs [EXT+9] through [EXT+17] used for OTI-087 native mode
 */

static void oak_setregs(const unsigned char regs[], int mode)
{
    int junk;

    oak_unlock();
    if (oak_chiptype == 87) {
	OAKSetByte(OTI87_XREADSEGMENT, regs[OTI87_XREADSEGMENT_DATA]);
	OAKSetByte(OTI87_XWRITESEGMENT, regs[OTI87_XWRITESEGMENT_DATA]);

	outb(0x3c4, 0);		/* reset sync. */
	junk = inb(0x3c5);
	outw(0x3c4, 0x00 + ((junk & 0xfd) << 8));

	OAKSetByte(OTI87_CLOCK, regs[EXT + 8]);
	OAKSetByte(OTI87_FIFO, regs[OTI87_FIFO_DATA]);
	OAKSetByte(OTI87_MODESELECT, regs[OTI87_MODESELECT_DATA]);

	outw(0x3c4, 0x00 + (junk << 8));	/* set sync. */

	OAKSetByte(OTI87_XCRTC, regs[OTI87_XCRTC_DATA]);
	OAKSetByte(OTI87_OVERFLOW, regs[EXT + 9]);
	OAKSetByte(OTI87_HSYNC2, regs[OTI87_HSYNC2_DATA]);
	OAK_WriteDAC(regs[OTI87_RAMDAC_DATA]);
    } else {
	OAKSetByte(OTI_SEGMENT, regs[OTI_SEGMENT_DATA]);

	/* doc says don't OTI-MISC unless sync. reset is off */
	outb(0x3c4, 0);
	junk = inb(0x3c5);
	outw(0x3C4, 0x00 + ((junk & 0xFD) << 8));	/* now disable the timing sequencer */

	OAKSetByte(OTI_MISC, regs[OTI_MISC_DATA]);

	/* put sequencer back */
	outw(0x3C4, 0x00 + (junk << 8));

	OAKSetByte(OTI_BCOMPAT, regs[OTI_BCOMPAT_DATA]);
	OAKSetByte(OTI_CONFIG, regs[OTI_CONFIG_DATA]);
	OAKSetByte(OTI_OVERFLOW, regs[OTI_OVERFLOW_DATA]);
	OAKSetByte(OTI87_HSYNC2, regs[OTI_HSYNC2_DATA]);
	OAKSetByte(OTI_OVERFLOW2, regs[OTI_OVERFLOW2_DATA]);
    }
}

/* oak_modeavailable( int mode )                        Check if mode supported

 * Verify that desired graphics mode can be displayed by chip/memory combo
 * Returns SVGADRV flag if SVGA, vga_chipsetfunctions if VGA, 0 otherwise
 */

static int oak_modeavailable(int mode)
{
    const unsigned char *regs;
    struct info *info;

    regs = LOOKUPMODE(oak_modes, mode);
    if (regs == NULL || mode == GPLANE16) {
	return __svgalib_vga_driverspecs.modeavailable(mode);
    }
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE) {
	return 0;
    }
    info = &__svgalib_infotable[mode];
    if (oak_memory * 1024 < info->ydim * info->xbytes) {
	return 0;
    }
    return SVGADRV;
}



/* oak_interlaced( int mode )                           Is mode interlaced?

 * Self-explanatory.
 * Returns non-zero if mode is interlaced (bit 7)
 */

static int oak_interlaced(int mode)
{
    const unsigned char *regs;

    if (oak_modeavailable(mode) != SVGADRV) {
	return 0;
    } else {
	regs = LOOKUPMODE(oak_modes, mode);
	if (regs == NULL || regs == DISABLE_MODE) {
	    return 0;
	} else {
	    return regs[EXT + 05] & 0x80;
	}
    }
}

/* oak_setmode( int mode, int prv_mode )                Set graphics mode

 * Attempts to set a graphics mode.
 * Returns 0 if successful, 1 if unsuccessful
 * 
 * Calls vga_chipsetfunctions if VGA mode)
 */

static int oak_setmode(int mode, int prv_mode)
{
    const unsigned char *rp;
    unsigned char regs[sizeof(g640x480x256_regs)];

    if ((oak_chiptype == 87) & (OTI87_OK_LINEAR == 1)) {
	oak_clearlinearmode();
    }
    rp = LOOKUPMODE(oak_modes, mode);
    if (rp == NULL || mode == GPLANE16)
	return (int) (__svgalib_vga_driverspecs.setmode(mode, prv_mode));
    if (!oak_modeavailable(mode))
	return 1;		/* mode not available */

    /* Update the register information */
    memcpy(regs, rp, sizeof(regs));

    /*  Number of memory chips */
    if (oak_chiptype != 87) {
	regs[OTI_MISC_DATA] &= 0x3F;
	regs[OTI_MISC_DATA] |= (oak_memory == 1024 ? 0x40 : 0x00);
	regs[OTI_MISC_DATA] |= (oak_memory >= 512 ? 0x80 : 0x00);
	if (oak_chiptype == 77) {
	    regs[OTI_CONFIG_DATA] |= 0x08;
	} else {
	    regs[OTI_CONFIG_DATA] &= 0xF7;
	}
    } else {			/* oak_chiptype == 87 */
	if (mode == G640x480x256) {
	    /* Oak-87 needs this bit, Oak-67 does not stand it - MW */
	    regs[SEQ + 1] |= 0x8;
	}
    }

    if (__svgalib_infotable[mode].colors == 16) {

	/* switch from 256 to 16 color mode (from XFree86) */
	regs[SEQ + 4] &= 0xf7;	/* Switch off chain 4 mode */
	if (oak_chiptype == 87) {
	    regs[OTI87_FIFO_DATA] &= 0xf0;
	    regs[OTI87_MODESELECT_DATA] &= 0xf3;
	} else {
	    regs[OTI_MISC_DATA] &= 0xf0;
	    regs[OTI_MISC_DATA] |= 0x18;
	}
    }
    __svgalib_setregs(regs);
    oak_setregs(regs, mode);
    if ((oak_chiptype == 87) & (OTI87_OK_LINEAR == 1)) {
	oak_setlinearmode();
    }
    return 0;
}




/* oak_unlock()                                         Unlock Oak registers

 * Enable register changes
 * 
 * _No effect with OTI-087 -- register is nonfunctional_
 */

static void oak_unlock(void)
{
    int temp;
    /* Unprotect CRTC[0-7] */
    outb(__svgalib_CRT_I, 0x11);
    temp = inb(__svgalib_CRT_D);
    outb(__svgalib_CRT_D, temp & 0x7F);
    temp = OAKGetByte(OTI_CRT_CNTL);
    OAKSetByte(OTI_CRT_CNTL, temp & 0xf0);
    oak_lockreg_save = temp;
}

/* oak_lock()                                           Lock Oak registers

 * Prevents registers from accidental change
 * 
 * _No effect with OTI-087 -- register is nonfunctional_
 */

static void oak_lock(void)
{
    int temp;
    /* don't set the i/o write test bit, though we cleared it on entry */
    OAKSetByte(OTI_CRT_CNTL, oak_lockreg_save & 0xf7);
    /* Protect CRTC[0-7] */
    outb(__svgalib_CRT_I, 0x11);
    temp = inb(__svgalib_CRT_D);
    outb(__svgalib_CRT_D, (temp & 0x7F) | 0x80);
}

/* oak_test()                                           Probe for Oak card

 * Checks for Oak segment register, then chip type and memory size.
 * 
 * Returns 1 for Oak, 0 otherwise.
 */

static int oak_test(void)
{
    int save, temp1;
    oak_unlock();

    save = OAKGetByte(OTI_SEGMENT);
    OAKSetByte(OTI_SEGMENT, save ^ 0x11);
    temp1 = OAKGetByte(OTI_SEGMENT);
    OAKSetByte(OTI_SEGMENT, save);
    if (temp1 != (save ^ 0x11)) {
	oak_lock();		/* unsuccesful */
	return 0;
    }
    /* HH: Only allow 087, 077 support seems to be broken for at */
    /* least one 512K card. May be due to mode dump configuring */
    /* for 1024K. */

    /* Reallow 067 and 077 */
    /* if (oak_chipdetect() < 87)
       return 0; */

    return oak_init(0, 0, 0);
}

/* oak_setpage( int page )                              Set read/write pages

 * Sets both read and write extended segments (64k bank number)
 * Should be good for 5 bits with OTI-087
 */

static void oak_setpage(int page)
{
    if (oak_chiptype == 87) {
	OAKSetByte(OTI87_XCOMMONSEGMENT, page);
    } else {
	OAKSetByte(OTI_SEGMENT, (last_page = page | (page << 4)));
    }
}

/* oak_setrdpage( int page )                            Set read page

 * Sets read extended segment (64k bank number)
 * Should be good for 5 bits with OTI-087
 */

static void oak_setrdpage(int page)
{
    if (oak_chiptype == 87) {
	OAKSetByte(OTI87_XREADSEGMENT, page);
    } else {
	last_page &= 0xF0;
	last_page |= page;
	OAKSetByte(OTI_SEGMENT, last_page);
    }
}

/* oak_setwrpage( int page )                            Set write page

 * Sets write extended segment (64k bank number)
 * Should be good for 5 bits with OTI-087
 */

static void oak_setwrpage(int page)
{
    if (oak_chiptype == 87) {
	OAKSetByte(OTI87_XWRITESEGMENT, page);
    } else {
	last_page &= 0x0F;
	last_page |= page << 4;
	OAKSetByte(OTI_SEGMENT, last_page);
    }
}

/* oak_setdisplaystart( int address )                   Set display address

 * Sets display start address.
 * First word goes into 0x3d4 indices 0x0c and 0x0d
 * If 067, bit 16 goes to OTI_OVERFLOW bit 3
 * If 077, bit 17 goes to OTI_OVERFLOW2 bit 3
 * If 087, bits 16, 17, and 18 go to OTI87_XCRTC bits 0-2
 */

static void oak_setdisplaystart(int address)
{
    outw(0x3d4, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
    outw(0x3d4, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    inb(0x3da);			/* set ATC to addressing mode */
    outb(0x3c0, 0x13 + 0x20);	/* select ATC reg 0x13 */
    outb(0x3c0, (inb(0x3c1) & 0xf0) | ((address & 3) << 1));
    /* write sa0-1 to bits 1-2 */

    if (oak_chiptype == 87) {
	OAKSetByte(OTI87_XCRTC, (address & 0x1c0000) >> 18);
    } else {
	OAKSetByte(OTI_OVERFLOW, (OAKGetByte(OTI_OVERFLOW) & 0xf7)
		   | ((address & 0x40000) >> 15));
	/* write sa18 to bit 3 */
	if (oak_chiptype == 77) {
	    OAKSetByte(OTI_OVERFLOW2,
		       (OAKGetByte(OTI_OVERFLOW2) & 0xf7)
		       | ((address & 0x80000) >> 16));
	    /* write sa19 to bit 3 */
	}
    }
}

/* oak_setlogicalwidth( int width )                     Set scanline length

 * Set logical scanline length (usually multiple of 8)
 * Multiples of 8 to 2040
 */

static void oak_setlogicalwidth(int width)
{
    outw(0x3d4, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
}

/* oak_init ( int force, int par1, int par2)            Initialize chipset

 * Detects Oak chipset type and installed video memory.
 * Returns 1 if chipset is supported, 0 otherwise
 */

static int oak_init(int force, int par1, int par2)
{
    if (force) {
	oak_chiptype = par1;
	oak_memory = par2;
    } else {
	oak_chiptype = oak_chipdetect();
	if (oak_chiptype == 0) {
	    return 0;		/* not supported */
	}
	oak_memory = oak_memorydetect();
    }
    if (__svgalib_driver_report) {
	printf("Using Oak driver (OTI-0%d, %dK).\n", oak_chiptype,
	       oak_memory);
    }
    __svgalib_driverspecs = &__svgalib_oak_driverspecs;
    return 1;
}

/* oak_memorydetect()                           Report installed video RAM

 * Returns size (in Kb) of installed video memory
 * Defined values are 256, 512, 1024, and 2048 (OTI-087 only)
 */

static int oak_memorydetect(void)
{
    int temp1;
    if (oak_chiptype == 87) {
	temp1 = OAKGetByte(OTI87_STATUS) & 0x06;
	if (temp1 == 0x06)
	    return 2048;
	else if (temp1 == 0x04)
	    return 1024;
	else if (temp1 == 0x02)
	    return 512;
	else if (temp1 == 0x00)
	    return 512;
	else {
	    printf("Oak driver: Invalid amount of memory. Using 256K.\n");
	    return 256;
	}
    } else {
	temp1 = OAKGetByte(OTI_MISC) & 0xc0;
	if (temp1 == 0xC0)
	    return 1024;
	else if (temp1 == 0x80)
	    return 512;
	else if (temp1 == 0x00)
	    return 256;
	else {
	    printf("Oak driver: Invalid amount of memory. Using 256K.\n");
	    return 256;
	}
    }
}

/* oak_chipdetect()                             Detect Oak chip type

 * Returns chip id (67, 77, 87) if video chipset is a supported Oak type
 * (57 is _not_ supported).
 * Returns 0 otherwise
 */

static int oak_chipdetect(void)
{
    int temp1;

    temp1 = inb(OTI_INDEX);
    temp1 &= 0xe0;
    if (temp1 == 0x40)
	return 67;
    if (temp1 == 0xa0)
	return 77;

    /* not a '67 or '77 ... try for '87 ... */

    temp1 = OAKGetByte(OTI87_PRODUCT) & 0x01;

    if (temp1 == 0x01)
	return 87;


    /* none of the above ... maybe the nonexistant '97 I've been 
       hearing about? */

    printf("Oak driver: Unknown chipset (id = %2x)\n", temp1);
    oak_lock();
    return 0;
}

/* oak_inlinearmode()                   Check if OTI-087 is in linear mode

 * Bit 0 set if yes, clear if no
 * Return: true/false
 */

static int oak_inlinearmode(void)
{
    return (OAKGetByte(OTI87_VIDMAP) & 0x01) != 0;
}

/* oak_setlinearmode()                  Set linear mode for OTI-087

 * For simplicity's sake, we're forcing map at the E0000 (14M) mark
 * We grab the memory size from OTI87_STATUS, and we're mapping it all.
 * 
 * Video map register looks like this:
 *      Bit 0:   0 = VGA mapping (A0000-BFFFF) 1 = Linear
 *      Bit 1:   0 = Enable DMA 1 = Disable DMA
 *      Bit 2/3: Aperature (256/512/1024/2048, bitwise)
 *      Bit 4-7: Starting address (High nybble, 1-F)
 */

static void oak_setlinearmode(void)
{
    int temp1;
    temp1 = ((OAKGetByte(OTI87_STATUS) & 0x06) << 1) | 0xe1;
    OAKSetByte(OTI87_VIDMAP, temp1);
#ifdef OSKIT
    osenv_mem_map_phys(0x40000000 + 0xe00000, oak_memory * 1024, &linearframebuffer, 0);
#else
    linearframebuffer = (unsigned char *) mmap((caddr_t) 0x40000000,
					       oak_memory * 1024,
					       PROT_READ | PROT_WRITE,
					       MAP_SHARED | MAP_FIXED,
					       __svgalib_mem_fd,
					       0xe00000);
#endif
    VGAMemory_Save = __svgalib_graph_mem;
/*      __svgalib_graph_mem = linearframebuffer; */
/*      graph_mem = __svgalib_graph_mem; */
    __svgalib_modeinfo_linearset |= IS_LINEAR;
}

/* oak_clearlinearmode()                        Stop linear mode for OTI-087

 * Switches linear mode off, reset aperture and address range
 * (see oak_golinearmode() for register description)
 */

static void oak_clearlinearmode(void)
{
    OAKSetByte(OTI87_VIDMAP, 0x00);
    __svgalib_modeinfo_linearset ^= IS_LINEAR;
    __svgalib_graph_mem = VGAMemory_Save;
    graph_mem = VGAMemory_Save;
    munmap((caddr_t) 0x40000000, oak_memory * 1024);

}

/* OAK_WriteDAC ( int dac_value )                       Write value to DAC

 * Writes function command to DAC at 0x03c6
 * (Assumes that DAC is SC11487 ... may not work for others)
 *
 */

static void OAK_WriteDAC(int dac_value)
{
    inb(DAC_REVERT);		/* reset DAC r/w pointer */
    inb(DAC_SC11487);		/* do this four times to set register A */
    inb(DAC_SC11487);
    inb(DAC_SC11487);
    inb(DAC_SC11487);
    outb(DAC_SC11487, dac_value);
    inb(DAC_REVERT);		/* and reset DAC ... */
}

/* OAKSetByte ( index, value)                           Set extended register

 * Puts a specified value in a specified extended register.
 * Splitting this commonly used instruction sequence into its own subroutine
 * saves about 100 bytes of code overall ...
 */

static void OAKSetByte(int OTI_index, int OTI_value)
{
    outb(OTI_INDEX, OTI_index);
    outb(OTI_R_W, OTI_value);
}

/* OAKGetByte ( index )                                 Returns ext. register

 * Returns value from specified extended register.
 * As with OakSetByte, this tightens the code considerably.
 */

static int OAKGetByte(int OTI_index)
{
    outb(OTI_INDEX, OTI_index);
    return inb(OTI_R_W);
}

/* Function table (exported) */

DriverSpecs __svgalib_oak_driverspecs =
{
    oak_saveregs,
    oak_setregs,
    oak_unlock,
    oak_lock,
    oak_test,
    oak_init,
    oak_setpage,
    oak_setrdpage,
    oak_setwrpage,
    oak_setmode,
    oak_modeavailable,
    oak_setdisplaystart,
    oak_setlogicalwidth,
    oak_getmodeinfo,
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
#if 0
    (int (*)()) oak_setlinearmode,
    (int (*)()) oak_clearlinearmode,
    (int (*)()) OAK_WriteDAC,
#endif
};
