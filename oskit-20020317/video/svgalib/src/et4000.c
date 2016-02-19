/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (c) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* ET4000 code taken from VGAlib
 * ET4000 code modified to handle HiColor modes better by David Monro
 * Dynamic register loading by Hartmut Schirmer
 * HH: ET4000/W32 detection added and support for more than 1Mb (based on
 *     vgadoc3) (untested).
 * HH: Detect newer ET4000/W32p.
 */


/* Note that the clock detection stuff is currently not used. */

/* ET4000 registers description (from vgadoc2)
   **
   **
   **
   ** 102h: Microchannel Setup Control
   ** bit 0  Disable Card if set
   **
   ** 3BFh (R/W): Hercules Compatability register
   ** 
   ** 3C0h index 16h: ATC Miscellaneous
   **    (Write data to 3C0h, Read from 3C1h  (May not be needed))
   ** bit 4,5  High resolution timings.
   **       7  Bypass the internal palette if set
   **
   ** 3C3h (R/W): Microchannel Video Subsystem Enable Register:
   ** bit 0  Enable Microchannel VGA if set
   **
   ** 3C4h index  6  (R/W): TS State Control
   ** bit 1-2  dots per characters in text mode 
   **       (bit 0: 3c4 index 1, bit 0)
   **       bit <2:0>   ! dots/char
   **           111     !    16
   **           100     !    12
   **           011     !    11
   **           010     !    10
   **           001     !     8
   **           000     !     9
   **
   ** 3C4h index  7  (R/W): TS Auxiliary Mode
   ** bit 0  If set select MCLK/4 (if bit 6 is also set to 1)
   **     1  If set select SCLK input from MCLK
   **   3,5  Rom Bios Enable/Disable:
   **     0 0  C000-C3FF Enabled
   **     0 1  Rom disabled
   **     1 0  C000-C5FF,C680-C7FF Enabled
   **     1 1  C000-C7FF Enabled
   **     6  MCLK/2 if set
   **     7  VGA compatible if set EGA else.
   **
   ** 3CBh (R/W): PEL Address/Data Wd
   **  
   ** 3CDh (R/W): Segment Select
   **     0-3  64k Write bank nr (0..15)
   **     4-7  64k Read bank nr (0..15)
   **
   ** 3CEh index  Dh (R/W): Microsequencer Mode
   **
   ** 3CEh index  Eh (R/W): Microsequencer Reset 
   **
   ** 3d4h index 24h (R/W): Compatibility Control
   ** bit 0  Enable Clock Translate
   **     1  Additional Master Clock Select
   **     2  Enable tri-state for all output pins
   **     3  Enable input A8 of 1MB DRAMs
   **     4  Reserved
   **     5  Enable external ROM CRTC translation
   **     6  Enable Double Scan and Underline Attribute
   **     7  CGA/MDA/Hercules
   **
   ** 3d4h index 32h (R/W): RAS/CAS Video Config
   **       Ram timing, System clock and Ram type. Sample values:
   **      00h  VRAM  80nsec
   **      09h  VRAM 100nsec
   **      00h  VRAM  28MHz
   **      08h  VRAM  36MHz
   **      70h  DRAM  40MHz
   **
   ** 3d4h index 33h (R/W): Extended start ET4000 
   ** bit 0-1  Display start address bits 16-17
   **     2-3  Cursor start address bits 16-17
   **     Can be used to ID ET4000
   **
   ** 3d4h index 34h (R/W): Compatibility Control Register
   ** bit 2  bit 3 of clock select (bit 1-0 in misc output)
   **     3  if set Video Subsystem Enable Register at 46E8h
   **             else at 3C3h.
   **
   ** 3d4h index 35h (R/W): Overflow High ET4000
   ** bit 0  Vertical Blank Start Bit 10
   **     1  Vertical Total Bit 10
   **     2  Vertical Display End Bit 10
   **     3  Vertical Sync Start Bit 10
   **     4  Line Compare Bit 10
   **     5  Gen-Lock Enabled if set (External sync)
   **     6  Read/Modify/Write Enabled if set. Currently not implemented.
   **     7  Vertical interlace if set
   **
   ** 3d4h index 36h (R/W): Video System Configuration 1
   ** bit 0-2 Refresh count per line - 1
   **     3   16 bit wide fonts if set, else 8 bit wide
   **     4   Linear addressing if set. Video Memory is 
   **         mapped as a 1 Meg block above 1MB. (set
   **      GDC index 6 bits 3,2 to zero (128k))
   **     5   TLI addressing mode if set
   **     6   16 bit data path (video memory) if set
   **     7   16 bit data (I/O operations) if set
   **
   ** 3d4h index 37h (R/W): Video System Configuration 2
   ** bit 0-1  Display memory data bus width
   **          0,1=8bit, 2=16bit, 3=32bit; may be
   **       read as number of memory banks (1,2,4)
   **    2  Bus read data latch control. If set latches
   **       databus at end of CAS cycle else one clock delay
   **       3  Clear if 64kx4 RAMs                   ???
   **     if set RAM size = (bit 0-1)*256k
   **       else RAM size = (bit 0-1)* 64k
   **       4  16 bit ROM access if set
   **       5  Memory bandwidth (0 better than 1) ???
   **       6  TLI internal test mode if set 
   **       7  VRAM installed if set DRAM else.
   **
   ** 3d4h index 3Fh (R/W):
   ** bit   7  This bit seems to be bit 8 of the CRTC offset register (3d4h index 13h).
   **
   ** 3d8h (R/W): Display Mode Control
   **
   ** 46E8h (R):  Video Subsystem Enable Register
   ** bit   3  Enable VGA if set
   **
   **
   **       3C4h index 05 used.
   ** 
   **
   **  Bank Switching:
   **
   **     64k banks are selected by the Segment Select Register at 3CDh.
   **     Both a Read and a Write segment can be selected.
   **
   **  The sequence: 
   **
   **      port[$3BF]:=3;
   **      port[$3D8]:=$A0;
   **
   **  is apparently needed to enable the extensions in the Tseng 4000.
   **
   **
   **  Used extended ET4000 registers (EXT+xx) :
   **
   **   00 : CRT (3d4) index 30
   **   01 : CRT (3d4) index 31
   **   02 : CRT (3d4) index 32
   **   03 : CRT (3d4) index 33
   **   04 : CRT (3d4) index 34
   **   05 : CRT (3d4) index 35
   **   06 : CRT (3d4) index 36
   **   07 : CRT (3d4) index 37
   **   08 : CRT (3d4) index 3f
   **   09 : SEQ (3c4) index 07
   **   0A : Microchannel register (3c3)
   **   0B : Segment select (3cd)
   **   0C : ATT (3c0) index 16
   **
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#ifdef USEGLIBC
#include <sys/io.h>
#endif
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "ramdac/ramdac.h"

#define SEG_SELECT 0x3CD

#define CHIP_ET4000	0	/* Chip types. */
#define CHIP_ET4000W32	1
#define CHIP_ET4000W32i	2
#define CHIP_ET4000W32p	3

