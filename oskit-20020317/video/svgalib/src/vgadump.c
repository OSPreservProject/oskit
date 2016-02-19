/*
 * vgadump.c:
 * 
 * Dump vga registers.
 * 
 * Rewritten Feb 1996 by Stephen Lee.  Copyright 1996 Stephen Lee.
 * This file is part of SVGAlib.  Original copyrights:
 * 
 * VGAlib version 1.2 - (c) 1993 Tommy Frandsen
 *
 * This library is free software; you can redistribute it and/or
 * modify it without any restrictions. This library is distributed
 * in the hope that it will be useful, but without any warranty.
 *
 * Multi-chipset support Copyright 1993 Harm Hanemaayer
 */

#include <stdio.h>
#include <stdarg.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

static void dumpregs(const unsigned char regs[], int n, const char *fmt, ...)
{
    int i;
    va_list ap;
    
    if (!n)
	return;
    i = 0;
    printf("  ");
    while (i < n) {
	printf("0x%02X,", regs[i]);
	i++;
	if (i % 8 == 0 || i == n) {
	    if (i <= 8) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	    }
	    printf("\n");
	    if (i != n)
		printf("  ");
	}
    }    
}

/* 
 * dump VGA registers.  Note the output has a comma at the end
 * (it's simpler to code and the standard allows it)
 */
void __svgalib_dumpregs(const unsigned char regs[], int n)
{
    printf("static unsigned char regs[%d] = {\n", n);

    dumpregs(regs + CRT, CRT_C, "\t/* CR00-CR%02x */", CRT_C);
    dumpregs(regs + ATT, ATT_C, "\t/* AR00-AR%02x */", ATT_C);
    dumpregs(regs + GRA, GRA_C, "\t/* GR00-GR%02x */", SEQ_C);
    dumpregs(regs + SEQ, SEQ_C, "\t\t\t/* SR00-SR%02x */", SEQ_C);
    dumpregs(regs + MIS, MIS_C, "\t\t\t\t\t\t/* MISC_OUT  */");
    n -= EXT;
    if (n) {
	printf("  /* Extended (count = 0x%02x) */\n", n);
	dumpregs(regs + EXT, n, "");
    }
    printf("};\n");
}

int vga_dumpregs(void)
{
    unsigned char regs[MAX_REGS];
    int n;

    __svgalib_getchipset();

    n = __svgalib_saveregs(regs);
    __svgalib_dumpregs(regs, n);

    return 0;
}
