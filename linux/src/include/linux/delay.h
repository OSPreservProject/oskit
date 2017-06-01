#ifndef _LINUX_DELAY_H
#define _LINUX_DELAY_H

#ifdef OSKIT

#include "osenv.h"

#define OSKIT_LINUX_loops_per_sec	1000000000 /* ns */
#define FDEV_LINUX___delay(nsecs)	osenv_timer_spin(nsecs)
#undef	udelay
#define udelay(usecs)			osenv_timer_spin((usecs) * 1000)
#define mdelay(msecs)			osenv_timer_spin((msecs) * 1000000)

#else

/*
 * Copyright (C) 1993 Linus Torvalds
 *
 * Delay routines, using a pre-computed "loops_per_second" value.
 */

extern unsigned long loops_per_sec;

#include <asm/delay.h>

/*
 * Using udelay() for intervals greater than a few milliseconds can
 * risk overflow for high loops_per_sec (high bogomips) machines. The
 * mdelay() provides a wrapper to prevent this.  For delays greater
 * than MAX_UDELAY_MS milliseconds, the wrapper is used.  Architecture
 * specific values can be defined in asm-???/delay.h as an override.
 * The 2nd mdelay() definition ensures GCC will optimize away the
 * while loop for the common cases where n <= MAX_UDELAY_MS  --  Paul G.
 */

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_MS	5
#endif

#ifdef notdef
#define mdelay(n) (\
	{unsigned long msec=(n); while (msec--) udelay(1000);})
#else
#define mdelay(n) (\
	(__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
	({unsigned long msec=(n); while (msec--) udelay(1000);}))
#endif

#endif /* OSKIT */

#endif /* defined(_LINUX_DELAY_H) */