static char *chipname[] =
{
    "ET4000",
    "ET4000/W32",
    "ET4000/W32i",
    "ET4000/W32p"
};

static int et4000_memory;
static int et4000_chiptype;

static int et4000_init(int, int, int);
static int et4000_interlaced(int mode);
static void et4000_unlock(void);

static void et4000_setlinear(int addr);

static int et4000_extdac;
static int pos_ext_settings, ext_settings;

/* Mode table */
#if defined(DYNAMIC)

static ModeTable *et4000_modes = NULL;
static ModeTable No_Modes = END_OF_MODE_TABLE;

#else				/* !defined(DYNAMIC) */

#include "et4000.regs"

#ifdef DAC_TYPE
static int et4000_dac = DAC_TYPE;
#endif

static ModeTable et4000_modes[] =
{
/* *INDENT-OFF* */
    OneModeEntry(320x200x32K),
    OneModeEntry(320x200x64K),
    OneModeEntry(320x200x16M),
    OneModeEntry(640x480x256),
    OneModeEntry(640x480x32K),
    OneModeEntry(640x480x64K),
    OneModeEntry(640x480x16M),
    OneModeEntry(800x600x16),
    OneModeEntry(800x600x256),
    OneModeEntry(800x600x32K),
    OneModeEntry(800x600x64K),
    OneModeEntry(1024x768x16),
    OneModeEntry(1024x768x256),
    OneModeEntry(1280x1024x16),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

#endif				/* !defined(DYNAMIC) */

#ifndef DAC_TYPE
static int et4000_dac = -1;
#endif

/* Fill in chipset specific mode information */

static void et4000_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    switch (modeinfo->colors) {
    case 16:			/* 4-plane 16 color mode */
	modeinfo->maxpixels = 65536 * 8;
	break;
    default:
	if (modeinfo->bytesperpixel > 0)
	    modeinfo->maxpixels = et4000_memory * 1024 /
		modeinfo->bytesperpixel;
	else
	    modeinfo->maxpixels = et4000_memory * 1024;
	break;
    }
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = 0xfffff;
    if (mode == G320x200x256)
	modeinfo->startaddressrange = 0;
    modeinfo->haveblit = 0;
    modeinfo->memory = et4000_memory * 1024;
    modeinfo->flags |= HAVE_RWPAGE | HAVE_EXT_SET;
    if (et4000_interlaced(mode))
	modeinfo->flags |= IS_INTERLACED;
    if (et4000_chiptype != CHIP_ET4000)
	modeinfo->flags |= EXT_INFO_AVAILABLE | CAPABLE_LINEAR;
}


