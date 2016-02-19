/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* ATI Mach32 driver (C) 1995 Michael Weller                       */
/* eowmob@exp-math.uni-essen.de mat42b@aixrs1.hrz.uni-essen.de     */
/* eowmob@pollux.exp-math.uni-essen.de                             */

/*
 * MICHAEL WELLER DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MICHAEL WELLER BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* This tool contains one routine out of Xfree86, therefore I repeat    */
/* its copyright here: (Actually it is longer than the copied code)     */

/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL, KEVIN E. MARTIN, AND RICKARD E. FAITH DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Rewritten for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 *
 */

/* Works only if Mach32 onboard VGA is enabled.                    */
/* Reads in eeprom.                                                */
/* There is a dirty hack in here to raise the linewidth for        */
/* 800x600 to 832 to keep my mem happy.. (and even though it's a   */
/* VRAM card... probably has to do something with mempages....     */
/* I change it by tweaking the info array.. so watch out.          */
/* The Number of additional pixels to append is set below, it has  */
/* to be a multiple of 8. This seems only to be needed for 16/24bpp */
/* Ok,later I found that the number of pixels has to be a multiple */
/* of 64 at least... somewhere in the  ATI docs, always choosing a */
/* multiple of 128 was suggested....                               */
/* So set the multiple to be used below..                          */

#define PIXALIGN 64

#define DAC_SAFETY 0x1d		/*reminder for people with DAC!=4,3,2,0 */
			/*set bits for well known DACTYPES */

/*
 * Sync allowance in 1/1000ths. 10 (1%) corresponds to a 315 Hz
 * deviation at 31.5 kHz, 1 Hz at 100 Hz
 */
#define SYNC_ALLOWANCE 10

#define SUPP_32BPP		/*Accept 32BPP modes */
/*#define USE_RGBa *//* ifdef(SUPP_32BPP) use RGBa format (R first in memory),
		      otherwise aBGR is used */
/*Pure experimental and probably specific to my card (VRAM 68800-3) */
#define MAXCLK8  2000		/*This effectly switches this off.. seems not
				   to be needed with PIXTWEAK>32 */
#define MAXCLK16 2000		/*This effectly switches this off.. seems not
				   to be needed with PIXTWEAK>32 */
#define MAXCLK24 49
#define MAXCLK32 39

/*Clock values to be set on original ATI Mach32 to emulate vga compatible timings */
/*Bit 4 is ATI3e:4, bits 3-2 are ATI39:1-0, bits 1-0 are ATI38:7-6 */
#define SVGA_CLOCK 0x09

/*And here are minimum Vfifo values.. just guessed values (valid settings are 0..15): */
#define VFIFO8	6
#define VFIFO16	9
#define VFIFO24	14
#define VFIFO32	14

/*Wait count for busywait loops */
#define BUSYWAIT 10000000	/* Around 1-2 sec on my 486-50 this should be enough for
				   any graphics command to complete, even on a fast machine.
				   however after that slower code is called to ensure a minimum
				   wait of another: */
#define ADDIWAIT 5		/* seconds. */

/*#define FAST_MEMCPY *//* Use a very fast inline memcpy instead of the standard libc one */
			/* Seems not to make a big difference for the Mach32 */
			/* Even more seems to be MUCH slower than the libc memcpy, even */
			/* though this uses a function call. Probably just rep movsb is */
			/* fastest. So => do not use this feature. */

#define MODESWITCHDELAY	50000	/* used to wait for clocks to stabilize */

/*#define DEBUG */
/*#define DEBUG_KEY *//*Wait for keypress at some locations */
/*#define EXPERIMENTAL *//*Experimental flags */
/*#define FAKEBGR *//* Use BGR instead of RGB, this is for debugging only. Leave it alone! */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#ifdef OSKIT
#include <oskit/time.h>
#include <oskit/dev/clock.h>
#include <oskit/com/services.h>
#include "osenv.h"
#else
#include <time.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>
#ifdef USEGLIBC
#include <sys/io.h>
#endif

#include "mach32.h"
#include "8514a.h"
#include "vga.h"
#include "libvga.h"
#include "driver.h"

#define OFF_ALLOWANCE(a) ((a) * (1.0f - (SYNC_ALLOWANCE/1000.0f)))
#define ADD_ALLOWANCE(a) ((a) * (1.0f + (SYNC_ALLOWANCE/1000.0f)))

/*List preallocate(for internal info table mixup commands): */
#define PREALLOC 16

/*Internal mixup commands: */
#define CMD_MSK 0xf000
#define CMD_ADD 0x0000
#define CMD_DEL 0x1000
#define CMD_MOD 0x2000
#define CMD_CPY 0x3000
#define CMD_MOV 0x4000

#define ATIPORT		0x1ce
#define ATIOFF		0x80
#define ATISEL(reg)	(ATIOFF+reg)

/* Ports we use: */
/* moved to 8514a.h - Stephen Lee */
#define VGA_DAC_MASK	0x3C6
#define DISP_STATUS	0x2E8

#define DAC_W_INDEX	0x02EC
#define DAC_DATA	0x02ED
#define DAC_MASK	0x02EA
#define DAC_R_INDEX	0x02EB

#define DAC0		0x02EC
#define DAC1		0x02ED
#define DAC2		0x02EA
#define DAC3		0x02EB

/* Bit masks: */
#define GE_BUSY		0x0200
#define ONLY_8514	1
#define LOCAL_BUS_CONF2	(1<<6)
#define BUS_TYPE	0x000E
#define PCI		0x000E
#define ISA		0x0000
#define Z4GBYTE		(1<<13)

/* ATI-EXT regs to save (ensure lock regs are set latest, that is they are listed first here!): */
static const unsigned char mach32_eregs[] =
{
	/* Lock regs: */
    0x38, 0x34, 0x2e, 0x2b,
	/* All other extended regs. */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x20, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2c,
    0x2d, 0x30, 0x31, 0x32, 0x33, 0x35, 0x36,
    0x37, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f};

/* Extended ATI VGA regs that have to be reset for plain vga-modes */
/* First num is ATI reg, 2nd has bits set where to clear bits in the */
/* ATI reg. Note: the most important clocksel regs 0x38 & 0x39 are */
/* handled manually */
static const unsigned char mach32_ati_ext[] =
{
    0x05, 0x80,			/* VGA std blink rate */
    0x23, 0x18,			/* clear ext CRTC bits */
    0x26, 0xc1,			/* no skew of disp enab, dashedunderl */
    0x27, 0xf1,
    0x2c, 0x41,
    0x2d, 0xff,			/* several ext bits */
    0x2e, 0xef,			/* ext page pointers */
    0x30, 0x65,
    0x31, 0x7b,
    0x32, 0xff,
    0x33, 0xc0,
    0x34, 0x03,
    0x35, 0xf7,
    0x36, 0xf7,
    0x3d, 0x0d};


/* Mach32 regs to save.. read, write address alternating: */
static const unsigned short mach32_acc_regs[] =
{
    0xB2EE, 0x06E8,		/* H_DISP(ALT H_TOTAL) */
    0xC2EE, 0x12E8,		/* V_TOTAL */
    0xC6EE, 0x16E8,		/* V_DISP */
    0xCAEE, 0x1AE8,		/* V_SYNC_STRT */
    0xD2EE, 0x1EE8,		/* V_SYNC_WID */
    0x4AEE, 0x4AEE,		/* CLOCK_SEL */
    0x96EE, 0x96EE,		/* BRES_COUNT */
    0x86E8, 0x86E8,		/* CUR_X */
    0x82E8, 0x82E8,		/* CUR_Y */
    0x22EE, 0x22EE,		/* DAC_CONT(PCI) */
    0xF2EE, 0xF2EE,		/* DEST_COLOR_CMP_MASK */
    0x92E8, 0x92E8,		/* ERR_TERM */
    0xA2EE, 0xA2EE,		/* LINEDRAW_OPT */
    0x32EE, 0x32EE,		/* LOCAL_CNTL */
    0x6AEE, 0x6AEE,		/* MAX_WAITSTATES / MISC_CONT(PCI) */
    0x36EE, 0x36EE,		/* MISC_OPTIONS */
    0x82EE, 0x82EE,		/* PATT_DATA_INDEX */
    0x8EEE, 0x7AEE,		/* EXT_GE_CONFIG */
    0xB6EE, 0x0AE8,		/* H_SYNC_STRT */
    0xBAEE, 0x0EE8,		/* H_SYNC_WID */
    0x92EE, 0x7EEE,		/* MISC_CNTL */
    0xDAEE, 0x8EE8,		/* SRC_X */
    0xDEEE, 0x8AE8,		/* SRC_Y */
    0x52EE, 0x52EE,		/* SCRATCH0 */
    0x56EE, 0x56EE,		/* SCRATCH1 */
    0x42EE, 0x42EE,		/* MEM_BNDRY */
    0x5EEE, 0x5EEE,		/* MEM_CFG */
};

/* Some internal flags */
#define PAGE_UNKNOWN 0
#define PAGE_DIFF    1
#define PAGE_BOTH    2

#define DAC_MODE8    0
#define DAC_MODEMUX  1
#define DAC_MODE555  2
#define DAC_MODE565  3
#define DAC_MODERGB  4
#define DAC_MODE32B  5
#define DAC_SEMICLK  0x80

#define EMU_POSS     1
#define EMU_OVER     2

#define EEPROM_USE_CHKSUM	1
#define EEPROM_USE_MEMCFG	2
#define EEPROM_USE_TIMING	4
#define EEPROM_UPDATE		8

/*DAC_MODETABLES:   */
static const unsigned char mach32_dac1[4] =
{0x00, 0x00, 0xa2, 0xc2};

static const unsigned char mach32_dac4[5] =
{0x00, 0x00, 0xa8, 0xe8, 0xf8};

static const unsigned char mach32_dac5[6] =
{0x00, 0x00, 0x20, 0x21, 0x40,
#ifdef USE_RGBa
 0x61
#else
 0x60
#endif
};

typedef struct {
    unsigned char vfifo16, vfifo24;
    unsigned char h_disp, h_total, h_sync_wid, h_sync_strt;
    unsigned short v_total, v_disp, v_sync_strt;
    unsigned char disp_cntl, v_sync_wid;
    unsigned short clock_sel, flags, mask, offset;
} mode_entry;

typedef enum {
    R_UNKNOWN, R_STANDARD, R_EXTENDED
} accelstate;

/*sizeof mode_entry in shorts: */
#define SOMOD_SH ((sizeof(mode_entry)+sizeof(short)-1)/sizeof(short))

/*I put the appropriate VFIFO values from my selfdefined modes in... raise them if you get
   strange screen flickering.. (but they have to be <=0xf !!!!) */
static const mode_entry predef_modes[] =
{
    {0x9, 0xe, 0x4f, 0x63, 0x2c, 0x52, 0x418, 0x3bf, 0x3d6, 0x23, 0x22, 0x50, 0, 0x0000, 0},
    {0x9, 0xe, 0x4f, 0x69, 0x25, 0x52, 0x40b, 0x3bf, 0x3d0, 0x23, 0x23, 0x24, 0, 0x0001, 7},
    {0x9, 0xe, 0x63, 0x84, 0x10, 0x6e, 0x580, 0x4ab, 0x4c2, 0x33, 0x2c, 0x7c, 0, 0x003f, 8},
    {0x9, 0xe, 0x63, 0x84, 0x10, 0x6d, 0x580, 0x4ab, 0x4c2, 0x33, 0x0c, 0x0c, 0, 0x003d, 8},
    {0x9, 0xe, 0x63, 0x7f, 0x09, 0x66, 0x4e0, 0x4ab, 0x4b0, 0x23, 0x02, 0x0c, 0, 0x003c, 8},
    {0x9, 0xe, 0x63, 0x83, 0x10, 0x68, 0x4e3, 0x4ab, 0x4b3, 0x23, 0x04, 0x30, 0, 0x0038, 8},
    {0x9, 0xe, 0x63, 0x7d, 0x12, 0x64, 0x4f3, 0x4ab, 0x4c0, 0x23, 0x2c, 0x1c, 0, 0x0030, 8},
    {0x9, 0xe, 0x63, 0x82, 0x0f, 0x6a, 0x531, 0x4ab, 0x4f8, 0x23, 0x06, 0x10, 0, 0x0020, 8},
    {0xd, 0xe, 0x7f, 0x9d, 0x16, 0x81, 0x668, 0x5ff, 0x600, 0x33, 0x08, 0x1c, 0, 0x0001, 9},
    {0xd, 0xe, 0x7f, 0xa7, 0x31, 0x82, 0x649, 0x5ff, 0x602, 0x23, 0x26, 0x3c, 0, 0x0003, 9},
    {0xd, 0xe, 0x7f, 0xad, 0x16, 0x85, 0x65b, 0x5ff, 0x60b, 0x23, 0x04, 0x38, 0, 0x0013, 9},
    {0xd, 0xe, 0x7f, 0xa5, 0x31, 0x83, 0x649, 0x5ff, 0x602, 0x23, 0x26, 0x38, 0, 0x0017, 9},
    {0xd, 0xe, 0x7f, 0xa0, 0x31, 0x82, 0x649, 0x5ff, 0x602, 0x23, 0x26, 0x38, 0, 0x001f, 9},
    {0xe, 0xe, 0x9f, 0xc7, 0x0a, 0xa9, 0x8f8, 0x7ff, 0x861, 0x33, 0x0a, 0x2c, 0, 0x0001, 10},
    {0xe, 0xe, 0x9f, 0xc7, 0x0a, 0xa9, 0x838, 0x7ff, 0x811, 0x33, 0x0a, 0x2c, 0, 0x0003, 10},
};

#define NUM_MODES (sizeof(predef_modes)/sizeof(mode_entry))

static const mode_entry **mach32_modes = NULL;

