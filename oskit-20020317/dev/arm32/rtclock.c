/*
 * Copyright (c) 1997-1999, 2001 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)clock.c	7.2 (Berkeley) 5/12/91
 *	$\Id: clock.c,v 1.35.2.2 1996/04/22 19:48:26 nate Exp $
 */

/*
 * inittodr, settodr and support routines written
 * by Christoph Robitschko <chmr@edvz.tu-graz.ac.at>
 *
 * reintroduced and updated by Chris Stenton <chris@gnome.co.uk> 8/10/94
 */

#include <oskit/time.h>
#include <oskit/dev/dev.h>
#include <oskit/dev/clock.h>
#include <oskit/arm32/shark/rtc.h>
#include <oskit/c/stdio.h>
#include <oskit/c/sys/types.h>
#include <oskit/c/time.h>

static unsigned char const bcd2bin_data[] = {
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 0, 0, 0, 0, 0, 0,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 0, 0, 0, 0, 0,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 0, 0, 0, 0, 0, 0,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 0, 0, 0, 0, 0, 0,
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 0, 0, 0, 0, 0, 0,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 0, 0, 0, 0, 0, 0,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 0, 0, 0, 0, 0, 0,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 0, 0, 0, 0, 0, 0,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 0, 0, 0, 0, 0, 0,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99
};

static unsigned char const bin2bcd_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99
};

#define bcd2bin(bcd)    (bcd2bin_data[bcd])
#define bin2bcd(bin)    (bin2bcd_data[bin])

/*
 * 32-bit time_t's can't reach leap years before 1904 or after 2036, so we
 * can use a simple formula for leap years.
 */
#define	LEAPYEAR(y) ((oskit_u32_t)(y) % 4 == 0)
#define DAYSPERYEAR   (31+28+31+30+31+30+31+31+30+31+30+31)
static	const unsigned char daysinmonth[] = 
	{31,28,31,30,31,30,31,31,30,31,30,31};

#if 0
static	unsigned char	rtc_statusa = RTCSA_DIVIDER | RTCSA_NOPROF;
#endif
static	unsigned char	rtc_statusb = RTCSB_24HR | RTCSB_PINTR;

static __inline int
readrtc(int port)
{
	return(bcd2bin(rtcin(port)));
}

/*
 * Initialize the time of day register.
 */
oskit_error_t
oskit_rtc_get(struct oskit_timespec *time)
{
	unsigned long	sec, days;
	int		yd;
	int		year, month;
	int		y, m;

	/* Look	if we have a RTC present and the time is valid */
	if (!(rtcin(RTC_STATUSD) & RTCSD_PWR))
		goto wrong_time;

	/* wait	for time update	to complete */
	/* If RTCSA_TUP	is zero, we have at least 244us	before next update */
	while (rtcin(RTC_STATUSA) & RTCSA_TUP);

	days = 0;
#ifdef USE_RTC_CENTURY
	year = readrtc(RTC_YEAR) + readrtc(RTC_CENTURY)	* 100;
#else
	year = readrtc(RTC_YEAR) + 1900;
	if (year < 1970)
		year += 100;
#endif
	if (year < 1970)
		goto wrong_time;
	month =	readrtc(RTC_MONTH);
	for (m = 1; m <	month; m++)
		days +=	daysinmonth[m-1];
	if ((month > 2)	&& LEAPYEAR(year))
		days ++;
	days +=	readrtc(RTC_DAY) - 1;
	yd = days;
	for (y = 1970; y < year; y++)
		days +=	DAYSPERYEAR + LEAPYEAR(y);
	sec = ((( days * 24 +
		  readrtc(RTC_HRS)) * 60 +
		  readrtc(RTC_MIN)) * 60 +
		  readrtc(RTC_SEC));
	/* sec now contains the	number of seconds, since Jan 1 1970,
	   in the local	time zone */

	osenv_intr_disable();
	time->tv_sec = sec;
	osenv_intr_enable();
	return 0;

wrong_time:
	osenv_log(OSENV_LOG_ERR, "Invalid	time in	real time clock.\n");
	osenv_log(OSENV_LOG_ERR, "Check and reset	the date immediately!\n");
	return 1;
}

/*
 * Write system	time back to RTC
 */
