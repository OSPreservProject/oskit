/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */
/* Changes around the config things by 101 (Attila Lendvai) */

/* The code is a bit of a mess; also note that the drawing functions */
/* are not speed optimized (the gl functions are much faster). */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#ifndef OSKIT
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <sys/vt.h>
#endif
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#ifdef USEGLIBC
#include <sys/io.h>
#endif
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#ifndef OSKIT
#include "mouse/vgamouse.h"
#include "keyboard/vgakeyboard.h"
#endif

#ifdef BACKGROUND
#include "vgabg.h"
#endif


/* Delay in microseconds after a mode is set (screen is blanked during this */
/* time), allows video signals to stabilize */
#define MODESWITCHDELAY 150000

/* Define this to disable video output during mode switches, in addition to */
/* 'turning off the screen', which is always done. */
/* Doesn't look very nice on my Cirrus. */
/* #define DISABLE_VIDEO_OUTPUT */

/* #define DONT_WAIT_VC_ACTIVE */

/* Use /dev/tty instead of /dev/tty0 (the previous behaviour may have been
 * silly). */
#define USE_DEVTTY


/* variables used to shift between monchrome and color emulation */
int __svgalib_CRT_I;			/* current CRT index register address */
int __svgalib_CRT_D;			/* current CRT data register address */
int __svgalib_IS1_R;			/* current input status register address */
int color_text;		/* true if color text emulation */

/* If == 0 then nothing is defined by the user... */
int __svgalib_default_mode = 0;

struct info infotable[] =
{
    {80, 25, 16, 160, 0},	/* VGAlib VGA modes */
    {320, 200, 16, 40, 0},
    {640, 200, 16, 80, 0},
    {640, 350, 16, 80, 0},
    {640, 480, 16, 80, 0},
    {320, 200, 256, 320, 1},
    {320, 240, 256, 80, 0},
    {320, 400, 256, 80, 0},
    {360, 480, 256, 90, 0},
    {640, 480, 2, 80, 0},

    {640, 480, 256, 640, 1},	/* VGAlib SVGA modes */
    {800, 600, 256, 800, 1},
    {1024, 768, 256, 1024, 1},
    {1280, 1024, 256, 1280, 1},

    {320, 200, 1 << 15, 640, 2},	/* Hicolor/truecolor modes */
    {320, 200, 1 << 16, 640, 2},
    {320, 200, 1 << 24, 320 * 3, 3},
    {640, 480, 1 << 15, 640 * 2, 2},
    {640, 480, 1 << 16, 640 * 2, 2},
    {640, 480, 1 << 24, 640 * 3, 3},
    {800, 600, 1 << 15, 800 * 2, 2},
    {800, 600, 1 << 16, 800 * 2, 2},
    {800, 600, 1 << 24, 800 * 3, 3},
    {1024, 768, 1 << 15, 1024 * 2, 2},
    {1024, 768, 1 << 16, 1024 * 2, 2},
    {1024, 768, 1 << 24, 1024 * 3, 3},
    {1280, 1024, 1 << 15, 1280 * 2, 2},
    {1280, 1024, 1 << 16, 1280 * 2, 2},
    {1280, 1024, 1 << 24, 1280 * 3, 3},

    {800, 600, 16, 100, 0},	/* SVGA 16-color modes */
    {1024, 768, 16, 128, 0},
    {1280, 1024, 16, 160, 0},

    {720, 348, 2, 90, 0},	/* Hercules emulation mode */

    {320, 200, 1 << 24, 320 * 4, 4},
    {640, 480, 1 << 24, 640 * 4, 4},
    {800, 600, 1 << 24, 800 * 4, 4},
    {1024, 768, 1 << 24, 1024 * 4, 4},
    {1280, 1024, 1 << 24, 1280 * 4, 4},

    {1152, 864, 16, 144, 0},
    {1152, 864, 256, 1152, 1},
    {1152, 864, 1 << 15, 1152 * 2, 2},
    {1152, 864, 1 << 16, 1152 * 2, 2},
    {1152, 864, 1 << 24, 1152 * 3, 3},
    {1152, 864, 1 << 24, 1152 * 4, 4},

    {1600, 1200, 16, 200, 0},
    {1600, 1200, 256, 1600, 1},
    {1600, 1200, 1 << 15, 1600 * 2, 2},
    {1600, 1200, 1 << 16, 1600 * 2, 2},
    {1600, 1200, 1 << 24, 1600 * 3, 3},
    {1600, 1200, 1 << 24, 1600 * 4, 4},

    {0, 0, 0, 0, 0},		/* 16 user definable modes */
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
};

#define MAX_MODES (sizeof(infotable) / sizeof(struct info))

unsigned long __svgalib_graph_base = GRAPH_BASE;

unsigned char __svgalib_novga = 0;     /* Does not have VGA circuitry on board */
unsigned char __svgalib_secondary = 0; /* this is not the main card with VC'S (not yet supported) */