static int mach32_clocks[32] =
{				/* init to zero for safety */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

static signed char mach32_clock_by2[32], mach32_clock_by3[32];

/*Bitmask is 1 256-col, 2 15-16 bpp, 4 24 bpp, 8 32bpp */
static unsigned char mach32_mmask[2][8] =
{
    { /* General supported modes.. */ 3, 3, 0xf, 1, 0x7, 0xf, 1, 1},
    { /* Modes above 80Mhz..      */ 0, 0, 1, 0, 0, 0xf, 0, 0}};

/* Modes that ATI mentions for it's DACS.. this is for safety and the internal
   predefined modes.. I think the above restrictions should be enough.. However
   I don't want to risk the life of your DAC so.. */
static unsigned char dacmo640[] =
{0x3, 0x3, 0xf, 0x1, 0x7, 0xf, 0x1, 0x1};
static unsigned char dacmo800[] =
{0x3, 0x3, 0xf, 0x1, 0x3, 0xf, 0x1, 0x1};
static unsigned char dacmo1024[] =
{0x3, 0x1, 0x3, 0x1, 0x1, 0xf, 0x1, 0x1};
static unsigned char dacmo1280[] =
{0x0, 0x0, 0x1, 0x0, 0x0, 0xf, 0x0, 0x0};

/* several globals.. mostly to record current state of the driver */
static char vfifo8 = VFIFO8, vfifo16 = VFIFO16, vfifo24 = VFIFO24, vfifo32 = VFIFO32,
 mach32_apsiz = 0;
static char dac_override = 127, svga_clock = SVGA_CLOCK, mach32_ast = 0;
static char *eeprom_fname = NULL;
static char emuimage = EMU_POSS | EMU_OVER;	/* Never set EMU_OVER w/o EMU_POSS !!! */
static char eeprom_option = EEPROM_USE_CHKSUM | EEPROM_USE_MEMCFG | EEPROM_USE_TIMING;

static unsigned short *mach32_eeprom, mach32_disp_shadow = 0, mach32_strictness = 0,
*mach32_modemixup;
static unsigned short mach32_apadd = 1, mach32_memcfg;

static unsigned short mach32_ge_pitch = 0, mach32_ge_off_h = 0, mach32_ge_off_l = 0;
static unsigned short mach32_bkgd_alu_fn = 7, mach32_frgd_alu_fn = 7;
static unsigned short mach32_frgd_col = 0xFFFF, mach32_bkgd_col = 0;
static unsigned short mach32_polyfill = 0, mach32_rop = ROP_COPY;

static int mach32_memory, mach32_dac, mach32_pagemode = PAGE_UNKNOWN,
 mach32_dacmode = DAC_MODE8;
static int mach32_chiptype, mixup_alloc = 0, mixup_ptr = 0, clocks_set = 0,
 ext_settings = 0;
static int mach32_maxclk8 = MAXCLK8, mach32_maxclk16 = MAXCLK16, mach32_maxclk24 = MAXCLK24;
static int mach32_maxclk32 = MAXCLK32, latchopt = ~0, bladj = -1;
static int pos_ext_settings = VGA_CLUT8;
static int acc_supp = HAVE_BLITWAIT | HAVE_FILLBLIT | HAVE_IMAGEBLIT | HAVE_BITBLIT | HAVE_HLINELISTBLIT;
static int accel_supp = ACCELFLAG_SETFGCOLOR | ACCELFLAG_SETBGCOLOR |
		    ACCELFLAG_SETTRANSPARENCY | ACCELFLAG_SETRASTEROP |
		    ACCELFLAG_FILLBOX | ACCELFLAG_SCREENCOPY |
		    ACCELFLAG_DRAWLINE | ACCELFLAG_PUTIMAGE |
		    ACCELFLAG_DRAWHLINELIST | ACCELFLAG_PUTBITMAP |
		    ACCELFLAG_SCREENCOPYMONO | ACCELFLAG_SETMODE |
		    ACCELFLAG_POLYLINE | ACCELFLAG_POLYHLINE |
		    ACCELFLAG_POLYFILLMODE | ACCELFLAG_SYNC;

static int palcurind = -1;
static int palsetget = -1;

static accelstate mach32_accelstate = R_UNKNOWN;

#ifdef DEBUG
static char verbose = 1;
#else
static char verbose = 0;
#endif
#ifdef EXPERIMENTAL
static unsigned mach32_experimental = 0;
#endif


static volatile char *mach32_aperture = NULL;
static volatile int dummy;

static signed char mach32_search_clk(int clk);
static void mach32_setstate(accelstate which);
static void mach32_blankadj(int adj);
static void mach32_modfill(const mode_entry * mode, int modemask, int forcein);
static int mach32_test(void);
static int mach32_init(int force, int chiptype, int memory);
static void mach32_i_bltwait(void);
static void mach32_bltwait(void);
static void mach32_wait(void);
static void mach32_experm(void);
static int mach32_eeclock(register int ati33);
static void mach32_eekeyout(register int ati33, register int offset, register int mask);
static int mach32_eeget(int offset);
static int mach32_saveregs(unsigned char regs[]);
static void mach32_setregs(const unsigned char regs[], int mode);
static void mach32_unlock(void);
static void mach32_lock(void);
static int mach32_sav_dac(int offset, unsigned char regs[]);
static int mach32_set_dac(int dac_mode, int clock_intended, int xres);
static void mach32_setpage(int page);
static void mach32_setrdpage(int page);
static void mach32_setwrpage(int page);
static void mach32_setappage(int page);
static void mach32_setdisplaystart(int address);
static void mach32_setlogicalwidth(int width);
static int mach32_modeavailable(int mode);
static void mach32_getmodeinfo(int mode, vga_modeinfo * modeinfo);
static int mach32_setmode(int mode, int previous);
static void mach32_bitblt(int srcaddr, int destaddr, int w, int h, int pitch);
static void mach32_fillblt(int destaddr, int w, int h, int pitch, int c);
static void mach32_imageblt(void *srcaddr, int destaddr, int w, int h, int pitch);
static void mach32_memimageblt(void *srcaddr, int destaddr, int w, int h, int pitch);
static void mach32_hlinelistblt(int ymin, int n, int *xmin, int *xmax, int pitch, int c);
static void mach32_readconfig(void);
static void mach32_final_modefixup(void);
static int mach32_ext_set(unsigned what, va_list params);
static int mach32_accel(unsigned operation, va_list params);
static void mach32_ge_reset(void);
static void slow_queue(unsigned short mask);

static void mach32_savepalette(unsigned char *red, unsigned char *green, unsigned char *blue);
static void mach32_restorepalette(const unsigned char *red, const unsigned char *green,
				  const unsigned char *blue);
static int  mach32_setpalette(int index, int red, int green, int blue);
static void mach32_getpalette(int index, int *red, int *green, int *blue);
static int mach32_screenon(void);
static void mach32_waitretrace(void);

DriverSpecs __svgalib_mach32_driverspecs =
{
    mach32_saveregs,		/* saveregs */
    mach32_setregs,		/* setregs */
    mach32_unlock,		/* unlock */
    mach32_lock,		/* lock */
    mach32_test,
    mach32_init,
    mach32_setpage,		/* __svgalib_setpage */
    mach32_setrdpage,		/* __svgalib_setrdpage */
    mach32_setwrpage,		/* __svgalib_setwrpage */
    mach32_setmode,		/* setmode */
    mach32_modeavailable,	/* modeavailable */
    mach32_setdisplaystart,	/* setdisplaystart */
    mach32_setlogicalwidth,	/* setlogicalwidth */
    mach32_getmodeinfo,		/* getmodeinfo */
    mach32_bitblt,		/* bitblt */
    mach32_imageblt,		/* imageblt */
    mach32_fillblt,		/* fillblt */
    mach32_hlinelistblt,	/* hlinelistblt */
    mach32_bltwait,		/* bltwait */
    mach32_ext_set,
    mach32_accel,
    0,				/* linear -- mach32 driver handles it differently */
    NULL,			/* Accelspecs */
    NULL,			/* Emulation */
};

static Emulation mach32_vgaemul = {
    mach32_savepalette,
    mach32_restorepalette,
    mach32_setpalette,
    mach32_getpalette,
    NULL, /* void (*savefont)(void); */
    NULL, /* void (*restorefont)(void); */
    NULL, /* int (*screenoff)(void); */
    mach32_screenon,
    mach32_waitretrace,
};

#ifdef FAST_MEMCPY
/* This memcpy is immediately derived from memset as given in speedtest.c in this
   same svgalib package written by Harm Hanemaayer */

static inline void *
 fast_memcpy(void *dest, void *source, size_t count)
{
    __asm__(
	       "cld\n\t"
	       "cmpl $12,%%edx\n\t"
	       "jl 1f\n\t"	/* if (count >= 12) */

	       "movl %%edx,%%ecx\n\t"
	       "negl %%ecx\n\t"
	       "andl $3,%%ecx\n\t"	/* (-s % 4) */
	       "subl %%ecx,%%edx\n\t"	/* count -= (-s % 4) */
	       "rep ; movsb\n\t"	/* align to longword boundary */

	       "movl %%edx,%%ecx\n\t"
	       "shrl $2,%%ecx\n\t"

    /* Doing any loop unrolling here proved to SLOW!!! down
       the copy on my system, at least it didn't show any speedup. */

	       "rep ; movsl\n\t"	/* copy remaining longwords */

	       "andl $3,%%edx\n"	/* copy last few bytes */
	       "1:\tmovl %%edx,%%ecx\n\t"	/* <= 12 entry point */
  "rep ; movsb\n\t":
  /* no outputs */ :
    /* inputs */
    /* SI= source address */ "S"(source),
    /* DI= destination address */ "D"(dest),
  /* CX= words to transfer */ "d"(count):
    /* eax, edx, esi, edi, ecx destructed: */ "%eax", "%edx", "%esi", "%edi", "%ecx");
    return dest;
}
#else
#define fast_memcpy memcpy
#endif

static inline int max(int a, int b)
{
    return (a > b) ? a : b;
}

static inline void checkqueue(int n)
{				/* This checks for at least n free queue positions */
/*Prepare mask: */
    unsigned short mask = (unsigned short) (0xffff0000 >> n);
    int i = BUSYWAIT;

    while (i--)
	if (!(inw(EXT_FIFO_STATUS) & mask))
	    return;

    slow_queue(mask);
}

#ifdef OSKIT
static void clock_spin(struct oskit_timespec *time)
{
	static struct oskit_clock *myclock;

	/*
	 * Look for a registered system clock.
	 */
	if (!myclock) {
		oskit_lookup_first(&oskit_clock_iid, (void *) &myclock);

		if (!myclock)
			panic("%s%d: no sys_clock defined!\n",
			       __FUNCTION__, __LINE__);

	}

        oskit_clock_spin(myclock, time);
}

static void slow_queue(unsigned short mask)
{
    struct oskit_timespec time;

    if (!(inw(EXT_FIFO_STATUS) & mask)) {
	return;
    }

    time.tv_nsec = 0;
    time.tv_sec = ADDIWAIT;
    clock_spin(&time);

    mach32_ge_reset();		/* Give up */
}
#else
static void slow_queue(unsigned short mask)
{
    clock_t clk_time;

    clk_time = clock();

    do {
	if (!(inw(EXT_FIFO_STATUS) & mask))
	    return;
    }
    while ((clock() - clk_time) < (CLOCKS_PER_SEC * ADDIWAIT));

    mach32_ge_reset();		/* Give up */
}
#endif

static void mach32_ge_reset(void)
{				/* This is intended as a safety bailout if we locked
				   up the card. */
    int queue_stat, ioerr, ge_stat;
    int i = 1000000;
    volatile int dummy;

/* Provide diagnostics: */
    ioerr = inw(SUBSYS_STAT);
    queue_stat = inw(EXT_FIFO_STATUS);
    ge_stat = inw(GE_STAT);


    outw(SUBSYS_CNTL, 0x800f);	/*Reset GE, disable all IRQ, reset all IRQ state bits */
    while (i--)
	dummy++;
    outw(SUBSYS_CNTL, 0x400f);	/*Continue normal operation */
/* Better reconfigure all used registers */
    mach32_accelstate = R_UNKNOWN;
    CRITICAL = 0;		/* Obviously we are idle */
    puts("\asvgalib: mach32: Warning! GE_Engine timed out, draw command\n"
	 "was probably corrupted! If you have a very fast machine (10*Pentium)\n"
	 "raise BUSYWAIT and ADDIWAIT in mach32.c, may also be a driver/card bug,\n"
	 "so report detailed info to me (Michael Weller).\nBUT:\n"
	 "This reaction is normal when svgalib is killed in a very critical operation\n"
	 "by a fatal signal like INT (pressing ^C). In this situation this reset just\n"
	 "guarantees that you can continue working on the console, so in this case\n"
	 "PLEASE don't bloat my mailbox with bug reports. Thx, Michael.");
    printf("POST-Mortem:\n\tSubsys stat: %04x - %sIOerror (is usually a queue overrun)\n"
	   "\tGE stat    : %04x - engine %s, %sdata ready for host.\n\tQueue stat : %04x\n",
	   ioerr, (ioerr & 4) ? "" : "no ", ge_stat,
	   (ge_stat & GE_BUSY) ? "busy" : "idle", (ge_stat & 0x100) ? "" : "no ", queue_stat);
}

static void mach32_setstate(accelstate which)
{				/* Set GE registers to values assumed elsewhere */
    mach32_accelstate = which;

    checkqueue(10);
    /*Effectively switch off hardware clipping: */
    outw(MULTI_FUNC_CNTL, 0x1000 | (0xfff & -1));	/*TOP */
    outw(MULTI_FUNC_CNTL, 0x2000 | (0xfff & -1));	/*LEFT */
    outw(MULTI_FUNC_CNTL, 0x3000 | 1535);	/*BOTTOM */
    outw(MULTI_FUNC_CNTL, 0x4000 | 1535);	/*RIGHT */
    /*Same for ATI EXT commands: */
    outw(EXT_SCISSOR_T, 0xfff & -512);	/*TOP */
    outw(EXT_SCISSOR_L, 0xfff & -512);	/*LEFT */
    outw(EXT_SCISSOR_B, 1535);	/*BOTTOM */
    outw(EXT_SCISSOR_R, 1535);	/*RIGHT */
    outw(MULTI_FUNC_CNTL, 0xa000);
    outw(DP_CONFIG, 0x3291);

    checkqueue(4);
    outw(DEST_CMP_FN, 0);
    if (which == R_STANDARD) {
	checkqueue(2);
	outw(BKGD_MIX, 0x0027);	/* Not used .. however ensure PIX_TRANS is not used */
	outw(ALU_FG_FN, 7);
    } else {
	checkqueue(5);
	outw(GE_OFFSET_LO, mach32_ge_off_l);
	outw(GE_OFFSET_HI, mach32_ge_off_h);
	outw(GE_PITCH, mach32_ge_pitch);
	outw(BKGD_MIX, mach32_bkgd_alu_fn);
	outw(FRGD_MIX, mach32_frgd_alu_fn | 0x20);
	checkqueue(6);
	outw(ALU_BG_FN, mach32_bkgd_alu_fn);
	outw(ALU_FG_FN, mach32_frgd_alu_fn);
        outw(FRGD_COLOR, mach32_frgd_col);
        outw(BKGD_COLOR, mach32_bkgd_col);
	outw(RD_MASK, (infotable[CM].colors == 32768) ? 0x7fff : 0xffff);
	outw(LINEDRAW_OPT, 0);
    }
}

static void mach32_setdisplaystart(int address)
{
#ifdef DEBUG
    printf("mach32_setdisplaystart(%x)\n", address);
#endif
    mach32_ge_off_l = address >> 2;
    outw(CRT_OFFSET_LO, mach32_ge_off_l);
    mach32_ge_off_h = 0xff & (address >> 18);
    outw(CRT_OFFSET_HI, mach32_ge_off_h);
    if (mach32_accelstate == R_EXTENDED) {
	checkqueue(2);
	outw(GE_OFFSET_LO, mach32_ge_off_l);
	outw(GE_OFFSET_HI, mach32_ge_off_h);
    }
    __svgalib_vga_driverspecs.setdisplaystart(address);
}

static void mach32_setlogicalwidth(int width)
{
    register int mywidth;
#ifdef DEBUG
    printf("mach32_setlogicalwidth(%d)\n", width);
#endif
    if (infotable[CM].bytesperpixel) {	/* always >= 1 for Mach32 modes */
	/*Unfortunately the Mach32 expects this value in Pixels not bytes: */
	mywidth = width / (infotable[CM].bytesperpixel);
	mywidth = (mywidth >> 3) & 0xff;
#ifdef DEBUG
	printf("mach32_setlogicalwidth: Mach32 width to %d pels.\n", mywidth * 8);
#endif
	outw(CRT_PITCH, mywidth);
	mach32_ge_pitch = mywidth;
	if (mach32_accelstate == R_EXTENDED) {
	    checkqueue(1);
	    outw(GE_PITCH, mywidth);
	}
    }
    __svgalib_vga_driverspecs.setlogicalwidth(width);
}


static void mach32_setpage(int page)
{
    register unsigned short tmp;

#ifdef DEBUG
    printf("mach32_setpage(%d)\n", page);
#endif
    if (mach32_pagemode != PAGE_BOTH) {
	outb(ATIPORT, ATISEL(0x3E));
	tmp = inb(ATIPORT + 1) & 0xf7;
	tmp = (tmp << 8) | ATISEL(0x3E);
	outw(ATIPORT, tmp);
	mach32_pagemode = PAGE_BOTH;
    }
    tmp = (page << 9) & 0x1e00;
    outw(ATIPORT, ATISEL(0x32) | tmp);
    outb(ATIPORT, ATISEL(0x2e));
    tmp = (inb(ATIPORT + 1) & 0xfc) | ((page >> 4) & 3);
    outw(ATIPORT, ATISEL(0x2e) | (tmp << 8));
}

static void mach32_setwrpage(int page)
{
    register unsigned short tmp;
    if (mach32_pagemode != PAGE_DIFF) {
	outb(ATIPORT, ATISEL(0x3E));
	outw(ATIPORT, (ATISEL(0x3E) << 8) | (inb(ATIPORT + 1) | 8));
	mach32_pagemode = PAGE_DIFF;
    }
    outb(ATIPORT, ATISEL(0x32));
    tmp = inb(ATIPORT + 1) & 0xe1;
    outw(ATIPORT, (ATISEL(0x32) << 8) | ((page << 1) & 0x1e));
    outb(ATIPORT, ATISEL(0x2e));
    tmp = inb(ATIPORT + 1) & 0xfc;
    outw(ATIPORT, (ATISEL(0x2e) << 8) | tmp | ((page >> 4) & 3));
}

static void mach32_setrdpage(int page)
{
    register unsigned short tmp;
    if (mach32_pagemode != PAGE_DIFF) {
	outb(ATIPORT, ATISEL(0x3E));
	outw(ATIPORT, (ATISEL(0x3E) << 8) | (inb(ATIPORT + 1) | 8));
	mach32_pagemode = PAGE_DIFF;
    }
    outb(ATIPORT, ATISEL(0x32));
    tmp = inb(ATIPORT + 1) & 0x1e;
    if (page & 8)
	tmp |= 1;
    outw(ATIPORT, (ATISEL(0x32) << 8) | tmp | ((page << 5) & 0xe0));
    outb(ATIPORT, ATISEL(0x2e));
    tmp = inb(ATIPORT + 1) & 0xf3;
    outw(ATIPORT, (ATISEL(0x2e) << 8) | tmp | ((page >> 2) & 0xc));
}

static void mach32_setappage(int page)
{
    outw(MEM_CFG, (mach32_memcfg | (0xc & (page << 2))));
}

static int mach32_sav_dac(int offset, unsigned char regs[])
{
/* I was unable to read out the actual DAC_config, so
   we just save which mode we are in: */

    regs[offset] = mach32_dacmode;
#ifdef DEBUG
    printf("mach32_sav_dac(%d,...): Dac:%d, dac_mode:%d\n", offset, mach32_dac,
	   regs[offset]);
#endif
    return offset + 1;
}

static inline void clean_clocks(void)
{
    outw(CLOCK_SEL, (inw(CLOCK_SEL) & ~0x7f) | 0x11);
}

static int mach32_set_dac(int dac_mode, int clock_intended, int xres)
{
    unsigned short act_ge_conf;
    const unsigned char *dac_reg;


    act_ge_conf = inw(R_EXT_GE_CONF) & 0x8fff;

#ifdef DEBUG
    printf("mach32_set_dac(%d,%d): Dac:%d\n", dac_mode, clock_intended, mach32_dac);
#endif
    mach32_dacmode = dac_mode;
    dac_mode &= 0x7f;
    mach32_blankadj((mach32_dac == MACH32_ATI68830) ? 0x4 : 0xc);
    switch (mach32_dac) {
    case MACH32_SC11483:
	dac_reg = mach32_dac1;
#ifdef DEBUG
	fputs("DAC1: ", stdout);
#endif
	if (dac_mode <= DAC_MODE565)
	    goto dac1_4;
	break;
    case MACH32_ATI6871:
	/*This one is a nightmare: */
	clean_clocks();
	outw(EXT_GE_CONF, 0x201a);

	switch (dac_mode) {

	default:
	case DAC_MODE8:
	case DAC_MODEMUX:
	    outb(DAC1, 0x00);
	    outb(DAC2, 0x30);
	    outb(DAC3, 0x2d);
#ifdef DEBUG
	    puts("DAC2: 0x00 0x30 0x2d (8bpp)");
#endif
	    if (dac_mode != DAC_MODEMUX)
		break;
	    outb(DAC2, 0x09);
	    outb(DAC3, 0x1d);
	    outb(DAC1, 0x01);
	    mach32_blankadj(1);
#ifdef DEBUG
	    puts("DAC2: 0x01 0x09 0x1d (8bpp MUX)");
#endif
	    break;
	case DAC_MODE555:
	case DAC_MODE565:
	case DAC_MODERGB:
	    mach32_blankadj(1);
	case DAC_MODE32B:
	    outb(DAC1, 0x01);
	    if ((!(clock_intended & 0xc0)) && (!(mach32_dacmode & DAC_SEMICLK))) {
		outb(DAC2, 0x00);
#ifdef DEBUG
		puts("DAC2: 0x01 0x00 0x0d (16/24bpp)");
#endif
	    } else {
		clock_intended &= 0xff3f;
		outb(DAC2, 0x08);
		mach32_dacmode |= DAC_SEMICLK;
		if (xres <= 640)
		    mach32_blankadj(2);
#ifdef DEBUG
		puts("DAC2: 0x01 0x08 0x0d (16/24bpp)");
#endif
	    }
	    outb(DAC3, 0x0d);
	    act_ge_conf |= 0x4000;
	    if (dac_mode == DAC_MODE32B)
		mach32_blankadj(3);
	    break;
	}
	if (bladj >= 0)
	    mach32_blankadj(bladj);
	break;
    case MACH32_BT481:
	dac_reg = mach32_dac4;
#ifdef DEBUG
	fputs("DAC4: ", stdout);
#endif
      dac1_4:
	if (dac_mode <= DAC_MODERGB) {
	    clean_clocks();
	    outw(EXT_GE_CONF, 0x101a);
#ifdef DEBUG
	    printf("%02x\n", dac_reg[dac_mode]);
#endif
	    outb(DAC2, dac_reg[dac_mode]);
	}
	break;
    case MACH32_ATI68860:
	clean_clocks();
	outw(EXT_GE_CONF, 0x301a);
#ifdef DEBUG
	printf("DAC5: %02x\n", mach32_dac5[dac_mode]);
#endif
	outb(DAC0, mach32_dac5[dac_mode]);
	break;
    }
/*Dac RS2,3 to 0 */
    act_ge_conf &= 0x8fff;
    if ((dac_mode == DAC_MODE8) || (dac_mode == DAC_MODEMUX))
	if (ext_settings & VGA_CLUT8)
	    act_ge_conf |= 0x4000;
    outw(EXT_GE_CONF, act_ge_conf);
/*Set appropriate DAC_MASK: */
    switch (dac_mode) {
    case DAC_MODE8:
    case DAC_MODEMUX:
#ifdef DEBUG
	puts("DAC-Mask to 0xff");
#endif
	outb(DAC_MASK, 0xff);
	break;
    default:
	switch (mach32_dac) {
	case MACH32_ATI6871:
	case MACH32_BT481:
#ifdef DEBUG
	    puts("DAC-Mask to 0x00");
#endif
	    outb(DAC_MASK, 0x00);
	    break;
	}
#ifdef DEBUG
	puts("VGA-DAC-Mask to 0x0f");
#endif
	outb(VGA_DAC_MASK, 0x0f);
	break;
    }
    return clock_intended;
}

static int mach32_saveregs(unsigned char regs[])
{
    int i, retval;
    unsigned short tmp;

    mach32_bltwait();		/* Ensure noone draws in the screen */

    for (i = 0; i < sizeof(mach32_eregs); i++) {
	outb(ATIPORT, ATISEL(mach32_eregs[i]));
	regs[EXT + i] = inb(ATIPORT + 1);
    }
    regs[EXT + i] = mach32_disp_shadow;		/* This is for DISP_CNTL */
    retval = i + EXT + 1;
    for (i = 0; i < (sizeof(mach32_acc_regs) / sizeof(unsigned short)); i += 2) {
	tmp = inw(mach32_acc_regs[i]);
	regs[retval++] = tmp;
	regs[retval++] = (tmp >> 8);
    }
    retval = mach32_sav_dac(retval, regs);
#ifdef DEBUG
    printf("mach32_saveregs: retval=%d\n", retval);
#endif
    return retval - EXT;
}

static void mach32_setregs(const unsigned char regs[], int mode)
{
    int i, offset, retval, clock_intended = 0;
    unsigned short tmp;

    mach32_bltwait();		/* Ensure noone draws in the screen */
    mach32_accelstate = R_UNKNOWN;	/* Accel registers need to be reset */

    offset = EXT + sizeof(mach32_eregs) + (sizeof(mach32_acc_regs) / sizeof(unsigned short)) + 1;

/*Find out which clock we want to use: */
    for (i = (sizeof(mach32_acc_regs) / sizeof(unsigned short)) - 1, retval = offset; i >= 0;
	 i -= 2, retval -= 2)
	if (mach32_acc_regs[i] == CLOCK_SEL) {
	    clock_intended = ((unsigned short) regs[--retval]) << 8;
	    clock_intended |= regs[--retval];
	    break;
	}
#ifdef DEBUG
    printf("mach32_setregs: offset=%d, clock_intended=%d\n", offset, clock_intended);
#endif

    retval = offset + 1;
    mach32_set_dac(regs[offset], clock_intended, 0);	/*We can savely call with a fake xres coz
							   blank adjust will be restored afterwards
							   anyways... */

    for (i = (sizeof(mach32_acc_regs) / sizeof(unsigned short)) - 1; i >= 0; i -= 2) {
	tmp = ((unsigned short) regs[--offset]) << 8;
	tmp |= regs[--offset];
	/* restore only appage in MEM_CFG... otherwise badly interaction
	   with X that may change MEM_CFG and does not only not restore it's
	   original value but also insist on it not being changed on VC
	   change... =:-o */
	if (mach32_acc_regs[i] == MEM_CFG)
	    tmp = (inw(MEM_CFG) & ~0xc) | (tmp & 0xc);
	if (mach32_ast && (mach32_acc_regs[i] == MISC_CTL))
	    continue;
        if (mach32_acc_regs[i] == H_DISP) {
	    /* H_TOTAL (alt) in H_DISP broken for some chipsets... */
	    outb(H_TOTAL, tmp >> 8);
	    outb(H_DISP, tmp);
	} else {
	    outw(mach32_acc_regs[i], tmp);
	}
    }
    mach32_disp_shadow = regs[--offset] & ~0x60;
    outb(DISP_CNTL, mach32_disp_shadow | 0x40);		/* Mach32 CRT reset */
    if (inw(CLOCK_SEL) & 1)	/* If in non VGA mode */
	outw(DISP_CNTL, mach32_disp_shadow | 0x20);	/* Mach32 CRT enabled */

    for (i = sizeof(mach32_eregs) - 1; i >= 0; i--) {
	outb(ATIPORT, ATISEL(mach32_eregs[i]));
	outb(ATIPORT + 1, regs[--offset]);
    }

}

static void mach32_unlock(void)
{
    unsigned short oldval;

#ifdef DEBUG
    puts("mach32_unlock");
#endif				/* DEBUG */
    outb(ATIPORT, ATISEL(0x2e));
    oldval = inb(ATIPORT + 1) & ~0x10;	/* Unlock CPUCLK Select */
    outw(ATIPORT, ATISEL(0x2e) | (oldval << 8));

    outb(ATIPORT, ATISEL(0x2b));
    oldval = inb(ATIPORT + 1) & ~0x18;	/* Unlock DAC, Dbl Scan. */
    outw(ATIPORT, ATISEL(0x2b) | (oldval << 8));

    outb(ATIPORT, ATISEL(0x34));
    oldval = inb(ATIPORT + 1) & ~0xfc;	/* Unlock Crt9[0:4,7], Vtiming, Cursr start/end,
					   CRT0-7,8[0-6],CRT14[0-4]. but disable ignore of CRT11[7] */
    outw(ATIPORT, ATISEL(0x34) | (oldval << 8));

    outb(ATIPORT, ATISEL(0x38));	/* Unlock ATTR00-0f, ATTR11, whole vga, port 3c2 */
    oldval = inb(ATIPORT + 1) & ~0x0f;
    outw(ATIPORT, ATISEL(0x38) | (oldval << 8));

/* Unlock vga-memory too: */

    outw(MEM_BNDRY, 0);

/* Unlock Mach32 CRT Shadowregisters... this one made me crazy...
   Thanx to the Xfree sources I finally figured it out.... */
    outw(SHADOW_SET, 1);
    outw(SHADOW_CTL, 0);
    outw(SHADOW_SET, 2);
    outw(SHADOW_CTL, 0);
    outw(SHADOW_SET, 0);
}

static void mach32_lock(void)
{
    unsigned short oldval;

#ifdef DEBUG
    puts("mach32_lock");
#endif				/* DEBUG */
/* I'm really not sure if calling this function would be a good idea */
/* Actually it is not called in svgalib */
    outb(ATIPORT, ATISEL(0x2b));
    oldval = inb(ATIPORT + 1) | 0x18;	/* Lock DAC, Dbl Scan. */
    outw(ATIPORT, ATISEL(0x2b) | (oldval << 8));

    outb(ATIPORT, ATISEL(0x34));
    oldval = inb(ATIPORT + 1) | 0xfc;	/* Lock Crt9[0:4,7], Vtiming, Cursr start/end,
					   CRT0-7,8[0-6],CRT14[0-4]. but disable ignore of CRT11[7] */
    outw(ATIPORT, ATISEL(0x34) | (oldval << 8));

    outb(ATIPORT, ATISEL(0x38));	/* Lock ATTR00-0f, ATTR11, whole vga, port 3c2 */
    oldval = inb(ATIPORT + 1) | 0x0f;
    outw(ATIPORT, ATISEL(0x38) | (oldval << 8));

    outw(SHADOW_SET, 1);
    outw(SHADOW_CTL, 0x7f);
    outw(SHADOW_SET, 2);
    outw(SHADOW_CTL, 0x7f);
    outw(SHADOW_SET, 0);
}

static void mach32_experm(void)
{
    printf("svgalib(mach32): Cannot get I/O permissions.\n");
    exit(-1);
}

#ifdef OSKIT
static void mach32_bltwait(void)
{
    int i = BUSYWAIT;
    struct oskit_timespec time;

    checkqueue(16);		/* Ensure nothing in the queue */

    while (i--)
	if (!(inw(GE_STAT) & (GE_BUSY | 1))) {
	  done:
	    /* Check additional stats */
	    if (inw(SUBSYS_STAT) & 4)
		goto failure;
	    CRITICAL = 0;	/* Obviously we're idle */
	    return;
	}

#if 0
    do {
#endif
	if (!(inw(GE_STAT) & (GE_BUSY | 1)))
	    goto done;
#if 0
    }
#endif

    time.tv_nsec = 0;
    time.tv_sec = ADDIWAIT;
    clock_spin(&time);

  failure:
    mach32_ge_reset();
}
#else
static void mach32_bltwait(void)
{
    int i = BUSYWAIT;
    clock_t clk_time;

    checkqueue(16);		/* Ensure nothing in the queue */

    while (i--)
	if (!(inw(GE_STAT) & (GE_BUSY | 1))) {
	  done:
	    /* Check additional stats */
	    if (inw(SUBSYS_STAT) & 4)
		goto failure;
	    CRITICAL = 0;	/* Obviously we're idle */
	    return;
	}

    clk_time = clock();

    do {
	if (!(inw(GE_STAT) & (GE_BUSY | 1)))
	    goto done;
    }
    while ((clock() - clk_time) < (CLOCKS_PER_SEC * ADDIWAIT));

  failure:
    mach32_ge_reset();
}
#endif

static void mach32_i_bltwait(void)
{
    int i;

    for (i = 0; i < 100000; i++)
	if (!(inw(GE_STAT) & (GE_BUSY | 1)))
	    break;
}

static int mach32_test(void)
{
    int result = 0;
    short tmp;

/* If IOPERM is set, assume permissions have already been set by Olaf Titz' */
/* ioperm(1). */
    if (getenv("IOPERM") == NULL)
	if (iopl(3) < 0)
	    mach32_experm();
/* Har, har.. now we can switch off interrupts to crash the system... ;-)=) */
/* (But actually we need only access to extended io-ports...) */

    tmp = inw(SCRATCH_PAD_0);
    outw(SCRATCH_PAD_0, 0x5555);
    mach32_i_bltwait();
    if (inw(SCRATCH_PAD_0) == 0x5555) {
	outw(SCRATCH_PAD_0, 0x2a2a);
	mach32_i_bltwait();
	if (inw(SCRATCH_PAD_0) == 0x2a2a) {
	    /* Aha.. 8514/a detected.. */
	    result = 1;
	}
    }
    outw(SCRATCH_PAD_0, tmp);
    if (!result)
	goto quit;
/* Now ensure it is not a plain 8514/a: */
    result = 0;
    outw(DESTX_DIASTP, 0xaaaa);
    mach32_i_bltwait();
    if (inw(R_SRC_X) == 0x02aa) {
	outw(DESTX_DIASTP, 0x5555);
	mach32_i_bltwait();
	if (inw(R_SRC_X) == 0x0555)
	    result = 1;
    }
    if (!result)
	goto quit;
    result = 0;
/* Ok, now we have a Mach32. Unfortunately we need also the VGA to be enabled: */
    if (inw(CONF_STAT1) & ONLY_8514) {
	if (__svgalib_driver_report)
	    puts("Mach32 detected, but unusable with VGA disabled.\nSorry.\n");
	goto quit;		/*VGA circuitry disabled. */
    }
    result = 1;
  quit:
    if (getenv("IOPERM") == NULL)
	iopl(0);		/* safety, mach32_init reenables it */
    if (result)
	mach32_init(0, 0, 0);
#ifdef DEBUG
    printf("mach32_test: returning %d.\n", result);
#endif				/* DEBUG */
    return result;
}

static void mach32_wait(void)
{
/* Wait for at least 22 us.. (got that out of a BIOS disassemble on my 486/50 ;-) ) ... */
    register int i;
    for (i = 0; i < 16; i++)
	dummy++;		/*Dummy is volatile.. */
}

static int mach32_eeclock(register int ati33)
{
    outw(ATIPORT, ati33 |= 0x200);	/* clock on */
    mach32_wait();
    outw(ATIPORT, ati33 &= ~0x200);	/* clock off */
    mach32_wait();
    return ati33;
}

static void mach32_eekeyout(register int ati33, register int offset, register int mask)
{
    do {
	if (mask & offset)
	    ati33 |= 0x100;
	else
	    ati33 &= ~0x100;
	outw(ATIPORT, ati33);
	mach32_eeclock(ati33);
    }
    while (mask >>= 1);
}

static int mach32_eeget(int offset)
{
    register int ati33;
    register int result, i;

/* get current ATI33 */
    outb(ATIPORT, ATISEL(0x33));
    ati33 = ((int) inw(ATIPORT + 1)) << 8;
    ati33 |= ATISEL(0x33);
/* prepare offset.. cut and add header and trailer */
    offset = (0x600 | (offset & 0x7f)) << 1;

/* enable eeprom sequence */
    ati33 = mach32_eeclock(ati33);
/*input to zero.. */
    outw(ATIPORT, ati33 &= ~0x100);
/*enable to one */
    outw(ATIPORT, ati33 |= 0x400);
    mach32_eeclock(ati33);
/*select to one */
    outw(ATIPORT, ati33 |= 0x800);
    mach32_eeclock(ati33);
    mach32_eekeyout(ati33, offset, 0x800);
    for (i = 0, result = 0; i < 16; i++) {
	result <<= 1;
	outb(ATIPORT, ATISEL(0x37));
	if (inb(ATIPORT + 1) & 0x8)
	    result |= 1;
	mach32_eeclock(ati33);
    }
/*deselect... */
    outw(ATIPORT, ati33 &= ~0x800);
    mach32_eeclock(ati33);
/*disable... */
    outw(ATIPORT, ati33 &= ~0x400);
    mach32_eeclock(ati33);
    return result;
}

static int mach32_max_mask(int x)
{
    if (x <= 0x4f)
	return dacmo640[mach32_dac];
    if (x <= 0x63)
	return dacmo800[mach32_dac];
    if (x <= 0x7f)
	return dacmo1024[mach32_dac];
    return dacmo1280[mach32_dac];
}

static int mach32_max_vfifo16(int x)
{
    int retval;

    if (x <= 0x63)
	retval = vfifo16;
    else if (x <= 0x7f)
	retval = vfifo16 + 4;
    else
	retval = vfifo16 + 5;
    return (retval < 15) ? retval : 15;
}

/* Shameless ripped out of Xfree2.1 (with slight changes) : */

static void mach32_scan_clocks(void)
{
    const int knownind = 7;
    const double knownfreq = 44.9;

    char hstrt, hsync;
    int htotndisp, vdisp, vtotal, vstrt, vsync, clck, i;

    int count, saved_nice, loop;
    double scale;

#ifndef OSKIT
    saved_nice = nice(0);
    nice(-20 - saved_nice);
#endif

    puts("Warning, about to measure clocks. Wait until system is completely idle!\n"
	 "Any activity will disturb measuring, and therefor hinder correct driver\n"
	 "function. Test will need about 3-4 seconds.\n"
	 "If your monitor doesn't switch off when it gets signals it can't sync on\n"
	 "better switch it off now before continuing. I'll beep when you can safely\n"
	 "switch it back on.");
#if 1
    puts("\n(Enter Y<Return> to continue, any other text to bail out)");

    if (getchar() != 'Y') {
      bailout:
	puts("\aBailed out.");
	exit(0);
    }
    if (getchar() != '\n')
	goto bailout;
#endif

    htotndisp = inw(R_H_TOTAL);
    hstrt = inb(R_H_SYNC_STRT);
    hsync = inb(R_H_SYNC_WID);
    vdisp = inw(R_V_DISP);
    vtotal = inw(R_V_TOTAL);
    vstrt = inw(R_V_SYNC_STRT);
    vsync = inw(R_V_SYNC_WID);
    clck = inw(CLOCK_SEL);

    outb(DISP_CNTL, 0x63);

    outb(H_TOTAL, 0x63);
    outb(H_DISP, 0x4f);
    outb(H_SYNC_STRT, 0x52);
    outb(H_SYNC_WID, 0x2c);
    outw(V_TOTAL, 0x418);
    outw(V_DISP, 0x3bf);
    outw(V_SYNC_STRT, 0x3d6);
    outw(V_SYNC_WID, 0x22);

    for (i = 0; i < 16; i++) {
	outw(CLOCK_SEL, (i << 2) | 0xac1);
	outb(DISP_CNTL, 0x23);

	usleep(MODESWITCHDELAY);

	count = 0;
	loop = 200000;

	while (!(inb(DISP_STATUS) & 2))
	    if (loop-- == 0)
		goto done;
	while (inb(DISP_STATUS) & 2)
	    if (loop-- == 0)
		goto done;
	while (!(inb(DISP_STATUS) & 2))
	    if (loop-- == 0)
		goto done;

	for (loop = 0; loop < 5; loop++) {
	    while (!(inb(DISP_STATUS) & 2))
		count++;
	    while ((inb(DISP_STATUS) & 2))
		count++;
	}
      done:
	mach32_clocks[i] = count;

	outb(DISP_CNTL, 0x63);
    }

    outw(CLOCK_SEL, clck);

    outb(H_TOTAL, htotndisp >> 8);
    outb(H_DISP, htotndisp);
    outb(H_SYNC_STRT, hstrt);
    outb(H_SYNC_WID, hsync);
    outw(V_DISP, vdisp);
    outw(V_TOTAL, vtotal);
    outw(V_SYNC_STRT, vstrt);
    outw(V_SYNC_WID, vsync);
#ifndef OSKIT
    nice(20 + saved_nice);
#endif

/*Recalculation: */
    scale = ((double) mach32_clocks[knownind]) * knownfreq;
    for (i = 0; i < 16; i++) {
	if (i == knownind)
	    continue;
	if (mach32_clocks[i])
	    mach32_clocks[i] = 0.5 + scale / ((double) mach32_clocks[i]);
    }
    mach32_clocks[knownind] = knownfreq + 0.5;
}

static signed char mach32_search_clk(int clk)
{
    int i;

    if (!clk)
	return 0;
    if (clk > 80)		/* Actually filter out the too high clocks here already */
	return 0;
    for (i = 0; i < 32; i++)
	if (abs(clk - mach32_clocks[i]) <= 1)
	    return i + 1;
    return 0;
}

static unsigned short dac14_clock_adjust(unsigned short clock, signed char *clock_map)
{
    unsigned short clock_ind;

    if ((mach32_dac != MACH32_SC11483) && (mach32_dac != MACH32_BT481))
	return clock;
    clock_ind = 0x1f & (clock >> 2);
    clock &= 0xff83;
    if (!clock_map[clock_ind]) {
	vga_setmode(TEXT);
	puts("svgalib-mach32: Panic, internal error: DAC1/4, invalid clock for >8bpp.");
	exit(1);
    }
    return clock | ((clock_map[clock_ind] - 1) << 2);
}

static int mach32_init(int force, int chiptype, int memory)
{
    char messbuf[12], eeapert = 0;
    static int max2msk[] =
    {1, 3, 0xf, 0xf};
    static const short mem_conf[] =
    {512, 1024, 2048, 4096};
    static const short mem_aper[] =
    {0,
     MACH32_APERTURE,
     MACH32_APERTURE | MACH32_APERTURE_4M,
     0};
    int i;
    unsigned short *ptr, cfg1, cfg2;
    unsigned long apertloc = 0;

#ifdef EXPERIMENTAL
    if (getenv("EXPERIMENTAL") != NULL) {
	mach32_experimental = atoi(getenv("EXPERIMENTAL"));
	printf("mach32: EXPERIMENTAL set to: %04x\n", mach32_experimental);
    } else {
	printf("mach32: EXPERIMENTAL defaults to: %04x\n", mach32_experimental);
    }
#endif

/* If IOPERM is set, assume permissions have already been set by Olaf Titz' */
/* ioperm(1). */
    if (getenv("IOPERM") == NULL)
	if (iopl(3) < 0)
	    mach32_experm();

    mach32_readconfig();
/* Ensure we are idle. (Probable dead lock if no 8514 installed) */
    mach32_bltwait();

/*get main config registers */
    cfg1 = inw(CONF_STAT1);
    cfg2 = inw(CONF_STAT2);

/* get 2 eeprom buffers */
    if (mach32_eeprom == NULL) {
	unsigned short sum, crea_eeprom = 1;

	if ((mach32_eeprom = malloc(sizeof(short) * 2 * 128)) == NULL) {
	    printf("svgalib: mach32: Fatal error,\n"
	      "not enough memory for EEPROM image (512 Bytes needed)\n");
	    /* svgamode not yet set.. */
	    exit(-1);
	}
	/* Do we have it already in a file ? */
	if ((eeprom_fname != NULL) && !(eeprom_option & EEPROM_UPDATE)) {
	    FILE *fd;

	    fd = fopen(eeprom_fname, "rb");
	    if (fd == NULL) {
	      readerr:
		printf("mach32: Warning, can't access EEPROM file >%s<\nError was %d - %s\n",
		       eeprom_fname, errno, strerror(errno));
		goto read_eeprom;
	    }
	    if (1 != fread(mach32_eeprom, sizeof(short) * 129, 1, fd)) {
		i = errno;
		fclose(fd);
		errno = i;
		goto readerr;
	    }
	    if (fclose(fd))
		goto readerr;
	    /*Checksum.. */
	    for (i = 0, sum = 0, ptr = mach32_eeprom; i < 128; i++)
		sum += *ptr++;
	    if (sum == *ptr) {
		crea_eeprom = 0;	/*Mark succesful read... */
		goto skip_read;	/* Use the read in file... */
	    }
	    sum = 0;		/*Mark unsuccesful read.. */
	    printf("mach32: Warning, EEPROM file >%s< corrupted.\n", eeprom_fname);
	}
	/* Read in eeprom */
      read_eeprom:
	if (!(eeprom_option & (EEPROM_USE_CHKSUM |
			       EEPROM_USE_MEMCFG | EEPROM_USE_TIMING))) {
	    /* No need to bother reading the EEPROM */
	    goto skip_read;
	}
	for (i = 0, ptr = mach32_eeprom; i < 128; i++)
	    *ptr++ = mach32_eeget(i);
	if (eeprom_option & EEPROM_USE_CHKSUM) {
	    for (i = 0, sum = 0, ptr = mach32_eeprom; i < 128; i++)
		sum += *ptr++;
	    sum &= 0xffff;
	    if (sum) {
		/*Hmm.. no ATI checksum... try AST: */
		sum -= (mach32_eeprom[0x7f] << 1) - 1;
	    }
	} else
	    sum = 0;
	if (sum & 0xffff) {
	    puts("mach32: Warning, Illegal checksum in Mach32 EEPROM.\n"
	     "Check configuration of your card and read README.mach32,\n"
		 "esp. the Mach32 Eeprom Woe chapter.\n"
		 "Ignoring contents...");
	    eeprom_option &= ~(EEPROM_USE_MEMCFG | EEPROM_USE_TIMING);
	}
      skip_read:

	if ((eeprom_fname != NULL) && (crea_eeprom || (eeprom_option & EEPROM_UPDATE)))
	    /* create file if not there or unsuccesful or requested */
	{
	    FILE *fd;

	    /*Calc valid checksum.. */
	    for (i = 0, sum = 0, ptr = mach32_eeprom; i < 128; i++)
		sum += *ptr++;

	    fd = fopen(eeprom_fname, "wb");
	    if (fd == NULL) {
	      writerr:
		printf("mach32: Warning, can't create new EEPROM file >%s<\nError was %d - %s\n",
		       eeprom_fname, errno, strerror(errno));
		crea_eeprom = 0;
		goto finish_w_eeprom;
	    }
	    if (1 != fwrite(mach32_eeprom, sizeof(short) * 128, 1, fd)) {
	      fwri_err:
		i = errno;
		fclose(fd);
		errno = i;
		goto writerr;
	    }
	    if (1 != fwrite(&sum, sizeof(short), 1, fd))
		 goto fwri_err;
	    if (fclose(fd))
		goto writerr;
	    printf("mach32: Notice: new EEPROM file >%s< succesful created.\n",
		   eeprom_fname);
	  finish_w_eeprom:
	}
	/* Change eeprom contents if requested: */
	if (!(eeprom_option & EEPROM_USE_MEMCFG))
	    mach32_eeprom[6] = 0;
	if (!(eeprom_option & EEPROM_USE_TIMING)) {
	    /* Consider ALL standard modes, but no user defined */
	    mach32_eeprom[0x07] = 0x01;		/*  640 x  480 */
	    mach32_eeprom[0x08] = 0x3f;		/*  640 x  480 */
	    mach32_eeprom[0x09] = 0x1f;		/*  640 x  480 */
	    mach32_eeprom[0x0A] = 0x03;		/*  640 x  480 */
	    mach32_eeprom[0x0B] = 0x00;		/*  no 1152x900 or 1120x750 */
	}
	if (!clocks_set) {
	    char linebuffer[512];

	    puts("mach32: Warning, no clocks defined.. please have a look at\n"
		 "README.Mach32 and README.config for details.\n");
	    mach32_scan_clocks();
	    fputs("\a\nResulting clocks command for your libvga.config should be:\n\n"
		  "clocks", stdout);
	    for (i = 0; i < 16; i++)
		printf(" %3d", mach32_clocks[i]);
#ifndef OSKIT
	    fputs("\n\nPlease edit \""
		  SVGALIB_CONFIG_FILE
		  "\" appropriately.\n"
		  "Or shall I try to do it for you? You have to have write access to the\n"
		  "config file to do it! (y/n) ", stdout);
	    i = getchar();
	    if (i != '\n')
		while (getchar() != '\n');
	    if ((i != 'y') && (i != 'Y')) {
		puts("Ok, then do it yourself.");
		exit(0);
	    }
	    /* Toast setuid settings coz of security breach of system().. */

	    setreuid(getuid(), getuid());
	    setuid(getuid());
	    setgid(getgid());

	    strcpy(linebuffer, "rm -f "
		   SVGALIB_CONFIG_FILE
		   ".bak; mv "
		   SVGALIB_CONFIG_FILE
		   " "
		   SVGALIB_CONFIG_FILE
		   ".bak; echo clocks");
	    for (i = 0; i < 16; i++)
		sprintf(strchr(linebuffer, 0), " %d", mach32_clocks[i]);
	    strcat(linebuffer, " | cat - "
		   SVGALIB_CONFIG_FILE
		   ".bak >"
		   SVGALIB_CONFIG_FILE);
	    if (system(linebuffer))
		puts("Failed (at least partial, maybe there was just no config file yet).");
	    else
		puts("Ok. (But you should check it anyway)");
#endif
	    exit(0);
	}
#ifdef DEBUG
	printf("ATI-EEPROM contents:");
	for (i = 0; i < 128; i++) {
	    if (i & 7)
		putchar(' ');
	    else
		puts("");
	    printf("%02x - %04x", i, mach32_eeprom[i]);
	}
#ifdef DEBUG_KEY
	fputs("\n(Hit Return)", stdout);
	while (getchar() != '\n');
#else
	putchar('\n');
#endif				/* DEBUG_KEY */
#endif				/* DEBUG */
    }
/*create a scratch copy. */
    memcpy(mach32_eeprom + 128, mach32_eeprom, (size_t) (128 * sizeof(short)));

/*Get current memcfg */
    mach32_memcfg = inw(MEM_CFG) & 0xfff3;

/*If aperture not currently enabled but configured in EEPROM or setup already,
   then setup a faked setuplinear, switched off with setuplinear 0 0 */

    if ((!(mach32_memcfg & 3)) && (((mach32_eeprom[6] + 1) & 3) > 1) && (!mach32_apsiz)) {
	/*create setup from eeprom: */
	mach32_apsiz = (mach32_eeprom[6] & 3);
	mach32_apadd = (mach32_eeprom[6] >> 4);
	eeapert = 1;
    }
/*setup up memory aperture if requested: */

    if (mach32_apsiz) {
	unsigned new_memcfg;

	if (!mach32_apadd)
	    mach32_memcfg = 0;	/*disable any aperture */
	else {
	    if (((cfg1 & BUS_TYPE) == PCI) || ((cfg2 & Z4GBYTE) && !(cfg2 & LOCAL_BUS_CONF2))) {
		/* 4GB address range... */
		new_memcfg = (mach32_apadd << 4) & 0xfff0;
		i = 4096;
	    } else {
		/* 128MB address range... */
		new_memcfg = (mach32_apadd << 8) & 0xff00;
		i = 128;
	    }
	    new_memcfg |= mach32_apsiz;
	    if ((cfg1 & BUS_TYPE) == ISA)
		i = 16;
	    i -= mach32_apadd;
	    i -= (mach32_apsiz == 1) ? 1 : 4;
	    if (i < 0) {
		puts("svgalib-mach32: Dangerous warning:\n"
		     "\tsetuplinear setting exceeds phys. address range of card!\n"
		     "\tSetting ignored.");
		mach32_apsiz = 0;
	    } else {
		/* will be actually set by setappage(0) call in setmode */
		mach32_memcfg = new_memcfg;
	    }
	}
    }
    if (dac_override > 5)
	mach32_dac = (cfg1 >> 9) & 7;
    else
	mach32_dac = dac_override;
    mach32_chiptype = mem_aper[mach32_memcfg & 3];
    if (!force)
	mach32_memory = mem_conf[(inw(MISC_OPTIONS) >> 2) & 3];
    else {
	mach32_memory = memory;
	if (chiptype & MACH32_FORCEDAC)
	    mach32_dac = chiptype & 0xf;
	if (chiptype & MACH32_FORCE_APERTURE) {
	    mach32_chiptype = (chiptype & (MACH32_APERTURE | MACH32_APERTURE_4M));
	    if (mach32_chiptype & MACH32_APERTURE_4M)
		mach32_chiptype |= MACH32_APERTURE;
	}
    }
    if (mach32_dac > 5) {
	printf("mach32: Warning, unknown DAC type: %d. Assuming stupid 8bpp DAC rated at 80Mhz!\n",
	       mach32_dac);
	/* Assumption is already done in all tables used... */
    }
    if (!(DAC_SAFETY & (1 << mach32_dac))) {
	char *cptr;
	/* Safety notification */
	cptr = getenv("SVGALIB_MACH32");
	if (cptr == NULL) {
	  messexit:
	    puts("\aWarning!! The mach32 driver of svgalib was never tried with a DAC type\n"
		 "!=4,3,2,0. Nevertheless, I put every knowledge I got from the DOCs in this\n"
		 "driver, so it should work. If you think you can stand this risk\n"
		 "(for example, your monitor will just switch off if he can't stand the\n"
		 "video signal) then set the environment variable SVGALIB_MACH32 to\n"
		 "ILLTRYIT or --- of course --- recompile svgalib after changing the\n"
		 "#define DAC_SAFETY in mach32.c\n\n"
		 "To reduce possibility of damage to your card you should check that no\n"
		 "video mode above 80Mhz dotclock is used with dacs !=2 or 5. To check\n"
		 "recompile with DEBUG defined in mach32.c and look at the output.\n"
		 "In any case, that is, if it works or not (and what went wrong),\n"
		 "please tell me about:\n"
		 "Michael Weller, eowmob@exp-math.uni-essen.de,\n"
		 "eowmob@pollux.exp-math.uni-essen.de, or mat42b@aixrs1.hrz.uni-essen.de.\n.");
	    exit(-1);
	}
	if (strcmp(cptr, "ILLTRYIT"))
	    goto messexit;
    }
    if (mach32_dac != 2)
	pos_ext_settings &= ~VGA_CLUT8;


/* Access aperture */
    if ((mach32_chiptype & MACH32_APERTURE) && (mach32_aperture == NULL)) {
	if (((cfg1 & BUS_TYPE) == PCI) || ((cfg2 & Z4GBYTE) && !(cfg2 & LOCAL_BUS_CONF2))) {
	    /* 4GB address range... */
	    apertloc = ((unsigned long) (mach32_memcfg & 0xfff0)) << 16;
	} else {
	    /* 128MB address range... */
	    apertloc = ((unsigned long) (mach32_memcfg & 0xff00)) << 12;
	}
#ifdef DEBUG
	printf("mach32: Physical aperture starts at %08lx.\n", apertloc);
#endif
#ifdef OSKIT
	osenv_mem_map_phys(apertloc,
			   (mach32_chiptype & MACH32_APERTURE_4M) ? 4194304 : 1048576,
			   &mach32_aperture,
			   0);
#else
	mach32_aperture = (char *) mmap(NULL,
	      (mach32_chiptype & MACH32_APERTURE_4M) ? 4194304 : 1048576,
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					__svgalib_mem_fd,
					apertloc);
#endif
	/* I trust the comment about memory waste in vgamisc.c... */
	if (((long) mach32_aperture) < 0) {
	    i = errno;
	    printf("mach32: Mmaping of aperture failed.\nError was %d", i);
	    errno = i;
	    perror(" - ");
	    exit(-1);
	}
#ifdef DEBUG
	printf("mach32: Aperture mapped to %08lx.\n", (long) mach32_aperture);
#endif
    }
    __svgalib_driverspecs = &__svgalib_mach32_driverspecs;

/* Check if imageblit emulation possible: */
    i = 0;
    if (mach32_chiptype & MACH32_APERTURE_4M)
	i = (4 << 10);
    else if (mach32_chiptype & MACH32_APERTURE)
	i = (1 << 10);
    if (i < mach32_memory) {	/*aperture to small */
	emuimage &= ~(EMU_OVER | EMU_POSS);
    }
    if (!mach32_modes) {
	mach32_modes = malloc(__svgalib_max_modes * sizeof(mode_entry *));
	if (!mach32_modes) {
	    puts("mach32: No memory for dynamic mode table.");
	    exit(1);
	}
    }
/*Calculate the double and triple clocks */
    for (i = 0; i < 32; i++)
	mach32_clock_by2[i] = mach32_search_clk(mach32_clocks[i] << 1);
    for (i = 0; i < 32; i++)
	mach32_clock_by3[i] = mach32_search_clk(mach32_clocks[i] * 3);

#ifdef DEBUG
    puts("15/16bpp DAC1/4 clock_maps:");
    for (i = 0; i < 32; i++) {
	if (!(i & 7))
	    putchar('\n');
	if (mach32_clock_by2[i])
	    printf("%3d->%3d  ", mach32_clocks[i], mach32_clocks[mach32_clock_by2[i] - 1]);
	else
	    printf("%3d->  -  ", mach32_clocks[i]);
    }
    puts("\n24bpp DAC1/4 clock_maps:");
    for (i = 0; i < 32; i++) {
	if (!(i & 7))
	    putchar('\n');
	if (mach32_clock_by3[i])
	    printf("%3d->%3d  ", mach32_clocks[i], mach32_clocks[mach32_clock_by3[i] - 1]);
	else
	    printf("%3d->  -  ", mach32_clocks[i]);
    }
    putchar('\n');
#endif

/*Populating database: */
    if (verbose)
	puts("Populating mode table:");

    for (i = 0; i < __svgalib_max_modes; i++)
	mach32_modes[i] = NULL;

    for (i = 0; i < NUM_MODES; i++) {
	if ((predef_modes[i].offset == 0) ||
	  (mach32_eeprom[predef_modes[i].offset] & predef_modes[i].mask))
	    mach32_modfill(predef_modes + i, mach32_max_mask(predef_modes[i].h_disp), -1);
    }

    if (mach32_eeprom[7] & 2) {
	i = (mach32_eeprom[7] >> 8) & 0xff;
	if ((mach32_eeprom[i] & 0x140) == 0x140)
	    mach32_modfill((mode_entry *) (mach32_eeprom + i + 2),
			   max2msk[3 & (mach32_eeprom[i] >> 9)], -1);
    }
    if (mach32_eeprom[8] & 0x80) {
	i = (mach32_eeprom[8] >> 8) & 0xff;
	if ((mach32_eeprom[i] & 0x140) == 0x140)
	    mach32_modfill((mode_entry *) (mach32_eeprom + i + 2),
			   max2msk[3 & (mach32_eeprom[i] >> 9)], -1);
    }
    if (mach32_eeprom[9] & 0x80) {
	i = (mach32_eeprom[9] >> 8) & 0xff;
	if ((mach32_eeprom[i] & 0x140) == 0x140)
	    mach32_modfill((mode_entry *) (mach32_eeprom + i + 2),
			   max2msk[3 & (mach32_eeprom[i] >> 9)], -1);
    }
    if (mach32_eeprom[10] & 0x80) {
	i = (mach32_eeprom[10] >> 8) & 0xff;
	if ((mach32_eeprom[i] & 0x140) == 0x140)
	    mach32_modfill((mode_entry *) (mach32_eeprom + i + 2),
			   max2msk[3 & (mach32_eeprom[i] >> 9)], -1);
    }
    if (verbose)
	puts("Squeeze in run-time config:");
    mach32_final_modefixup();

    if (__svgalib_driver_report) {
	sprintf(messbuf, " at %dM", (int) (apertloc >> 20));
	printf("Using Mach32 driver 2.1 (%s apert%s (%s), %dK mem, DAC %d%s%s).\n",
	       (mach32_chiptype & MACH32_APERTURE) ?
	       ((mach32_chiptype & MACH32_APERTURE_4M) ? "4M" : "1M")
	       : "no",
	       (mach32_chiptype & MACH32_APERTURE) ? messbuf : "",
	    mach32_apsiz ? (eeapert ? "EEPROM" : "setup") : "autodetect",
	    mach32_memory, mach32_dac, (dac_override < 6) ? "(set)" : "",
	       force ? ", forced" : "");
    }
    return 0;
}

static inline int col2msk(struct info *iptr)
{
    switch (iptr->colors) {
    case 1 << 24:
	if (iptr->bytesperpixel == 3)
	    return 4;
#ifdef SUPP_32BPP
	if (iptr->bytesperpixel == 4)
	    return 8;
#endif
	else
	    return 0;
    case 1 << 15:
    case 1 << 16:
	if (iptr->bytesperpixel == 2)
	    return 2;
	else
	    return 0;
    case 256:
	if (iptr->bytesperpixel == 1)
	    return 1;
	else
	    return 0;
    }
    return 0;
}

static inline int col2bypp(struct info *iptr)
{
    return iptr->bytesperpixel;
}

static int mach32_log2(struct info *iptr)
{
    int res = -1, n = iptr->colors;

    while (n) {
	res++;
	n >>= 1;
    }
    if ((res == 24) && (iptr->bytesperpixel == 4))
	return 32;
    return res;
}

static void mach32_modfill(const mode_entry * mode, int modemask, int forcein)
{
    register int i;
    register struct info *iptr;
    register unsigned wid, hei;

    float horz, vert, n_horz, n_vert, cmpvert;
    int clock, n_clock, tmp;

    clock = mach32_clocks[n_clock = 0x1f & (mode->clock_sel >> 2)];
    if (!clock) {
	if (verbose)
	    puts("Illegal clock #%d of unknown frequency! (rejected)");
	return;
    }
    horz = (1e3 * clock) / (float) (mode->h_total * 8 + 8);
    hei = mode->v_total;
    if (!(mode->disp_cntl & 0x6))
	hei = ((hei >> 2) & 0xfffe) | (hei & 1);
    else
	hei = ((hei >> 1) & 0xfffc) | (hei & 3);
    hei++;
    vert = (horz * 1000) / hei;

    wid = mode->h_disp * 8 + 8;
    hei = mode->v_disp;
    if (!(mode->disp_cntl & 0x6))
	hei = ((hei >> 2) & 0xfffe) | (hei & 1);
    else
	hei = ((hei >> 1) & 0xfffc) | (hei & 3);
    hei++;

/*Maskout impossible colorsettings for high clock rates: */
    i = modemask;
    modemask &= mach32_mmask[(clock > 80) ? 1 : 0][mach32_dac];
    if (verbose && (i != modemask)) {
	printf("Modemask (32bpp,24bpp,16bpp,8bpp) %x cut down to %x by DAC restrictions.\n", i, modemask);
    }
/*Check if needed multiples are available */
    if ((mach32_dac == MACH32_BT481) || (mach32_dac == MACH32_SC11483)) {
	if ((modemask & 2) && (!mach32_clock_by2[n_clock])) {
	    modemask &= ~2;
	    if (verbose)
		printf("DAC1/4: There is no clock with two times the requested\n"
		       "\tfreq. of %dMHz => no 15 or 16 bpp.\n", clock);
	}
	if ((mach32_dac == MACH32_BT481) && (modemask & 4) && (!mach32_clock_by3[n_clock])) {
	    modemask &= ~4;
	    if (verbose)
		printf("DAC4: There is no clock with three times the requested\n"
		       "\tfreq. of %dMHz => no 24bpp.\n", clock);
	}
    }
    i = modemask;
/*Rate down clocks exceeding mem bandwidth */
    if (clock > mach32_maxclk8)
	modemask &= ~1;
    if (clock > mach32_maxclk16)
	modemask &= ~2;
    if (clock > mach32_maxclk24)
	modemask &= ~4;
    if (clock > mach32_maxclk32)
	modemask &= ~8;

    if (verbose && (i != modemask))
	printf("Modemask (32bpp,24bpp,16bpp,8bpp) %x cut down to %x by maxclocks.\n", i, modemask);

    if (forcein >= 0) {
	/*Well hack the next loop to run exactly one time for i=forcein.. */
	/*I know it's dirty... */
	i = forcein;
	iptr = infotable + i;
	goto jumpin;		/*Show that to a Pascal coder and he falls down dead.. */
    }
    for (i = 0, iptr = infotable; (i < __svgalib_max_modes) && (forcein < 0); i++, iptr++) {
      jumpin:
	if ((iptr->xdim != wid) || (iptr->ydim != hei) || !(modemask & col2msk(iptr)))
	    continue;
	cmpvert = (mode->disp_cntl & 0x10) ? 2.0 * vert  : vert;
	if (verbose) {
	    printf("%4ux%4ux%2u: ",
		   wid, hei, mach32_log2(iptr));
	    printf("%3d MHz Clock, %6.3f KHz horz., %6.3f Hz vert.,\n\t%s, VFIFO(16-%d,24-%d)",
		   clock, horz, cmpvert,
	      (mode->disp_cntl & 0x10) ? "Interlaced" : "Non-Interlaced",
		   mode->vfifo16, mode->vfifo24);
	}
	if (iptr->xbytes * iptr->ydim > mach32_memory * 1024) {
	    if (verbose)
		puts(" (not enough memory)");
	    continue;
	}
	if (__svgalib_horizsync.max < OFF_ALLOWANCE(horz * 1000)) {
	    if (verbose)
		puts(" (rejected, hsync too high)");
	    continue;
	}
	if (__svgalib_horizsync.min > ADD_ALLOWANCE(horz * 1000)) {
	    if (verbose)
		puts(" (rejected, hsync too low)");
	    continue;
	}
	if (__svgalib_vertrefresh.max < OFF_ALLOWANCE(cmpvert)) {
	    if (verbose)
		puts(" (rejected, vsync too high)");
	    continue;
	}
	if (__svgalib_vertrefresh.min > ADD_ALLOWANCE(cmpvert)) {
	    if (verbose)
		puts(" (rejected, vsync too low)");
	    continue;
	}
	if ((mach32_modes[i] == NULL) || (forcein >= 0)) {
	    if (verbose)
		puts(" (accepted)");
	  fillin:
	    mach32_modes[i] = mode;
	    continue;
	}
	if ((mach32_modes[i]->disp_cntl ^ mode->disp_cntl) & 0x10) {
	    if (mode->disp_cntl & 0x10) {
		if (verbose)
		    puts(" (rejected, is interlaced)");
		continue;
	    } else {
		if (verbose)
		    puts(" (preferred, is non-interlaced)");
		goto fillin;
	    }
	}
	n_clock = mach32_clocks[0x1f & (mach32_modes[i]->clock_sel >> 2)];
	n_horz = (1e3 * n_clock) / (float) (mach32_modes[i]->h_total * 8 + 8);
	tmp = mach32_modes[i]->v_total;
	if (!(mach32_modes[i]->disp_cntl & 0x6))
	    tmp = ((tmp >> 2) & 0xfffe) | (tmp & 1);
	else
	    tmp = ((tmp >> 1) & 0xfffc) | (tmp & 3);
	tmp++;
	/* Note: both modes are either interlaced or not, hence we directly compare vsyncs */
	n_vert = (n_horz * 1000) / tmp;
	if (n_vert < vert) {
	    if (verbose)
		puts(" (higher V_SYNC preferred)");
	    goto fillin;
	}
	if (verbose)
	    puts(" (rejected, have a better one already)");
    }
}

static int mach32_modeavailable(int mode)
{
    if (mode >= __svgalib_max_modes)
	return 0;
    if (mach32_modes[mode] == NULL)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    return SVGADRV;
}

static void mach32_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    if (mode >= __svgalib_max_modes)
	return;
    modeinfo->flags |= HAVE_EXT_SET;
    modeinfo->haveblit = 0;

    if (mach32_modes[mode] == NULL)
	return;

    if (infotable[mode].bytesperpixel <= 2)
	modeinfo->haveblit = acc_supp;
    else if (emuimage & EMU_POSS) {	/* imageblt can be emulated with memory aperture */
	modeinfo->haveblit = acc_supp & HAVE_IMAGEBLIT;
    }
    modeinfo->flags |= (HAVE_RWPAGE | EXT_INFO_AVAILABLE);
    if (mach32_modes[mode]->disp_cntl & 0x10)
	modeinfo->flags |= IS_INTERLACED;

    if (modeinfo->colors == 256)
	modeinfo->linewidth_unit = 8;
    else if (modeinfo->colors == (1 << 24)) {
	modeinfo->linewidth_unit = (PIXALIGN * infotable[mode].bytesperpixel);
	if ((infotable[mode].bytesperpixel == 4) ||
#ifdef FAKEBGR
	    ((infotable[mode].bytesperpixel == 3) && (mach32_dac == MACH32_ATI6871)))
#else
	    ((infotable[mode].bytesperpixel == 3) && (mach32_dac == MACH32_BT481)))
#endif
	    modeinfo->flags |= RGB_MISORDERED;
    } else
	modeinfo->linewidth_unit = (PIXALIGN * 2);

    modeinfo->bytesperpixel = infotable[mode].bytesperpixel;
    modeinfo->linewidth = infotable[mode].xbytes;
    modeinfo->maxlogicalwidth = 0xff * 8 * modeinfo->bytesperpixel;