void
oskit_rtc_set(struct oskit_timespec *time)
{
	oskit_time_t	tm;
	int		y, m;

	osenv_intr_disable();
	tm = time->tv_sec;
	osenv_intr_enable();

	/* Disable RTC updates and interrupts. */
	rtcout(RTC_STATUSB, RTCSB_HALT | RTCSB_24HR);

	rtcout(RTC_SEC, bin2bcd(tm%60)); tm /= 60;	/* Write back Seconds */
	rtcout(RTC_MIN, bin2bcd(tm%60)); tm /= 60;	/* Write back Minutes */
	rtcout(RTC_HRS, bin2bcd(tm%24)); tm /= 24;	/* Write back Hours   */

	/* We have now the days	since 01-01-1970 in tm */
	rtcout(RTC_WDAY, (tm+4)%7);			/* Write back Weekday */
	for (y = 1970, m = DAYSPERYEAR + LEAPYEAR(y);
	     tm >= m;
	     y++,      m = DAYSPERYEAR + LEAPYEAR(y))
	     tm -= m;

	/* Now we have the years in y and the day-of-the-year in tm */
	rtcout(RTC_YEAR, bin2bcd(y%100));		/* Write back Year    */
#ifdef USE_RTC_CENTURY
	rtcout(RTC_CENTURY, bin2bcd(y/100));		/* ... and Century    */
#endif
	for (m = 0; ; m++) {
		int ml;

		ml = daysinmonth[m];
		if (m == 1 && LEAPYEAR(y))
			ml++;
		if (tm < ml)
			break;
		tm -= ml;
	}

	rtcout(RTC_MONTH, bin2bcd(m + 1));            /* Write back Month   */
	rtcout(RTC_DAY, bin2bcd(tm + 1));             /* Write back Month Day */

	/* Reenable RTC updates and interrupts. */
	rtcout(RTC_STATUSB, rtc_statusb);
}

/*
 * XXX
 * Use the RTC for a periodic clock interrupt. The slowest it runs is
 * 128 HZ (ticks per second), so divide that down as best as possible
 * to 100. Needs to be more precise at some point ...
 */
static void(*handler)();

/*
 * XXX mike's sleezy hack for time.
 *
 * We need to take 100 of every 128 ticks for our timing base.
 * That is exactly 25 of every 32, so I have a 32-bit mask (cmask)
 * with 7 zeros distributed as evenly as possible (every 3rd or 4th bit).
 * We shift through this mask 1 bit every clock tick and, if the high bit
 * is one, call the handler.  I abuse two data-representation features here:
 *
 * 1. The high-order bit is the sign bit, if set the current mask will
 *    be negative, ow positive,
 *
 * 2. The low-order bit in the mask is 1 so, that when left-shifting,
 *    the value of the current mask will always be non-zero until we
 *    have shifted out all 32 bits.
 *
 * Note that neither the old trick of skipping every 5th tick, or this
 * trick is the right answer as they both introduce discontinuities in
 * sub-second timing.  We shouldn't be mapping to 100hz here at all,
 * our clock should provide services (timers) at whatever frequency it
 * supports.  Higher level code should perform the mapping when it exports
 * the time.
 */
static int cmask = 0xef77bbdd;

static void
rtclock_intr()
{
	static int bits;

	/*
	 * Clear the clock interrupt before calling the handler.
	 */
	rtcin(RTC_INTR);

#if 1
	if (bits == 0)
		bits = cmask;
	if (bits < 0)
		handler();
	bits <<= 1;
#else
	/*
	 * Ignore every 5 tick!
	 */
	if (++count == 5) {
		count = 0;
		return;
	}
	handler();
#endif
}

int
oskit_timer_rtc_init(int freq, void (*timer_intr)())
{
	handler = timer_intr;

	/*
	 * Install interrupt handler.
	 */
	if (osenv_irq_alloc(8, rtclock_intr, 0, 0))
		osenv_panic(__FUNCTION__": couldn't install intr handler");

	/*
	 * Set the frequency and turn on the interrupt.
	 */
	rtcout(RTC_STATUSA, RTCSA_DIVIDER|RTCSA_128);
	rtcout(RTC_STATUSB, RTCSB_24HR|RTCSB_PINTR);

	return freq;
}

void
oskit_timer_rtc_shutdown()
{
	osenv_irq_free(8, handler, 0);
}
