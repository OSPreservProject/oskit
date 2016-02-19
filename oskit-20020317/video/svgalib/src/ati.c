/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993, 1999 Harm Hanemaayer */

/*
 * Initial ATI Driver January 1995, Scott D. Heavner (sdh@po.cwru.edu)
 */

/* ==== SO FAR THE ONLY SPECIAL THING THIS ATI DRIVER DOES IS ALLOW 
 * ==== GRAPHICS MODES TO FUNCTION WITH A 132col TERMINAL, THERE ARE NO
 * ==== NEW MODES YET (there will be).  No the XF86 driver won't help as
 * ==== XF863.1 doesn't work properly with my ATI Graphics Ultra, 
 * ==== I will be having a look at the XF86 drivers now that I've found
 * ==== the problem an implemented a fix here.
 *
 * ---- If this driver detects a mach32 chip, it does not init the ATI
 * ---- driver, hopefully, the autodetect will find the Mach32 driver.
 * ---- We should probably change this to run the Mach32 as an ATI SVGA
 * ---- board, but probe for the Mach32 first, this would allow the user
 * ---- to force ATI mode in a config file.
 */

#include <stdio.h>
#include <stdlib.h>		/* for NULL */
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "vgaregs.h"

#ifdef OSKIT
#include "osenv.h"
#endif

#define ATI_UNKNOWN	0
#define	ATI_18800	1
#define	ATI_18800_1	2
#define	ATI_28800_2	3
#define	ATI_28800_4	4
#define	ATI_28800_5	5
#define	ATI_68800	6

#define ATIREG(s)	(EXT+(s)-0xa0)

#define ATI_TOTAL_REGS	(VGA_TOTAL_REGS + 0xbf - 0xa0)

#define MIN(a,b)	(((a)<(b))?(a):(b))
#define ABS(a)		(((a)<0)?(-(a)):(a))

#define ATIGETEXT(index)	(outb(ati_base, (index)), inb(1 + ati_base))
#define ATISETEXT(index, value)	outw(ati_base, ((value) << 8 | (index)))

static unsigned char ati_flags_42, ati_flags_44;
static int ati_chiptype;
static int ati_memory;
static short ati_base = 0;
static char *ati_name[] =
{"unknown", "18800", "18800-1", "28800-2", "28800-4", "28800-5", "68800"};

static void nothing(void)
{
}

/* Initialize chipset (called after detection) */
static int ati_init(int force, int par1, int par2)
{
    __svgalib_driverspecs = &__svgalib_ati_driverspecs;

    /* Have to give ourselves some more permissions -- last port currently used is 0x3df */
    if (ioperm(MIN(ati_base, 0x3df), ABS(0x3df - ati_base), 1)) {
	printf("IOPERM FAILED IN ATI\n");
	exit(-2);
    }
    /*
     * Find out how much video memory the VGA Wonder side thinks it has.
     */
    if (ati_chiptype < ATI_28800_2) {
	if (ATIGETEXT(0xbb) & 0x20)
	    ati_memory = 512;
	else
	    ati_memory = 256;
    } else {
	unsigned char b = ATIGETEXT(0xB0);

	if (b & 0x08)
	    ati_memory = 1024;
	else if (b & 0x10)
	    ati_memory = 512;
	else
	    ati_memory = 256;
    }

    if (__svgalib_driver_report)
	printf("Using ATI (mostly VGA) driver, (%s, %dK).\n", ati_name[ati_chiptype], ati_memory);

    return 1;
}

