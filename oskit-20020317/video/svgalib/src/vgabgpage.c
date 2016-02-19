/* */

#ifdef BACKGROUND

#include <stdio.h>
#include "vga.h"
#include "vgabg.h"

static int lastrpage = -1, lastwpage = -1;

inline void vga_setpage(int p)
{
    if (p == __svgalib_currentpage)
	return;
    __svgalib_dont_switch_vt_yet();
    if (!__svgalib_oktowrite) {
#if BACKGROUND == 1
	__svgalib_map_virtual_screen(p);
#endif
#if BACKGROUND == 2
	__svgalib_graph_mem = graph_buf + (GRPAH_SIZE * p);
#endif
    } else {
	__svgalib_setpage(p);
    }
    __svgalib_currentpage = lastrpage = lastwpage = p;
    __svgalib_is_vt_switching_needed();
    return;
}

inline void __svgalib_fast_setpage(int p)
/* This does not check vt switching. */
{
    if (p == __svgalib_currentpage)
	return;
    if (!__svgalib_oktowrite) {
#if BACKGROUND == 1
	__svgalib_map_virtual_screen(p);
#endif
#if BACKGROUND == 2
	__svgalib_graph_mem = graph_buf + (GRPAH_SIZE * p);
#endif
    } else {
	__svgalib_setpage(p);
    }
    __svgalib_currentpage = lastrpage = lastwpage = p;
}

inline void __svgalib_setpage_fg(int p)
{
    if (p == __svgalib_currentpage)
	return;
    __svgalib_dont_switch_vt_yet();
    __svgalib_setpage(p);
    __svgalib_currentpage = lastrpage = lastwpage = p;
    __svgalib_is_vt_switching_needed();
}

inline void __svgalib_setpage_bg(int p)
{
    if (p == __svgalib_currentpage)
	return;
    __svgalib_dont_switch_vt_yet();
#if BACKGROUND == 1
    __svgalib_map_virtual_screen(p);
#endif
#if BACKGROUND == 2
    __svgalib_graph_mem = graph_buf + (GRPAH_SIZE * p);
#endif
    __svgalib_currentpage = lastrpage = lastwpage = p;
    __svgalib_is_vt_switching_needed();
    return;
}

void vga_setreadpage(int p)
{
    puts("svgalib: vga_setreadpage() call impossible in background mode.");
    exit(2); 
}


void vga_setwritepage(int p)
{
    puts("svgalib: vga_setwritepage() call impossible in background mode.");
    exit(2); 
}

#endif				/* BACKGROUND */
