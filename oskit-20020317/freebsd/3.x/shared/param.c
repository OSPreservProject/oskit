/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
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
#include <sys/param.h>
#include <sys/tty.h>

#include <oskit/x86/pc/isa.h>


#define HZ		100

#ifndef TIMER_FREQ
#define TIMER_FREQ   1193182
#endif

#define MAXUSERS	64

#define NPROC (20 + 16 * MAXUSERS)
#define MAXFILES (NPROC*2)

/* maximum # of mbuf clusters */
#ifndef NMBCLUSTERS
#define NMBCLUSTERS (512 + MAXUSERS * 16)
#endif
int     nmbclusters = NMBCLUSTERS;

#if MAXFILES > NMBCLUSTERS
#define MAXSOCKETS MAXFILES
#else
#define MAXSOCKETS NMBCLUSTERS
#endif
int     maxsockets = MAXSOCKETS;

int	hz = HZ;
unsigned int	timer_freq = TIMER_FREQ;
int	tick = 1000000 / HZ; 


int	atdevbase = IOM_BEGIN;

struct tty *constty;