/* Read and store chipset-specific registers */
static int ati_saveregs(unsigned char regs[])
{
    regs[ATIREG(0xb2)] = ATIGETEXT(0xb2);

    if (ati_chiptype > ATI_18800) {
	regs[ATIREG(0xbe)] = ATIGETEXT(0xbe);
	if (ati_chiptype >= ATI_28800_2) {
	    regs[ATIREG(0xbf)] = ATIGETEXT(0xbf);
	    regs[ATIREG(0xa3)] = ATIGETEXT(0xa3);
	    regs[ATIREG(0xa6)] = ATIGETEXT(0xa6);
	    regs[ATIREG(0xa7)] = ATIGETEXT(0xa7);
	    regs[ATIREG(0xab)] = ATIGETEXT(0xab);
	    regs[ATIREG(0xac)] = ATIGETEXT(0xac);
	    regs[ATIREG(0xad)] = ATIGETEXT(0xad);
	    regs[ATIREG(0xae)] = ATIGETEXT(0xae);
	}
    }
    regs[ATIREG(0xb0)] = ATIGETEXT(0xb0);
    regs[ATIREG(0xb1)] = ATIGETEXT(0xb1);
    regs[ATIREG(0xb3)] = ATIGETEXT(0xb3);
    regs[ATIREG(0xb5)] = ATIGETEXT(0xb5);
    regs[ATIREG(0xb6)] = ATIGETEXT(0xb6);
    regs[ATIREG(0xb8)] = ATIGETEXT(0xb8);
    regs[ATIREG(0xb9)] = ATIGETEXT(0xb9);
    regs[ATIREG(0xba)] = ATIGETEXT(0xba);
    regs[ATIREG(0xbd)] = ATIGETEXT(0xbd);

    return ATI_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */
static void ati_setregs(const unsigned char regs[], int mode)
{
    ATISETEXT(0xb2, regs[ATIREG(0xb2)]);

    if (ati_chiptype > ATI_18800) {
	ATISETEXT(0xbe, regs[ATIREG(0xbe)]);
	if (ati_chiptype >= ATI_28800_2) {
	    ATISETEXT(0xbf, regs[ATIREG(0xbf)]);
	    ATISETEXT(0xa3, regs[ATIREG(0xa3)]);
	    ATISETEXT(0xa6, regs[ATIREG(0xa6)]);
	    ATISETEXT(0xa7, regs[ATIREG(0xa7)]);
	    ATISETEXT(0xab, regs[ATIREG(0xab)]);
	    ATISETEXT(0xac, regs[ATIREG(0xac)]);
	    ATISETEXT(0xad, regs[ATIREG(0xad)]);
	    ATISETEXT(0xae, regs[ATIREG(0xae)]);
	}
    }
    ATISETEXT(0xb0, regs[ATIREG(0xb0)]);
    ATISETEXT(0xb1, regs[ATIREG(0xb1)]);
    ATISETEXT(0xb3, regs[ATIREG(0xb3)]);
    ATISETEXT(0xb5, regs[ATIREG(0xb5)]);
    ATISETEXT(0xb6, regs[ATIREG(0xb6)]);
    ATISETEXT(0xb8, regs[ATIREG(0xb8)]);
    ATISETEXT(0xb9, regs[ATIREG(0xb9)]);
    ATISETEXT(0xba, regs[ATIREG(0xba)]);
    ATISETEXT(0xbd, regs[ATIREG(0xbd)]);
}

/* Fill in chipset specific mode information */
static void ati_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    __svgalib_vga_driverspecs.getmodeinfo(mode, modeinfo);
}

/* Return non-zero if mode is available */
static int ati_modeavailable(int mode)
{
    return __svgalib_vga_driverspecs.modeavailable(mode);
}

/* Set a mode */
static int ati_setmode(int mode, int prv_mode)
{
    unsigned char regs[ATI_TOTAL_REGS];

    if (mode != TEXT) {
	/* Mess with the timings before passing it off to the vga driver */
	ati_saveregs(regs);
	regs[ATIREG(0xb8)] |= 0x40;
	regs[ATIREG(0xbe)] |= 0x10;
	ati_setregs(regs, mode);
    }
    return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
}

/*
 * Lifted from the genoa driver, maybe we should let everyone do the checks
 * while we have the vbios mmapped.
 */
