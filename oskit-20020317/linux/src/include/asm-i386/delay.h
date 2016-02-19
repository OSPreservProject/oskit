#ifndef _I386_DELAY_H
#define _I386_DELAY_H

#ifdef OSKIT
/* A few drivers bogusly include <asm/delay.h> instead of <linux/delay.h>. */
#include <linux/delay.h>
#else

/*
 * Copyright (C) 1993 Linus Torvalds
 *
 * Delay routines calling functions in arch/i386/lib/delay.c
 */

extern void __udelay(unsigned long usecs);
extern void __const_udelay(unsigned long usecs);
extern void __delay(unsigned long loops);

#define udelay(n) (__builtin_constant_p(n) ? \
	__const_udelay((n) * 0x10c6ul) : \
	__udelay(n))

#ifdef OSKIT
/*
 * Not used as a delay anymore.
 */
extern __inline__ unsigned long
muldiv(unsigned long a, unsigned long b, unsigned long c)
{
	return a * b / c;
}
#endif

#endif

#endif /* defined(_I386_DELAY_H) */