/* This is obsolete.. modeinfo->bytesperpixel should always be > 0 here */
    if (modeinfo->bytesperpixel > 0) {
	modeinfo->maxpixels = (mach32_memory * 1024) / modeinfo->bytesperpixel;
    } else {
	modeinfo->maxpixels = (mach32_memory * 1024);
    }

    modeinfo->startaddressrange = 0xfffffffc & (mach32_memory * 1024 - 1);

    modeinfo->chiptype = mach32_dac | (mach32_chiptype & (MACH32_APERTURE_4M | MACH32_APERTURE));
    if (mach32_chiptype & MACH32_APERTURE_4M)
	modeinfo->aperture_size = (4 << 10);
    else if (mach32_chiptype & MACH32_APERTURE)
	modeinfo->aperture_size = (1 << 10);
    else
	modeinfo->aperture_size = 0;
    modeinfo->memory = mach32_memory;
    if (modeinfo->aperture_size >= mach32_memory)
	modeinfo->flags |= CAPABLE_LINEAR;
    modeinfo->linear_aperture = (char *) mach32_aperture;
    modeinfo->set_aperture_page = mach32_setappage;
    modeinfo->extensions = (mach32_eeprom + 128);
}

static void mach32_blankadj(int adj)
{
    if(mach32_ast)
	return;
#ifdef DEBUG
    printf("mach32_blankadj(%d)\n", adj);
#endif
    outb(MISC_CTL + 1, (inb(R_MISC_CTL + 1) & 0xf0) | adj);
}