/* ----------------------------------------------------------------- */
/* Set/get the actual clock frequency */

#if defined(USE_CLOCKS) || defined(DYNAMIC)
#ifndef CLOCKS
#define CLOCKS 24
#endif
#ifndef CLOCK_VALUES
#define CLOCK_VALUES { 0 }
#endif

static unsigned clocks[CLOCKS] = CLOCK_VALUES;
#endif


#ifdef USE_CLOCKS

/* Only include the rest of the clock stuff if USE_CLOCKS is defined. */

static int et4000_clocks(int clk)
{
    unsigned char temp;
    int res;

    /* get actual clock */
    res = (inb(MIS_R) >> 2) & 3;	/* bit 0..1 */
    outb(__svgalib_CRT_I, 0x34);
    res |= (inb(__svgalib_CRT_D) & 2) << 1;	/* bit 2 */
    outb(SEQ_I, 0x07);
    temp = inb(SEQ_D);
    if (temp & 0x41 == 0x41)
	res |= 0x10;		/* bit 3..4 */
    else
	res |= (temp & 0x40) >> 3;	/* bit 3 */

    if (clk >= CLOCKS)
	clk = CLOCKS - 1;
    if (clk >= 0) {
	/* Set clock */
	temp = inb(MIS_R) & 0xF3;
	outb(MIS_W, temp | ((clk & 3) << 2));
	outb(__svgalib_CRT_I, 0x34);
	temp = inb(__svgalib_CRT_D) & 0xFD;
	outb(__svgalib_CRT_D, temp | ((clk & 4) >> 1));
	outb(SEQ_I, 0x07);
	temp = inb(SEQ_D) & 0xBE;
	temp |= (clk & 0x10 ? 0x41 : 0x00);
	temp |= (clk & 0x08) << 3;
	outb(SEQ_D, temp);
	usleep(5000);
    }
    return res;
}

#define FRAMES 2

/* I think the Xfree86 uses a similar BAD HACK ... */
static int measure(int frames)
{
    unsigned counter;
    __asm__ volatile (
			 "	xorl	%0,%0		\n"
			 "__W1:	inb	%1,%%al		\n"
			 "	testb	$8,%%al		\n"
			 "	jne	__W1		\n"
			 "__W2:	inb	%1,%%al		\n"
			 "	testb	$8,%%al		\n"
			 "	je	__W2		\n"
			 "__L1:	inb	%1,%%al		\n"
			 "	incl	%0		\n"
			 "	testb	$8,%%al		\n"
			 "	jne	__L1		\n"
			 "__L2:	inb	%1,%%al		\n"
			 "	incl	%0		\n"
			 "	testb	$8,%%al		\n"
			 "	je	__L2		\n"
			 "__L3:	decl	%2		\n"
			 "	jns	__L1		\n"
			 :"=b" (counter)
			 :"d"((unsigned short) (0x3da)), "c"(frames)
			 :"ax", "cx"
    );
    return counter;
}

#define BASIS_FREQUENCY 28322

static int measure_clk(int clk)
{
    unsigned new;
    int old_clk, state;
    static unsigned act = 0;

    old_clk = et4000_clocks(-1);
    if ((state = SCREENON))
	vga_screenoff();
    if (act == 0) {
	unsigned char save = inb(MIS_R);
	outb(MIS_W, (save & 0xF3) | 0x04);
	act = measure(FRAMES);
	outb(MIS_W, save);
    }
    et4000_clocks(clk);
    new = measure(FRAMES);
    et4000_clocks(old_clk);
    if (state)
	vga_screenon();

    return (((long long) BASIS_FREQUENCY) * act) / new;
}

#define set1(f,v) ({ if ((f)==0) (f)=(v); })

static void set3(unsigned *f, int clk, unsigned base)
{
    if (clk - 16 >= 0)
	set1(f[clk - 16], 4 * base);
    if (clk - 8 >= 0)
	set1(f[clk - 8], 2 * base);
    set1(f[clk], base);
    if (clk + 8 < CLOCKS)
	set1(f[clk + 8], base / 2);
    if (clk + 16 < CLOCKS)
	set1(f[clk + 16], base / 4);
}