static unsigned char *map_vbios(void)
{
    unsigned char *vga_bios;

    /* Changed to use valloc(). */
#ifdef OSKIT
    osenv_mem_map_phys(0xC0000, 4096, &vga_bios, 0);
#else
    if ((vga_bios = valloc(4096)) == NULL) {
	fprintf(stderr, "svgalib: malloc error\n");
	exit(-1);
    }
    vga_bios = (unsigned char *) mmap
	(
	    (caddr_t) vga_bios,
	    4096,
	    PROT_READ,
	    MAP_SHARED | MAP_FIXED,
	    __svgalib_mem_fd,
	    0xc0000
	);
#endif

    if ((long) vga_bios < 0) {
	fprintf(stderr, "svgalib: mmap error\n");
	exit(-1);
    }
    return vga_bios;
}

static int ati_test(void)
{
    int result;
    unsigned char *vga_bios;

    vga_bios = map_vbios();

    if (strncmp("761295520", (vga_bios + 0x31), 9) ||	/* Identify as an ATI product */
	strncmp("31", (vga_bios + 0x40), 2)) {	/* Identify as an ATI super vga (vs EGA/Basic-16) */
	result = 0;
    } else {
	result = 1;

	memcpy(&ati_base, (vga_bios + 0x10), 2);

	switch (vga_bios[0x43]) {	/* Identify Gate revision */
	case '1':
	    ati_chiptype = ATI_18800;
	    break;
	case '2':
	    ati_chiptype = ATI_18800_1;
	    break;
	case '3':
	    ati_chiptype = ATI_28800_2;
	    break;
	case '4':
	    ati_chiptype = ATI_28800_4;
	    break;
	case '5':
	    ati_chiptype = ATI_28800_5;
	    break;
	case 'a':
	    ati_chiptype = ATI_68800;	/* Mach 32, should someone else handle this ? *
					 *                                            *
					 * On Mach32 we come here only if chipset ATI *
					 * was EXPLICITLY set (what we support) - MW  */
	    break;
	default:
	    ati_chiptype = ATI_UNKNOWN;
	}

	ati_flags_42 = vga_bios[0x42];
	ati_flags_44 = vga_bios[0x44];

    }

    if (result)
	result = ati_init(0, 0, 0);

    munmap((caddr_t) vga_bios, 4096);

    return (result);

}

#if 0
/* Set 64k bank (r/w) number */
static void ati_setpage(int bank)
{
    unsigned char b, mask;

    mask = (ati_chiptype > ATI_18800_1) ? 0x0f : 0x07;

    b = ((bank & mask) << 4) | (bank & mask);
    ATISETEXT(0xb2, b);
}

static void ati_setrdpage(int bank)
{
}

static void ati_setwrpage(int bank)
{
}

#endif

/* Set display start */
static void ati_setdisplaystart(int address)
{
#if 0
    unsigned char b;

    b = ((address >> 10) & ~0x3f) | (ATIGETEXT(0xb0) & 0x3f);
    ATISETEXT(0xb0, b);
#endif
    __svgalib_vga_driverspecs.setdisplaystart(address);
}

/* Set logical scanline length (usually multiple of 8) */
static void ati_setlogicalwidth(int width)
{
    __svgalib_vga_driverspecs.setlogicalwidth(width);
}

/* Function table (exported) */
DriverSpecs __svgalib_ati_driverspecs =
{
    ati_saveregs,
    ati_setregs,
    nothing,			/* unlock */
    nothing,			/* lock */
    ati_test,
    ati_init,
    (void (*)(int)) nothing,	/* __svgalib_setpage */
    (void (*)(int)) nothing,	/* __svgalib_setrdpage */
    (void (*)(int)) nothing,	/* __svgalib_setwrpage */
    ati_setmode,
    ati_modeavailable,
    ati_setdisplaystart,
    ati_setlogicalwidth,
    ati_getmodeinfo,
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