/* default palette values */
static const unsigned char default_red[256]
=
{0, 0, 0, 0, 42, 42, 42, 42, 21, 21, 21, 21, 63, 63, 63, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 0, 16, 31, 47, 63, 63, 63, 63, 63, 63, 63, 63, 63, 47, 31, 16,
 0, 0, 0, 0, 0, 0, 0, 0, 31, 39, 47, 55, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 55, 47, 39, 31, 31, 31, 31, 31, 31, 31, 31,
 45, 49, 54, 58, 63, 63, 63, 63, 63, 63, 63, 63, 63, 58, 54, 49,
 45, 45, 45, 45, 45, 45, 45, 45, 0, 7, 14, 21, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 21, 14, 7, 0, 0, 0, 0, 0, 0, 0, 0,
 14, 17, 21, 24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 24, 21, 17,
 14, 14, 14, 14, 14, 14, 14, 14, 20, 22, 24, 26, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 26, 24, 22, 20, 20, 20, 20, 20, 20, 20, 20,
 0, 4, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 8, 4,
 0, 0, 0, 0, 0, 0, 0, 0, 8, 10, 12, 14, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 14, 12, 10, 8, 8, 8, 8, 8, 8, 8, 8,
 11, 12, 13, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 13, 12,
 11, 11, 11, 11, 11, 11, 11, 11, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char default_green[256]
=
{0, 0, 42, 42, 0, 0, 21, 42, 21, 21, 63, 63, 21, 21, 63, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 31, 47, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 47, 31, 16, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 39, 47, 55, 63, 63, 63, 63, 63, 63, 63, 63, 63, 55, 47, 39,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 49, 54, 58, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 58, 54, 49, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 7, 14, 21, 29, 28, 28, 28, 28, 28, 28, 28, 28, 21, 14, 7,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 17, 21, 24, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 24, 21, 17, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 22, 24, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 24, 22,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 12, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 10, 12, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 10,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 13, 15, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 13, 12, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char default_blue[256]
=
{0, 42, 0, 42, 0, 42, 0, 42, 21, 63, 21, 63, 21, 63, 21, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 63, 63, 63, 63, 63, 47, 31, 16, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 16, 31, 47, 63, 63, 63, 63, 63, 63, 63, 63, 63, 55, 47, 39,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 39, 47, 55, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 58, 54, 49, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 49, 54, 58, 63, 63, 63, 63, 28, 28, 28, 28, 28, 21, 14, 7,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 14, 21, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 24, 21, 17, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 17, 21, 24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 24, 22,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 24, 26, 28, 28, 28, 28,
 16, 16, 16, 16, 16, 12, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 4, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 10,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 10, 12, 14, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 13, 12, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 12, 13, 15, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0};


unsigned char text_regs[MAX_REGS];	/* VGA registers for saved text mode */


/* saved text mode palette values */
unsigned char text_red[256];
unsigned char text_green[256];
unsigned char text_blue[256];

#ifndef OSKIT
/* saved graphics mode palette values */
static unsigned char graph_red[256];
static unsigned char graph_green[256];
static unsigned char graph_blue[256];
#endif

int prv_mode = TEXT;	/* previous video mode      */
#ifndef OSKIT
static int flip_mode = TEXT;	/* flipped video mode       */
#endif

int CM = TEXT;			/* current video mode       */
struct info CI;			/* current video parameters */
int COL;			/* current color            */


extern int initialized;		/* flag: initialize() called ?  */
static int flip = 0;		/* flag: executing vga_flip() ? */

/* svgalib additions: */

int __svgalib_chipset = UNDEFINED;
int __svgalib_driver_report = 1;
	/* report driver used after chipset detection */
int __svgalib_videomemoryused = -1;
int __svgalib_modeX = 0;	/* true after vga_setmodeX() */
int __svgalib_modeflags = 0;	/* copy of flags for current mode */
int __svgalib_critical = 0;	/* indicates blitter is busy */
int __svgalib_screenon = 1;	/* screen visible if != 0 */
RefreshRange __svgalib_horizsync =
{31500U, 0U};			/* horz. refresh (Hz) min, max */
RefreshRange __svgalib_vertrefresh =
{50U, 70U};			/* vert. refresh (Hz) min, max */
int __svgalib_grayscale = 0;	/* grayscale vs. color mode */
int __svgalib_modeinfo_linearset = 0;	/* IS_LINEAR handled via extended vga_modeinfo */
const int __svgalib_max_modes = MAX_MODES;	/* Needed for dynamical allocated tables in mach32.c */

unsigned __svgalib_maxhsync[] =
{
    31500, 35100, 35500, 37900, 48300, 56000, 60000
};


static int lastmodenumber = __GLASTMODE;	/* Last defined mode */
#ifndef BACKGROUND
static int __svgalib_currentpage;
#endif
static int vga_page_offset;	/* offset to add to all vga_set*page() calls */
static int currentlogicalwidth;
static int currentdisplaystart;
#ifndef OSKIT
static int mouse_support = 0;
static int mouse_mode = 0;
static int mouse_type = -1;
static int mouse_modem_ctl = 0;
static char *mouse_device = "/dev/mouse";
#endif /* !OSKIT */
#ifndef BACKGROUND
static int __svgalib_oktowrite = 1;
#endif
int modeinfo_mask = ~0;

#ifndef OSKIT
int __svgalib_mem_fd = -1;	/* /dev/mem file descriptor  */
int __svgalib_tty_fd = -1;	/* /dev/tty file descriptor */
int __svgalib_nosigint = 0;	/* Don't generate SIGINT in graphics mode */
int __svgalib_runinbackground = 0;
static int svgalib_vc = -1, startup_vc = -1;
static int __svgalib_security_revokeallprivs = 1;
#endif /* !OSKIT */
static unsigned fontbufsize = 8192; /* compatibility */

/* Dummy buffer for mmapping grahics memory; points to 64K VGA framebuffer. */
unsigned char *__svgalib_graph_mem;
/* Exported variable (read-only) is shadowed from internal variable, for */
/* better shared library performance. */
unsigned char *graph_mem;

/* Some new organisation for backgrund running. */
/* Alpha do not have ability to run in background (yet). */
/* Bg runnin has two different methods 1 and (2). */
    
   
#ifdef BACKGROUND

static unsigned char *__svgalib_graph_mem_orginal;
static unsigned char *__svgalib_graph_mem_check;
unsigned char *__svgalib_graph_mem_linear_orginal;
unsigned char *__svgalib_graph_mem_linear_check;
static int __svgalib_linear_is_background=0;
int __svgalib_linear_memory_size;
int __svgalib_oktowrite=1;

/* __svgalib_oktowrite tells if it is safe to write registers. */

int __svgalib_currentpage;
static int __svgalib_virtual_mem_fd=-1;
static int __svgalib_processnumber=-1;

#endif


#ifdef __alpha__

/* same as graph mem, but mapped through sparse memory: */
unsigned char *__svgalib_sparse_mem;

#endif

static unsigned char *graph_buf = NULL;		/* saves graphics data during flip */

unsigned char *font_buf1;	/* saved font data - plane 2 */
unsigned char *font_buf2;	/* saved font data - plane 3 */

int __svgalib_flipchar = '\x1b';		/* flip character - initially  ESCAPE */


/* Chipset specific functions */

DriverSpecs *__svgalib_driverspecs = &__svgalib_vga_driverspecs;

#ifndef BACKGROUND
static void (*__svgalib_setpage) (int);	/* gives little faster vga_setpage() */
static void (*__svgalib_setrdpage) (int);
static void (*__svgalib_setwrpage) (int);
#endif
#ifdef BACKGROUND
void (*__svgalib_setpage) (int);	/* gives little faster vga_setpage() */
void (*__svgalib_setrdpage) (int);
void (*__svgalib_setwrpage) (int);
#endif

DriverSpecs *__svgalib_driverspecslist[] =
{
    NULL,			/* chipset undefined */
    &__svgalib_vga_driverspecs,
#ifdef INCLUDE_ET4000_DRIVER
    &__svgalib_et4000_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_CIRRUS_DRIVER
    &__svgalib_cirrus_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_TVGA_DRIVER
    &__svgalib_tvga8900_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_OAK_DRIVER
    &__svgalib_oak_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_EGA_DRIVER
    &__svgalib_ega_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_S3_DRIVER
    &__svgalib_s3_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ET3000_DRIVER
    &__svgalib_et3000_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MACH32_DRIVER
    &__svgalib_mach32_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_GVGA6400_DRIVER
    &__svgalib_gvga6400_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ARK_DRIVER
    &__svgalib_ark_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ATI_DRIVER
    &__svgalib_ati_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ALI_DRIVER
    &__svgalib_ali_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MACH64_DRIVER
    &__svgalib_mach64_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_CHIPS_DRIVER
    &__svgalib_chips_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_APM_DRIVER
    &__svgalib_apm_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_NV3_DRIVER
    &__svgalib_nv3_driverspecs,
#else
    NULL
#endif
};


/*#define DEBUG */

/* Debug config file parsing.. */
/*#define DEBUG_CONF */

#ifdef DEBUG
static void _DEBUG(int dnr)
{
    static int first = 1;
    FILE *dfile;

    dfile = fopen("svgalib.debug", (first ? "w" : "a"));
    first = 0;
    if (dfile == NULL)
	exit(1);
    fprintf(dfile, "debug #%d\n", dnr);
    fclose(dfile);
#ifndef OSKIT
    sync();
#endif /* !OSKIT */
}
#else
#define _DEBUG(d)
#endif



void __svgalib_delay(void)
{
    int i;
    for (i = 0; i < 10; i++);
}

/* The Xfree server uses a slow copy, may help us too ... */
#if defined(CONFIG_ALPHA_JENSEN)
extern unsigned long vga_readl(unsigned long base, unsigned long off);
extern void vga_writel(unsigned long b, unsigned long base, unsigned long off);
void slowcpy_from_sm(unsigned char *dest, unsigned char *src, unsigned bytes)
{
    long i;
    if (((long) dest & 7) || ((long) src & 7) || bytes & 7) {
	printf("svgalib: unaligned slowcpy()!\n");
	exit(1);
    }
    for (i = 0; i < bytes; i++) {
	*(dest + i) = (*(unsigned long *) (src + (i << 7)) >> ((i & 0x03) * 8))
	    & 0xffUL;
    }
}
void slowcpy_to_sm(unsigned char *dest, unsigned char *src, unsigned bytes)
{
    long i;
    if (((long) dest & 7) || ((long) src & 7) || bytes & 7) {
	printf("svgalib: unaligned slowcpy()!\n");
	exit(1);
    }
    for (i = 0; i < bytes; i++) {
	*(unsigned long *) (dest + (i << 7)) =
	    (*(unsigned char *) (src + i)) * 0x01010101UL;
    }
}

#else
void slowcpy(unsigned char *dest, unsigned char *src, unsigned bytes)
{
#ifdef __alpha__
    if (((long) dest & 7) || ((long) src & 7) || bytes & 7) {
	printf("svgalib: unaligned slowcpy()!\n");
	exit(1);
    }
    while (bytes > 0) {
	*(long *) dest = *(long *) src;
	dest += 8;
	src += 8;
	bytes -= 8;
    }
#else
    while (bytes-- > 0)
	*(dest++) = *(src++);
#endif
}
#endif

int __svgalib_saveregs(unsigned char *regs)
{
    int i;

    if (__svgalib_chipset == EGA || __svgalib_novga) {
	/* Special case: Don't save standard VGA registers. */
	return chipset_saveregs(regs);
    }
    /* save VGA registers */
    for (i = 0; i < CRT_C; i++) {
	port_out(i, __svgalib_CRT_I);
	regs[CRT + i] = port_in(__svgalib_CRT_D);
    }
    for (i = 0; i < ATT_C; i++) {
	port_in(__svgalib_IS1_R);
	__svgalib_delay();
	port_out(i, ATT_IW);
	__svgalib_delay();
	regs[ATT + i] = port_in(ATT_R);
	__svgalib_delay();
    }
    for (i = 0; i < GRA_C; i++) {
	port_out(i, GRA_I);
	regs[GRA + i] = port_in(GRA_D);
    }
    for (i = 0; i < SEQ_C; i++) {
	port_out(i, SEQ_I);
	regs[SEQ + i] = port_in(SEQ_D);
    }
    regs[MIS] = port_in(MIS_R);

    i = chipset_saveregs(regs);	/* save chipset-specific registers */
    /* i : additional registers */
    if (!SCREENON) {		/* We turned off the screen */
	port_in(__svgalib_IS1_R);
	__svgalib_delay();
	port_out(0x20, ATT_IW);
    }
    return CRT_C + ATT_C + GRA_C + SEQ_C + 1 + i;
}


int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    if(__svgalib_novga) return 1;

    if (__svgalib_chipset == EGA) {
	/* Enable graphics register modification */
	port_out(0x00, GRA_E0);
	port_out(0x01, GRA_E1);
    }
    /* update misc output register */
    port_out(regs[MIS], MIS_W);

    /* synchronous reset on */
    port_out(0x00, SEQ_I);
    port_out(0x01, SEQ_D);

    /* write sequencer registers */
    port_out(1, SEQ_I);
    port_out(regs[SEQ + 1] | 0x20, SEQ_D);
    for (i = 2; i < SEQ_C; i++) {
	port_out(i, SEQ_I);
	port_out(regs[SEQ + i], SEQ_D);
    }

    /* synchronous reset off */
    port_out(0x00, SEQ_I);
    port_out(0x03, SEQ_D);

    if (__svgalib_chipset != EGA) {
	/* deprotect CRT registers 0-7 */
	port_out(0x11, __svgalib_CRT_I);
	port_out(port_in(__svgalib_CRT_D) & 0x7F, __svgalib_CRT_D);
    }
    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
	port_out(i, __svgalib_CRT_I);
	port_out(regs[CRT + i], __svgalib_CRT_D);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
	port_out(i, GRA_I);
	port_out(regs[GRA + i], GRA_D);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
	port_in(__svgalib_IS1_R);		/* reset flip-flop */
	__svgalib_delay();
	port_out(i, ATT_IW);
	__svgalib_delay();
	port_out(regs[ATT + i], ATT_IW);
	__svgalib_delay();
    }

    return 0;
}