static int get_clock(int clk)
{
    static int first = 1;

    if (clk < 0 || clk >= CLOCKS)
	return 0;
    if (first) {
	int act_clk;
	first = 0;
	act_clk = et4000_clocks(-1);
	act_clk &= 0xFC;
	set3(clocks, act_clk, 25175);	/* act_clk  : 25175 KHz */
	set3(clocks, act_clk + 1, 28322);	/* act_clk+1: 28322 KHz */
	for (act_clk = 0; act_clk < CLOCKS; ++act_clk)
	    if (clocks[act_clk] != 0)
		set3(clocks, act_clk, clocks[act_clk]);
    }
    if (clocks[clk] == 0) {
	int c = clk & 7;
	clocks[16 + c] = measure_clk(16 + c);
	clocks[8 + c] = 2 * clocks[16 + c];
	clocks[c] = 2 * clocks[8 + c];
    }
    return clocks[clk];
}

#endif				/* defined(USE_CLOCKS) */

/* ----------------------------------------------------------------- */


/* Read and store chipset-specific registers */

static int et4000_saveregs(unsigned char regs[])
{
    int i;

    et4000_unlock();
    /* save extended CRT registers */
    for (i = 0; i < 8; i++) {
	port_out(0x30 + i, __svgalib_CRT_I);
	regs[EXT + i] = port_in(__svgalib_CRT_D);
    }
    port_out(0x3f, __svgalib_CRT_I);
    regs[EXT + 8] = port_in(__svgalib_CRT_D);

    /* save extended sequencer register */
    port_out(7, SEQ_I);
    regs[EXT + 9] = port_in(SEQ_D);

    /* save some other ET4000 specific registers */
    regs[EXT + 10] = port_in(0x3c3);
    regs[EXT + 11] = port_in(0x3cd);

    /* save extended attribute register */
    port_in(__svgalib_IS1_R);		/* reset flip flop */
    port_out(0x16, ATT_IW);
    regs[EXT + 12] = port_in(ATT_R);

    if (et4000_extdac) {
	_ramdac_dactocomm();
	regs[EXT + 13] = inb(PEL_MSK);
	_ramdac_dactocomm();
	outb(PEL_MSK, regs[EXT + 13] | 0x10);
	_ramdac_dactocomm();
	inb(PEL_MSK);
	outb(PEL_MSK, 3);		/* write index low */
	outb(PEL_MSK, 0);		/* write index high */
	regs[EXT + 14] = inb(PEL_MSK);	/* primary ext. pixel select */
	regs[EXT + 15] = inb(PEL_MSK);	/* secondary ext. pixel select */
	regs[EXT + 16] = inb(PEL_MSK);	/* PLL control register */
	_ramdac_dactocomm();
	outb(PEL_MSK, regs[EXT + 13]);
	return 17;
    }
    return 13;			/* ET4000 requires 13 additional registers */
}


/* Set chipset-specific registers */

static void et4000_setregs(const unsigned char regs[], int mode)
{
    int i;
    unsigned char save;

    /* make sure linear mode is forced off */
    et4000_setlinear(0);

    et4000_unlock();
    /* write some ET4000 specific registers */
    port_out(regs[EXT + 10], 0x3c3);
    port_out(regs[EXT + 11], 0x3cd);

    /* write extended sequencer register */
    port_out(7, SEQ_I);
    port_out(regs[EXT + 9], SEQ_D);

#if 0				/* This writes to registers we shouldn't write to. */
    /* write extended CRT registers */
    for (i = 0; i < 6; i++) {
	port_out(0x32 + i, __svgalib_CRT_I);
	port_out(regs[EXT + i], __svgalib_CRT_D);
    }
    port_out(0x3f, __svgalib_CRT_I);
    port_out(regs[EXT + 6], __svgalib_CRT_D);
#endif

    /* deprotect CRT register 0x35 */
    port_out(0x11, __svgalib_CRT_I);
    save = port_in(__svgalib_CRT_D);
    port_out(save & 0x7F, __svgalib_CRT_D);

    /* write extended CRT registers */
    for (i = 0; i < 6; i++) {
	port_out(0x30 + i, __svgalib_CRT_I);
	port_out(regs[EXT + i], __svgalib_CRT_D);
    }
    port_out(0x36, __svgalib_CRT_I);
    port_out((port_in(__svgalib_CRT_D) & 0x40) | (regs[EXT + 6] & 0xbf), __svgalib_CRT_D);
    port_out(0x37, __svgalib_CRT_I);
    port_out((port_in(__svgalib_CRT_D) & 0xbb) | (regs[EXT + 7] & 0x44), __svgalib_CRT_D);
    port_out(0x3f, __svgalib_CRT_I);
    port_out(regs[EXT + 8], __svgalib_CRT_D);

    /* set original CRTC 0x11 */
    port_out(0x11, __svgalib_CRT_I);
    port_out(save, __svgalib_CRT_D);

    /* write extended attribute register */
    port_in(__svgalib_IS1_R);		/* reset flip flop */
    port_out(0x16, ATT_IW);
    port_out(regs[EXT + 12], ATT_IW);

    if (et4000_extdac) {
	_ramdac_dactocomm();
	outb(PEL_MSK, regs[EXT + 13] | 0x10);
	_ramdac_dactocomm();
	inb(PEL_MSK);
	outb(PEL_MSK, 3);		/* write index low */
	outb(PEL_MSK, 0);		/* write index high */
	outb(PEL_MSK, regs[EXT + 14]);	/* primary ext. pixel select */
	outb(PEL_MSK, regs[EXT + 15]);	/* secondary ext. pixel select */
	outb(PEL_MSK, regs[EXT + 16]);	/* PLL control register */
	_ramdac_dactocomm();
	outb(PEL_MSK, regs[EXT + 13]);
    }
}


