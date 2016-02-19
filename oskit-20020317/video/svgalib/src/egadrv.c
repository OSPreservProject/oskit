/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* EGA support from EGAlib by Kapil Paranjape */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */


#include <stdio.h>
#include <string.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

static int ega_init(int force, int par1, int par2);

/* BIOS mode 0Dh - 320x200x16 */
static char g320x200x16_regs[60] =
{
  0x37, 0x27, 0x2D, 0x37, 0x30, 0x14, 0x04, 0x11, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xE1, 0x24, 0xC7, 0x14, 0x00, 0xE0, 0xF0, 0xE3,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,
    0x03, 0x0B, 0x0F, 0x00, 0x06,
    0x23
};

/* BIOS mode 0Eh - 640x200x16 */
static char g640x200x16_regs[60] =
{
  0x70, 0x4F, 0x59, 0x2D, 0x5E, 0x06, 0x04, 0x11, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xE0, 0x23, 0xC7, 0x28, 0x00, 0xDF, 0xEF, 0xE3,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,
    0x03, 0x01, 0x0F, 0x00, 0x06,
    0x23
};

/* BIOS mode 10h - 640x350x16 */
static char g640x350x16_regs[60] =
{
  0x5B, 0x4F, 0x53, 0x37, 0x52, 0x00, 0x6C, 0x1F, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x5E, 0x2B, 0x5E, 0x28, 0x0F, 0x5F, 0x0A, 0xE3,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,
    0x03, 0x01, 0x0F, 0x00, 0x06,
    0xA7
};

/* EGA registers for saved text mode 03* */
static char text_regs[60] =
{
  0x5B, 0x4F, 0x53, 0x37, 0x51, 0x5B, 0x6C, 0x1F, 0x00, 0x0D, 0x0a, 0x0c,
  0x00, 0x00, 0x00, 0x00, 0x5E, 0x2B, 0x5D, 0x28, 0x0F, 0x5E, 0x0A, 0xA3,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x0A, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF,
    0x03, 0x01, 0x03, 0x00, 0x03,
    0xA7
};

/* Mode table */
static ModeTable __ega_modes[] =
{
/* *INDENT-OFF* */
    OneModeEntry(320x200x16),
    OneModeEntry(640x200x16),
    OneModeEntry(640x350x16),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

static ModeTable *ega_modes = NULL;


/* Fill in chipset-specific modeinfo */

static void getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    /* Taken from vga 16 colors. Correct ??? */
    modeinfo->maxpixels = 65536 * 8;
    modeinfo->startaddressrange = 0x7ffff;
    modeinfo->maxlogicalwidth = 2040;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~(IS_INTERLACED | HAVE_RWPAGE);
}

static void nothing(void)
{
}


/* Return nonzero if mode available */

static int modeavailable(int mode)
{
    const unsigned char *regs;

    regs = LOOKUPMODE(ega_modes, mode);
    if (regs != NULL && regs != DISABLE_MODE)
	return STDVGADRV;
    return 0;
}

static int lastmode = TEXT;

static int saveregs(unsigned char *regs)
{
    /* We can't read the registers from an EGA card. */
    /* We just report the expected values. */

    const unsigned char *r;
    int i;

    if (lastmode == TEXT)
	r = text_regs;
    else
	r = LOOKUPMODE(ega_modes, lastmode);
    if (r == NULL) {
	printf("svgalib: egadrv.c/saveregs(): internal error\n");
	exit(-1);
    }
    memcpy(regs, r, CRT_C + ATT_C + GRA_C + SEQ_C + MIS_C);

    /* save all readable EGA registers; others are default */
    /* is this correct (even in graphics mode) ?? */
    for (i = 0x0C; i < 0x10; i++) {
	port_out(i, __svgalib_CRT_I);
	regs[CRT + i] = port_in(__svgalib_CRT_D);
    }

    return CRT_C + ATT_C + GRA_C + SEQ_C + MIS_C;
}


/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{
    /* Enable graphics register modification */
    port_out(0x00, GRA_E0);
    port_out(0x01, GRA_E1);

    __svgalib_setregs(regs);
}

/* Set a mode */

static int setmode(int mode, int prv_mode)
/* standard EGA driver: setmode */
{
    const unsigned char *regs;

    regs = LOOKUPMODE(ega_modes, mode);
    if (regs == NULL || regs == DISABLE_MODE)
	return 1;
    lastmode = mode;

    setregs(regs, mode);
    return 0;
}


/* Set display start */

static void setdisplaystart(int address)
{
    inb(__svgalib_IS1_R);
    __svgalib_delay();
    outb(ATT_IW, 0x13 + 0x20);
    __svgalib_delay();
    outb(ATT_IW, (address & 7));
    /* write sa0-2 to bits 0-2 */
    address >>= 3;
    outw(__svgalib_CRT_I, 0x0d + (address & 0x00ff) * 256);	/* sa0-sa7 */
    outw(__svgalib_CRT_I, 0x0c + (address & 0xff00));	/* sa8-sa15 */
}

static void setlogicalwidth(int width)
{
    outw(__svgalib_CRT_I, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
}


/* Indentify chipset; return non-zero if detected */

static int ega_test(void)
{
    unsigned char save, back;

    /* Check if a DAC is present */
    save = inb(PEL_IW);
    __svgalib_delay();
    outb(PEL_IW, ~save);
    __svgalib_delay();
    back = inb(PEL_IW);
    __svgalib_delay();
    outb(PEL_IW, save);
    save = ~save;
    if (back != save) {
	ega_init(0, 0, 0);
	return 1;
    }
    return 0;
}


DriverSpecs __svgalib_ega_driverspecs =
{				/* EGA */
    saveregs,
    setregs,
    nothing /* unlock */ ,
    nothing /* lock */ ,
    ega_test,
    ega_init,
    (void (*)(int)) nothing /* __svgalib_setpage */ ,
    (void (*)(int)) nothing /* __svgalib_setrdpage */ ,
    (void (*)(int)) nothing /* __svgalib_setwrpage */ ,
    setmode,
    modeavailable,
    setdisplaystart,
    setlogicalwidth,
    getmodeinfo,
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

static int ega_init(int force, int par1, int par2)
{

    if (__svgalib_driver_report)
	printf("Using EGA driver.\n");

    /* Read additional modes from file if available */
    if (ega_modes == NULL) {
	ega_modes = __ega_modes;
#ifdef EGA_REGS
	{
	    FILE *regs;
	    regs = fopen(EGA_REGS, "r");
	    if (regs != 0) {
		__svgalib_readmodes(regs, &ega_modes, NULL, NULL);
		fclose(regs);
	    }
	}
#endif
    }
    __svgalib_driverspecs = &__svgalib_ega_driverspecs;

    return 0;
}