static int mach32_setmode(int mode, int previous)
{
    register const mode_entry *mptr;
    int type, clock, clock_intended, vfifo;
    unsigned short act_ge_conf;

    type = mach32_modeavailable(mode);
    if (!type)
	return 1;

#ifdef DEBUG
    printf("mach32_setmode: %d -> %d\n", previous, mode);
#endif

    if (mach32_modeavailable(previous) == SVGADRV)
	mach32_bltwait();	/* Ensure noone draws in the screen */

    mach32_accelstate = R_UNKNOWN;	/* Accel registers need to be reset */

#ifdef DEBUG
    puts("mach32_setmode: accel set.");
#endif

    if (type != SVGADRV) {
	int i;
	const unsigned char *ptr;
	unsigned int val;

#ifdef DEBUG
	printf("mach32_setmode: standard mode:%d\n", mode);
#endif

	__svgalib_mach32_driverspecs.emul = NULL;

	/*if imageblit works at all, it'll be this one: */
	__svgalib_mach32_driverspecs.imageblt = mach32_memimageblt;

	/*Set std dac-mode */
	mach32_set_dac(DAC_MODE8, 0, 0);

	/* Ensure we are in VGA-mode: */
	outw(CLOCK_SEL, (inw(CLOCK_SEL) & ~0x7f) | 0x10);	/* slow clock and disable
								   Mach CRT */
	outw(DISP_CNTL, mach32_disp_shadow | 0x40);	/* Mach32 CRT reset */

	/* Force all extended ati regs to std. vga mode. This is needed for
	   correct function when using non standard console text modes. */

	/* Stop vga sequencer: */
	outb(SEQ_I, 0x00);
	outb(SEQ_D, 0x00);

	/* Handle the important reg 0x38: clock-diff by from bits 0+1 svga_clock, no herc emul */

	outb(ATIPORT, ATISEL(0x38));
	val = inb(ATIPORT + 1);
	if (svga_clock >= 0)
	    val = (val & 0x3f) | ((svga_clock << 6) & 0xc0);
	outw(ATIPORT, ATISEL(0x38) | (val << 8));

	/* Handle the important reg 0x39: Select some clock */

	outb(ATIPORT, ATISEL(0x39));
	val = inb(ATIPORT + 1);
	if (svga_clock >= 0)
	    val = (val & 0xfc) | ((svga_clock >> 2) & 0x03);
	outw(ATIPORT, ATISEL(0x39) | (val << 8));

	/* Register 3e, reset several bits and set clock as needed */
	outb(ATIPORT, ATISEL(0x3e));
	val = inb(ATIPORT + 1) & 0x30;
	if (svga_clock >= 0)
	    val = (val & 0xef) | (svga_clock & 0x10);
	outw(ATIPORT, ATISEL(0x3e) | (val << 8));

	/* reset several remaining extended bits: */
	for (i = 0, ptr = mach32_ati_ext; i < (sizeof(mach32_ati_ext) / 2); i++, ptr += 2) {
	    outb(ATIPORT, ATISEL(*ptr));
	    val = inb(ATIPORT + 1) & ~ptr[1];
	    outw(ATIPORT, ATISEL(*ptr) | (val << 8));
	}

	/* sequencer is reenabled by vga setmode */
	return __svgalib_vga_driverspecs.setmode(mode, previous);
	/* vga_setmode ensured vga is notblanked... */
    } else {
	/* handle the selection of imageblit */
	if ((infotable[mode].colors != (1 << 24))	/* mode is supported by engine */
	    &&(!(emuimage & EMU_OVER))	/* and should not be overridden */
	    )
	    __svgalib_mach32_driverspecs.imageblt = mach32_imageblt;
	else
	    __svgalib_mach32_driverspecs.imageblt = mach32_memimageblt;
    }

    __svgalib_mach32_driverspecs.emul = &mach32_vgaemul;

#ifdef DEBUG
    printf("mach32_setmode: Setting vga in 8bpp graphmode.\n");
#endif

/*Ripped out from Xfree.. mach32 Server.. */
    outw(SEQ_I, 4 | (0x0a << 8));
/* Enable write access to all memory maps */
    outw(SEQ_I, 2 | (0x0f << 8));
/* Set linear addressing mode */
    outw(ATIPORT, ATISEL(0x36) | (0x05 << 8));
/* Set the VGA display buffer to 0xa0000 */
    outw(GRA_I, 6 | (0x05 << 8));
/*These are mine...someone turned the VGA to erase on write....(Eeeek!) */
    outw(GRA_I, 1 | (0x00 << 8));
    outw(GRA_I, 3 | (0x00 << 8));
    outw(GRA_I, 5 | (0x00 << 8));
    outw(GRA_I, 7 | (0x00 << 8));
    outw(GRA_I, 8 | (0xff << 8));
/*Switch on any videomemory latches we have: */
    outw(MISC_OPTIONS, (inw(MISC_OPTIONS) & 0x8f7f) | (latchopt & 0x7080));
#ifdef DEBUG
    printf("mach32_setmode: extended mode:%d\n", mode);
#endif

    mptr = mach32_modes[mode];

    mach32_disp_shadow = mptr->disp_cntl & 0x7f;
    outw(DISP_CNTL, mach32_disp_shadow | 0x60);		/* Mach32 CRT reset */
    outw(MULTI_FUNC_CNTL, 0x5006);	/* Linear memory layout */
/*set dac: */

    clock_intended = mptr->clock_sel;
    clock = mach32_clocks[0x1f & (clock_intended >> 2)];

    switch (infotable[mode].colors) {
    default:			/* 256 */
	clock_intended = mach32_set_dac((clock > 80) ? DAC_MODEMUX : DAC_MODE8,
				   clock_intended, infotable[mode].xdim);
	act_ge_conf = inw(R_EXT_GE_CONF) & 0xf80f;
	if (clock > 80)
	    act_ge_conf |= 0x0110;
	else
	    act_ge_conf |= 0x0010;
	/* Force VFIFO in clock_sel to zero: */
	clock_intended &= 0xf0ff;
	clock_intended |= (vfifo8 << 8);
	break;
    case 1 << 15:
	clock_intended = mach32_set_dac(DAC_MODE555,
		    dac14_clock_adjust(clock_intended, mach32_clock_by2),
					infotable[mode].xdim);
	act_ge_conf = inw(R_EXT_GE_CONF) & 0xf80f;
	act_ge_conf |= 0x0020;
	/* Force VFIFO in clock_sel to zero: */
	clock_intended &= 0xf0ff;
	vfifo = max(mptr->vfifo16, mach32_max_vfifo16(infotable[mode].xdim));
	clock_intended |= (vfifo << 8);
	break;
    case 1 << 16:
	clock_intended = mach32_set_dac(DAC_MODE565,
		    dac14_clock_adjust(clock_intended, mach32_clock_by2),
					infotable[mode].xdim);
	act_ge_conf = inw(R_EXT_GE_CONF) & 0xf80f;
	act_ge_conf |= 0x0060;
	/* Force VFIFO in clock_sel to zero: */
	clock_intended &= 0xf0ff;
	vfifo = max(mptr->vfifo16, mach32_max_vfifo16(infotable[mode].xdim));
	clock_intended |= (vfifo << 8);
	break;
    case 1 << 24:
	clock_intended = mach32_set_dac(
	(infotable[mode].bytesperpixel == 3) ? DAC_MODERGB : DAC_MODE32B,
		    dac14_clock_adjust(clock_intended, mach32_clock_by3),
					   infotable[mode].xdim);
	act_ge_conf = inw(R_EXT_GE_CONF) & 0xf80f;
	if (infotable[mode].bytesperpixel == 3) {
	    act_ge_conf |= 0x0030;
#ifdef FAKEBGR
	    if (mach32_dac == MACH32_ATI6871)
		act_ge_conf |= 0x0400;
#endif
	    vfifo = max(mptr->vfifo24, vfifo24);
	}
#ifdef SUPP_32BPP
	else {
#ifdef USE_RGBa
	    act_ge_conf |= 0x0630;
#else
	    act_ge_conf |= 0x0230;
#endif
	    vfifo = max(mptr->vfifo24, vfifo32);
	}
#endif
	/* Force VFIFO in clock_sel to zero: */
	clock_intended &= 0xf0ff;
	clock_intended |= (vfifo << 8);
	break;
    }
    outw(EXT_GE_CONF, act_ge_conf);
/*Set clock and disable VGA... those pinheads from ATI have the clock
   settings in all their tables with the VGA enabled....Gee.. */
    clock_intended |= 1;

#ifdef DEBUG
    printf("GE_CONF:%hx\n", act_ge_conf);
    printf("Setting mode:\th_disp: %x\th_total: %x\th_sync_wid: %x\th_sync_strt: %x\t"
	   "v_disp: %x\tv_total: %x\tv_sync_wid: %x\tv_sync_strt: %x\tclock_sel: %x\tdisp_ctl: %x\n",
	   mptr->h_disp, mptr->h_total, mptr->h_sync_wid, mptr->h_sync_strt, mptr->v_disp, mptr->v_total,
	   mptr->v_sync_wid, mptr->v_sync_strt, clock_intended, mach32_disp_shadow);
#endif
    outb(H_DISP, mptr->h_disp);
    outb(H_TOTAL, mptr->h_total);
    outb(H_SYNC_STRT, mptr->h_sync_strt);
    outb(H_SYNC_WID, mptr->h_sync_wid);
    outw(V_TOTAL, mptr->v_total);
    outw(V_DISP, mptr->v_disp);
    outw(V_SYNC_STRT, mptr->v_sync_strt);
    outw(V_SYNC_WID, mptr->v_sync_wid);

#ifdef DEBUG
    outw(CLOCK_SEL, clock_intended);
#else
    outw(CLOCK_SEL, clock_intended & ~1);	/*Screen does not show up yet..
						   will be fired up by final vga_screenon
						   called from setmode (still time to
						   erase the screen.. ;) */
#endif

    mach32_setlogicalwidth(infotable[mode].xbytes);
    mach32_setdisplaystart(0);
    mach32_setappage(0);	/* This sets up a requested mem aperture too.. */

    outw(DISP_CNTL, mach32_disp_shadow | 0x20);		/* Mach32 CRT enable */

#if defined(DEBUG_KEY)&&defined(DEBUG)
    fputs("\n(Hit Return)", stdout);
    while (getchar() != '\n');
#endif
    return 0;
}

