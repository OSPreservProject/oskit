/*
 * Copyright (c) 2000 University of Utah and the Flux Group.
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

/*
 * A little diddy to demonstrate the use of the Pentium performance
 * counters in an OSKit kernel. Obviously, this kernel will work only
 * on machines that have the counters. Pentium Pro or better?
 */

#include <stdio.h>
#include <stdlib.h>
#include <oskit/clientos.h>
#include <oskit/machine/perfmon.h>

int
main(int argc, char **argv)
{
	unsigned long long	starttime, endtime;
	unsigned long long	startcounter0, endcounter0;
	unsigned long long	startcounter1, endcounter1;

	oskit_clientos_init();

	enable_perfcounter(0, PMC6_INST_RETIRED,         0, PMCF_OS|PMCF_USR);
#define COUNTER0_STRING "Instructions Retired: " 
	enable_perfcounter(1, PMC6_DCU_MISS_OUTSTANDING, 0, PMCF_OS|PMCF_USR);
#define COUNTER1_STRING "DCU Miss Outstanding: "

	printf("\n");

	startcounter0 = read_perfcounter(0);
	startcounter1 = read_perfcounter(1);
	starttime     = get_tsc(); /* Time Stamp */

	printf("Started measurements ... \n\n");
	
	endcounter0 = read_perfcounter(0);
	endcounter1 = read_perfcounter(1);
	endtime     = get_tsc(); /* Time Stamp */

	printf("Well, that printf took a whopping %d cycles\n",
	       (int) (endtime - starttime));
	printf("%s: %d \n", COUNTER0_STRING,
	       (int) (endcounter0 - startcounter0));
	printf("%s: %d \n", COUNTER1_STRING,
	       (int) (endcounter1 - startcounter1));
	printf("\n\n");

	return 0;
}
