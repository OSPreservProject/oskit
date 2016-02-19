
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vga.h"
#include "libvga.h"


#define MODENAME_LENGTH 20

/* This is set by vga.c if there's a 'default_mode' line in the config */
extern int __svgalib_default_mode;

/* This one won't type an error message ... */
int __svgalib_name2number(char *m)
{
    int i;

    for (i = G320x200x16; i <= GLASTMODE; i++) {
	if (strcasecmp(m, vga_getmodename(i)) == 0)	/* check name */
	    return i;
    }
    return -1;
}

int vga_getmodenumber(char *m)
{
    int i;
    char s[3];

    __svgalib_getchipset();		/* Do initialisation first */
    i = __svgalib_name2number(m);
    if (i > 0)
	return i;

    for (i = G320x200x16; i <= GLASTMODE; i++) {
	sprintf(s, "%d", i);
	if (strcasecmp(m, s) == 0)	/* check number */
	    return i;
    }
    if (strcasecmp(m, "PROMPT") == 0)
	return -1;

    fprintf(stderr, "Invalid graphics mode \'%s\'.\n", m);
    return -1;
}

char *
 vga_getmodename(int m)
{
    static char modename[MODENAME_LENGTH];
    int x, y, c;

    if (m <= TEXT || m > GLASTMODE)
	return "";
    x = __svgalib_infotable[m].xdim;
    y = __svgalib_infotable[m].ydim;
    switch (c = __svgalib_infotable[m].colors) {
    case 1 << 15:
	sprintf(modename, "G%dx%dx32K", x, y);
	break;
    case 1 << 16:
	sprintf(modename, "G%dx%dx64K", x, y);
	break;
    case 1 << 24:
	sprintf(modename, (__svgalib_infotable[m].bytesperpixel == 3) ?
		"G%dx%dx16M" : "G%dx%dx16M32", x, y);
	break;
    default:
	sprintf(modename, "G%dx%dx%d", x, y, c);
	break;
    }
    return modename;
}

int vga_getdefaultmode(void) {
    char *stmp = getenv("SVGALIB_DEFAULT_MODE");

/* Process env var first so mode might be overridden by it. */
    if (stmp != NULL && strcmp(stmp, "") != 0) {
      int mode;
      if ( (mode = vga_getmodenumber(stmp)) != -1)
	return mode;
    } else if (__svgalib_default_mode) {
      return __svgalib_default_mode;
    }
    return -1;	/* Not defined */
}