#ifndef OSKIT
static void idle_accel(void) {
    /* wait for the accel to finish, we assume one of the both interfaces suffices */
    if (vga_ext_set(VGA_EXT_AVAILABLE, VGA_AVAIL_ACCEL) & ACCELFLAG_SYNC)
        vga_accel(ACCEL_SYNC);
    else if (vga_getmodeinfo(CM)->haveblit & HAVE_BLITWAIT)
	vga_blitwait();
}
#endif

int __svgalib_getchipset(void)
{
    readconfigfile();		/* Make sure the config file is read. */

#ifndef OSKIT
    __svgalib_get_perm();
#endif /* !OSKIT */
    if (CHIPSET == UNDEFINED) {
	CHIPSET = VGA;		/* Protect against recursion */
#ifdef INCLUDE_CHIPS_DRIVER_TEST
	if (__svgalib_chips_driverspecs.test())
	    CHIPSET = CHIPS;
	else
#endif
#ifdef INCLUDE_MACH64_DRIVER_TEST
	if (__svgalib_mach64_driverspecs.test())
	    CHIPSET = MACH64;
	else
#endif
#ifdef INCLUDE_MACH32_DRIVER_TEST
	if (__svgalib_mach32_driverspecs.test())
	    CHIPSET = MACH32;
	else
#endif
#ifdef INCLUDE_EGA_DRIVER_TEST
	if (__svgalib_ega_driverspecs.test())
	    CHIPSET = EGA;
	else
#endif
#ifdef INCLUDE_ET4000_DRIVER_TEST
	if (__svgalib_et4000_driverspecs.test())
	    CHIPSET = ET4000;
	else
#endif
#ifdef INCLUDE_TVGA_DRIVER_TEST
	if (__svgalib_tvga8900_driverspecs.test())
	    CHIPSET = TVGA8900;
	else
#endif
#ifdef INCLUDE_CIRRUS_DRIVER_TEST
	    /* The Cirrus detection is not very clean. */
	if (__svgalib_cirrus_driverspecs.test())
	    CHIPSET = CIRRUS;
	else
#endif
#ifdef INCLUDE_OAK_DRIVER_TEST
	if (__svgalib_oak_driverspecs.test())
	    CHIPSET = OAK;
	else
#endif
#ifdef INCLUDE_S3_DRIVER_TEST
	if (__svgalib_s3_driverspecs.test())
	    CHIPSET = S3;
	else
#endif
#ifdef INCLUDE_ET3000_DRIVER_TEST
	if (__svgalib_et3000_driverspecs.test())
	    CHIPSET = ET3000;
	else
#endif
#ifdef INCLUDE_NV3_DRIVER_TEST
	if (__svgalib_nv3_driverspecs.test())
	    CHIPSET = NV3;
	else
#endif
#ifdef INCLUDE_ARK_DRIVER_TEST
	if (__svgalib_ark_driverspecs.test())
	    CHIPSET = ARK;
	else
#endif
#ifdef INCLUDE_GVGA6400_DRIVER_TEST
	if (__svgalib_gvga6400_driverspecs.test())
	    CHIPSET = GVGA6400;
	else
#endif
#ifdef INCLUDE_ATI_DRIVER_TEST
	if (__svgalib_ati_driverspecs.test())
	    CHIPSET = ATI;
	else
#endif
#ifdef INCLUDE_ALI_DRIVER_TEST
	if (__svgalib_ali_driverspecs.test())
	    CHIPSET = ALI;
	else
#endif
#ifdef INCLUDE_APM_DRIVER_TEST
/* Note: On certain cards this may toggle the video signal on/off which
   is ugly. Hence we test this last. */
	if (__svgalib_apm_driverspecs.test())
	    CHIPSET = APM;
	else
#endif

	if (__svgalib_vga_driverspecs.test())
	    CHIPSET = VGA;
	else
	    /* else */
	{
	    fprintf(stderr, "svgalib: Cannot find EGA or VGA graphics device.\n");
	    exit(1);
	}
	__svgalib_setpage = __svgalib_driverspecs->__svgalib_setpage;
	__svgalib_setrdpage = __svgalib_driverspecs->__svgalib_setrdpage;
	__svgalib_setwrpage = __svgalib_driverspecs->__svgalib_setwrpage;
    }
    return CHIPSET;
}

void vga_setchipset(int c)
{
    CHIPSET = c;
#ifdef DEBUG
    printf("Setting chipset\n");
#endif
    if (c == UNDEFINED)
	return;
    if (__svgalib_driverspecslist[c] == NULL) {
	printf("svgalib: Invalid chipset. The driver may not be compiled in.\n");
	CHIPSET = UNDEFINED;
	return;
    }
#ifndef OSKIT
    __svgalib_get_perm();
#endif /* !OSKIT */
    __svgalib_driverspecslist[c]->init(0, 0, 0);
    __svgalib_setpage = __svgalib_driverspecs->__svgalib_setpage;
    __svgalib_setrdpage = __svgalib_driverspecs->__svgalib_setrdpage;
    __svgalib_setwrpage = __svgalib_driverspecs->__svgalib_setwrpage;
}

void vga_setchipsetandfeatures(int c, int par1, int par2)
{
    CHIPSET = c;
#ifdef DEBUG
    printf("Forcing chipset and features\n");
#endif
#ifndef OSKIT
    __svgalib_get_perm();
#endif /* !OSKIT */
    __svgalib_driverspecslist[c]->init(1, par1, par2);
#ifdef DEBUG
    printf("Finished forcing chipset and features\n");
#endif
    __svgalib_setpage = __svgalib_driverspecs->__svgalib_setpage;
    __svgalib_setrdpage = __svgalib_driverspecs->__svgalib_setrdpage;
    __svgalib_setwrpage = __svgalib_driverspecs->__svgalib_setwrpage;
}


void savepalette(unsigned char *red, unsigned char *green,
			unsigned char *blue)
{
    int i;

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->savepalette) 
        return (__svgalib_driverspecs->emul->savepalette(red, green, blue));

    if (CHIPSET == EGA || __svgalib_novga) 
	return;

    /* save graphics mode palette - first select palette index 0 */
    port_out(0, PEL_IR);

    /* read RGB components - index is autoincremented */
    for (i = 0; i < 256; i++) {
	__svgalib_delay();
	*(red++) = port_in(PEL_D);
	__svgalib_delay();
	*(green++) = port_in(PEL_D);
	__svgalib_delay();
	*(blue++) = port_in(PEL_D);
    }
}

