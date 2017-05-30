/*
 * Copyright (c) 1994-1995, 1998 University of Utah and the Flux Group.
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

#include <oskit/x86/i16.h>

CODE16

void i16_writehexdigit(unsigned char digit)
{
	digit &= 0xf;
	i16_putchar(digit < 10 ? digit+'0' : digit-10+'A');
}

void i16_writehexb(unsigned char val)
{
	i16_writehexdigit(val >> 4);
	i16_writehexdigit(val);
}

void i16_writehexw(unsigned short val)
{
	i16_writehexb(val >> 8);
	i16_writehexb(val);
}

void i16_writehexl(unsigned long val)
{
	i16_writehexw(val >> 16);
	i16_writehexw(val);
}

void i16_writehexll(unsigned long long val)
{
	i16_writehexl(val >> 32);
	i16_writehexl(val);
}