/* Return non-zero if mode is available */

static int et4000_modeavailable(int mode)
{
    const unsigned char *regs;
    struct info *info;

    regs = LOOKUPMODE(et4000_modes, mode);
    if (regs == NULL || mode == GPLANE16)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE)
	return 0;

    info = &__svgalib_infotable[mode];
    if (et4000_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    switch (info->colors) {
    case 1 << 15:
	/* All HiColor dacs handle 15 bit */
	if ((et4000_dac & 1) == 0)
	    return 0;
	break;
    case 1 << 16:
	/* Unfortunately, we can't tell the difference between a Sierra Mark 2 */
	/* and a Mark 3, and the mark 2 is only 15 bit. If this gives you trouble */
	/* change it to (8|16) rather than (2|8|16) */
	/* Currently allow Sierra Mark2/3, AT&T, STG-170x and AcuMos */
	if ((et4000_dac & (2 | 8 | 16 | 32 | 64)) == 0)
	    return 0;
	break;
    case 1 << 24:
	/* Don't know how to set dac command register for Diamond SS2410 Dac */
	/* Only allow AT&T, STG-170x and AcuMos */
	if ((et4000_dac & (8 | 16 | 32 | 64)) == 0)
	    return 0;
	break;
    }
    return SVGADRV;
}


/* Check if mode is interlaced */

static int et4000_interlaced(int mode)
{
    const unsigned char *regs;

    if (et4000_modeavailable(mode) != SVGADRV)
	return 0;
    regs = LOOKUPMODE(et4000_modes, mode);
    if (regs == NULL || regs == DISABLE_MODE)
	return 0;
    return (regs[EXT + 5] & 0x80) != 0;		/* CRTC 35H */
}


/* Set a mode */

static int et4000_setmode(int mode, int prv_mode)
{
    const unsigned char *regs;
    unsigned char i;

    if (et4000_dac)
	/* Standard dac behaviour */
	__svgalib_hicolor(et4000_dac, STD_DAC);
    switch (et4000_modeavailable(mode)) {
    case STDVGADRV:
	/* Reset extended register that is set to non-VGA */
	/* compatible value for 132-column textmodes (at */
	/* least on some cards). */
	et4000_unlock();
	outb(__svgalib_CRT_I, 0x34);
	i = inb(__svgalib_CRT_D);
	if ((i & 0x02) == 0x02)
	    outb(__svgalib_CRT_D, (i & 0xfe));
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    case SVGADRV:
	regs = LOOKUPMODE(et4000_modes, mode);
	if (regs != NULL)
	    break;
    default:
	return 1;		/* mode not available */
    }

    if (et4000_dac && !et4000_extdac)
	switch (vga_getmodeinfo(mode)->colors) {
	case 1 << 15:
	    __svgalib_hicolor(et4000_dac, HI15_DAC);
	    break;
	case 1 << 16:
	    __svgalib_hicolor(et4000_dac, HI16_DAC);
	    break;
	case 1 << 24:
	    __svgalib_hicolor(et4000_dac, TC24_DAC);
	    break;
	}
    __svgalib_setregs(regs);
    et4000_setregs(regs, mode);
    return 0;
}

/* Unlock chipset-specific registers */

static void et4000_unlock(void)
{
    /* get access to extended registers */
    port_out(3, 0x3bf);
    if (port_in(0x3cc) & 1)
	port_out(0xa0, 0x3d8);
    else
	port_out(0xa0, 0x3b8);
}


/* Relock chipset-specific registers */

static void et4000_lock(void)
{
}

/* Enable linear mode at a particular 4M page (0 to turn off) */

static void et4000_setlinear(int addr)
{
    et4000_unlock();
    outb(0x3D4, 0x36);
    if (addr)
	outb(0x3D5, inb(0x3D5) | 16);	/* enable linear mode */
    else
	outb(0x3D5, inb(0x3D5) & ~16);	/* disable linear mode */
    outb(0x3D4, 0x30);
    outb(0x3D5, addr);
    et4000_lock();
}