static void restorepalette(const unsigned char *red,
		   const unsigned char *green, const unsigned char *blue)
{
    int i;

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->restorepalette) 
        return (__svgalib_driverspecs->emul->restorepalette(red, green, blue));

    if (CHIPSET == EGA || __svgalib_novga)
	return;

    /* restore saved palette */
    port_out(0, PEL_IW);

    /* read RGB components - index is autoincremented */
    for (i = 0; i < 256; i++) {
	__svgalib_delay();
	port_out(*(red++), PEL_D);
	__svgalib_delay();
	port_out(*(green++), PEL_D);
	__svgalib_delay();
	port_out(*(blue++), PEL_D);
    }
}



#ifndef BACKGROUND

#ifndef OSKIT
/* Virtual console switching */

static int forbidvtrelease = 0;
static int forbidvtacquire = 0;
static int lock_count = 0;
static int release_flag = 0;

static void __svgalib_takevtcontrol(void);

void __svgalib_flipaway(void);
static void __svgalib_flipback(void);
#endif /* !OSKIT */

#endif

void setcoloremulation(void)
{
    /* shift to color emulation */
    __svgalib_CRT_I = CRT_IC;
    __svgalib_CRT_D = CRT_DC;
    __svgalib_IS1_R = IS1_RC;
    if (CHIPSET != EGA && !__svgalib_novga)  
	port_out(port_in(MIS_R) | 0x01, MIS_W);
}

#ifndef BACKGROUND

inline void vga_setpage(int p)
{
    p += vga_page_offset;
    if (p == __svgalib_currentpage)
	return;
    (*__svgalib_setpage) (p);
    __svgalib_currentpage = p;
}


void vga_setreadpage(int p)
{
    p += vga_page_offset;
    if (p == __svgalib_currentpage)
	return;
    (*__svgalib_setrdpage) (p);
    __svgalib_currentpage = -1;
}


void vga_setwritepage(int p)
{
    p += vga_page_offset;
    if (p == __svgalib_currentpage)
	return;
    (*__svgalib_setwrpage) (p);
    __svgalib_currentpage = -1;
}

#endif


static void prepareforfontloading(void)
{
    if (__svgalib_chipset == CIRRUS) {
	outb(0x3c4, 0x0f);
	/* Disable CRT FIFO Fast-Page mode. */
	outb(0x3c5, inb(0x3c5) | 0x40);
    }
}

static void fontloadingcomplete(void)
{
    if (__svgalib_chipset == CIRRUS) {
	outb(0x3c4, 0x0f);
	/* Re-enable CRT FIFO Fast-Page mode. */
	outb(0x3c5, inb(0x3c5) & 0xbf);
    }
}


int vga_setmode(int mode)
{
#ifdef BACKGROUND
    __svgalib_dont_switch_vt_yet();
    /* Can't initialize if screen is flipped. */
    if (__svgalib_oktowrite)
#endif
    if (!initialized)
	initialize();

    if (mode != TEXT && !chipset_modeavailable(mode))
       {
#ifdef BACKGROUND
         __svgalib_is_vt_switching_needed();
#endif
	return -1;
       }
       
#ifdef BACKGROUND
    if (!__svgalib_oktowrite)
        {
	 prv_mode=CM;
	 CM=mode;
	 vga_setpage(0);
	 __svgalib_is_vt_switching_needed();
	 return(0);
	 /* propably this was enough. */
	 /* hmm.. there... virtual screen... */
	}
#endif
       
/*    if (!flip)
   vga_lockvc(); */
#ifndef OSKIT
    disable_interrupt();
#endif /* !OSKIT */

    prv_mode = CM;
    CM = mode;

    /* disable video */
    vga_screenoff();

    if(!__svgalib_novga) {
    /* Should be more robust (eg. grabbed X modes) */
	if (__svgalib_getchipset() == ET4000
	     && prv_mode != G640x480x256
	     && SVGAMODE(prv_mode))
	    chipset_setmode(G640x480x256, prv_mode);

	/* This is a hack to get around the fact that some C&T chips
	 * are programmed to ignore syncronous resets. So if we are
	 * a C&T wait for retrace start
	 */
	if (__svgalib_getchipset() == CHIPS) {
	   while (((port_in(__svgalib_IS1_R)) & 0x08) == 0x08 );/* wait VSync off */
	   while (((port_in(__svgalib_IS1_R)) & 0x08) == 0 );   /* wait VSync on  */
	   port_outw(0x3C4,0x07);         /* reset hsync - just in case...  */
	}
    }

    if (mode == TEXT) {
	/* Returning to textmode. */

#ifndef OSKIT
	if (prv_mode != TEXT && mouse_mode == prv_mode) {
#ifdef DEBUG
	    printf("svgalib: Closing mouse.\n");
#endif
	    mouse_close();
	    mouse_mode = 0;
	    /* vga_unlockvc(); */
	}
#endif /* !OSKIT */
	if (SVGAMODE(prv_mode))
	    vga_setpage(0);


	/* The extended registers are restored either by the */
	/* chipset setregs function, or the chipset setmode function. */

	/* restore font data - first select a 16 color graphics mode */
	/* Note: this should restore the old extended registers if */
	/* setregs is not defined for the chipset. */
        if(__svgalib_novga) __svgalib_driverspecs->setmode(TEXT, prv_mode);
        if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->restorefont) {
           __svgalib_driverspecs->emul->restorefont(); 
	   chipset_setregs(text_regs, mode);
        } else if(!__svgalib_novga) {
	  __svgalib_driverspecs->setmode(GPLANE16, prv_mode);

	  if (CHIPSET != EGA)
	      /* restore old extended regs */
	      chipset_setregs(text_regs, mode);

	  /* disable Set/Reset Register */
	  port_out(0x01, GRA_I);
	  port_out(0x00, GRA_D);

	  prepareforfontloading();

	  /* restore font data in plane 2 - necessary for all VGA's */
	  port_out(0x02, SEQ_I);
	  port_out(0x04, SEQ_D);
#ifdef __alpha__
	  port_out(0x06, GRA_I);
	  port_out(0x00, GRA_D);
#endif

#ifdef BACKGROUND
          if (-1 == mprotect(font_buf1,FONT_SIZE*2,PROT_READ|PROT_WRITE))
          {
	   printf("svgalib: Memory protect error\n");
	   exit(-1);
	  }
#endif

#if defined(CONFIG_ALPHA_JENSEN)
	  slowcpy_to_sm(SM, font_buf1, FONT_SIZE);
#else
	  slowcpy(GM, font_buf1, FONT_SIZE);
#endif

	  /* restore font data in plane 3 - necessary for Trident VGA's */
	  port_out(0x02, SEQ_I);
	  port_out(0x08, SEQ_D);
#if defined(CONFIG_ALPHA_JENSEN)
	  slowcpy_to_sm(SM, font_buf2, FONT_SIZE);
#else
	  slowcpy(GM, font_buf2, FONT_SIZE);
#endif
#ifdef BACKGROUND
          if (-1 == mprotect(font_buf1,FONT_SIZE*2,PROT_READ))
          {
	   printf("svgalib: Memory protect error\n");
	   exit(1);
	  }
#endif
	  fontloadingcomplete();

	  /* change register adresses if monochrome text mode */
	  /* EGA is assumed to use color emulation. */
	  if (!color_text) {
	      __svgalib_CRT_I = CRT_IM;
	      __svgalib_CRT_D = CRT_DM;
	      __svgalib_IS1_R = IS1_RM;
	      port_out(port_in(MIS_R) & 0xFE, MIS_W);
	  }
        } else chipset_setregs(text_regs, mode);
	/* restore saved palette */
	restorepalette(text_red, text_green, text_blue);

	/* restore text mode VGA registers */
	__svgalib_setregs(text_regs);

	/* Set VMEM to some minimum value .. probably pointless.. */
	{
	    vga_claimvideomemory(12);
	}

#ifndef OSKIT
/*      if (!flip) */
	/* enable text output - restores the screen contents */
	ioctl(__svgalib_tty_fd, KDSETMODE, KD_TEXT);
#endif /* !OSKIT */

	usleep(MODESWITCHDELAY);	/* wait for signal to stabilize */

	/* enable video */
	vga_screenon();

#ifndef OSKIT
	if (!flip)
	    /* restore text mode termio */
	    set_texttermio();
#endif /* !OSKIT */
    } else {
	/* Setting a graphics mode. */

#ifndef OSKIT
	/* disable text output */
	ioctl(__svgalib_tty_fd, KDSETMODE, KD_GRAPHICS);
#endif

	if (SVGAMODE(prv_mode)) {
	    /* The current mode is an SVGA mode, and we now want to */
	    /* set a standard VGA mode. Make sure the extended regs */
	    /* are restored. */
	    /* Also used when setting another SVGA mode to hopefully */
	    /* eliminate lock-ups. */
	    vga_setpage(0);
	    chipset_setregs(text_regs, mode);
	    /* restore old extended regs */
	}
	/* shift to color emulation */
	setcoloremulation();

	CI.xdim = infotable[mode].xdim;
	CI.ydim = infotable[mode].ydim;
	CI.colors = infotable[mode].colors;
	CI.xbytes = infotable[mode].xbytes;
	CI.bytesperpixel = infotable[mode].bytesperpixel;

	chipset_setmode(mode, prv_mode);

	MODEX = 0;

	/* Set default claimed memory (moved here from initialize - Michael.) */
	if (mode == G320x200x256)
	    VMEM = 65536;
	else if (STDVGAMODE(mode))
	    VMEM = 256 * 1024;	/* Why always 256K ??? - Michael */
	else {
	    vga_modeinfo *modeinfo;

	    modeinfo = vga_getmodeinfo(mode);
	    VMEM = modeinfo->linewidth * modeinfo->height;
            CI.xbytes = modeinfo->linewidth;
	}

	if (!flip) {
	    /* set default palette */
	    if (CI.colors <= 256)
		restorepalette(default_red, default_green, default_blue);

	    /* clear screen (sets current color to 15) */
	    __svgalib_currentpage = -1;
	    vga_clear();

	    if (SVGAMODE(__svgalib_cur_mode))
		vga_setpage(0);
	}
	__svgalib_currentpage = -1;
	currentlogicalwidth = CI.xbytes;
	currentdisplaystart = 0;

	usleep(MODESWITCHDELAY);	/* wait for signal to stabilize */

	/* enable video */
#ifndef OSKIT
	if (!flip)
#endif /* !OSKIT */
	    vga_screenon();

#ifndef OSKIT
	if (mouse_support) {
#ifdef DEBUG
	    printf("svgalib: Opening mouse (type = %x).\n", mouse_type | mouse_modem_ctl);
#endif
	    if (mouse_init(mouse_device, mouse_type | mouse_modem_ctl, MOUSE_DEFAULTSAMPLERATE))
		printf("svgalib: Failed to initialize mouse.\n");
	    else {
		/* vga_lockvc(); */
		mouse_setxrange(0, CI.xdim - 1);
		mouse_setyrange(0, CI.ydim - 1);
		mouse_setwrap(MOUSE_NOWRAP);
		mouse_mode = mode;
	    }
	} 
#endif /* !OSKIT */
	{
	    vga_modeinfo *modeinfo;
	    modeinfo = vga_getmodeinfo(mode);
	    MODEX = ((MODEFLAGS = modeinfo->flags) & IS_MODEX);
	}
#ifndef OSKIT
	if (!flip)
	    /* set graphics mode termio */
	    set_graphtermio();
	else if (__svgalib_kbd_fd < 0)
	    enable_interrupt();
#endif /* !OSKIT */

    }

