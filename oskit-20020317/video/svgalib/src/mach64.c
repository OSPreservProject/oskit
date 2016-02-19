/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */
/* THIS DRIVER DOES NOT WORK */



/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993, 1999 Harm Hanemaayer */

/* 
   Initial ATI Mach64 Driver. 
   August 1995. 
   Asad F. Hanif (w81h@unb.ca)
   Copyright (C) 1995 Asad F. Hanif (w81h@unb.ca)
 */

/*
   Note: This code is based on the material provided to me by
   the folks at ATI.  As well portions are based on
   the Mach64 XFree86 Server code.  

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "vga.h"
#include "libvga.h"
#include "driver.h"

#ifdef OSKIT
#include "osenv.h"
#endif

#include "mach64.h"

/* globals */
static int DUMPINDEX;
static char DUMPOUT[33];
static US ioBaseAddress, PciIOLocation;
static UB cTabSize, *RomLocation;
static UL *mreg;

static M64_ClockInfo ClockInfo;
static M64_MaxColorDepth MaxCDT1[50], MaxCDT2[50], FreqCDT1[50], FreqCDT2[50];
static M64_StartUpInfo StartupInfo;
/*static CRTC_Table *pcrtc; */

/* prototypes */
/* svgalib - driverspec functions */
static void nothing(void);
static int mach64_init(int, int, int);
static int mach64_test(void);
static int mach64_saveregs(UB *);
static void mach64_setregs(const UB *, int);
static void mach64_getmodeinfo(int, vga_modeinfo *);
static int mach64_modeavailable(int);
static int mach64_setmode(int, int);
static void mach64_setdisplaystart(int);
static void mach64_setlogicalwidth(int);
static void mach64_setpage(int);

/* misc. functions */
static UB *mach64_rom_base(void);
static UB *mach64_map_vbios(UB *);

/*static void mach64_unmap_vbios(UB *,UB *); */

static void mach64_get_alot_of_info(UB *);
static void mach64_report(void);

/* dac control and programming */
#define DAC_TYPE	StartupInfo.Dac_Type
#define DAC_SUBTYPE	StartupInfo.Dac_Subtype

static int mach64_dac_ati68860(US, US);
static void mach64_dac_program(US, US, US);
static void mach64_crtc_programming(US, US, US);

static void mach64_small_aperature(void);

/* regs to save */
static int sr_accel[13] =
{ioCRTC_GEN_CNTL,
 ioCRTC_H_SYNC_STRT_WID,
 ioCRTC_H_TOTAL_DISP,
 ioCRTC_V_SYNC_STRT_WID,
 ioCRTC_V_TOTAL_DISP,
 ioCRTC_OFF_PITCH,
 ioCLOCK_SEL_CNTL,
 ioOVR_CLR,
 ioOVR_WID_LEFT_RIGHT,
 ioOVR_WID_TOP_BOTTOM,
 ioMEM_CNTL,
 ioMEM_VGA_RP_SEL,
 ioMEM_VGA_WP_SEL};

