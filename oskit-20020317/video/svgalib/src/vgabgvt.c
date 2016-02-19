/*   */

#ifdef BACKGROUND

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "mouse/vgamouse.h"
#include "keyboard/vgakeyboard.h"
#include "vgabg.h"

#define RELEASE 1
#define ACQUIRE 2
/* #define QUEUE_SIZE 200*/

#if 0
#define SETSIG(sa, sig, fun) {\
	sa.sa_handler = fun; \
	sa.sa_flags = 0; \
	sa.sa_mask = vt_block_mask; \
	sigaction(sig, &sa, NULL); \
}
#endif

#define SETSIG_RE(sa, sig, fun) {\
	sa.sa_handler = fun; \
	sa.sa_flags = 0; \
	sa.sa_mask = __vt_block_mask_release; \
	sigaction(sig, &sa, NULL); \
}

#define SETSIG_AQ(sa, sig, fun) {\
	sa.sa_handler = fun; \
	sa.sa_flags = 0; \
	sa.sa_mask = __vt_block_mask_aquire; \
	sigaction(sig, &sa, NULL); \
}


/*#define DEBUG  */

/* Virtual console switching */

static int forbidvtrelease = 0;
static int forbidvtacquire = 0;
static int lock_count = 0;
static int release_flag = 0;

static int __vt_switching_not_ok = 0;
static int __vt_switching_asked = 0;
static int __vt_check_last_signal = 0;	/* Never the same signal twice */
static sigset_t __vt_block_mask_aquire;
static sigset_t __vt_block_mask_release;
static sigset_t __vt_block_mask_switching;

#if 0
static sigset_t vt_block_mask;

unsigned char *__svga_graph_mem_orginal;
unsigned char *__svga_graph_mem_check;

static int __svgalib_oktowrite = 1;
/* __svgalib_oktowrite tells if it is safe to write registers. */

static int __svgalib_virtual_mem_fd = -1;
static int __svga_processnumber = -1;
#endif

static int __vt_switching_state=RELEASE;
static int __vt_switching_signal_no;
/*
static int __vt_switching_to_queue[QUEUE_SIZE] = {0};
static int __vt_switching_signal_no_queue[QUEUE_SIZE] = {0};
static int __vt_signalqueuepointer = 0;
static int __vt_signalqueuenewpointer = 0;
*/

/* User defined functions for vc switching. */
void (*__svgalib_go_to_background) (void) = 0;
void (*__svgalib_come_from_background) (void) = 0;


#endif

#ifdef BACKGROUND

static void releasevt_signal(int n);
static void acquirevt_signal(int n);

static void __releasevt_signal(int n)
{
/*    struct sigaction siga; */


    if (lock_count) {
	release_flag = 1;
	return;
    }
#ifdef DEBUG
    printf("Release request.\n");
#endif
    forbidvtacquire = 1;
    /*SETSIG(siga, SIGUSR1, releasevt_signal); */

    if (forbidvtrelease) {
	forbidvtacquire = 0;
	ioctl(__svgalib_tty_fd, VT_RELDISP, 0);
	return;
    }
    if (__svgalib_go_to_background)
	(__svgalib_go_to_background) ();

    __vt_switching_state=RELEASE;
    __svgalib_flipaway();
    ioctl(__svgalib_tty_fd, VT_RELDISP, 1);
#ifdef DEBUG
    printf("Finished release.\n");
#endif
    forbidvtacquire = 0;

    /* Suspend program until switched to again. */
#ifdef DEBUG
    printf("Suspended.\n");
#endif
    __svgalib_oktowrite = 0;
    if (!__svgalib_runinbackground)
	__svgalib_waitvtactive();
#ifdef DEBUG
    printf("Waked.\n");
#endif
}

static void releasevt_signal(int n)
{
    struct sigaction siga;

#ifdef DEBUG
    printf("vt: rel ");
#endif
    if (__vt_switching_not_ok || __vt_switching_asked) {
	if (__vt_check_last_signal != RELEASE)
	    {
	     __vt_switching_asked=1;
	     __vt_switching_signal_no=n;
	     __vt_switching_state=RELEASE;
	    }
    } else {
	/* __svgalib_dont_switch_vt_yet(); */
	if (__vt_check_last_signal != RELEASE)
	    __releasevt_signal(n);
	/* __svgalib_is_vt_switching_needed(); */
    }
    __vt_check_last_signal = RELEASE;
    SETSIG_AQ(siga, SIGUSR2, acquirevt_signal);
    SETSIG_RE(siga, SIGUSR1, releasevt_signal);
#ifdef DEBUG
    printf("ok\n");
#endif
    return;
}

static void __acquirevt_signal(int n)
{
/*    struct sigaction siga; */
#ifdef DEBUG
    printf("Acquisition request.\n");
#endif
    /* vt_check_last_signal=ACQUIRE; */
    forbidvtrelease = 1;
    /*SETSIG(siga, SIGUSR2, acquirevt_signal); */
    if (forbidvtacquire) {
	forbidvtrelease = 0;
	return;
    }
    __vt_switching_state=ACQUIRE;
    __svgalib_flipback();
    ioctl(__svgalib_tty_fd, VT_RELDISP, VT_ACKACQ);
#ifdef DEBUG
    printf("Finished acquisition.\n");
#endif
    /*forbidvtrelease = 0; */
    __svgalib_oktowrite = 1;
    if (__svgalib_come_from_background)
	(__svgalib_come_from_background) ();
    forbidvtrelease = 0;
}