/*    if (!flip)
   vga_unlockvc(); */
#ifdef BACKGROUND
 __svgalib_is_vt_switching_needed();
#endif
    return 0;
}

void vga_gettextfont(void *font)
{
    unsigned int getsize;

    getsize = fontbufsize;
    if (getsize > FONT_SIZE)
	getsize = FONT_SIZE;
    memcpy(font, font_buf1, getsize);
    if (fontbufsize > getsize)
	memset(((char *)font) + getsize, 0, (size_t)(fontbufsize - getsize));
}

void vga_puttextfont(void *font)
{
    unsigned int putsize;

#ifdef BACKGROUND
        if (-1 == mprotect(font_buf1,FONT_SIZE*2,PROT_READ|PROT_WRITE))
        {
	 printf("svgalib: Memory protect error\n");
	 exit(-1);
	}
#endif

    putsize = fontbufsize;
    if (putsize > FONT_SIZE)
	putsize = FONT_SIZE;
    memcpy(font_buf1, font, putsize);
    memcpy(font_buf2, font, putsize);
    if (putsize < FONT_SIZE) {
	memset(font_buf1 + putsize, 0, (size_t)(FONT_SIZE - putsize));
	memset(font_buf2 + putsize, 0, (size_t)(FONT_SIZE - putsize));
    }

#ifdef BACKGROUND
        if (-1 == mprotect(font_buf1,FONT_SIZE*2,PROT_READ))
        {
	 printf("svgalib: Memory protect error\n");
	 exit(-1);
	}
#endif
}


void vga_gettextmoderegs(void *regs)
{
    memcpy(regs, text_regs, MAX_REGS);
}

void vga_settextmoderegs(void *regs)
{
    memcpy(text_regs, regs, MAX_REGS);
}

int vga_getcurrentmode(void)
{
    return CM;
}

int vga_getcurrentchipset(void)
{
    return __svgalib_getchipset();
}

void vga_disabledriverreport(void)
{
    DREP = 0;
}

vga_modeinfo *vga_getmodeinfo(int mode)
{
    static vga_modeinfo modeinfo;
    int is_modeX = (CM == mode) && MODEX;

    modeinfo.linewidth = infotable[mode].xbytes;
    __svgalib_getchipset();
    if (mode > vga_lastmodenumber())
	return NULL;
    modeinfo.width = infotable[mode].xdim;
    modeinfo.height = infotable[mode].ydim;
    modeinfo.bytesperpixel = infotable[mode].bytesperpixel;
    modeinfo.colors = infotable[mode].colors;
    if (is_modeX) {
	modeinfo.linewidth = modeinfo.width / 4;
	modeinfo.bytesperpixel = 0;
    }
    if (mode == TEXT) {
	modeinfo.flags = HAVE_EXT_SET;
	return &modeinfo;
    }
    modeinfo.flags = 0;
    if ((STDVGAMODE(mode) && mode != G320x200x256) || is_modeX)
	__svgalib_vga_driverspecs.getmodeinfo(mode, &modeinfo);
    else
	/* Get chipset specific info for SVGA modes and */
	/* 320x200x256 (chipsets may support more pages) */
	chipset_getmodeinfo(mode, &modeinfo);

    if (modeinfo.colors == 256 && modeinfo.bytesperpixel == 0)
	modeinfo.flags |= IS_MODEX;
    if (mode > __GLASTMODE)
	modeinfo.flags |= IS_DYNAMICMODE;

    /* Maskout CAPABLE_LINEAR if requested by config file */
    modeinfo.flags &= modeinfo_mask;

    /* If all needed info is here, signal if linear support has been enabled */
    if ((modeinfo.flags & (CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) ==
	(CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) {
	modeinfo.flags |= __svgalib_modeinfo_linearset;
    }

#ifdef BACKGROUND
    if (__svgalib_runinbackground) /* these cannot be provided if we are really in background */
	modeinfo.flags &= ~HAVE_RWPAGE;
#endif

    return &modeinfo;
}


int vga_hasmode(int mode)
{
    readconfigfile();		/* Make sure the config file is read. */
    __svgalib_getchipset();		/* Make sure the chipset is known. */
    if (mode == TEXT)
	return 1;
    if (mode < 0 || mode > lastmodenumber)
	return 0;
    return (chipset_modeavailable(mode) != 0);
}


int vga_lastmodenumber(void)
{
    __svgalib_getchipset();
    return lastmodenumber;
}


int __svgalib_addmode(int xdim, int ydim, int cols, int xbytes, int bytespp)
{
    int i;

    for (i = 0; i <= lastmodenumber; ++i)
	if (infotable[i].xdim == xdim &&
	    infotable[i].ydim == ydim &&
	    infotable[i].colors == cols &&
	    infotable[i].bytesperpixel == bytespp &&
	    infotable[i].xbytes == xbytes)
	    return i;
    if (lastmodenumber >= MAX_MODES - 1)
	return -1;		/* no more space available */
    ++lastmodenumber;
    infotable[lastmodenumber].xdim = xdim;
    infotable[lastmodenumber].ydim = ydim;
    infotable[lastmodenumber].colors = cols;
    infotable[lastmodenumber].xbytes = xbytes;
    infotable[lastmodenumber].bytesperpixel = bytespp;
    return lastmodenumber;
}

int vga_setcolor(int color)
{
    switch (CI.colors) {
    case 2:
	if (color != 0)
	    color = 15;
    case 16:			/* update set/reset register */
#ifdef BACKGROUND
        __svgalib_dont_switch_vt_yet();
        if (!__svgalib_oktowrite)
           {
            color=color & 15;
            COL = color;
           }
	 else
	   {
#endif
	port_out(0x00, GRA_I);
	port_out((color & 15), GRA_D);
#ifdef BACKGROUND
           }
        __svgalib_is_vt_switching_needed();
#endif
	break;
    default:
	COL = color;
	break;
    }
    return 0;
}


int vga_screenoff(void)
{
    int tmp = 0;

    SCREENON = 0;

    if(__svgalib_novga) return 0; 
#ifdef BACKGROUND 
    __svgalib_dont_switch_vt_yet();
    if (!__svgalib_oktowrite) {
	__svgalib_is_vt_switching_needed();
	return(0);
    }
#endif

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenoff) {
	tmp = __svgalib_driverspecs->emul->screenoff();
    } else {
	/* turn off screen for faster VGA memory acces */
	if (CHIPSET != EGA) {
	    port_out(0x01, SEQ_I);
	    port_out(port_in(SEQ_D) | 0x20, SEQ_D);
	}
	/* Disable video output */
#ifdef DISABLE_VIDEO_OUTPUT
	port_in(__svgalib_IS1_R);
	__svgalib_delay();
	port_out(0x00, ATT_IW);
#endif
    }

#ifdef BACKGROUND
    __svgalib_is_vt_switching_needed();
#endif
    return tmp;
}


int vga_screenon(void)
{
    int tmp = 0;

    SCREENON = 1;
    if(__svgalib_novga) return 0; 
#ifdef BACKGROUND
    __svgalib_dont_switch_vt_yet();
    if (!__svgalib_oktowrite) {
	__svgalib_is_vt_switching_needed();
	return(0);
    }
#endif
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenon) {
	tmp = __svgalib_driverspecs->emul->screenon();
    } else {
	/* turn screen back on */
	if (CHIPSET != EGA) {
	    port_out(0x01, SEQ_I);
	    port_out(port_in(SEQ_D) & 0xDF, SEQ_D);
	}
/* #ifdef DISABLE_VIDEO_OUTPUT */
	/* enable video output */
	port_in(__svgalib_IS1_R);
	__svgalib_delay();
	port_out(0x20, ATT_IW);
/* #endif */
    }

#ifdef BACKGROUND
    __svgalib_is_vt_switching_needed();
#endif
    return 0;
}