static short *
 push(int num)
{
    short *ptr;

    if (mixup_ptr + num >= mixup_alloc) {
	if (mach32_modemixup == NULL)
	    mach32_modemixup = malloc(PREALLOC * sizeof(short));
	else
	    mach32_modemixup = realloc(mach32_modemixup, (mixup_alloc += PREALLOC) * sizeof(short));
	if (mach32_modemixup == NULL) {
	    puts("mach32-config: Fatal error: Out of memory.");
	    exit(-1);
	}
    }
    ptr = mach32_modemixup + mixup_ptr;
    mixup_ptr += num;
    return ptr;
}


#ifndef OSKIT
static int isnumber(char *str)
{
    if (str == NULL)
	return 0;
    return strlen(str) == strspn(str, "0123456789");
}
#endif

static int parsemode(char *str, int linelength, int create)
{
    char tmpstr[6], *ptr;
    int i;
    unsigned width, height, colors, bytesperpixel;

    if (!str)
	return -1;
    for (ptr = str; *ptr; ptr++)
	*ptr = tolower(*ptr);
    if (sscanf(str, "%ux%ux%5s", &width, &height, tmpstr) != 3)
	return -1;
    if (!strcmp(tmpstr, "256")) {
	colors = 256;
	bytesperpixel = 1;
    } else if (!strcmp(tmpstr, "32k")) {
	colors = (1 << 15);
	bytesperpixel = 2;
    } else if (!strcmp(tmpstr, "64k")) {
	colors = (1 << 16);
	bytesperpixel = 2;
    } else if (!strcmp(tmpstr, "16m")) {
	colors = (1 << 24);
	bytesperpixel = 3;
    }
#ifdef SUPP_32BPP
    else if (!strcmp(tmpstr, "16m32")) {
	colors = (1 << 24);
	bytesperpixel = 4;
    } else if (!strcmp(tmpstr, "16m4")) {	/* compatibility */
	colors = (1 << 24);
	bytesperpixel = 4;
    }
#endif
    else {
	printf("mach32-config: mode %s unsupported, only 256, 32K, 64K, 16M, 16M32 colors allowed.\n", tmpstr);
	return -2;
    }
    width = (width + 7) & ~7;
    if (linelength < width)
	linelength = width;
    else
	linelength = (linelength + 7) & ~7;

    linelength *= bytesperpixel;

    for (i = 0; i < __svgalib_max_modes; i++) {
	if ((infotable[i].xdim == width) && (infotable[i].ydim == height) &&
	    (infotable[i].colors == colors) && (infotable[i].bytesperpixel == bytesperpixel)) {
	    infotable[i].xbytes = linelength;
	    return i;
	}
    }
    if (!create)
	return -2;
    if ((i = __svgalib_addmode(width, height, colors, linelength, bytesperpixel)) < 0) {
	printf("mach32-config: no more dynamic modes, %s ignored.\n", tmpstr);
	return -2;
    }
    return i;
}

static inline unsigned int mach32_skip2(unsigned int vtime)
{
    return ((vtime << 1) & 0xfff8) | (vtime & 3) | ((vtime & 0x80) >> 5);
}

static char *mach32_conf_commands[] =
{
    "mach32eeprom", "inhibit", "define", "setlinelength", "maxclock16",
    "maxclock24", "clocks", "variablelinelength", "strictlinelength", "duplicatelinelength",
    "maxclock8", "maxclock32", "verbose", "quiet", "vfifo8",
    "latch", "blank", "vfifo16", "vfifo24", "vfifo32",
    "setuplinear", "blit", "ramdac", "svgaclock", "vendor", "misc_ctl",
    NULL};

