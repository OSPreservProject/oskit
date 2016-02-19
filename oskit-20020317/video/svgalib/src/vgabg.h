/* These things here are only for BG-runing module.
   These things are NOT intend for USER programs.
   If you use these things in user program you may
   result version depending code i.e. YOUR code may NOT 
   work in future.
   
   Something like that and so on.
   
*/

/* For vgabgvt.c */

#ifndef _VGABG_H_
#define _VGABG_H_

/* Just big enough. */

#define VIDEOSCREEN 1000000

struct vga_bg_info

{
 int videomode;
 int draw_color;
 int bg_color;
};

/* Linear video memory things */

extern void *__svgalib_linearframebuffer;
extern unsigned char *__svgalib_graph_mem_linear_orginal;
extern unsigned char *__svgalib_graph_mem_linear_check;
extern int __svgalib_linear_memory_size;

extern int __svgalib_oktowrite; 
extern int __svgalib_currentpage;
/* extern int __svgalib_ignore_vt_switching; */

/* gives little faster vga_setpage() */
extern void (*__svgalib_setpage) (int);
extern void (*__svgalib_setrdpage) (int);
extern void (*__svgalib_setwrpage) (int);

/* */
extern void __svgalib_takevtcontrol(void);
extern int __svgalib_flip_status(void);
extern void __svgalib_flipaway(void);
extern void __svgalib_flipback(void);

extern void __svgalib_map_virtual_screen(int page);
extern void __svgalib_fast_setpage(int p);

extern int __svgalib_fast_drawpixel(int x, int y);
extern int __svgalib_fast_getpixel(int x, int y);

extern void __svgalib_dont_switch_vt_yet(void);
extern void __svgalib_is_vt_switching_needed(void);

extern struct vt_mode oldvtmode; /* This should be static. */

#endif