/* Driverspecs function table (exported) */
DriverSpecs __svgalib_mach64_driverspecs =
{
    mach64_saveregs,
    mach64_setregs,
    (void (*)(void)) nothing,	/* unlock */
    (void (*)(void)) nothing,	/* lock */
    mach64_test,
    mach64_init,
    mach64_setpage,		/* __svgalib_setpage */
    (void (*)(int)) nothing,	/* __svgalib_setrdpage */
    (void (*)(int)) nothing,	/* __svgalib_setwrpage */
    mach64_setmode,
    mach64_modeavailable,
    mach64_setdisplaystart,
    mach64_setlogicalwidth,
    mach64_getmodeinfo,
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

/* empty function */
static void nothing(void)
{
}

/***************************************************************************/

/* Initialize chipset */
static int mach64_init(int force, int par1, int par2)
{
    UB *atibios;

    if (__svgalib_driver_report)
	fprintf(stdout, "Using ATI Mach64 driver.\n");

    __svgalib_driverspecs = &__svgalib_mach64_driverspecs;

    /* Make sure we are operating at IO priveleges are level 3 */
    if (iopl(3) < 0) {
	fprintf(stdout, "iopl(3) failed in Mach64\n");
	exit(-1);
    }
    /* download  configuration data */
    atibios = mach64_map_vbios(RomLocation);
    mach64_get_alot_of_info(atibios);
/* 
   This seems to be a cause of segfaults.
   mach64_unmap_vbios(RomLocation,atibios);
 */
    mach64_report();

#ifdef BAILOUT
    fprintf(stdout, "\n*** WARNING ALPHA DRIVER ***\n" \
	    "*** If the above diagnostics look ok then press 'Y' to" \
	    "continue.\n\n");
    if (getchar() != 'Y') {
	fprintf(stdout, "\n*** Bailing out! ***\n");
	exit(-1);
    }
#endif
    return 1;
}

/* test for presence of mach64 */
static int mach64_test(void)
{
    UB *atibios;
    UB ati_sig1[] = "761295520";
    UB ati_sig2[] = "GXCX";
    UB ati_sig3[] = "MACH64";
    int ati_found, i;
    UL sv;
    US romtabptr, *romoff;

    /* Make sure we are operating at IO privlege level 3 */
    if (iopl(3) < 0) {
	fprintf(stdout, "iopl(3) failed in Mach64\n");
	exit(-1);
    }
    /* !!should loop through the relocatable bios areas - instead */
    /* !!should read the address cause it may be relocateable */
    RomLocation = mach64_rom_base();

    /* map the vga bios */
    atibios = mach64_map_vbios(RomLocation);

    /* scan for the signature */
    ati_found = 0;
    for (i = 0; i < 128; i++) {
	if (strncmp(ati_sig1, atibios + i, sizeof(ati_sig1) - 1) == 0) {
	    ati_found = 1;
	    break;
	}
    }

    /* scan for either GXCX or MACH64 */
    for (i = 0; i < 1024; i++) {
	if (strncmp(ati_sig2, atibios + i, sizeof(ati_sig2) - 1) == 0) {
	    ati_found++;
	    break;
	}
	if (strncmp(ati_sig3, atibios + i, sizeof(ati_sig3) - 1) == 0) {
	    ati_found++;
	    break;
	}
    }

    /* if its not even ati return a failure */
    if (ati_found < 2)
	return 0;

    /* grab the address for io access - fixed and relocatable from bios 
       I can't tell if its VLB or PCI yet.  If the ioBaseAddress is not 
       0x2EC, 0x1CC, or 0x1C8 then its obviosly relocatable (we hope).
       If its relocatable then we pray its PCI. */

    romoff = (US *) atibios;
    romtabptr = romoff[0x48 >> 1];
    ioBaseAddress = romoff[(romtabptr >> 1) + 2];
    PciIOLocation = romoff[(romtabptr >> 1) + 13];
    switch (ioBaseAddress) {
    case 0x2ec:
	break;
    case 0x1cc:
    case 0x1c8:
    default:
	fprintf(stdout, "M64: Can't handle io address %x yet.\n", 
		ioBaseAddress);
	exit(-1);
    }
#ifdef DEBUG
    fprintf(stdout, "M64: IO Base Address: %x\n", ioBaseAddress);
#endif
    /* unmap the bios */
/*      This seems to be the cause of segfaults 
   mach64_unmap_vbios(RomLocation,atibios);
 */
    /* do the scratch register io confirmation test */
    /* assume not mach64 */
    ati_found = 0;

    /* save scratch value */
    sv = inl(ioSCRATCH_REG0);

    /* test odd bits for readability */
    outl(ioSCRATCH_REG0, 0x55555555);
    if (inl(ioSCRATCH_REG0) == 0x55555555) {
	/* test even bits for readability */
	outl(ioSCRATCH_REG0, 0xaaaaaaaa);
	if (inl(ioSCRATCH_REG0) == 0xaaaaaaaa) {
	    ati_found = 1;
	}
    }
    /* restore */
    outl(ioSCRATCH_REG0, sv);

    return (ati_found) ? mach64_init(0, 0, 0) : ati_found;
}

/**************************************************************************/
/* locate rom locatation */
static UB *mach64_rom_base(void)
{
    US a;
    UB *b;

    a = inw(ioSCRATCH_REG1);
    a &= 0x7f;
    a += 0xc000;
    b = (UB *) (a * 0x10);

    return (b);
}

/* map the vga bios - pretty much lifted from ati.c */
static UB *mach64_map_vbios(UB * map_me)
{
    UB *vga_bios;

    /* allocate 32k for bios map */
#ifdef OSKIT
    osenv_mem_map_phys(map_me, M64_BIOS_SIZE, &vga_bios, 0);
#else
    if ((vga_bios = valloc(M64_BIOS_SIZE)) == NULL) {
	fprintf(stdout, "SVGALIB valloc failure\n");
	exit(-1);
    }
    vga_bios = (UB *) mmap((caddr_t) vga_bios,
			   M64_BIOS_SIZE,
			   PROT_READ,
			   MAP_SHARED | MAP_FIXED,
			   __svgalib_mem_fd,
			   (off_t) map_me);
#endif

    if ((long) vga_bios < 0) {
	fprintf(stdout, "SVGALIB mmap failure\n");
	exit(-1);
    }
    return (UB *) vga_bios;
}

/*
   static void mach64_unmap_vbios(UB *unmap_me,UB *free_me)
   {
   if (munmap((caddr_t)unmap_me,M64_BIOS_SIZE)) {
   fprintf(stdout,"SVGALIB munmap failure\n");
   exit(-1);
   }
   free((void *)free_me);
   }
 */

/****************************************************************************/

/* collect information from rom and registers */
static void mach64_get_alot_of_info(UB * mapped_area)
{
    UB *bbios;
    US *sbios;
    int i, j;
    US RomTableOff, FreqTablePtr, CdepthTablePtr;
    UL RegL;
    UB RegB;
    UB mask;

    /* setup bios area mappings and initial offsets */
    sbios = (US *) mapped_area;
    bbios = (UB *) mapped_area;

    RomTableOff = sbios[0x48 >> 1];
    FreqTablePtr = sbios[(RomTableOff >> 1) + 8];

    /* grab the CONFIG_CHIP_ID information */
    RegL = inl(ioCONFIG_CHIP_ID);
    StartupInfo.Chip_Type = (US) (RegL & 0xffff);
    StartupInfo.Chip_Class = (UB) ((RegL >> 16) && 0xff);
    StartupInfo.Chip_Rev = (UB) ((RegL >> 24) && 0xff);

    switch (StartupInfo.Chip_Type) {
    case 0xd7:
	if (StartupInfo.Chip_Rev <= 0x03)
	    StartupInfo.Chip_ID_Index = StartupInfo.Chip_Rev;
	else
	    StartupInfo.Chip_ID_Index = 0x04;
	break;
    case 0x57:
	if (StartupInfo.Chip_Rev == 0x01)
	    StartupInfo.Chip_ID_Index = 0x05;
	else
	    StartupInfo.Chip_ID_Index = 0x06;
	break;
    case 0x4354:
	StartupInfo.Chip_ID_Index = 0x07;
	break;
    case 0x4554:
	StartupInfo.Chip_ID_Index = 0x08;
	break;
    default:
	StartupInfo.Chip_ID_Index = 0x09;
    }

#ifdef REPORT
    if (StartupInfo.Chip_ID_Index < 0x07)
	fprintf(stdout, "M64: ATI Mach64 88800 %s detected.\n",
		M64_Chip_Name[StartupInfo.Chip_ID_Index]);
    else
	fprintf(stdout, "M64: ATI Mach64 88800 %s Rev. %x detected.\n",
		M64_Chip_Name[StartupInfo.Chip_ID_Index],
		StartupInfo.Chip_Rev);
#endif

    /* read the CONFIG_STAT0 and CONFIG_STAT1 */
    RegL = inl(ioCONFIG_STAT0);
    StartupInfo.Cfg_Bus_type = (UB) (RegL & 0x07);
    StartupInfo.Cfg_Ram_type = (UB) ((RegL >> 3) & 0x7);
    RegL = inl(ioCONFIG_STAT1);

#ifdef REPORT
    fprintf(stdout, "M64: Bus Type: %s\n",
	    M64_Bus_Name[StartupInfo.Cfg_Bus_type]);
    fprintf(stdout, "M64: Memory Type: %s\n",
	    M64_Ram_Name[StartupInfo.Cfg_Ram_type]);
#endif

    /* read memory configurations stuff */
    RegL = inl(ioMEM_CNTL);
    StartupInfo.Mem_Size = (UB) (RegL & 0x07);

#ifdef REPORT
    fprintf(stdout, "M64: Memory Installed: %d Kilobytes\n",
	    M64_Mem_Size[StartupInfo.Mem_Size]);
#endif

    /* read the dac type and sub type */
    RegB = inb(ioDAC_CNTL + 2);
    StartupInfo.Dac_Type = RegB & 0x07;
    RegB = inb(ioSCRATCH_REG1 + 1);
    StartupInfo.Dac_Subtype = (RegB & 0xf0) | StartupInfo.Dac_Type;

#ifdef REPORT
    fprintf(stdout, "M64: Dac Type: %d %s Sub Type: %d\n",
	    StartupInfo.Dac_Type,
	    M64_Dac_Name[StartupInfo.Dac_Type],
	    StartupInfo.Dac_Subtype);
#endif

    /* grab a pile of clock info */
    ClockInfo.ClockType = bbios[FreqTablePtr + 0];
    ClockInfo.MinFreq = sbios[(FreqTablePtr >> 1) + 1];
    ClockInfo.MaxFreq = sbios[(FreqTablePtr >> 1) + 2];
    ClockInfo.RefFreq = sbios[(FreqTablePtr >> 1) + 4];
    ClockInfo.RefDivdr = sbios[(FreqTablePtr >> 1) + 5];
    ClockInfo.N_adj = sbios[(FreqTablePtr >> 1) + 6];
    ClockInfo.Dram_Mem_Clk = sbios[(FreqTablePtr >> 1) + 8];
    ClockInfo.Vram_Mem_Clk = sbios[(FreqTablePtr >> 1) + 9];
    ClockInfo.Coproc_Mem_Clk = sbios[(FreqTablePtr >> 1) + 12];
    ClockInfo.CX_Clk = bbios[FreqTablePtr + 6];
    ClockInfo.VGA_Clk = bbios[FreqTablePtr + 7];
    ClockInfo.Mem_Clk_Entry = bbios[FreqTablePtr + 22];
    ClockInfo.SClk_Entry = bbios[FreqTablePtr + 23];
    ClockInfo.CX_DMcycle = bbios[RomTableOff + 0];
    ClockInfo.VGA_DMcycle = bbios[RomTableOff + 0];
    ClockInfo.CX_VMcycle = bbios[RomTableOff + 0];
    ClockInfo.VGA_VMcycle = bbios[RomTableOff + 0];
    CdepthTablePtr = sbios[(FreqTablePtr >> 1) - 3];
    FreqTablePtr = sbios[(FreqTablePtr >> 1) - 1];
    FreqTablePtr >>= 1;

    /* Read the default clocks */
    for (i = 0; i <= M64_CLOCK_MAX; i++)
	ClockInfo.ClkFreqTable[i] = sbios[FreqTablePtr + i];

    /* grab the color depth tables */
    cTabSize = bbios[CdepthTablePtr - 1];

#ifdef REPORT
    fprintf(stdout, "M64: ClockType: %d %s\n", ClockInfo.ClockType,
	    M64_Clock_Name[ClockInfo.ClockType]);
    fprintf(stdout, "M64: MinFreq: %d\n", ClockInfo.MinFreq);
    fprintf(stdout, "M64: MaxFreq: %d\n", ClockInfo.MaxFreq);
    fprintf(stdout, "M64: RefFreq: %d\n", ClockInfo.RefFreq);
    fprintf(stdout, "M64: RefDivdr: %d\n", ClockInfo.RefDivdr);
    fprintf(stdout, "M64: N_adj: %d\n", ClockInfo.N_adj);
    fprintf(stdout, "M64: DramMemClk: %d\n", ClockInfo.Dram_Mem_Clk);
    fprintf(stdout, "M64: VramMemClk: %d\n", ClockInfo.Vram_Mem_Clk);
    fprintf(stdout, "M64: CoprocMemClk: %d\n", ClockInfo.Coproc_Mem_Clk);
    fprintf(stdout, "M64: CX_Clk: %d\n", ClockInfo.CX_Clk);
    fprintf(stdout, "M64: VGA_Clk: %d\n", ClockInfo.VGA_Clk);
    fprintf(stdout, "M64: Mem_Clk_Entry: %d\n", ClockInfo.Mem_Clk_Entry);
    fprintf(stdout, "M64: SClk_Entry: %d\n", ClockInfo.SClk_Entry);
    fprintf(stdout, "M64: V/D_MCycle: %d\n", ClockInfo.CX_DMcycle);

    for (i = 0; i <= M64_CLOCK_MAX; i++) {
	if (i % 8 == 0)
	    fprintf(stdout, "\nM64: Clocks: ");
	fprintf(stdout, "%d.%d ", ClockInfo.ClkFreqTable[i] / 100,
		ClockInfo.ClkFreqTable[i] % 100);
    }
    fprintf(stdout, "\n");
#endif

    /* Get color depth tables that are valid for current dac */
    mask = 1 << StartupInfo.Dac_Type;

    for (j = i = 0; bbios[CdepthTablePtr + i] != 0; i += cTabSize) {
	if (bbios[CdepthTablePtr + i + 1] & mask) {
	    MaxCDT1[j].h_disp = bbios[CdepthTablePtr + i];
	    MaxCDT1[j].dacmask = bbios[CdepthTablePtr + i + 1];
	    MaxCDT1[j].ram_req = bbios[CdepthTablePtr + i + 2];
	    MaxCDT1[j].max_dot_clk = bbios[CdepthTablePtr + i + 3];
	    MaxCDT1[j].color_depth = bbios[CdepthTablePtr + i + 4];
	    ++j;
	}
    }
    MaxCDT1[j].h_disp = 0;

    /* Get second color depth table if exists that are valid subtype */
    CdepthTablePtr += i + 2;
    cTabSize = bbios[CdepthTablePtr - 1];
    for (j = i = 0; bbios[CdepthTablePtr + i] != 0; i += cTabSize) {
	if (bbios[CdepthTablePtr + i + 1] == StartupInfo.Dac_Subtype) {
	    MaxCDT2[j].h_disp = bbios[CdepthTablePtr + i];
	    MaxCDT2[j].dacmask = bbios[CdepthTablePtr + i + 1];
	    MaxCDT2[j].ram_req = bbios[CdepthTablePtr + i + 2];
	    MaxCDT2[j].max_dot_clk = bbios[CdepthTablePtr + i + 3];
	    MaxCDT2[j].color_depth = bbios[CdepthTablePtr + i + 4];
	    ++j;
	    fprintf(stdout, "%d\n", j);
	}
    }
    MaxCDT2[j].h_disp = 0;

    /* Filter out the useless entries based on bpp and memory */
    mask = StartupInfo.Cfg_Ram_type & VRAM_MASK;
    mask = (mask) ? 4 : 0;


    /* table 1 */
    for (i = 0, j = 0; MaxCDT1[i].h_disp != 0; i++) {
	/* pick out 8 bpp modes for now */
	switch (MaxCDT1[i].color_depth) {
	case BPP_8:
	    if ((MaxCDT1[i].ram_req >> mask) >
		StartupInfo.Mem_Size)
		break;

	    FreqCDT1[j].h_disp = MaxCDT1[i].h_disp;
	    FreqCDT1[j].dacmask = MaxCDT1[i].dacmask;
	    FreqCDT1[j].ram_req = MaxCDT1[i].ram_req;
	    FreqCDT1[j].max_dot_clk = MaxCDT1[i].max_dot_clk;
	    FreqCDT1[j].color_depth = MaxCDT1[i].color_depth;
	    j++;
	    break;
	case BPP_32:
	case BPP_16:
	case BPP_15:
	case BPP_4:
	default:
	    break;
	}
	FreqCDT1[j].h_disp = 0;
    }
    /* table 2 */
    for (i = 0, j = 0; MaxCDT2[i].h_disp != 0; i++) {
	/* pick out 8 bpp modes for now */
	switch (MaxCDT2[i].color_depth) {
	case BPP_8:
	    if ((MaxCDT2[i].ram_req >> mask) >
		StartupInfo.Mem_Size)
		break;

	    FreqCDT2[j].h_disp = MaxCDT2[i].h_disp;
	    FreqCDT2[j].dacmask = MaxCDT2[i].dacmask;
	    FreqCDT2[j].ram_req = MaxCDT2[i].ram_req;
	    FreqCDT2[j].max_dot_clk = MaxCDT2[i].max_dot_clk;
	    FreqCDT2[j].color_depth = MaxCDT2[i].color_depth;
	    j++;
	    break;
	case BPP_32:
	case BPP_16:
	case BPP_15:
	case BPP_4:
	default:
	    break;
	}
	FreqCDT2[j].h_disp = 0;
    }

#ifdef REPORT_CLKTAB
    fprintf(stdout, "Color Depth Table 1\n");
    for (i = 0; MaxCDT1[i].h_disp != 0; i++) {
	fprintf(stdout, "%d %d %d %d %d\n", MaxCDT1[i].h_disp,
		MaxCDT1[i].dacmask, MaxCDT1[i].ram_req,
		MaxCDT1[i].max_dot_clk, MaxCDT1[i].color_depth);
    }
    fprintf(stdout, "Color Depth Table 2 - if present\n");
    for (i = 0; MaxCDT2[i].h_disp != 0; i++) {
	fprintf(stdout, "%d %d %d %d %d\n", MaxCDT2[i].h_disp,
		MaxCDT2[i].dacmask, MaxCDT2[i].ram_req,
		MaxCDT2[i].max_dot_clk, MaxCDT2[i].color_depth);
    }
#endif
#ifdef REPORT
    fprintf(stdout, "Valid Color Depth Table 1\n");
    for (i = 0; FreqCDT1[i].h_disp != 0; i++) {
#ifdef REPORT_CLKTAB
	fprintf(stdout, "%d %d %d %d %d\n", FreqCDT1[i].h_disp,
		FreqCDT1[i].dacmask, FreqCDT1[i].ram_req,
		FreqCDT1[i].max_dot_clk, FreqCDT1[i].color_depth);
#endif
	fprintf(stdout, "Width: %d, MaxDotClk: %dMHz, Depth: %d bpp\n",
		FreqCDT1[i].h_disp * 8, FreqCDT1[i].max_dot_clk,
		M64_BPP[FreqCDT1[i].color_depth]);
    }
    fprintf(stdout, "Valid Color Depth Table 2 - if present\n");
    for (i = 0; FreqCDT2[i].h_disp != 0; i++) {
#ifdef REPORT_CLKTAB
	fprintf(stdout, "%d %d %d %d %d\n", FreqCDT2[i].h_disp,
		FreqCDT2[i].dacmask, FreqCDT2[i].ram_req,
		FreqCDT2[i].max_dot_clk, FreqCDT2[i].color_depth);
#endif
	fprintf(stdout, "Width: %d, MaxDotClk: %dMHz, Depth: %d bpp\n",
		FreqCDT2[i].h_disp * 8, FreqCDT2[i].max_dot_clk,
		M64_BPP[FreqCDT2[i].color_depth]);
    }
#endif
}

/*************************************************************************/

/* generates a quick register dump */
static void mach64_report(void)
{
    UL reg;

    reg = inl(ioBUS_CNTL);
    printf("BUS_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCONFIG_CHIP_ID);
    printf("CONFIG_CHIP_ID: %lx ", reg);
    DUMP(reg);


    reg = inl(ioCONFIG_CNTL);
    printf("CONFIG_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCONFIG_STAT0);
    printf("CONFIG_STAT0: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCONFIG_STAT1);
    printf("CONFIG_STAT1: %lx ", reg);
    DUMP(reg);

    reg = inl(ioGEN_TEST_CNTL);
    printf("GEN_TEST_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioMEM_CNTL);
    printf("MEM_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioMEM_VGA_RP_SEL);
    printf("MEM_VGA_RP_SEL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioMEM_VGA_WP_SEL);
    printf("MEM_VGA_WP_SEL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioDAC_CNTL);
    printf("DAC_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCLOCK_SEL_CNTL);
    printf("CLOCK_CNTL: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCRTC_GEN_CNTL);
    printf("CRTC_GEN: %lx ", reg);
    DUMP(reg);

    reg = inl(ioSCRATCH_REG0);
    printf("SCRATCH 0: %lx ", reg);
    DUMP(reg);

    reg = inl(ioSCRATCH_REG1);
    printf("SCRATCH 1: %lx ", reg);
    DUMP(reg);

    reg = inl(ioCRTC_H_SYNC_STRT_WID);
    printf("H_S_S_W: %lx ", reg);
    DUMP(reg);
}

/*************************************************************************/

/* program the ati68860 ramdac -- standard on old mach64's */
static int mach64_dac_ati68860(US c_depth, US accelmode)
{
    US gmode, dsetup, temp, mask;

    /* filter pixel depth */
    if (c_depth == BPP_8) {
	gmode = 0x83;
	dsetup = 0x61;
    } else {
	printf("unsupported bpp\n");
	exit(1);
    }

    /* if we are in vga mode */
    if (!accelmode) {
	gmode = 0x80;
	dsetup = 0x60;
    }
    temp = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (temp & ~DAC_EXT_SEL_RS2) | DAC_EXT_SEL_RS3);

    outb(ioDAC_REGS + 2, 0x1d);
    outb(ioDAC_REGS + 3, gmode);
    outb(ioDAC_REGS, 0x02);

    temp = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, temp | DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3);

    if (M64_Mem_Size[StartupInfo.Mem_Size] < 1024)
	mask = 0x04;
    else if (M64_Mem_Size[StartupInfo.Mem_Size] == 1024)
	mask = 0x08;
    else
	mask = 0x0c;

    temp = inb(ioDAC_REGS);
    outb(ioDAC_REGS, (dsetup | mask) | (temp & 0x80));
    temp = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (temp & ~(DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3)));

    /* pixel delay from ati */
    temp = inb(ioCRTC_H_SYNC_STRT_WID + 1);
    temp = 0x1c;		/*was temp+= */
    outb(ioCRTC_H_SYNC_STRT_WID + 1, temp & 0x07);
    temp >>= 3;
    temp = temp + inb(ioCRTC_H_SYNC_STRT_WID);
    outb(ioCRTC_H_SYNC_STRT_WID, temp);

    return 0;
}

/* general ramdac program */
static void mach64_dac_program(US c_depth, US accelmode, US dotclock)
{
    US temp, mux;

    /* check to see if we are using an accelerator mode and turn on
       the extended mode display and crtc */
    if (accelmode) {
	temp = inb(CRTC_GEN_CNTL) & ~CRTC_PIX_BY_2_EN;
	outb(ioCRTC_GEN_CNTL, temp);
	outb(ioCRTC_GEN_CNTL + 3, CRTC_EXT_DISP_EN | CRTC_EXT_EN);
    } else {
	outb(ioCRTC_GEN_CNTL + 3, 0);
    }

    temp = inb(ioCRTC_GEN_CNTL + 3);
    outb(ioCRTC_GEN_CNTL + 3, temp | CRTC_EXT_DISP_EN);

    /* switch on the dac type */
    mux = mach64_dac_ati68860(c_depth, accelmode);

    inb(ioDAC_REGS);
    outb(ioCRTC_GEN_CNTL + 3, temp);

    temp = inb(ioCRTC_GEN_CNTL) & ~CRTC_PIX_BY_2_EN;
    if (mux)
	temp = temp | CRTC_PIX_BY_2_EN;
    outb(ioCRTC_GEN_CNTL, temp);
}

/* setup the crtc registers */
static void mach64_crtc_programming(US mode, US c_depth, US refrate)
{
    CRTC_Table *pcrtc;
    UB temp3, temp0;

    /* fix to mode 640x480 */
    pcrtc = &CRTC_tinfo[0];

    /* determine the fifo value and dot clock */
    pcrtc->fifo_vl = 0x02;
    pcrtc->pdot_clock = pcrtc->dot_clock;

    temp3 = inb(ioCRTC_GEN_CNTL + 3);
    temp0 = inb(ioCRTC_GEN_CNTL + 0);
    outb(ioCRTC_GEN_CNTL + 3, temp3 & ~CRTC_EXT_EN);

    /* here would would program the clock... but we won't */

    /* horizontal */
    outb(CRTC_H_TOTAL, pcrtc->h_total);
    outb(CRTC_H_DISP, pcrtc->h_disp);
    outb(CRTC_H_SYNC_STRT, pcrtc->h_sync_strt);
    outb(CRTC_H_SYNC_WID, pcrtc->h_sync_wid);

    printf("CRTC_H_TD: %x\n", inl(ioCRTC_H_TOTAL_DISP));
    printf("CRTC_H_SN: %x\n", inl(ioCRTC_H_SYNC_STRT_WID));

    /* vertical */
    outw(CRTC_V_TOTAL, pcrtc->v_total);
    outw(CRTC_V_DISP, pcrtc->v_disp);
    outw(CRTC_V_SYNC_STRT, pcrtc->v_sync_strt);
    outb(CRTC_V_SYNC_WID, pcrtc->v_sync_wid);

    printf("CRTC_V_TD: %lx\n", (UL) inl(ioCRTC_V_TOTAL_DISP));
    printf("CRTC_V_SN: %lx\n", (UL) inl(ioCRTC_V_SYNC_STRT_WID));

    /* clock stuff */
    /* 50/2 */
    pcrtc->clock_cntl = 0x00 | 0x10;
    /* CX clock */
    pcrtc->clock_cntl = 0x08;
    /*outb(CLOCK_CNTL,pcrtc->clock_cntl|CLOCK_STROBE); */
    outb(CLOCK_CNTL, pcrtc->clock_cntl);
    printf("CLK: %lx\n", (UL) inl(ioCLOCK_SEL_CNTL));

    /* overscan */
    outb(OVR_WID_LEFT, 0);
    outb(OVR_WID_RIGHT, 0);
    outb(OVR_WID_BOTTOM, 0);
    outb(OVR_WID_TOP, 0);
    outb(OVR_CLR_8, 0);
    outb(OVR_CLR_B, 0);
    outb(OVR_CLR_G, 0);
    outb(OVR_CLR_R, 0);

    /* pitch */
    outl(ioCRTC_OFF_PITCH, (UL) 80 << 22);
    mreg[mSRC_OFF_PITCH / 4 + 0xC00 / 4] = (UL) 80 << 22;
    mreg[mDST_OFF_PITCH / 4 + 0xC00 / 4] = (UL) 80 << 22;

    /* turn on display */
    outb(ioCRTC_GEN_CNTL + 0, temp0 &
	 ~(CRTC_PIX_BY_2_EN | CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN));
    outb(ioCRTC_GEN_CNTL + 1, c_depth);
    outb(ioCRTC_GEN_CNTL + 2, pcrtc->fifo_vl);
    outb(ioCRTC_GEN_CNTL + 3, temp3 | CRTC_EXT_DISP_EN | CRTC_EXT_EN);

    mreg[mDP_PIX_WIDTH / 4 + 0xC00 / 4] = (UL) 0x01020202;
    mreg[mDP_CHAIN_MASK / 4 + 0xC00 / 4] = (UL) 0x8080;
    mreg[mCONTEXT_MASK / 4 + 0xC00 / 4] = (UL) 0xffffffff;
    /* woudn't a macro be nice */
    mreg[mDST_CNTL / 4 + 0xC00 / 4] = (UL) 0x00;

    printf("CRTC_GEN_IN_CRTC: %x\n", inl(ioCRTC_GEN_CNTL));
    /* configure ramdac */
    mach64_dac_program(c_depth, 1, pcrtc->pdot_clock);

    mach64_report();
}

/*************************************************************************/
static int mach64_saveregs(UB * regs)
{
    int i, retval;
    UL temp;
    UB tb;

    /* store all the crtc, clock, memory registers */
    retval = EXT;
    for (i = 0; i < 13; i++) {
	temp = inl(sr_accel[i]);
	printf("Saved: %d %lx\n", i, temp);
	regs[retval++] = (UB) temp & 0x000000ff;
	temp = temp >> 8;
	regs[retval++] = (UB) temp & 0x000000ff;
	temp = temp >> 8;
	regs[retval++] = (UB) temp & 0x000000ff;
	temp = temp >> 8;
	regs[retval++] = (UB) temp & 0x000000ff;

    }

    /* store the dac --68860 */
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (tb & ~DAC_EXT_SEL_RS2) | DAC_EXT_SEL_RS3);
    regs[retval++] = (UB) inb(ioDAC_REGS + 2);
    regs[retval++] = (UB) inb(ioDAC_REGS + 3);
    regs[retval++] = (UB) inb(ioDAC_REGS);
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, tb | DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3);
    regs[retval++] = (UB) inb(ioDAC_REGS);
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (tb & ~(DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3)));

    return retval - EXT + 1;
}

static void mach64_setregs(const UB * regs, int mode)
{
    int i, retval;
    UL temp;
    UB tb;

    outb(0x3ce, 6);
    outb(0x3cf, 0);
    outb(ioCONFIG_CNTL, (inb(ioCONFIG_CNTL) & 0xfb));
    outb(ioCRTC_GEN_CNTL + 3, 0);

    return;
    /* restore all the crtc, clock, memory registers */
    retval = EXT;
    for (i = 0; i < 13; i++) {
	temp = (UL) 0;
	temp = temp | (regs[retval++] << 24);
	temp = temp | (regs[retval++] << 16);
	temp = temp | (regs[retval++] << 8);
	temp = temp | regs[retval++];
	outl(sr_accel[i], temp);

    }

    /* store the dac --68860 */
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (tb & ~DAC_EXT_SEL_RS2) | DAC_EXT_SEL_RS3);
    outb(ioDAC_REGS + 2, regs[retval++]);
    outb(ioDAC_REGS + 3, regs[retval++]);
    outb(ioDAC_REGS, regs[retval++]);
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, tb | DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3);
    outb(ioDAC_REGS, regs[retval++]);
    tb = inb(ioDAC_CNTL);
    outb(ioDAC_CNTL, (tb & ~(DAC_EXT_SEL_RS2 | DAC_EXT_SEL_RS3)));

}

static void mach64_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    /*__svgalib_vga_driverspecs.getmodeinfo(mode, modeinfo); */
    modeinfo->startaddressrange = 0xfffff;
/*      modeinfo->startaddressrange=0xfffffffc&(2048*1024-1);
 */

    modeinfo->maxpixels = 640 * 480;
/*      modeinfo->bytesperpixel=infotable[mode].bytesperpixel;
   modeinfo->linewidth_unit=8;
   modeinfo->linewidth=infotable[mode].xbytes;
   modeinfo->maxlogicalwidth=0xff*8*modeinfo->bytesperpixel;
   modeinfo->startaddressrange=0xffc00000;
 */
}

static int mach64_modeavailable(int mode)
{
/*
   if (mode>__svgalib_max_modes)
 */
    if (mode > __GLASTMODE)
	return 0;
    if (mode < G640x480x256 || mode == G720x348x2)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    else if (mode != G640x480x256)
	return 0;

    return SVGADRV;
}

static int mach64_setmode(int mode, int prv_mode)
{
    const UB *regs = NULL;
    UB di;

    mach64_setregs(regs, mode);

    if (mode < G640x480x256 || mode == G720x348x2)
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);

    if (mode == G640x480x256) {

	/* I don't think this is necessary since I dont even
	   use the vga */
	/* set packed pixel register */
	outb(ATIPORT, ATISEL(0x30));
	di = inb(ATIPORT);
	printf("pre ext: %d\n", di);
	outb(ATIPORT, ATISEL(0x30));
	outb(ATIPORT, di | 0x20);

	/* all maps */
	outb(0x3c4, 2);
	outb(0x3c5, 0x0f);

	/* linear */
	outb(ATIPORT, ATISEL(0x36));
	di = inb(ATIPORT);
	outb(ATIPORT, ATISEL(0x36));
	outb(ATIPORT, di | 0x04);

	/* turn off memory boundary */
	outl(ioMEM_CNTL, inl(ioMEM_CNTL) & 0xfff8ffff);

	/* disable interrups */
	outl(ioCRTC_INT_CNTL, 0);

	/* 8 bit dac */
	outb(ioDAC_CNTL + 1, inb(ioDAC_CNTL + 1) | 0x01);
	outb(ioCRTC_GEN_CNTL + 3, 3);
	/* setup small aperature */
	mach64_small_aperature();


	mach64_setpage(0);
#ifdef DEBUG
	printf("About to call crtc programming...\n");
#endif
	mach64_crtc_programming(0, BPP_8, 60);
	di = 0;
/*              for (di=0;di<32;di++){ 
   mach64_setpage(di);usleep(10000);
   for (i=0;i<64*1024;i++){
   graph_mem[i]=i%16;}
   }
	 *//*            for (j=0;j<1024*2048/8;j+=640){
	   usleep(1);
	   outw(ioCRTC_OFF_PITCH,j/8);
	   }  
	 *//*            outw(ioCRTC_OFF_PITCH,4000);
	   outb(ioCRTC_OFF_PITCH+2,inb(ioCRTC_OFF_PITCH+2)|0x10);
	   outw(ioCRTC_OFF_PITCH,80*480);
	 */ outw(ioCRTC_OFF_PITCH, 0);
	mach64_setpage(0);
/*              for(j=0;j<64000;j++){
   graph_mem[j]=0x0c;
   }
   sleep(2);
 */ mach64_setpage(0);
	mach64_report();
	return 0;
    }
    return 1;


}