/* Indentify chipset; return non-zero if detected */

static int et4000_test(void)
{
    unsigned char new, old, val;
    int base;

    et4000_unlock();

    /* test for Tseng clues */
    old = port_in(0x3cd);
    port_out(0x55, 0x3cd);
    new = port_in(0x3cd);
    port_out(old, 0x3cd);

    /* return false if not Tseng */
    if (new != 0x55)
	return 0;

    /* test for ET4000 clues */
    if (port_in(0x3cc) & 1)
	base = 0x3d4;
    else
	base = 0x3b4;
    port_out(0x33, base);
    old = port_in(base + 1);
    new = old ^ 0xf;
    port_out(new, base + 1);
    val = port_in(base + 1);
    port_out(old, base + 1);

    /* return true if ET4000 */
    if (val == new) {
	et4000_init(0, 0, 0);
	return 1;
    }
    return 0;
}



static unsigned char last_page = 0;

/* Bank switching function - set 64K bank number */
static void et4000_setpage(int page)
{
    /* Set both read and write bank. */
    port_out(last_page = (page | ((page & 15) << 4)), SEG_SELECT);
    if (et4000_chiptype >= CHIP_ET4000W32i) {
	/* Write page4-5 to bits 0-1 of ext. bank register, */
	/* and to bits 4-5. */
	outb(0x3cb, (inb(0x3cb) & ~0x33) | (page >> 4) | (page & 0x30));
    }
}


/* Bank switching function - set 64K read bank number */
static void et4000_setrdpage(int page)
{
    last_page &= 0x0F;
    last_page |= (page << 4);
    port_out(last_page, SEG_SELECT);
    if (et4000_chiptype >= CHIP_ET4000W32i) {
	/* Write page4-5 to bits 4-5 of ext. bank register. */
	outb(0x3cb, (inb(0x3cb) & ~0x30) | (page & 0x30));
    }
}

/* Bank switching function - set 64K write bank number */
static void et4000_setwrpage(int page)
{
    last_page &= 0xF0;
    last_page |= page;
    port_out(last_page, SEG_SELECT);
    if (et4000_chiptype >= CHIP_ET4000W32i) {
	/* Write page4-5 to bits 0-1 of ext. bank register. */
	outb(0x3cb, (inb(0x3cb) & ~0x03) | (page >> 4));
    }
}


/* Set display start address (not for 16 color modes) */
/* ET4000 supports any address in video memory (up to 1Mb) */
/* ET4000/W32i/p supports up to 4Mb */

static void et4000_setdisplaystart(int address)
{
    outw(0x3d4, 0x0d + ((address >> 2) & 0x00ff) * 256);	/* sa2-sa9 */
    outw(0x3d4, 0x0c + ((address >> 2) & 0xff00));	/* sa10-sa17 */
    inb(0x3da);			/* set ATC to addressing mode */
    outb(0x3c0, 0x13 + 0x20);	/* select ATC reg 0x13 */
    outb(0x3c0, (inb(0x3c1) & 0xf0) | ((address & 3) << 1));
    /* write sa0-1 to bits 1-2 */
    outb(0x3d4, 0x33);
    if (et4000_chiptype >= CHIP_ET4000W32i)
	/* write sa18-21 to bit 0-3 */
	outb(0x3d5, (inb(0x3d5) & 0xf0) | ((address & 0x3c0000) >> 18));
    else
	/* write sa18-19 to bit 0-3 */
	outb(0x3d5, (inb(0x3d5) & 0xfc) | ((address & 0xc0000) >> 18));
}


/* Set logical scanline length (usually multiple of 8) */
/* ET4000 supports multiples of 8 to 4088 */