static char *
 mach32_procopt(int command, int mode)
{
    mode_entry *mptr;
    char *ptr;
    int i, currptr, flag;

    switch (command) {
    case 0:
	{
	    int position = 0;	/* We are handling the POSITION-th parameter */

	    if (!mode) {
	      access_denied:
		printf("mach32-config: %s command access denied.",
		       mach32_conf_commands[command]);
		break;
	    }
	    for (;;) {
		position++;
		ptr = strtok(NULL, " ");
		if (ptr == NULL) {
		    if (position <= 1)
			puts("mach32-config: mach32eeprom "
			     "command expects a parameter.");
		    return ptr;
		}
		if (!strcasecmp(ptr, "compatible")) {
		    eeprom_option |= EEPROM_USE_CHKSUM |
			EEPROM_USE_MEMCFG | EEPROM_USE_TIMING;
		} else if (!strcasecmp(ptr, "ignore")) {
		    eeprom_option &= ~(EEPROM_USE_CHKSUM |
				  EEPROM_USE_MEMCFG | EEPROM_USE_TIMING);
		} else if (!strcasecmp(ptr, "useaperture")) {
		    eeprom_option |= EEPROM_USE_MEMCFG;
		} else if (!strcasecmp(ptr, "usetimings")) {
		    eeprom_option |= EEPROM_USE_TIMING;
		} else if (!strcasecmp(ptr, "nofile")) {
		    eeprom_fname = NULL;
		} else if (!strcasecmp(ptr, "updatefile")) {
		    eeprom_option |= EEPROM_UPDATE;
		} else if (!strcasecmp(ptr, "keepfile")) {
		    eeprom_option &= ~EEPROM_UPDATE;
		} else if (!strcasecmp(ptr, "file")) {
		    ptr = strtok(NULL, " ");
		    if (ptr == NULL) {
			puts("mach32-config: mach32eeprom file subcommand "
			     "expects a parameter.");
			return ptr;
		    }
		    goto fileparam;
		} else if (position == 1) {
		  fileparam:
		    if (*ptr == '/') {
			if (eeprom_fname != NULL)
			    free(eeprom_fname);
			eeprom_fname = strdup(ptr);
			if (eeprom_fname == NULL) {
			    puts("mach32-config: Fatal error: out of memory.");
			    exit(-1);
			}
		    } else {
			puts("mach32-config: mach32eeprom: filename has "
			     "to start with a / !!");
		    }
		} else
		    return ptr;
	    }
	}
	break;
    case 1:
	for (;;) {
	    ptr = strtok(NULL, " ");
	    i = parsemode(ptr, 0, 0);
	    if (i == -1)
		return ptr;
	    if (i >= 0)
		*push(1) = CMD_DEL + i;
	}
	break;
    case 2:			/* Define a new mode.. */
	currptr = mixup_ptr;
	flag = 0;
	for (;;) {
	    ptr = strtok(NULL, " ");
	    i = parsemode(ptr, 0, 1);
	    if (i == -1)
		break;
	    if (i >= 0) {
		*push(1) = CMD_ADD + i;
		flag = 1;
	    }
	}
	if (!flag) {
	    puts("mach32-config: Invalid define command, no valid vga-mode given");
	  ex_inv_mod:
	    mixup_ptr = currptr;
	    return ptr;
	}
	*push(1) = CMD_MOD;
	mptr = (mode_entry *) push(SOMOD_SH);
	/* Read the mode in, this is almost like Xconfig.. */
	/*The clock is the only thing that may differ: */
	if (ptr == NULL) {
	  inv_clk:
	    puts("mach32-config: Invalid define command, clock is invalid");
	    goto ex_inv_mod;
	}
	if (*ptr == ':') {	/*No. of clock given */
	    if (!isnumber(ptr + 1))
		goto inv_clk;
	    i = atoi(ptr + 1);
	} else {		/* search clock in table */
	    if (!isnumber(ptr))
		goto inv_clk;
	    flag = atoi(ptr);
	    for (i = 0; i < 32; i++)
		if (flag == mach32_clocks[i])
		    break;
	}
	if (i > 31)
	    goto inv_clk;
	mptr->clock_sel = (i << 2);
#ifdef DEBUG
	printf("Constructed clock_sel is: %d\n", mptr->clock_sel);
#endif
	mptr->disp_cntl = 0x23;	/* Assume non interlaced */
	/* The rest is straight forward: */
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr)) {
	  inv_time:
	    puts("mach32-config: Invalid define command, timing is invalid");
	    goto ex_inv_mod;
	}
	mptr->h_disp = (atoi(ptr) >> 3) - 1;
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	i = atoi(ptr);
	mptr->h_sync_strt = (i >> 3) - 1;
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	mptr->h_sync_wid = ((atoi(ptr) - i) >> 3);
	if (mptr->h_sync_wid > 0x1f)
	    mptr->h_sync_wid = 0x1f;
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	mptr->h_total = (atoi(ptr) >> 3) - 1;

	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	mptr->v_disp = mach32_skip2(atoi(ptr) - 1);
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	i = atoi(ptr);
	mptr->v_sync_strt = mach32_skip2(i - 1);
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	mptr->v_sync_wid = atoi(ptr) - i;
	if (mptr->v_sync_wid > 0x1f)
	    mptr->v_sync_wid = 0x1f;
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr))
	    goto inv_time;
	mptr->v_total = mach32_skip2(atoi(ptr) - 1);
	for (;;) {		/* Parse for additional goodies */
	    ptr = strtok(NULL, " ");
	    if (!ptr)
		break;
	    if (!strcasecmp(ptr, "interlace"))
		mptr->disp_cntl |= 0x10;
	    else if (!strcasecmp(ptr, "+hsync"))
		mptr->h_sync_wid &= ~0x20;
	    else if (!strcasecmp(ptr, "-hsync"))
		mptr->h_sync_wid |= 0x20;
	    else if (!strcasecmp(ptr, "+vsync"))
		mptr->v_sync_wid &= ~0x20;
	    else if (!strcasecmp(ptr, "-vsync"))
		mptr->v_sync_wid |= 0x20;
	    else
		break;
	}
	/*final fixup */
	if (mptr->h_disp > 0x4f) {	/* x>640 */
	    mptr->vfifo24 = 0xe;
	    if (mptr->h_disp > 0x7f)
		mptr->vfifo16 = 0xe;
	    else if (mptr->h_disp > 0x63)
		mptr->vfifo16 = 0xd;
	    else
		mptr->vfifo16 = 0x9;
	} else {
	    mptr->vfifo16 = 0;
	    mptr->vfifo24 = 0;
	}
	return ptr;
    case 3:
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr)) {
	    puts("mach32-config: illegal setlinelength command.\n"
		 "Usage: setlinelength integer modes...");
	    return ptr;
	}
	i = atoi(ptr);
	do
	    ptr = strtok(NULL, " ");
	while (parsemode(ptr, i, 0) != -1);
	return ptr;
    case 4:
	ptr = strtok(NULL, " ");

	if (!mode) {
	  maxclk_deny:
	    puts("Don't use the maxclock's commands out of the environment variable.");
	    return ptr;
	}
	if (!isnumber(ptr)) {
	  ilmaxclk:
	    puts("mach32-config: illegal maxclock16 or maxclock24 command.\n"
		 "Usage: maxclock16 integer or maxclock24 integer");
	    return ptr;
	}
	mach32_maxclk16 = atoi(ptr);
	break;
    case 5:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto maxclk_deny;
	if (!isnumber(ptr))
	    goto ilmaxclk;
	mach32_maxclk24 = atoi(ptr);
	break;
    case 6:
	if (!mode)
	    goto access_denied;
	for (i = 0; i < 16; i++) {
	    ptr = strtok(NULL, " ");
	    clocks_set = 1;
	    if (!isnumber(ptr)) {
		puts("mach32-config: illegal clocks command.\n"
		     "Usage: clocks integer integer ...\n"
		     "16 clocks have to be specified.\n"
		     "specify 0 for unsupported clocks\n"
		     "(using 0's for the mistyped clocks)");
		if (mode) {
		    while (i < 16)
			mach32_clocks[i++] = 0;
		    /*Set halfed clocks: */
		    for (i = 0; i < 16; i++)
			mach32_clocks[i + 16] = mach32_clocks[i] / 2;
		}
		return ptr;
	    }
	    if (mode)
		mach32_clocks[i] = atoi(ptr);
	}
	/*Set halfed clocks: */
	for (i = 0; i < 16; i++)
	    mach32_clocks[i + 16] = mach32_clocks[i] / 2;
	break;
    case 7:
	mach32_strictness = 0;
	break;
    case 8:
	mach32_strictness = 1;
	break;
    case 9:
	mach32_strictness = 2;
	break;
    case 10:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto maxclk_deny;
	if (!isnumber(ptr))
	    goto ilmaxclk;
	mach32_maxclk8 = atoi(ptr);
	break;
    case 11:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto maxclk_deny;
	if (!isnumber(ptr))
	    goto ilmaxclk;
	mach32_maxclk32 = atoi(ptr);
	break;
    case 12:
	verbose = 1;
	break;
    case 13:
#ifndef DEBUG
	verbose = 0;
#endif
	break;
    case 14:
	ptr = strtok(NULL, " ");
	if (!mode) {
	  tweak_deny:
	    puts("The vfifo, latch, blank commands are not allowed out of the environment.");
	    return ptr;
	}
	if (!isnumber(ptr)) {
	  ilvfi:
	    puts("Illegal vfifo command");
	    return ptr;
	}
	vfifo8 = atoi(ptr) & 0xf;
	break;
    case 15:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto tweak_deny;
	if (!isnumber(ptr)) {
	    puts("Illegal latch command");
	    return ptr;
	}
	latchopt = atoi(ptr);
	break;
    case 16:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto tweak_deny;
	if (!isnumber(ptr)) {
	    puts("Illegal blank command");
	    return ptr;
	}
	bladj = atoi(ptr) & 0xf;
	break;
    case 17:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto tweak_deny;
	if (!isnumber(ptr))
	    goto ilvfi;
	vfifo16 = atoi(ptr) & 0xf;
	break;
    case 18:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto tweak_deny;
	if (!isnumber(ptr))
	    goto ilvfi;
	vfifo24 = atoi(ptr) & 0xf;
	break;
    case 19:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto tweak_deny;
	if (!isnumber(ptr))
	    goto ilvfi;
	vfifo32 = atoi(ptr) & 0xf;
	break;
    case 20:
	ptr = strtok(NULL, " ");
	if (!isnumber(ptr)) {
	  ilsetupli:
	    puts("Illegal setuplinear command.\n"
		 "usage: setuplinear address size\n"
		 "where size = 1 or 4 and both are in MB");
	    return ptr;
	}
	i = atoi(ptr);
	ptr = strtok(NULL, " ");
	if (!strcmp(ptr, "4"))
	    flag = 1;
	else if (!strcmp(ptr, "1"))
	    flag = 0;
	else if (!strcmp(ptr, "0")) {
	    flag = 0;
	    i = 0;
	    goto setuplin;
	} else
	    goto ilsetupli;
	if ((i & ~0xfff) || (!i)) {
	    puts("setuplinear: address out of range");
	    return ptr;
	}
      setuplin:
	if (!mode) {
	    puts("setuplinear config option strictly disallowed from the environment.");
	    return ptr;
	}
	mach32_apsiz = flag + 1;
	mach32_apadd = i;
	break;
    case 21:
	for (;;) {
	    ptr = strtok(NULL, " ");
	    if (!ptr)
		break;
	    if (!strcasecmp(ptr, "bit"))
		acc_supp |= HAVE_BITBLIT;
	    else if (!strcasecmp(ptr, "nobit"))
		acc_supp &= ~HAVE_BITBLIT;
	    else if (!strcasecmp(ptr, "fill"))
		acc_supp |= HAVE_FILLBLIT;
	    else if (!strcasecmp(ptr, "nofill"))
		acc_supp &= ~HAVE_FILLBLIT;
	    else if (!strcasecmp(ptr, "image")) {
		acc_supp |= HAVE_IMAGEBLIT;
		emuimage &= ~EMU_OVER;
	    } else if (!strcasecmp(ptr, "noimage")) {
		acc_supp &= ~HAVE_IMAGEBLIT;
	    } else if (!strcasecmp(ptr, "memimage")) {
		acc_supp |= HAVE_IMAGEBLIT;
		emuimage |= EMU_OVER | EMU_POSS;
	    } else if (!strcasecmp(ptr, "nomemimage")) {
		emuimage &= ~(EMU_OVER | EMU_POSS);
	    } else if (!strcasecmp(ptr, "hlinelist")) {
		acc_supp |= HAVE_HLINELISTBLIT;
	    } else if (!strcasecmp(ptr, "nohlinelist")) {
		acc_supp &= ~HAVE_HLINELISTBLIT;
	    } else if (!strcasecmp(ptr, "settrans")) {
		accel_supp |= ACCELFLAG_SETTRANSPARENCY;
	    } else if (!strcasecmp(ptr, "nosettrans")) {
		accel_supp &= ~ACCELFLAG_SETTRANSPARENCY;
	    } else if (!strcasecmp(ptr, "setrop")) {
		accel_supp |= ACCELFLAG_SETRASTEROP;
	    } else if (!strcasecmp(ptr, "nosetrop")) {
		accel_supp &= ~ACCELFLAG_SETRASTEROP;
	    } else if (!strcasecmp(ptr, "fillbox")) {
		accel_supp |= ACCELFLAG_FILLBOX;
	    } else if (!strcasecmp(ptr, "nofillbox")) {
		accel_supp &= ~ACCELFLAG_FILLBOX;
	    } else if (!strcasecmp(ptr, "screencopy")) {
		accel_supp |= ACCELFLAG_SCREENCOPY;
	    } else if (!strcasecmp(ptr, "noscreencopy")) {
		accel_supp &= ~ACCELFLAG_SCREENCOPY;
	    } else if (!strcasecmp(ptr, "drawline")) {
		accel_supp |= ACCELFLAG_DRAWLINE;
	    } else if (!strcasecmp(ptr, "nodrawline")) {
		accel_supp &= ~ACCELFLAG_DRAWLINE;
	    } else if (!strcasecmp(ptr, "putimage")) {
		accel_supp |= ACCELFLAG_PUTIMAGE;
		emuimage &= ~EMU_OVER;
	    } else if (!strcasecmp(ptr, "noputimage")) {
		accel_supp &= ~ACCELFLAG_PUTIMAGE;
	    } else if (!strcasecmp(ptr, "drawhlinelist")) {
		accel_supp |= ACCELFLAG_DRAWHLINELIST;
	    } else if (!strcasecmp(ptr, "nodrawhlinelist")) {
		accel_supp &= ~ACCELFLAG_DRAWHLINELIST;
	    } else if (!strcasecmp(ptr, "putbitmap")) {
		accel_supp |= ACCELFLAG_PUTBITMAP;
	    } else if (!strcasecmp(ptr, "noputbitmap")) {
		accel_supp &= ~ACCELFLAG_PUTBITMAP;
	    } else if (!strcasecmp(ptr, "screencopymono")) {
		accel_supp |= ACCELFLAG_SCREENCOPYMONO;
	    } else if (!strcasecmp(ptr, "noscreencopymono")) {
		accel_supp &= ~ACCELFLAG_SCREENCOPYMONO;
	    } else if (!strcasecmp(ptr, "setmode")) {
		accel_supp |= ACCELFLAG_SETMODE | ACCELFLAG_SYNC;
	    } else if (!strcasecmp(ptr, "nosetmode")) {
		accel_supp &= ~(ACCELFLAG_SETMODE | ACCELFLAG_SYNC);
	    } else if (!strcasecmp(ptr, "polyline")) {
		accel_supp |= ACCELFLAG_POLYLINE;
	    } else if (!strcasecmp(ptr, "nopolyline")) {
		accel_supp &= ~ACCELFLAG_POLYLINE;
	    } else if (!strcasecmp(ptr, "polyhline")) {
		accel_supp |= ACCELFLAG_POLYHLINE;
	    } else if (!strcasecmp(ptr, "nopolyhline")) {
		accel_supp &= ~ACCELFLAG_POLYHLINE;
	    } else if (!strcasecmp(ptr, "polyfillmode")) {
		accel_supp |= ACCELFLAG_POLYFILLMODE;
	    } else if (!strcasecmp(ptr, "nopolyfillmode")) {
		accel_supp &= ~ACCELFLAG_POLYFILLMODE;
	    } else
		return ptr;
	}
	break;
    case 22:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto access_denied;
	if (!ptr)
	    goto invpar;
	else if (!strcasecmp(ptr, "auto"))
	    dac_override = 127;
	else if (!strcasecmp(ptr, "dumb"))
	    dac_override = MACH32_BT476;
	else if ((ptr[0] >= '0') && (ptr[0] <= '5') && (ptr[1] == 0))
	    dac_override = *ptr - '0';
	else {
	  invpar:
	    printf("Invalid parameter:  \"%s %s\"\n",
		   mach32_conf_commands[command], ptr);
	}
	break;
    case 23:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto access_denied;
	if (!ptr)
	    goto invpar;
	else if (!strcasecmp(ptr, "keep"))
	    svga_clock = (-1);
	else {
	    if (!isnumber(ptr))
		goto invpar;
	    i = atoi(ptr);
	    if ((i < 0) || (i > 0x1f))
		goto invpar;
	    svga_clock = i;
	}
	break;
    case 24:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto access_denied;
	if (!ptr)
	    goto invpar;
	else if (!strcasecmp(ptr, "ati")) {
	    dac_override = 127;
	    svga_clock = SVGA_CLOCK;
	    eeprom_option |= EEPROM_USE_CHKSUM |
		EEPROM_USE_MEMCFG | EEPROM_USE_TIMING;
	    mach32_ast = 0;
	} else if (!strcasecmp(ptr, "dell")) {
	    dac_override = MACH32_BT476;
	    svga_clock = 0x00;
	    mach32_ast = 0;
	    eeprom_option &= ~(EEPROM_USE_CHKSUM |
			       EEPROM_USE_MEMCFG | EEPROM_USE_TIMING);
	} else if (!strcasecmp(ptr, "ast")) {
	    dac_override = 127;
	    svga_clock = SVGA_CLOCK;
	    mach32_ast = 1;
	    eeprom_option &= ~(EEPROM_USE_CHKSUM |
			       EEPROM_USE_MEMCFG | EEPROM_USE_TIMING);
	} else
	    goto invpar;
	break;
    case 25:
	ptr = strtok(NULL, " ");
	if (!mode)
	    goto access_denied;
	if (!ptr)
	    goto invpar;
	else if (!strcasecmp(ptr, "keep-off")) {
	    mach32_ast = 1;
	} else if (!strcasecmp(ptr, "use")) {
	    mach32_ast = 0;
	} else
	    goto invpar;
	break;
    }
    return strtok(NULL, " ");
}

static void mach32_readconfig(void)
{
    static int conf_read = 0;
    int i, j, limit, newxbytes;

    if (conf_read)
	return;
    conf_read = 1;

/*Hack info table.. see comment at start of file */
    for (i = 0; i < __svgalib_max_modes; i++) {
	if (infotable[i].bytesperpixel < 2)
	    continue;		/* Not needed for those.. also excludes STDVGA non MACH32 modes */
	j = infotable[i].xdim % PIXALIGN;
	if (j)
	    infotable[i].xbytes += infotable[i].bytesperpixel * (PIXALIGN - j);
    }
    __svgalib_read_options(mach32_conf_commands, mach32_procopt);
    if (mach32_strictness) {	/* Do something about the modes where we redefined xbytes.. */
	limit = GLASTMODE;
	for (i = G640x480x256; i < limit; i++) {	/* leave the non SVGA modes alone */
	    if (!col2bypp(infotable + i))
		continue;	/* Don't deal with non mach32-modes */
	    if (infotable[i].xbytes != infotable[i].xdim * col2bypp(infotable + i)) {	/* mode was redefined */
		newxbytes = infotable[i].xbytes;
		/*Let infotable[i] look like a virgin again: */
		infotable[i].xbytes = infotable[i].xdim * col2bypp(infotable + i);
		j = __svgalib_addmode(infotable[i].xdim, infotable[i].ydim, infotable[i].colors,
				  newxbytes, infotable[i].bytesperpixel);
		if (j < 0) {
		    puts("mach32: Out of dynamic modes for redefinition of modes with\n"
			 "        non-standard linelength. Use inhibit in case of problems.\n");
		    goto giveup;
		}
		if (mach32_strictness == 1)
		    *push(1) = CMD_MOV + i;	/* later move this mode up to the newly defined.. */
		else
		    *push(1) = CMD_CPY + i;	/* Nope, copy instead of move.. */
		*push(1) = j;	/* The destination mode. */
#if 1
		printf("Redefining mode %d to %d:%dx%dx%d\n", i, j,
		       infotable[i].xdim, infotable[i].ydim, infotable[i].colors);
#endif
	    }
	}
    }
  giveup:
    *push(1) = CMD_MSK;		/* End of fixup commands */
    mach32_modemixup = realloc(mach32_modemixup, mixup_ptr * sizeof(short));
    if (mach32_modemixup == NULL) {
	puts("mach32-config: Fatal: queue shrink failed.");
	exit(-1);
    }
}