static void mach64_setdisplaystart(int address)
{
    /*__svgalib_vga_driverspecs.setdisplaystart(address); */
}

static void mach64_setlogicalwidth(int width)
{
    /*__svgalib_vga_driverspecs.setlogicalwidth(width); */
}

static void mach64_setpage(int page)
{
    printf("PAGE: %d\n", page);
    outb(ioMEM_VGA_WP_SEL, 255 - (page * 2));
    outb(ioMEM_VGA_WP_SEL + 2, 255 - ((page * 2) + 1));
    outb(ioMEM_VGA_RP_SEL, 255 - (page * 2));
    outb(ioMEM_VGA_RP_SEL + 2, 255 - ((page * 2) + 1));
}

static void mach64_small_aperature(void)
{
    int i;

    /* 256 color mode */
    outb(0x3ce, 5);
    printf("pre 5:%d\n", inb(0x3cf));
    outb(0x3ce, 5);
    outb(0x3cf, 0x60);
    outb(0x3ce, 5);
    printf("afe 5:%d\n", inb(0x3cf));
    /* APA mode with 128K at A0000 */
    outb(0x3ce, 6);
    printf("pre 6:%d\n", inb(0x3cf));
    outb(0x3ce, 6);
    outb(0x3cf, 1);
    outb(0x3ce, 6);
    printf("afe 6:%d\n", inb(0x3cf));

    /* setup small aperature */
    outb(ioCONFIG_CNTL, (inb(ioCONFIG_CNTL) | 0x04) & 0xff);
    /* map ram */
#ifdef OSKIT
    { 
	void *tmp;
	osenv_mem_map_phys(0xbf000, 4 * 1024, &tmp, 0);
    }
#else
    mreg = (UL *) mmap((caddr_t) 0,
		       4 * 1024,
		       PROT_READ | PROT_WRITE,
		       MAP_SHARED | MAP_FIXED,
		       __svgalib_mem_fd,
		       (off_t) ((UB *) 0xbf000));
#endif
    if (mreg < 0) {
	printf("Mapping failed\n");
	exit(1);
    }
    printf("IOPORT: %x\n", inl(ioCONFIG_STAT0));
    for (i = 0; i <= 0xce; i++) {
	printf("MEMA: %x %lx\n", i, mreg[i + 0xc00 / 4]);
    }
}