static void acquirevt_signal(int n)
{
    struct sigaction siga;

#ifdef DEBUG
    printf("vt: acq ");
#endif
    if (__vt_switching_not_ok || __vt_switching_asked) {
	if (__vt_check_last_signal != ACQUIRE)
	    {
	     __vt_switching_asked=1;
	     __vt_switching_signal_no=n;
             __vt_switching_state=ACQUIRE;
	    }
    } else {
	/* __svgalib_dont_switch_vt_yet(); */
	if (__vt_check_last_signal != ACQUIRE)
	    __acquirevt_signal(n);
	/* __svgalib_is_vt_switching_needed(); */
    }
    __vt_check_last_signal = ACQUIRE;
    SETSIG_RE(siga, SIGUSR1, releasevt_signal);
    SETSIG_AQ(siga, SIGUSR2, acquirevt_signal);
#ifdef DEBUG
    printf("ok\n");
#endif
    return;
}

void __svgalib_dont_switch_vt_yet(void)
{
    __vt_switching_not_ok++;
    return;
}

void __svgalib_is_vt_switching_needed(void)
{
     
    __vt_switching_not_ok--;
    if (__vt_switching_not_ok < 0) {
	__vt_switching_not_ok = 0;
	printf("svgalib: lock warning, over done.\n");
    }
      
    if (__vt_switching_not_ok) return;

    if (__vt_switching_asked)
        {
         /* Block signals... hmm ignore them */
         sigprocmask(SIG_BLOCK,&__vt_block_mask_switching,NULL);
	 if (__vt_switching_state==RELEASE &&
	     __vt_check_last_signal!=RELEASE) {
#ifdef DEBUG
	     printf("RELEASE ");
#endif
	     __releasevt_signal(__vt_switching_signal_no);
	     __vt_switching_signal_no=0;
             __vt_switching_state=ACQUIRE;
#ifdef DEBUG
	     printf("OK.\n");
#endif
	 }
	  else
	  {
	   if (__vt_switching_state==ACQUIRE &&
	       __vt_check_last_signal!=ACQUIRE) {
#ifdef DEBUG
	       printf("ACQUIRE ");
#endif
	       __acquirevt_signal(__vt_switching_signal_no);
	       __vt_switching_signal_no=0;
               __vt_switching_state=RELEASE;
#ifdef DEBUG
	       printf("OK.\n");
#endif
	    }
	   }
              
	 __vt_switching_asked=0;
         sigprocmask(SIG_UNBLOCK,&__vt_block_mask_switching,NULL);
        }
    return;
}

/* These both variables should be static. However, oldvtmode is
   restored in vga.c.. so.. */

struct vt_mode __svgalib_oldvtmode;
static struct vt_mode newvtmode;

void __svgalib_takevtcontrol(void)
{
    struct sigaction siga;

    ioctl(__svgalib_tty_fd, VT_GETMODE, &__svgalib_oldvtmode);
    newvtmode = __svgalib_oldvtmode;
    newvtmode.mode = VT_PROCESS;	/* handle VT changes */
    newvtmode.relsig = SIGUSR1;	/* I didn't find SIGUSR1/2 anywhere */
    newvtmode.acqsig = SIGUSR2;	/* in the kernel sources, so I guess */
                                        /* they are free */


    sigemptyset(&__vt_block_mask_switching);
    sigaddset(&__vt_block_mask_switching,SIGUSR1|SIGUSR2);

    sigemptyset(&__vt_block_mask_aquire);
    sigaddset(&__vt_block_mask_aquire, SIGUSR1);
    sigemptyset(&__vt_block_mask_release);
    sigaddset(&__vt_block_mask_release, SIGUSR2);
    SETSIG_RE(siga, SIGUSR1, releasevt_signal);
    SETSIG_AQ(siga, SIGUSR2, acquirevt_signal);
    ioctl(__svgalib_tty_fd, VT_SETMODE, &newvtmode);
    return;
}

void vga_lockvc(void)
{
    lock_count++;
    if (__svgalib_flip_status())
	__svgalib_waitvtactive();
    return;
}

void vga_unlockvc(void)
{
    if (--lock_count <= 0) {
	lock_count = 0;
	if (release_flag) {
	    release_flag = 0;
	    releasevt_signal(SIGUSR1);
	}
    }
    return;
}

void vga_runinbackground(int stat,...)
{
    va_list params;

    va_start(params, stat);
    switch (stat) {
    case VGA_GOTOBACK:
	__svgalib_go_to_background = va_arg(params, void *);
	break;
    case VGA_COMEFROMBACK:
	__svgalib_come_from_background = va_arg(params, void *);
	break;
    default:
	__svgalib_runinbackground = stat;
	break;
    }
    va_end(params);
}

#endif