static char *
 colstr(struct info *mode)
{
    static char str[4];
    if (mode->colors <= 256) {
	sprintf(str, "%d", mode->colors);
	return str;
    }
    switch (mode->colors) {
    case 1 << 15:
	return "32K";
    case 1 << 16:
	return "64K";
    case 1 << 24:
	if (mode->bytesperpixel == 3)
	    return "16M";
	if (mode->bytesperpixel == 4)
	    return "16M32";
    }
    return "?";
}

static void mach32_final_modefixup(void)
{
    int i;
    const mode_entry *oldentry;
    short *ptr, *ptm;


    for (ptr = mach32_modemixup;; ptr++)
	switch (*ptr & CMD_MSK) {
	case CMD_MSK:		/* end marker.. */
	    return;
	case CMD_DEL:		/* any mode found here is invalid. */
	    mach32_modes[*ptr & ~CMD_MSK] = NULL;
	    break;
	case CMD_MOD:		/* skip embedded mode table */
	    ptr += SOMOD_SH;
	    break;
	case CMD_MOV:		/* move from this position to the one in next command short. */
	    i = (*ptr++) & ~CMD_MSK;
	    mach32_modes[*ptr & ~CMD_MSK] = mach32_modes[i];
	    /* erase old entry... */
	    mach32_modes[i] = NULL;
	    break;
	case CMD_CPY:		/*Same as above, but without erase.. */
	    i = (*ptr++) & ~CMD_MSK;
	    mach32_modes[*ptr & ~CMD_MSK] = mach32_modes[i];
	    break;
	case CMD_ADD:		/* Add next mode in supplied position.. */
	    /*First find the mode table.. */
	    ptm = ptr + 1;
	    while ((*ptm & CMD_MSK) != CMD_MOD)
		ptm++;
	    /* one more to skip modemarker.. */
	    ptm++;
	    /*Override any entry we had already for this mode..  (if the mode is acceptable) */
	    i = *ptr & ~CMD_MSK;
	    oldentry = mach32_modes[i];
#ifdef DEBUG
	    puts("Calling mach32_modfill to add new mode.");
#endif
	    mach32_modfill((mode_entry *) ptm, col2msk(infotable + i), i);
	    if (mach32_modes[i] == oldentry) {
		printf("mach32: Setting of mode %dx%dx%s failed.\n"
		       "Mode cannot be realized with your hardware\n"
		     "(or due to configured restrictions ;-) ), sorry.\n"
		       "specify verbose in config file or SVGALIB_CONFIG to get\n"
		       "detailed information\n",
		       infotable[i].xdim, infotable[i].ydim, colstr(infotable + i));
	    }
	    break;
	}
}

static int mach32_canaccel(void)
{
    if (CM >= __svgalib_max_modes)
	return 0;
    if (mach32_modes[CM] == NULL)
	return 0;
    return (infotable[CM].bytesperpixel <= 2);
}