static void et4000_setlogicalwidth(int width)
{
    outw(0x3d4, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
    outb(0x3d4, 0x3f);
    outb(0x3d5, (inb(0x3d5) & 0x7f)
	 | ((width & 0x800) >> 5));	/* write lw12 to bit 7 */
}

static int et4000_ext_set(unsigned what, va_list params)
{
    int param2, old_values;

    switch (what) {
    case VGA_EXT_AVAILABLE:
	param2 = va_arg(params, int);
	switch (param2) {
	case VGA_AVAIL_SET:
	    return VGA_EXT_AVAILABLE | VGA_EXT_SET | VGA_EXT_CLEAR | VGA_EXT_RESET;
	case VGA_AVAIL_ACCEL:
	    return 0;
	case VGA_AVAIL_FLAGS:
	    return pos_ext_settings;
	}
	return 0;
    case VGA_EXT_SET:
	old_values = ext_settings;
	ext_settings |= (va_arg(params, int)) & pos_ext_settings;
      params_changed:
	if (((old_values ^ ext_settings) & pos_ext_settings)) {
	    int cmd;
	    CRITICAL = 1;
	    vga_lockvc();
	    _ramdac_dactocomm();
	    cmd = inb(PEL_MSK);
	    _ramdac_dactocomm();
	    if (ext_settings & VGA_CLUT8)
		cmd |= 2;
	    else
		cmd &= ~2;
	    outb(PEL_MSK, cmd);
	    vga_unlockvc();
	    CRITICAL = 0;
	}
	return old_values;
    case VGA_EXT_CLEAR:
	old_values = ext_settings;
	ext_settings &= ~((va_arg(params, int)) & pos_ext_settings);
	goto params_changed;
    case VGA_EXT_RESET:
	old_values = ext_settings;
	ext_settings = (va_arg(params, int)) & pos_ext_settings;
	goto params_changed;
    default:
	return 0;
    }
}

static int et4000_linear(int op, int param)
{
    /* linear mode not supported on original chipset */
    if (et4000_chiptype == CHIP_ET4000)
	return -1;
    else if (op == LINEAR_QUERY_GRANULARITY)
	return 4 * 1024 * 1024;
    else if (op == LINEAR_QUERY_RANGE)
	return 256;
    else if (op == LINEAR_ENABLE) {
	et4000_setlinear(param / (4 * 1024 * 1024));
	return 0;
    } else if (op == LINEAR_DISABLE) {
	et4000_setlinear(0);
	return 0;
    } else
	return -1;
}

/* Function table (exported) */

DriverSpecs __svgalib_et4000_driverspecs =
{
    et4000_saveregs,
    et4000_setregs,
    et4000_unlock,
    et4000_lock,
    et4000_test,
    et4000_init,
    et4000_setpage,
    et4000_setrdpage,
    et4000_setwrpage,
    et4000_setmode,
    et4000_modeavailable,
    et4000_setdisplaystart,
    et4000_setlogicalwidth,
    et4000_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    et4000_ext_set,
    0,
    et4000_linear,
    NULL,                       /* Accelspecs */
    NULL,                       /* Emulation */
};


/* Hicolor DAC detection derived from vgadoc (looks tricky) */


#ifndef DAC_TYPE
static int testdac(void)
{
    int x, y, z, v, oldcommreg, oldpelreg, retval;

    /* Use the following return values:
     *      0: Ordinary DAC
     *      1: Sierra SC11486 (32k)
     *      3: Sierra 32k/64k
     *      5: SS2410 15/24bit DAC
     *      9: AT&T 20c491/2
     *     10: STG-170x
     */

    retval = 0;			/* default to no fancy dac */
    _ramdac_dactopel();
    x = inb(PEL_MSK);
    do {			/* wait for repeat of value */
	y = x;
	x = inb(PEL_MSK);
    } while (x != y);
    z = x;
    inb(PEL_IW);
    inb(PEL_MSK);
    inb(PEL_MSK);
    inb(PEL_MSK);
    x = inb(PEL_MSK);
    y = 8;
    while ((x != 0x8e) && (y > 0)) {
	x = inb(PEL_MSK);
	y--;
    }
    if (x == 0x8e) {
	/* We have an SS2410 */
	retval = 1 | 4;
	_ramdac_dactopel();
    } else {
	_ramdac_dactocomm();
	oldcommreg = inb(PEL_MSK);
	_ramdac_dactopel();
	oldpelreg = inb(PEL_MSK);
	x = oldcommreg ^ 0xFF;
	outb(PEL_MSK, x);
	_ramdac_dactocomm();
	v = inb(PEL_MSK);
	if (v != x) {
	    v = _ramdac_setcomm(x = oldcommreg ^ 0x60);
	    /* We have a Sierra SC11486 */
	    retval = 1;

	    if ((x & 0xe0) == (v & 0xe0)) {
		/* We have a Sierra 32k/64k */
		x = inb(PEL_MSK);
		_ramdac_dactopel();
		retval = 1 | 2;

		if (x == inb(PEL_MSK)) {
		    /* We have an ATT 20c491 or 20c492 */
		    retval = 1 | 8;
		    if (_ramdac_setcomm(0xFF) != 0xFF) {
			/* We have a weird dac, AcuMos ADAC1 */
			retval = 1 | 16;
		    } else {	/* maybe an STG-170x */
			/* try to enable ext. registers */
			_ramdac_setcomm(oldcommreg | 0x10);
			outb(PEL_MSK, 0);
			outb(PEL_MSK, 0);
			if (inb(PEL_MSK) == 0x44) {
			    /* it's a 170x */
			    /* Another read from PEL_MSK gets the chiptype, 0x02 == 1702. */
			    retval = 1 | 64;
			    et4000_extdac = 1;
			}
		    }
		}
	    }
	    _ramdac_dactocomm();
	    outb(PEL_MSK, oldcommreg);
	}
	_ramdac_dactopel();
	outb(PEL_MSK, oldpelreg);
    }
    return retval;
}
#endif				/* !defined(DAC_TYPE) */


/* Initialize chipset (called after detection) */

static int et4000_init(int force, int par1, int par2)
{
    int value;
    int old, new, val;
#ifdef USE_CLOCKS
    int i;
#endif

#ifdef DYNAMIC
    if (et4000_modes == NULL) {
	FILE *regs;

	regs = fopen(ET4000_REGS, "r");
	if (regs != 0) {
	    et4000_modes = NULL;
	    __svgalib_readmodes(regs, &et4000_modes, &et4000_dac, &clocks[0]);
	    fclose(regs);
	} else
	    et4000_modes = &No_Modes;
    }
#endif

    /* Determine the ET4000 chip type. */

    /* Test for ET4000/W32. */
    old = inb(0x3cb);
    outb(0x3cb, 0x33);
    new = inb(0x3cb);
    outb(0x3cb, old);
    if (new != 0x33)
	et4000_chiptype = CHIP_ET4000;
    else {
	/* ET4000/W32. */
	if (getenv("IOPERM") == NULL && iopl(3) < 0)
	    /* Can't get further I/O permissions -- */
	    /* assume ET4000/W32. */
	    et4000_chiptype = CHIP_ET4000W32;
	else {
	    outb(0x217a, 0xec);
	    val = inb(0x217b) >> 4;
	    switch (val) {
	    case 0:
		et4000_chiptype = CHIP_ET4000W32;
		break;
	    case 1:
	    case 3:
		et4000_chiptype = CHIP_ET4000W32i;
		break;
	    case 2:
	    case 5:
	    default:
		et4000_chiptype = CHIP_ET4000W32p;
	    }
	    /* Do NOT set back iopl if set before, since */
	    /* caller most likely still needs it! */
	    if (getenv("IOPERM") == NULL)
		iopl(0);
	}
    }

    if (force)
	et4000_memory = par1;
    else {
	outb(0x3d4, 0x37);
	value = inb(0x3d5);
	et4000_memory = (value & 0x08) ? 256 : 64;
	switch (value & 3) {
	case 2:
	    et4000_memory *= 2;
	    break;
	case 3:
	    et4000_memory *= 4;
	    break;
	}
	if (value & 0x80)
	    et4000_memory *= 2;
	outb(0x3d4, 0x32);
	if (inb(0x3d5) & 0x80) {
	    /* Interleaved memory on ET4000/W32i/p: */
	    /* Multiply by 2. */
	    et4000_memory *= 2;
	}
    }
#ifndef DAC_TYPE
    if (et4000_dac < 0)
	et4000_dac = testdac();
#endif

#ifdef USE_CLOCKS
    /* Measure clock frequencies */
    for (i = 0; i < CLOCKS; ++i)
	get_clock(i);
#endif

    if (__svgalib_driver_report) {
	char dacname[60];
	switch (et4000_dac & ~1) {
	case 0:
	    strcpy(dacname, ", Hicolor Dac: Sierra SC11486");
	    break;
	case 2:
	    strcpy(dacname, ", Hicolor Dac: Sierra 32k/64k");
	    break;
	case 4:
	    strcpy(dacname, ", Hicolor Dac: SS2410");
	    break;
	case 8:
	    strcpy(dacname, ", Hicolor Dac: ATT20c491/2");
	    break;
	case 16:
	    strcpy(dacname, ", Hicolor Dac: ACUMOS ADAC1");
	    break;
	case 32:
	    strcpy(dacname, ", Hicolor Dac: Sierra 15025/6 (24-bit)");
	    break;
	case 64:
	    strcpy(dacname, ", Hicolor Dac: STG-170x (24-bit)");
	    break;
	default:
	    strcpy(dacname, ", Hicolor Dac: Unknown");
	    break;
	}

	printf("Using Tseng ET4000 driver (%s %d%s).",
	       chipname[et4000_chiptype], et4000_memory,
	       et4000_dac & 1 ? dacname : "");
#ifdef USE_CLOCKS
	printf(" Clocks:");
	for (i = 0; i < 8; ++i)
	    printf(" %d", (clocks[i] + 500) / 1000);
#endif
	printf("\n");
    }
    __svgalib_driverspecs = &__svgalib_et4000_driverspecs;

    pos_ext_settings = 0;
    if (et4000_dac & (8 | 16 | 32 | 64))
	pos_ext_settings = VGA_CLUT8;

    return 0;
}
