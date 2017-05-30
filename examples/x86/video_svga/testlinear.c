
/* 
   Simple test program for Cirrus linear addressing/color expansion.
   vgagl can take advantage of it (linear addressing).
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <oskit/svgalib/vga.h>
#include <time.h>
#if 0
#include "../src/libvga.h"
#else
#include <oskit/x86/pio.h>
#endif
#include <oskit/clientos.h>

#define USE_LINEAR_ADDRESSING
/* #define USE_BY16_ADDRESSING */


unsigned char *vbuf;

#if 0
/* This function is Cirrus specific and has nothing to do with linear
 * addressing. */
void by8test(void)
{
    int i;
    int startclock, diffclock;

    /* Enable extended write modes and BY8/16 addressing. */
    outb(0x3ce, 0x0b);

#ifdef USE_BY16_ADDRESSING
    outb(0x3cf, inb(0x3cf) | 0x16);
#else
    outb(0x3cf, inb(0x3cf) | 0x06);
#endif
    /* Set extended write mode 4. */
    outb(0x3ce, 0x05);
    outb(0x3cf, (inb(0x3cf) & 0xf8) | 4);

    /* Set pixel mask register (coincides with VGA plane mask register). */
    outw(0x3c4, 0xff02);

    startclock = clock();
    for (i = 0; i < 248; i++) {
	outw(0x3ce, 0x01 + (i << 8));	/* Set foreground color. */
#ifdef USE_BY16_ADDRESSING
	outw(0x3ce, 0x11 + (i << 8));	/* Set high byte. */
	memset(vbuf, 0xff, 640 * 480 / 16);
#else
	memset(vbuf, 0xff, 640 * 480 / 8);
#endif
    }
    diffclock = clock() - startclock;
    printf("Color expansion framebuffer fill speed: %dK/s\n",
	   640 * 480 * 248 / diffclock / 10);
}
#endif

void main(int argc, char *argv[])
{
    int i,j;

    oskit_clientos_init();

    if (!(argc == 2 && strcmp(argv[1], "--force") == 0))
	if (!(vga_getmodeinfo(G640x480x256)->flags & CAPABLE_LINEAR)) {
	    printf("Linear addressing not supported for this chipset.\n");
	    exit(1);
	}
    vga_init();
    vga_setmode(G640x480x256);
    vga_setpage(0);
#ifdef USE_LINEAR_ADDRESSING
    if (vga_setlinearaddressing() == -1) {
	vga_setmode(TEXT);
	printf("Could not set linear addressing.\n");
	exit(-1);
    }
#endif

    /* Should not mess with bank register after this. */

    vbuf = vga_getgraphmem();
    printf("vbuf mapped at %08lx.\n", (unsigned long) vbuf);

    getchar();

#ifdef USE_LINEAR_ADDRESSING
    memset(vbuf, 0x88, 640 * 480);
    sleep(1);

    memset(vbuf, 0, 640 * 480);
    for (i = 0; i < 100000; i++)
	*(vbuf + (rand() & 0xfffff)) = rand();
#endif

#if 0
    if (vga_getcurrentchipset() == CIRRUS)
	/* Show the bandwidth of the extended write modes of the */
	/* Cirrus chip. */
	by8test();
#endif

    getchar();
    for(i = 0;i < 44;i++){
      *(vbuf + i) = 0x1c;
      *(vbuf + 640*17 + i) = 0x1c;
      *(vbuf + 640*480 - i) = 0x1c;
      *(vbuf + 640*480 - 1 - 640*17 - i) = 0x1c;
      for(j = 1;j < 17;j++){
	*(vbuf + 640*j + i) = (i == 0 || i == 43)? 0x1c:0;
	*(vbuf + 640*480 - 1 - 640*j - i) = (i == 0 || i == 43)? 0x1c:0;
      }
    }
    for(i = 3;i < 10;i++)
      for(j = 4;j < 10;j++){
	*(vbuf + i + 640*j) = 0x3f;
	*(vbuf + 640*480 -1 -640*j - i) = 0x3f;
      }
    getchar();
    vga_setmode(TEXT);
}