static int mach32_ext_set(unsigned what, va_list params)
{
    int param2, old_values, retval;

    switch (what) {
    case VGA_EXT_AVAILABLE:
	param2 = va_arg(params, int);
	switch (param2) {
	case VGA_AVAIL_SET:
	    return VGA_EXT_AVAILABLE | VGA_EXT_SET | VGA_EXT_CLEAR | VGA_EXT_RESET;
	case VGA_AVAIL_ACCEL:
	    if (mach32_canaccel()) {
	    	retval = ACCELFLAG_SETFGCOLOR | ACCELFLAG_SETBGCOLOR |
		    ACCELFLAG_SETTRANSPARENCY | ACCELFLAG_SETRASTEROP |
		    ACCELFLAG_FILLBOX | ACCELFLAG_SCREENCOPY |
		    ACCELFLAG_DRAWLINE | ACCELFLAG_PUTIMAGE |
		    ACCELFLAG_DRAWHLINELIST | ACCELFLAG_PUTBITMAP |
		    ACCELFLAG_SCREENCOPYMONO | ACCELFLAG_SETMODE |
		    ACCELFLAG_POLYLINE | ACCELFLAG_POLYHLINE |
		    ACCELFLAG_POLYFILLMODE | ACCELFLAG_SYNC;
		retval = accel_supp & retval;
		if (! (retval & ~(ACCELFLAG_SETFGCOLOR | ACCELFLAG_SETBGCOLOR |
				  ACCELFLAG_SETTRANSPARENCY | ACCELFLAG_SETRASTEROP |
				  ACCELFLAG_POLYFILLMODE | ACCELFLAG_SYNC |
				  ACCELFLAG_SETMODE)) )
		     return 0; /* No real operations left at all */
		else
		     return retval;
	    } else if ((emuimage & EMU_POSS) && (CM < __svgalib_max_modes) &&
		       (mach32_modes[CM] != NULL)) {
		return ACCELFLAG_PUTIMAGE; /* putimage can be emulated with memory aperture */
	    } else
		return 0;
	case VGA_AVAIL_FLAGS:
	    if (((mach32_dacmode & 0x7f) == DAC_MODE8) ||
		((mach32_dacmode & 0x7f) == DAC_MODEMUX))
		return pos_ext_settings;
	    else
		return pos_ext_settings & ~VGA_CLUT8;
	case VGA_AVAIL_ROP:
	    if (mach32_canaccel() && (accel_supp & ACCELFLAG_SETRASTEROP)) {
	    	retval = ACCELFLAG_FILLBOX | ACCELFLAG_SCREENCOPY |
		    ACCELFLAG_PUTIMAGE | ACCELFLAG_DRAWLINE |
		    ACCELFLAG_SETFGCOLOR | ACCELFLAG_SETBGCOLOR |
		    ACCELFLAG_PUTBITMAP | ACCELFLAG_SCREENCOPYMONO |
		    ACCELFLAG_POLYLINE | ACCELFLAG_POLYHLINE |
		    ACCELFLAG_POLYFILLMODE | ACCELFLAG_DRAWHLINELIST;
		return accel_supp & retval;
	    } else
		return 0;
	case VGA_AVAIL_TRANSPARENCY:
	    if (mach32_canaccel() && (accel_supp & ACCELFLAG_SETTRANSPARENCY)) {
	        retval = ACCELFLAG_PUTBITMAP | ACCELFLAG_SCREENCOPYMONO | ACCELFLAG_POLYFILLMODE;
		return accel_supp & retval;
	    } else
		return 0;
	case VGA_AVAIL_ROPMODES:
	    if (mach32_canaccel() && (accel_supp & ACCELFLAG_SETRASTEROP))
		return (1 << ROP_COPY) | (1 << ROP_OR) | (1 << ROP_AND) |
			(1 << ROP_XOR) | (1 << ROP_INVERT);
	    else
		return 0;
	case VGA_AVAIL_TRANSMODES:
	    if (mach32_canaccel() && (accel_supp & ACCELFLAG_SETTRANSPARENCY))
		return (1 << ENABLE_BITMAP_TRANSPARENCY);
	    else
		return 0;
	}
	return 0;
    case VGA_EXT_SET:
	old_values = ext_settings;
	ext_settings |= (va_arg(params, int)) & pos_ext_settings;
      params_changed:
	if (((old_values ^ ext_settings) & pos_ext_settings) &&
	    (((mach32_dacmode & 0x7f) == DAC_MODE8) ||
	     ((mach32_dacmode & 0x7f) == DAC_MODEMUX))) {
	    CRITICAL = 1;
	    vga_lockvc();
	    if (ext_settings & VGA_CLUT8)
		outw(EXT_GE_CONF, inw(R_EXT_GE_CONF) | 0x4000);
	    else
		outw(EXT_GE_CONF, inw(R_EXT_GE_CONF) & 0xbfff);
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

static void mach32_bitblt(int srcaddr, int destaddr, int w, int h, int pitch)
{
    int base, src_x, des_x, adj_w, forw_cpy;

#ifdef DEBUG
    printf("mach32_bitblt(%xh,%xh,%d,%d,%d)\n", srcaddr, destaddr, w, h, pitch);
#endif

    base = (forw_cpy = (srcaddr > destaddr)) ? destaddr : srcaddr;

    base &= ~3;
    srcaddr -= base;
    destaddr -= base;

/*calculate resulting x values */
    src_x = srcaddr % pitch;
    des_x = destaddr % pitch;

/*width in bytes */
    adj_w = (infotable[CM].bytesperpixel == 2) ? w << 1 : w;

#ifdef DEBUG
    printf("   base=%xh,src=%xh,des=%xh,src_x=%d,des_x=%d,adj_w=%d\n", base, srcaddr,
	   destaddr, src_x, des_x, adj_w);
#endif

/*Currentsetting invalid? */
    if (((src_x + adj_w) > pitch) || ((des_x + adj_w) > pitch)) {
	int adj;
	/* try to adjust: */
	adj = des_x - src_x;
	if (adj < 0)
	    adj = -adj;
	adj = pitch - (adj & ~3);
	srcaddr += adj;;
	destaddr += adj;
	base -= adj;
	if ((src_x += adj) >= pitch)
	    src_x -= pitch;
	if ((des_x += adj) >= pitch)
	    des_x -= pitch;

#ifdef DEBUG
	printf("   src=%xh,des=%xh,src_x=%d,des_x=%d,adj=%d\n", srcaddr, destaddr, src_x, des_x, adj);
#endif
	/*Still invalid? */
	if (((src_x + adj_w) > pitch) || ((des_x + adj_w) > pitch))
	    goto f_ugh;
    }
/*reduce addresses to y values: */
    srcaddr /= pitch;
    destaddr /= pitch;

#ifdef DEBUG
    printf("   >>src=%xh,des=%xh,base=%xh\n", srcaddr, destaddr, base);
#endif

    if ((srcaddr > (1535 - h)) || (destaddr > (1535 - h)))
	goto f_ugh;
/*Reduce values to pixels */
    if (infotable[CM].bytesperpixel == 2) {
	if ((src_x & 1) || (des_x & 1) || (pitch & 0xfffff00f)) {
	  f_ugh:
	    puts("\asvgalib: mach32: mach32_bitblt can't emulate Cirrus.");
	    return;
	}
	src_x >>= 1;
	des_x >>= 1;
	pitch >>= 4;
    } else {
	if (pitch & 0xfffff807)
	    goto f_ugh;
	pitch >>= 3;
    }

/*y in range is not checked. */

/* Do not disturb us: */
    CRITICAL = 1;
    vga_lockvc();

/* Ensure std GE config to emulate simple Cirrus */
    if (mach32_accelstate != R_STANDARD)
	mach32_setstate(R_STANDARD);

    checkqueue(5);		/* Need FIFO room for commands */

/* set GE base close to destination: */
    outw(GE_OFFSET_LO, (unsigned short) (base >> 2));
    outw(GE_OFFSET_HI, (unsigned short) (base >> 18));
    outw(GE_PITCH, (unsigned char) pitch);
    outw(DP_CONFIG, 0x6011);	/* Replace with VRAM blit source */
    checkqueue(10);		/* Need new FIFO room for commands */
/*start coords: */
#ifdef DEBUG
    printf("   ##x=%x,y=%x,xd=%x,yd=%x\n", src_x, srcaddr, des_x, destaddr);
#endif
    if (forw_cpy) {
	/* cpy from lower to higher addr, x, y */
	/* Use Mach32 extended blit. */
	outw(SRC_X, src_x);
	outw(SRC_X_START, src_x);
	outw(SRC_Y, srcaddr);
	outw(SRC_X_END, 0x7ff & (src_x + w));
	outw(SRC_Y_DIR, 1);
	outw(CUR_X, des_x);
	outw(DEST_X_START, des_x);
	outw(CUR_Y, destaddr);
	outw(DEST_X_END, 0x7ff & (des_x + w));
	outw(DEST_Y_END, 0x7ff & (destaddr + h));
    } else {
	/* rev_cpy, adj start coords. */
	/* Use Mach32 extended blit. */
	outw(SRC_X, 0x7ff & (src_x + w));
	outw(SRC_X_START, 0x7ff & (src_x + w));
	outw(SRC_Y, 0x7ff & (srcaddr + h - 1));
	outw(SRC_X_END, src_x);
	outw(SRC_Y_DIR, 0);
	outw(CUR_X, 0x7ff & (des_x + w));
	outw(DEST_X_START, 0x7ff & (des_x + w));
	outw(CUR_Y, 0x7ff & (destaddr + h - 1));
	outw(DEST_X_END, des_x);
	outw(DEST_Y_END, 0x7ff & (destaddr - 1));
    }
/* Wait then unlock vc */
    mach32_bltwait();		/* resets critical as well */
    outw(DP_CONFIG, 0x3291);	/* reset to standard config.. otherwise dramatical speed down */
#ifdef DEBUG
    printf("   **x=%xh,y=%xh\n", (int) inw(CUR_X), (int) inw(CUR_Y));
#endif

    vga_unlockvc();
}

static void mach32_fillblt(int destaddr, int w, int h, int pitch, int c)
{
#ifdef DEBUG
    printf("mach32_fillblt(%xh,%d,%d,%d,%d)\n", destaddr, w, h, pitch, c);
#endif

/* Do not disturb us: */
    CRITICAL = 1;
    vga_lockvc();

/* Ensure std GE config to emulate simple Cirrus */
    if (mach32_accelstate != R_STANDARD)
	mach32_setstate(R_STANDARD);

    checkqueue(5);		/* Need FIFO room for commands */
/* set GE base close to destination: */
    outw(GE_OFFSET_LO, (unsigned short) (destaddr >> 2));
    outw(GE_OFFSET_HI, (unsigned short) (destaddr >> 18));
    destaddr &= 3;		/* remaining x offset */

    if (infotable[CM].bytesperpixel == 2) {
	if ((destaddr & 1) || (pitch & 0xfffff00f)) {
	  ugh:
	    puts("\asvgalib: mach32: mach32_fillblt can't emulate Cirrus.");
	    CRITICAL = 0;
	    vga_unlockvc();
	    return;
	}
	destaddr >>= 1;
	pitch >>= 4;
    } else {
	if (pitch & 0xfffff807)
	    goto ugh;
	pitch >>= 3;
    }
    outw(GE_PITCH, (unsigned char) pitch);
    outw(FRGD_COLOR, (unsigned short) c);
    outw(FRGD_MIX, 0x27);	/* replace with FRGD_COLOR */
    checkqueue(5);		/* Need new FIFO room for commands */
/*start coords: */
    outw(CUR_X, destaddr);
    outw(CUR_Y, 0);
/*width+height */
    outw(MAJ_AXIS_PCNT, w - 1);
    outw(MULTI_FUNC_CNTL, 0x7ff & (h - 1));
/*kick it off */
    outw(CMD, 0x50B3);		/*Rectangle fill with horz lines,pos x+y dir,draw last pel */

/* Wait then unlock vc */
    mach32_bltwait();		/* resets critical as well */
    vga_unlockvc();
}

static void mach32_hlinelistblt(int ymin, int n, int *xmin, int *xmax, int pitch, int c)
{
#ifdef DEBUG
    printf("mach32_hlinelistblt(%d,%d,%08xh,%08xh,%d,%d)\n", ymin, n, (unsigned) xmin, (unsigned) xmax, pitch, c);
#endif

/* Do not disturb us: */
    CRITICAL = 1;
    vga_lockvc();

/* Ensure std GE config to emulate simple Cirrus */
    if (mach32_accelstate != R_STANDARD)
	mach32_setstate(R_STANDARD);

    checkqueue(5);		/* Need FIFO room for commands */
    /* This is how I understand the Cirrus-code. Since we have to be compatible: */
    outw(GE_OFFSET_LO, 0);
    outw(GE_OFFSET_HI, 0);

    if (infotable[CM].bytesperpixel == 2) {
	if (pitch & 0xfffff00f) {
	  ugh:
	    puts("\asvgalib: mach32: mach32_hlinelistblt can't emulate Cirrus.");
	    CRITICAL = 0;
	    vga_unlockvc();
	    return;
	}
	pitch >>= 4;
    } else {
	if (pitch & 0xfffff807)
	    goto ugh;
	pitch >>= 3;
    }
    outw(GE_PITCH, (unsigned char) pitch);
    outw(FRGD_COLOR, (unsigned short) c);

/*DP_CONFIG is replace with foreground already... */
/*And now close your eyes and enjoy.. this is exactly what a Mach32 is built for.. */

    checkqueue(12);		/* Need new FIFO room for commands */

/*I think we can trust the hardware handshake from now on. */
/*Sigh.. no we can't.. queue overruns in 16bpp */

    while (n-- > 0) {
	outw(CUR_X, 0x7ff & (*xmin));
	outw(CUR_Y, 0x7ff & (ymin++));
	if (*xmin <= *xmax)
	    outw(SCAN_TO_X, (0x7ff & (*xmax + 1)));
	xmin++;
	xmax++;

	if (!(n & 3))
	    checkqueue(12);
    }

/* Wait then unlock vc */
    mach32_bltwait();		/* resets critical as well */
    vga_unlockvc();
}

static void mach32_imageblt(void *srcaddr, int destaddr, int w, int h, int pitch)
{
    unsigned count;		/* 16bit words needed to transfer */

/* Do not disturb us: */
    CRITICAL = 1;
    vga_lockvc();

#ifdef DEBUG
    printf("mach32_imageblt(%xh,%xh,%d,%d,%d)\n", (int) srcaddr, destaddr, w, h, pitch);
#endif

/* Ensure std GE config to emulate simple Cirrus */
    if (mach32_accelstate != R_STANDARD)
	mach32_setstate(R_STANDARD);

    checkqueue(4);		/* Need FIFO room for commands */
/* set GE base close to destination: */
    outw(GE_OFFSET_LO, (unsigned short) (destaddr >> 2));
    outw(GE_OFFSET_HI, (unsigned short) (destaddr >> 18));
    destaddr &= 3;		/* remaining x offset */

    count = w * h;
    if (infotable[CM].bytesperpixel == 2) {
	if ((destaddr & 1) || (pitch & 0xfffff00f)) {
	  ugh:
	    puts("\asvgalib: mach32: mach32_imageblt can't emulate Cirrus.");

	    CRITICAL = 0;
	    vga_unlockvc();
	    return;
	}
	destaddr >>= 1;
	pitch >>= 4;
    } else {
	if (pitch & 0xfffff807)
	    goto ugh;
	pitch >>= 3;
	count = (count + 1) >> 1;	/* two pixels transferred in one blow */
    }

    if (!count) {
	CRITICAL = 0;
	vga_unlockvc();
	return;			/* safety bailout */
    }
    outw(GE_PITCH, (unsigned char) pitch);
    outw(FRGD_MIX, 0x47);	/* replace with value of PIX_TRANS */
    checkqueue(6);		/* Need new FIFO room for commands, one more than actually needed to ensure
				   correct hostdata fifo operation. */
/*start coords: */
    outw(CUR_X, destaddr);
    outw(CUR_Y, 0);
/*width+height */
    outw(MAJ_AXIS_PCNT, w - 1);
    outw(MULTI_FUNC_CNTL, 0x7ff & (h - 1));
/*kick it off */
    outw(CMD, 0x53B1);		/*Rectangle fill with horz lines,pos x+y dir,draw last pel */
    /*(set also wait for MSB in the hope that it will ensure the correct
       thing if we write out an odd number of bytes) */

/* Do the transfer: All handshake is done in hardware by waitstates. Note that the first
   word can be transferred w/o problems.. after that automatic handshake by waitstates is enabled
   by the ATI on it's own.
   The outs is regarding to my assembler docs not only the most simple but also by far the
   fastest way to do this. Loop unrolling will gain nothing, so it is not done.. */

#ifdef __alpha__
    printf("mach32_imageblt: not done yet\n");
#else
    asm("cld\n"			/* Increasing addresses.. */
	"	movw $0xe2e8,%%dx\n"	/* PIX_TRANS port */
	"	rep\n"		/* repeat.. count is in ECX */
  "	outsw\n":		/* Blast data out in 16bit pieces */
  /* no outputs */ :
    /* inputs */
    /* SI= source address */ "S"(srcaddr),
  /* CX= words to transfer */ "c"(count):
    /* dx destructed: */ "%dx"
	);
#endif

    checkqueue(2);
/*Sigh, it seems that DP_CONFIG gets affected by the 8514/A settings. so reset to standard. */
    outw(DP_CONFIG, 0x3291);

/* The bltwait is more intended for a final checkup if everything worked ok and we
   didn't lock up the Mach32 fifo */

/* Wait then unlock vc */
    mach32_bltwait();		/* resets critical as well */
    vga_unlockvc();
}

static void mach32_memimageblt(void *srcaddr, int destaddr, int w, int h, int pitch)
{
    char *source = srcaddr, *dest = (char * /*discard volatile */ ) mach32_aperture + destaddr;

#ifdef DEBUG
    printf("mach32_memimageblt(%xh,%xh,%d,%d,%d)\n", (int) srcaddr, destaddr, w, h, pitch);
    printf("\tmemtrans: %xh -> %xh\n", (int) source, (int) dest);
#endif

    if (!infotable[CM].bytesperpixel)
	return;

    w *= infotable[CM].bytesperpixel;

    if (w == pitch) {
	fast_memcpy(dest, source, w * h);
    } else {
	while (h--) {
	    fast_memcpy(dest, source, w);
	    dest += pitch;
	    source += w;
	}
    }
#ifdef DEBUG
    puts("mach32_mach32_imageblt ended.");
#endif
}

static unsigned char mach32ropdefs[] = { 0x7, 0xB, 0xC, 0x5, 0x0 };

static void enter_accel(void)
{
    if (!CRITICAL) {
	CRITICAL = 1;
	vga_lockvc();
    }
    if (mach32_accelstate != R_EXTENDED)
	mach32_setstate(R_EXTENDED);
}

static unsigned short *mk_bitrot(void)
{
    unsigned short *bitrot, *ptr;
    int i = 0;    

    ptr = bitrot = malloc(sizeof(unsigned short) * (1 << 16));
    if (!bitrot) {
	printf("svgalib: mach32: Not enough memory for PUTBITMAP rotation table.\n");
	return NULL;
    }
    for (i = 0; i < 16; i++)
	*ptr++ = ((i & 1) << 15) | ((i & 2) << 13) | ((i & 4) << 11) | ((i & 8) << 9);
    for (; i < 256; i++)
	*ptr++ = bitrot[i & 15] | (bitrot[i >> 4] >> 4);
    for (; i < (1 << 16); i++)
	*ptr++ = bitrot[i & 255] | (bitrot[i >> 8] >> 8);
    return bitrot;
}

static int mach32_accel(unsigned operation, va_list params)
{
    static int accel_blockmode = BLITS_SYNC;
    static unsigned short *bitrot = NULL;
    unsigned short x, y, x2, y2, w, h;

    switch(operation) {
	case ACCEL_SETRASTEROP:
	    enter_accel();
	    x = va_arg(params, int);
	    if (x <= sizeof(mach32ropdefs)) {
		mach32_rop = x;
		checkqueue(4); /* could check for 2 + 2 but this is probably not worth it */
		mach32_frgd_alu_fn = mach32ropdefs[x];
		outw(FRGD_MIX, mach32_frgd_alu_fn | 0x20);
		outw(ALU_FG_FN, mach32_frgd_alu_fn);
		if (mach32_bkgd_alu_fn != 0x3) {
		    mach32_bkgd_alu_fn = mach32_frgd_alu_fn;
		    outw(BKGD_MIX, mach32_bkgd_alu_fn);
		    outw(ALU_BG_FN, mach32_bkgd_alu_fn);
		}
	    }
	    break;
	case ACCEL_SETTRANSPARENCY:
	    enter_accel();
	    x = va_arg(params, int);
	    if (x == ENABLE_BITMAP_TRANSPARENCY) {
		if (mach32_bkgd_alu_fn != 0x3) {
	    	    checkqueue(2);
		    mach32_bkgd_alu_fn = 0x3;
		    outw(BKGD_MIX, mach32_bkgd_alu_fn);
		    outw(ALU_BG_FN, mach32_bkgd_alu_fn);
		}
	    } else if (x == DISABLE_BITMAP_TRANSPARENCY) {
		if (mach32_bkgd_alu_fn == 0x3) {
	    	    checkqueue(2);
		    mach32_bkgd_alu_fn = mach32_frgd_alu_fn;
		    outw(BKGD_MIX, mach32_bkgd_alu_fn);
		    outw(ALU_BG_FN, mach32_bkgd_alu_fn);
		}
	    }
	    break;
	case ACCEL_SETFGCOLOR:
	    enter_accel();
	    mach32_frgd_col = va_arg(params, int);
	    checkqueue(1);
	    outw(FRGD_COLOR, mach32_frgd_col);
	    break;
	case ACCEL_SETBGCOLOR:
	    enter_accel();
	    mach32_bkgd_col = va_arg(params, int);
	    checkqueue(1);
	    outw(BKGD_COLOR, mach32_bkgd_col);
	    break;
	case ACCEL_SETMODE:
	    x = va_arg(params, int);
	    switch(x) {
		case BLITS_IN_BACKGROUND:
		case BLITS_SYNC:
		    accel_blockmode = x;
		    break;
	    }
	    break;
	case ACCEL_SYNC:
	    if (CRITICAL) {
	        mach32_bltwait();
	        vga_unlockvc();
	    }
	    return 0;
	default:
	    return -1;
	case ACCEL_FILLBOX:
	    enter_accel();
	    checkqueue(5);	/* Need new FIFO room for commands */
		/*start coords: */
	    outw(CUR_X, (unsigned short)va_arg(params, int));
	    outw(CUR_Y, (unsigned short)va_arg(params, int));
		/*width+height */
	    outw(MAJ_AXIS_PCNT, (unsigned short)(va_arg(params, int) - 1));
	    outw(MULTI_FUNC_CNTL, (unsigned short)(0x7ff & (va_arg(params, int) - 1)));
		/*kick it off */
	    outw(CMD, 0x50B3);	/*Rectangle fill with horz lines,pos x+y dir,draw last pel */
	    break;
	case ACCEL_SCREENCOPY:
	    enter_accel();
	    checkqueue(1);
	    outw(DP_CONFIG, 0x6011);	/* Replace with VRAM blit source */
	    x = va_arg(params, int);
	    y = va_arg(params, int);
	    x2 = va_arg(params, int);
	    y2 = va_arg(params, int);
	    w = va_arg(params, int);
	    h = va_arg(params, int);
	    checkqueue(10);		/* Need new FIFO room for commands */
	    if ((y > y2) || ((y == y2) && (x >= x2))) {
		/* cpy from lower to higher addr, x, y */
		/* Use Mach32 extended blit. */
		outw(SRC_X, x & 0x7ff);
		outw(SRC_X_START, x & 0x7ff);
		outw(SRC_Y, y & 0x7ff);
		outw(SRC_X_END, 0x7ff & (x + w));
		outw(SRC_Y_DIR, 1);
		outw(CUR_X, x2 & 0x7ff);
		outw(DEST_X_START, x2 & 0x7ff);
		outw(CUR_Y, y2 & 0x7ff);
		outw(DEST_X_END, 0x7ff & (x2 + w));
		outw(DEST_Y_END, 0x7ff & (y2 + h));
	    } else {
		/* rev_cpy, adj start coords. */
		/* Use Mach32 extended blit. */
		outw(SRC_X, 0x7ff & (x + w));
		outw(SRC_X_START, 0x7ff & (x + w));
		outw(SRC_Y, 0x7ff & (y + h - 1));
		outw(SRC_X_END, x & 0x7ff);
		outw(SRC_Y_DIR, 0);
		outw(CUR_X, 0x7ff & (x2 + w));
		outw(DEST_X_START, 0x7ff & (x2 + w));
		outw(CUR_Y, 0x7ff & (y2 + h - 1));
		outw(DEST_X_END, x2 & 0x7ff);
		outw(DEST_Y_END, 0x7ff & (y2 - 1));
	    }
	    checkqueue(1);
	    outw(DP_CONFIG, 0x3291);
	    break;
	case ACCEL_DRAWHLINELIST:
	    {
	    int *xmin, *xmax;

	    enter_accel();
	    y = va_arg(params, int);
	    h = va_arg(params, int);
	    xmin = va_arg(params, int *);
	    xmax = va_arg(params, int *);
	    checkqueue(12);
	    while (h-- > 0) {
		x = *xmin++;
		x2 =  *xmax++;
		if (x > x2) {
		    x = x2;
		    x2 = xmin[-1];
		}
		outw(CUR_X, x);
		outw(CUR_Y, y++);
		outw(SCAN_TO_X, 0x7ff & (x2 + 1));

		if (!(h & 3))
		    checkqueue(12);
	    }

	    }
	    break;
	case ACCEL_PUTIMAGE:
	    {
	    unsigned char *data;
	    unsigned long count;

	    enter_accel();
	    if (((emuimage & EMU_OVER) || infotable[CM].colors == (1 << 24)) &&
		    mach32_rop == ROP_COPY) {
		/* Ensure the engine is idle */
	        mach32_bltwait();
		/* Emulate putimage: */
		x = va_arg(params, int);
		y = va_arg(params, int);
		w = va_arg(params, int);
		h = va_arg(params, int);
		data = va_arg(params, unsigned char *);
		mach32_memimageblt(data,
		    (((unsigned long)mach32_ge_off_l) << 2) +
		    (((unsigned long)mach32_ge_off_h) << 18) +
		    infotable[CM].xbytes * y +
		    infotable[CM].bytesperpixel * x,
		    w, h, infotable[CM].bytesperpixel * (int)(mach32_ge_pitch << 3));
		break;
	    }
	    checkqueue(12);
	    outw(FRGD_MIX, mach32_frgd_alu_fn | 0x40);
	    outw(CUR_X, va_arg(params, int));
	    outw(CUR_Y, va_arg(params, int));
	    w = va_arg(params, int);
	    h = va_arg(params, int);
	    count = w * (unsigned long)h;
	    outw(MAJ_AXIS_PCNT, w - 1);
	    outw(MULTI_FUNC_CNTL, 0x7ff & (h - 1));
	    data = va_arg(params, unsigned char *);
	    outw(CMD, 0x53B1);
#ifdef __alpha__
	    printf("mach32_ACCEL_PUTIMAGE: not done yet\n");
#else
    	    if (infotable[CM].bytesperpixel == 2) {
		asm("cld\n"
		    "movw $0xe2e8, %%dx\n"
		    "rep\n"
	  	    "outsw\n": : "S"(data), "c"(count): "%dx"
		);
	    } else {
		if (count > 1)
			asm("cld\n"
			    "movw $0xe2e8, %%dx\n"
			    "rep\n"
	  		    "outsw\n": : "S"(data), "c"(count >> 1): "%dx"
			);
		if (count & 1)
		    outw(PIX_TRANS, ((unsigned short)data[count - 1]));
	    }
#endif
	    checkqueue(2);
	    outw(FRGD_MIX, mach32_frgd_alu_fn | 0x20);
	    outw(DP_CONFIG, 0x3291);
	    }
	    break;
	case ACCEL_DRAWLINE:
	    enter_accel();
	    checkqueue(6);
	    outw(LINEDRAW_OPT, mach32_polyfill);
	    outw(LINEDRAW_INDEX, 0);
	    outw(LINEDRAW, va_arg(params, int));
	    outw(LINEDRAW, va_arg(params, int));
	    outw(LINEDRAW, va_arg(params, int));
	    outw(LINEDRAW, va_arg(params, int));
	    break;
	case ACCEL_PUTBITMAP:
	    {
	    unsigned short *data;
	    unsigned long count;

	    if (!bitrot)
		bitrot = mk_bitrot();
	    if (!bitrot)
		break;
	
	    enter_accel();
	    checkqueue(1);
	    outw(DP_CONFIG, 0x2255);
	    x = va_arg(params, int);
	    y = va_arg(params, int);
	    w = va_arg(params, int);
	    h = va_arg(params, int);
	    checkqueue(10);
	    /* cpy from lower to higher addr, x, y */
	    /* Use Mach32 extended blit for packed bitmap. */
	    outw(EXT_SCISSOR_R, x + w - 1);
	    w = (w + 31) & ~31;
	    outw(CUR_X, x & 0x7ff);
	    outw(DEST_X_START, x & 0x7ff);
	    outw(CUR_Y, y & 0x7ff);
	    outw(DEST_X_END, 0x7ff & (x + w));
	    outw(DEST_Y_END, 0x7ff & (y + h));
	    count = (w * (unsigned long)h) / 16;
	    data = va_arg(params, unsigned short *);
	    checkqueue(12);
	    while(count--)
		outw(PIX_TRANS, bitrot[*data++]);
	    checkqueue(2);
	    outw(DP_CONFIG, 0x3291);
	    outw(EXT_SCISSOR_R, 1535);
	    }
	    break;
	case ACCEL_SCREENCOPYMONO:
	    enter_accel();
	    checkqueue(1);
	    if (mach32_polyfill)
		outw(DP_CONFIG, 0x3217);
	    else
		outw(DP_CONFIG, 0x3275);
	    x = va_arg(params, int);
	    y = va_arg(params, int);
	    x2 = va_arg(params, int);
	    y2 = va_arg(params, int);
	    w = va_arg(params, int);
	    h = va_arg(params, int);
	    checkqueue(10);
	    /* cpy from lower to higher addr, x, y */
	    outw(SRC_X, x & 0x7ff);
	    outw(SRC_X_START, x & 0x7ff);
	    outw(SRC_Y, y & 0x7ff);
	    outw(SRC_X_END, 0x7ff & (x + w));
	    outw(SRC_Y_DIR, 1);
	    outw(CUR_X, x2 & 0x7ff);
	    outw(DEST_X_START, x2 & 0x7ff);
	    outw(CUR_Y, y2 & 0x7ff);
	    outw(DEST_X_END, 0x7ff & (x2 + w));
	    outw(DEST_Y_END, 0x7ff & (y2 + h));
	    checkqueue(1);
	    outw(DP_CONFIG, 0x3291);
	    break;
	case ACCEL_SETOFFSET:
	    {
	    unsigned int address;

	    address = va_arg(params, unsigned int);
	    enter_accel();
	    checkqueue(2);
	    mach32_ge_off_l = address >> 2;
	    outw(GE_OFFSET_LO, mach32_ge_off_l);
	    mach32_ge_off_h = 0xff & (address >> 18);
	    outw(GE_OFFSET_HI, mach32_ge_off_h);
	    }
	    break;
	case ACCEL_POLYLINE:
	    {
	    unsigned short *data;
	    int count, flag;

	    enter_accel();
	    flag = va_arg(params, int);
	    count = va_arg(params, int) << 1;
	    data = va_arg(params, unsigned short *);

	    checkqueue(5);
	    if (flag & ACCEL_START) {
		outw(LINEDRAW_OPT, 0x4 | mach32_polyfill);
	        outw(LINEDRAW_INDEX, 0);
	    }
	    if (flag & ACCEL_END) {
		count -= 2;
	    }
	    if (count > 0)
		asm("cld\n"
		    "movw $0xfeee, %%dx\n"
		    "rep\n"
	  	    "outsw\n": : "S"(data), "c"(count): "%dx"
		);
	    if (flag & ACCEL_END) {
	    	checkqueue(3);
		outw(LINEDRAW_OPT, mach32_polyfill);
	        outw(LINEDRAW, data[count++]);
	        outw(LINEDRAW, data[count]);
	    }
	    }
	    break;
	case ACCEL_POLYHLINE:
	    {
	    unsigned short *data;
	    int flag;
	    static int loc_y;

	    enter_accel();
	    flag = va_arg(params, int);
	    y = va_arg(params, int);
	    h = va_arg(params, int);
	    data = va_arg(params, unsigned short *);
	    checkqueue(12);

	    if (flag & ACCEL_START) {
		loc_y = y;
	    }
	    while (h--) {
		w = (*data++) - 1;
		outw(CUR_X, *data++);
		outw(CUR_Y, loc_y++);
		for(; w > 1; w -= 2) {
		    outw(SCAN_TO_X, (*data++) + 1);
		    outw(SCAN_TO_X, (*data++));
		}
		outw(SCAN_TO_X, (*data++) + 1);
		if (!(h & 3))
		    checkqueue(12);
	    }
	    }
	    break;
	case ACCEL_POLYFILLMODE:
	    {
	    if (va_arg(params, int))
		mach32_polyfill = 2;
	    else
		mach32_polyfill = 0;
	    }
	    return 0;
    }
    if (accel_blockmode == BLITS_SYNC) {
	mach32_bltwait();	/* resets critical as well */
	vga_unlockvc();
    }
    return 0;
}

static void mach32_savepalette(unsigned char *red, unsigned char *green, unsigned char *blue) {
    int i;

    port_out(0, PEL8514_IR);
    for (i = 0; i < 256; i++) {
	__svgalib_delay();
	*(red++) = port_in(PEL8514_D);
	__svgalib_delay();
	*(green++) = port_in(PEL8514_D);
	__svgalib_delay();
	*(blue++) = port_in(PEL8514_D);
    }
}

static void mach32_restorepalette(const unsigned char *red, const unsigned char *green,
				  const unsigned char *blue) {
    int i;

    port_out(0, PEL8514_IW);
    for (i = 0; i < 256; i++) {
	__svgalib_delay();
	port_out(*(red++), PEL8514_D);
	__svgalib_delay();
	port_out(*(green++), PEL8514_D);
	__svgalib_delay();
	port_out(*(blue++), PEL8514_D);
    }
}

static int mach32_setpalette(int index, int red, int green, int blue) {
    if ((index != palcurind) || (palsetget != 1)) {
	port_out(index, PEL8514_IW);
	palcurind = index + 1;
	palsetget = 1;
    } else {
	palcurind++;
    }
    __svgalib_delay();
    port_out(red, PEL8514_D);
    __svgalib_delay();
    port_out(green, PEL8514_D);
    __svgalib_delay();
    port_out(blue, PEL8514_D);
    return 0;
}

static void mach32_getpalette(int index, int *red, int *green, int *blue) {
    if ((index != palcurind) || (palsetget != 0)) {
	port_out(index, PEL8514_IR);
	palcurind = index + 1;
	palsetget = 0;
    } else {
	palcurind++;
    }
    __svgalib_delay();
    *red = (int) port_in(PEL8514_D);
    __svgalib_delay();
    *green = (int) port_in(PEL8514_D);
    __svgalib_delay();
    *blue = (int) port_in(PEL8514_D);
}

static int mach32_screenon(void) {
    /* Force Mach32 to ATI mode (used by setmode) */
    outw(0x4AEE, inw(0x4AEE) | 1);
    return 0;
}

static void mach32_waitretrace(void) {
    unsigned char csync;

    csync = inb(0xd2ee) >> 3;       /* Current sync polarity in bit 2,
                                       0-pos, 1-neg  */
    /*Wait until in displayed area.. */
    while ((inb(0x2e8) ^ csync) & 2);
    /*Wait until V_SYNC */
    csync = ~csync;
    while ((inb(0x2e8) ^ csync) & 2);
}