int vga_getxdim(void)
{
    return CI.xdim;
}


int vga_getydim(void)
{
    return CI.ydim;
}


int vga_getcolors(void)
{
    return CI.colors;
}

int vga_white(void)
{
    switch (CI.colors) {
    case 2:
    case 16:
    case 256:
	return 15;
    case 1 << 15:
	return 32767;
    case 1 << 16:
	return 65535;
    case 1 << 24:
	return (1 << 24) - 1;
    }
    return CI.colors - 1;
}

int vga_claimvideomemory(int m)
{
    vga_modeinfo *modeinfo;
    int cardmemory;

    modeinfo = vga_getmodeinfo(CM);
    if (m < VMEM)
	return 0;
    if (modeinfo->colors == 16)
	cardmemory = modeinfo->maxpixels / 2;
    else
	cardmemory = (modeinfo->maxpixels * modeinfo->bytesperpixel
		      + 2) & 0xffff0000;
    /* maxpixels * bytesperpixel can be 2 less than video memory in */
    /* 3 byte-per-pixel modes; assume memory is multiple of 64K */
    if (m > cardmemory)
	return -1;
    VMEM = m;
    return 0;
}

int vga_setmodeX(void)
{
    switch (CM) {
    case TEXT:
/*    case G320x200x256: */
    case G320x240x256:
    case G320x400x256:
    case G360x480x256:
	return 0;
    }
    if (CI.colors == 256 && VMEM < 256 * 1024) {
	port_out(4, SEQ_I);	/* switch from linear to plane memory */
	port_out((port_in(SEQ_D) & 0xF7) | 0x04, SEQ_D);
	port_out(0x14, __svgalib_CRT_I);	/* switch double word mode off */
	port_out((port_in(__svgalib_CRT_D) & 0xBF), __svgalib_CRT_D);
	port_out(0x17, __svgalib_CRT_I);
	port_out((port_in(__svgalib_CRT_D) | 0x40), __svgalib_CRT_D);
	CI.xbytes = CI.xdim / 4;
	vga_setpage(0);
	MODEX = 1;
	return 1;
    }
    return 0;
}

#ifdef NOTYET

static int saved_page;
static int saved_logicalwidth;
static int saved_displaystart;
static int saved_modeX;

static void savestate(void)
{
    int i;

    vga_screenoff();

    savepalette(graph_red, graph_green, graph_blue);

    saved_page = __svgalib_currentpage;
    saved_logicalwidth = currentlogicalwidth;
    saved_displaystart = currentdisplaystart;
    saved_modeX = MODEX;

    if (CM == G320x200x256 && VMEM <= 65536) {
#ifndef BACKGROUND
	/* 320x200x256 is a special case; only 64K is addressable */
	/* (unless more has been claimed, in which case we assume */
	/* SVGA bank-switching) */
	if ((graph_buf = malloc(GRAPH_SIZE)) == NULL) {
#endif	
#ifdef BACKGROUND
#if BACKGROUND == 1
        if ((graph_buf = valloc(GRAPH_SIZE)) == NULL)
	    {
#endif
#if BACKGROUND == 2
	if ((graph_buf = malloc(GRAPH_SIZE)) == NULL) {
#endif
#endif
	    printf("Cannot allocate memory for VGA state\n");
	    vga_setmode(TEXT);
	    exit(1);
	}
	memcpy(graph_buf, GM, GRAPH_SIZE);
    } else if (MODEX || CM == G800x600x16 || (STDVGAMODE(CM) && CM != G320x200x256)) {
	/* for planar VGA modes, save the full 256K */
	__svgalib_vga_driverspecs.setmode(GPLANE16, prv_mode);
#ifndef BACKGROUND	
	if ((graph_buf = malloc(4 * GRAPH_SIZE)) == NULL) {
#endif	
#ifdef BACKGROUND
#if BACKGROUND == 1
        if ((graph_buf = valloc(4 * GRAPH_SIZE)) == NULL)
	    {
#endif
#if BACKGROUND == 2
        if ((graph_buf = malloc(4 * GRAPH_SIZE)) == NULL) {
#endif 
#endif	
	    printf("Cannot allocate memory for VGA state\n");
	    vga_setmode(TEXT);
	    exit(1);
	}
	for (i = 0; i < 4; i++) {
	    /* save plane i */
	    port_out(0x04, GRA_I);
	    port_out(i, GRA_D);
	    memcpy(graph_buf + i * GRAPH_SIZE, GM, GRAPH_SIZE);
	}
    } else if (CI.colors == 16) {
	int page, size, sbytes;
	unsigned char *sp;

	size = VMEM;
#ifndef BACKGROUND
	if ((graph_buf = malloc(4 * size)) == NULL) {
#endif
#ifdef BACKGROUND
#if BACKGROUND == 1
	if ((graph_buf = valloc(4 * size)) == NULL) {
#endif
#if BACKGROUND == 2
	if ((graph_buf = malloc(4 * size)) == NULL) {
#endif
#endif
	    printf("Cannot allocate memory for VGA state\n");
	    vga_setmode(TEXT);
	    exit(1);
	}
	sp = graph_buf;
	for (page = 0; size > 0; ++page) {
	    vga_setpage(page);
	    sbytes = (size > GRAPH_SIZE) ? GRAPH_SIZE : size;
	    for (i = 0; i < 4; i++) {
		/* save plane i */
		port_out(0x04, GRA_I);
		port_out(i, GRA_D);
		memcpy(sp, GM, sbytes);
		sp += sbytes;
	    }
	    size -= sbytes;
	}
    } else {			/* SVGA, and SVGA 320x200x256 if videomemoryused > 65536 */
	int size;
	int page;

	size = VMEM;

#ifdef DEBUG
	printf("Saving %dK of video memory.\n", (size + 2) / 1024);
#endif
#ifndef BACKGROUND
	if ((graph_buf = malloc(size)) == NULL) {
#endif
#ifdef BACKGROUND
#if BACKGROUND == 1
        /* Must allocate hole videopage. */
	if ((graph_buf = valloc((size/GRAPH_SIZE+1)*GRAPH_SIZE)) == NULL) {
#endif
#if BACKGROUND == 2
	if ((graph_buf = malloc(size)) == NULL) {
#endif
#endif
	    printf("Cannot allocate memory for SVGA state.\n");
	    vga_setmode(TEXT);
	    exit(1);
	}
	page = 0;
	while (size >= 65536) {
	    vga_setpage(page);
	    memcpy(graph_buf + page * 65536, GM, 65536);
	    page++;
	    size -= 65536;
	}
#ifdef BACKGROUND
#if BACKGROUND == 1
        /* Whole page must be written for mmap(). */
	if (size > 0) {
	    vga_setpage(page);
	    memcpy(graph_buf + page * 65536, GM, GRAPH_SIZE);
	}
#endif
#if BACKGROUND == 2
	if (size > 0) {
	    vga_setpage(page);
	    memcpy(graph_buf + page * 65536, GM, size);
	}
#endif
#endif
#ifndef BACKGROUND
	if (size > 0) {
	    vga_setpage(page);
	    memcpy(graph_buf + page * 65536, GM, size);
	}
#endif
    }
}

static void restorestate(void)
{
    int i;

    vga_screenoff();

    if (saved_modeX)
	vga_setmodeX();

    restorepalette(graph_red, graph_green, graph_blue);

    if (CM == G320x200x256 && VMEM <= 65536) {
	memcpy(GM, graph_buf, 65536);
    } else if (MODEX || CM == G800x600x16 || (STDVGAMODE(CM) && CM != G320x200x256)) {
	int setresetreg, planereg;
	/* disable Set/Reset Register */
	port_out(0x01, GRA_I);
	setresetreg = inb(GRA_D);
	port_out(0x00, GRA_D);
	outb(SEQ_I, 0x02);
	planereg = inb(SEQ_D);

	for (i = 0; i < 4; i++) {
	    /* restore plane i */
	    port_out(0x02, SEQ_I);
	    port_out(1 << i, SEQ_D);
	    memcpy(GM, graph_buf + i * GRAPH_SIZE, GRAPH_SIZE);
	}
	outb(GRA_I, 0x01);
	outb(GRA_D, setresetreg);
	outb(SEQ_I, 0x02);
	outb(SEQ_D, planereg);
    } else if (CI.colors == 16) {
	int page, size, rbytes;
	unsigned char *rp;
	int setresetreg, planereg;

	/* disable Set/Reset Register */
	port_out(0x01, GRA_I);
	if (CHIPSET == EGA)
	    setresetreg = 0;
	else
	    setresetreg = inb(GRA_D);
	port_out(0x00, GRA_D);
	port_out(0x02, SEQ_I);
	if (CHIPSET == EGA)
	    planereg = 0;
	else
	    planereg = inb(SEQ_D);

	size = VMEM;
	rp = graph_buf;
	for (page = 0; size > 0; ++page) {
	    vga_setpage(page);
	    rbytes = (size > GRAPH_SIZE) ? GRAPH_SIZE : size;
	    for (i = 0; i < 4; i++) {
		/* save plane i */
		port_out(0x02, SEQ_I);
		port_out(1 << i, SEQ_D);
		memcpy(GM, rp, rbytes);
		rp += rbytes;
	    }
	    size -= rbytes;
	}

	outb(GRA_I, 0x01);
	outb(GRA_D, setresetreg);
	outb(SEQ_I, 0x02);
	outb(SEQ_D, planereg);
    } else {
/*              vga_modeinfo *modeinfo; */
	int size;
	int page;
	size = VMEM;

#ifdef DEBUG
	printf("Restoring %dK of video memory.\n", (size + 2) / 1024);
#endif
	page = 0;
	while (size >= 65536) {
	    vga_setpage(page);
	    memcpy(GM, graph_buf + page * 65536, 65536);
	    size -= 65536;
	    page++;
	}
	if (size > 0) {
	    vga_setpage(page);
	    memcpy(GM, graph_buf + page * 65536, size);
	}
    }

    if (saved_logicalwidth != CI.xbytes)
	vga_setlogicalwidth(saved_logicalwidth);
    if (saved_page != 0)
	vga_setpage(saved_page);
    if (saved_displaystart != 0)
	vga_setdisplaystart(saved_displaystart);

    vga_screenon();

    free(graph_buf);
}


int vga_getch(void)
{
    char c;

    if (CM == TEXT)
	return -1;

    while ((read(__svgalib_tty_fd, &c, 1) < 0) && (errno == EINTR));

    return c;
}

#ifdef BACKGROUND
int __svgalib_flip_status(void)

{
 return(flip);
}
#endif

/* I have kept the slightly funny 'flip' terminology. */

void __svgalib_flipaway(void)
{
    /* Leaving console. */
    flip_mode = CM;
#ifndef SVGA_AOUT
    __joystick_flip_vc(0);
#endif
    if (CM != TEXT) {
	/* wait for any blitter operation to finish */
	idle_accel();
	/* Save state and go to textmode. */
	savestate();
	flip = 1;
	vga_setmode(TEXT);
	flip = 0;

#ifdef BACKGROUND
/* Let's put bg screen to right place */

#if BACKGROUND == 1
        if (__svgalib_currentpage<0) __svgalib_currentpage=0;
        __svgalib_map_virtual_screen(__svgalib_currentpage);
#endif
#if BACKGROUND == 2
        __svgalib_graph_mem=graph_buf+(GRAPH_SIZE*__svgalib_currentpage);
#endif
	__svgalib_oktowrite=0; /* screen is fliped, not __svgalib_oktowrite. */
#endif
    }
}

#ifndef BACKGROUND
static void __svgalib_flipback(void)
#endif
#ifdef BACKGROUND
void __svgalib_flipback(void)
#endif
{
#ifdef BACKGROUND
 int tmp_page=__svgalib_currentpage;
#endif
    /* Entering console. */
    /* Hmmm... and how about unlocking anything someone else locked? */
#ifndef SVGA_AOUT
    __joystick_flip_vc(1);
#endif
    chipset_unlock();
    if (flip_mode != TEXT) {
	/* Restore graphics mode and state. */
#ifdef BACKGROUND
	__svgalib_oktowrite=1;
#endif
	flip = 1;
	vga_setmode(flip_mode);
	flip = 0;
#ifdef BACKGROUND
#if BACKGROUND == 1
        __svgalib_map_virtual_screen(1000000);	
#endif
#if BACKGROUND == 2
        __svgalib_graph_mem=__svgalib_graph_mem_orginal;
#endif
#endif	
	restorestate();
#ifdef BACKGROUND
 /* Has to make sure right page is on. */
        vga_setpage(tmp_page);
	vga_setcolor(COL);
#endif
       }
}

int vga_flip(void)
{
#ifdef BACKGROUND
 int tmp_page=__svgalib_currentpage;
#endif    
    if (CM != TEXT) {		/* save state and go to textmode */
	savestate();
	flip_mode = CM;
	flip = 1;
	vga_setmode(TEXT);
	flip = 0;
#ifdef BACKGROUND
/* Lets put bg screen to right place */

#if BACKGROUND == 1
        if (__svgalib_currentpage<0) __svgalib_currentpage=0;
        __svgalib_map_virtual_screen(__svgalib_currentpage);
#endif
#if BACKGROUND == 2
        __svgalib_graph_mem=graph_buf+(GRAPH_SIZE*__svgalib_currentpage);;
#endif
	__svgalib_oktowrite=0; /* screen is fliped, not __svgalib_oktowrite. */
#endif
    } else {			/* restore graphics mode and state */
#ifdef BACKGROUND
        __svgalib_oktowrite=1;
	/* Probably here too ...  */
	chipset_unlock();
#endif
	flip = 1;
	vga_setmode(flip_mode);
	flip = 0;
#ifdef BACKGROUND
#if BACKGROUND == 1
        __svgalib_map_virtual_screen(1000000);	
#endif
#if BACKGROUND == 2
        __svgalib_graph_mem=__svgalib_graph_mem_orginal;
#endif
#endif	
	restorestate();
#ifdef BACKGROUND
 /* Has to make sure right page is on. */
        vga_setpage(tmp_page);
	vga_setcolor(COL);
#endif
    }
    return 0;
}


int vga_setflipchar(int c)
/* This function is obsolete. Retained for VGAlib compatibility. */
{
    __svgalib_flipchar = c;

    return 0;
}

#endif /* NOTYET */

void vga_setlogicalwidth(int w)
{
    __svgalib_driverspecs->setlogicalwidth(w);
    currentlogicalwidth = w;
}

void vga_setdisplaystart(int a)
{
    currentdisplaystart = a;
    if (CHIPSET != VGA && CHIPSET != EGA)
	if (MODEX || CI.colors == 16) {
	    /* We are currently using a Mode X-like mode on a */
	    /* SVGA card, use the standard VGA function */
	    /* that works properly for Mode X. */
	    /* Same goes for 16 color modes. */
	    __svgalib_vga_driverspecs.setdisplaystart(a);
	    return;
	}
    /* Call the regular display start function for the chipset */
    __svgalib_driverspecs->setdisplaystart(a);
}

void vga_bitblt(int srcaddr, int destaddr, int w, int h, int pitch)
{
    __svgalib_driverspecs->bitblt(srcaddr, destaddr, w, h, pitch);
}

void vga_imageblt(void *srcaddr, int destaddr, int w, int h, int pitch)
{
    __svgalib_driverspecs->imageblt(srcaddr, destaddr, w, h, pitch);
}

void vga_fillblt(int destaddr, int w, int h, int pitch, int c)
{
    __svgalib_driverspecs->fillblt(destaddr, w, h, pitch, c);
}

void vga_hlinelistblt(int ymin, int n, int *xmin, int *xmax, int pitch,
		      int c)
{
    __svgalib_driverspecs->hlinelistblt(ymin, n, xmin, xmax, pitch, c);
}

void vga_blitwait(void)
{
    __svgalib_driverspecs->bltwait();
}

int vga_ext_set(unsigned what,...)
{
    va_list params;
    register int retval = 0;

    switch(what) {
	case VGA_EXT_AVAILABLE:
	    /* Does this use of the arglist corrupt non-AVAIL_ACCEL ext_set? */
	    va_start(params, what);
	    switch (va_arg(params, int)) {
		case VGA_AVAIL_ACCEL:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->operations;
		break;
	    case VGA_AVAIL_ROP:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->ropOperations;
		break;
	    case VGA_AVAIL_TRANSPARENCY:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->transparencyOperations;
		break;
	    case VGA_AVAIL_ROPMODES:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->ropModes;
		break;
	    case VGA_AVAIL_TRANSMODES:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->transparencyModes;
		break;
	    case VGA_AVAIL_SET:
		retval = (1 << VGA_EXT_PAGE_OFFSET) |
			 (1 << VGA_EXT_FONT_SIZE);	/* These are handled by us */
		break;
	    }
	    va_end(params);
	    break;
	case VGA_EXT_PAGE_OFFSET:
	    /* Does this use of the arglist corrupt it? */
	    va_start(params, what);
	    retval = vga_page_offset;
	    vga_page_offset = va_arg(params, int);
	    va_end(params);
	    return retval;
	case VGA_EXT_FONT_SIZE:
	    va_start(params, what);
	    what = va_arg(params, unsigned int);
	    va_end(params);
	    if (!what)
		return FONT_SIZE;
	    retval = fontbufsize;
	    fontbufsize = what;
	    return retval;
    }
    if ((CM != TEXT) && (MODEFLAGS & HAVE_EXT_SET)) {
	va_start(params, what);
	retval |= __svgalib_driverspecs->ext_set(what, params);
	va_end(params);
    }
    return retval;
}

#ifndef OSKIT
int vga_getmousetype(void)
{
    readconfigfile();
    return mouse_type | mouse_modem_ctl;
}
#endif /* !OSKIT */

int vga_getmonitortype(void)
{				/* obsolete */
    int i;
    readconfigfile();
    for (i = 1; i <= MON1024_72; i++)
	if (__svgalib_horizsync.max < __svgalib_maxhsync[i])
	    return i - 1;

    return MON1024_72;
}

#ifndef OSKIT
void vga_setmousesupport(int s)
{
    mouse_support = s;
}
#endif /* !OSKIT */

#ifdef BACKGROUND

unsigned char *__svgalib_give_graph_red(void)

{
 return(graph_red);
}

unsigned char *__svgalib_give_graph_green(void)

{
 return(graph_green);
}

unsigned char *__svgalib_give_graph_blue(void)

{
 return(graph_blue);
}

#endif

int vga_oktowrite(void)
{
    return __svgalib_oktowrite;
}


int vga_init(void)
{
    int retval = 0;

#ifndef OSKIT
    __svgalib_open_devconsole();
    if (__svgalib_tty_fd < 0) {
	/* Return with error code. */
	/* Since the configuration file hasnt been read yet, security will
	   only be set to 1 if it has been compiled into the program */
	retval = -1;
    } else {
#endif /* !OSKIT */
        readconfigfile();
        vga_hasmode(TEXT); /* Force driver message and initialization. */
#ifndef OSKIT
    }
    /* Michael: I assume this is a misunderstanding, when svgalib was developed,
       there were no saved uids, thus setting effective uid sufficed... */
    if ( __svgalib_security_revokeallprivs == 1 ) {
	setuid(getuid());  
	setgid(getgid());
    }
    seteuid(getuid());
    setegid(getgid());
#endif /* !OSKIT */
    return retval;
}

#ifdef NOTYET

#ifdef __alpha__

#define vuip	volatile unsigned int *

extern void sethae(unsigned long hae);

static struct hae hae;


int iopl(int level)
{
    return 0;
}

unsigned long vga_readb(unsigned long base, unsigned long off)
{
    unsigned long result, shift;
#if !defined(CONFIG_ALPHA_JENSEN)
    unsigned long msb;
#endif

    shift = (off & 0x3) * 8;
#if !defined(CONFIG_ALPHA_JENSEN)
    if (off >= (1UL << 24)) {
	msb = off & 0xf8000000;
	off -= msb;
	if (msb && msb != hae.cache) {
	    sethae(msb);
	}
    }
#endif
    result = *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_BYTE);
    result >>= shift;
    return 0xffUL & result;
}

unsigned long vga_readw(unsigned long base, unsigned long off)
{
    unsigned long result, shift;
#if !defined(CONFIG_ALPHA_JENSEN)
    unsigned long msb;
#endif

    shift = (off & 0x3) * 8;
#if !defined(CONFIG_ALPHA_JENSEN)
    if (off >= (1UL << 24)) {
	msb = off & 0xf8000000;
	off -= msb;
	if (msb && msb != hae.cache) {
	    sethae(msb);
	}
    }
#endif
    result = *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_WORD);
    result >>= shift;
    return 0xffffUL & result;
}

#if defined(CONFIG_ALPHA_JENSEN)
unsigned long vga_readl(unsigned long base, unsigned long off)
{
    unsigned long result;
    result = *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_LONG);
    return 0xffffffffUL & result;
}
#endif

void vga_writeb(unsigned char b, unsigned long base, unsigned long off)
{
#if !defined(CONFIG_ALPHA_JENSEN)
    unsigned long msb;
    unsigned int w;

    if (off >= (1UL << 24)) {
	msb = off & 0xf8000000;
	off -= msb;
	if (msb && msb != hae.cache) {
	    sethae(msb);
	}
    }
  asm("insbl %2,%1,%0": "r="(w):"ri"(off & 0x3), "r"(b));
    *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_BYTE) = w;
#else
    *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_BYTE) = b * 0x01010101;
#endif
}

void vga_writew(unsigned short b, unsigned long base, unsigned long off)
{
#if !defined(CONFIG_ALPHA_JENSEN)
    unsigned long msb;
    unsigned int w;

    if (off >= (1UL << 24)) {
	msb = off & 0xf8000000;
	off -= msb;
	if (msb && msb != hae.cache) {
	    sethae(msb);
	}
    }
  asm("inswl %2,%1,%0": "r="(w):"ri"(off & 0x3), "r"(b));
    *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_WORD) = w;
#else
    *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_WORD) = b * 0x00010001;
#endif
}

#if defined(CONFIG_ALPHA_JENSEN)
void vga_writel(unsigned long b, unsigned long base, unsigned long off)
{
    *(vuip) ((off << MEM_SHIFT) + base + MEM_TYPE_LONG) = b;
}

#endif

#endif


#endif /* !NOTYET */
