/*
 * Copyright (c) 1996, 1998 University of Utah and the Flux Group.
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

#include <ctype.h>
#include <stdio.h>

/*
 * This is called by macros in <stdio.h>.
 *
 * Print a buffer hexdump style.  Example:
 *
 * .---------------------------------------------------------------------------.
 * | 00000000       2e2f612e 6f757400 5445524d 3d787465       ./a.out.TERM=xte |
 * | 00000010       726d0048 4f4d453d 2f616673 2f63732e       rm.HOME=/afs/cs. |
 * | 00000020       75746168 2e656475 2f686f6d 652f6c6f       utah.edu/home/lo |
 * | 00000030       6d657700 5348454c 4c3d2f62 696e2f74       mew.SHELL=/bin/t |
 * | 00000040       63736800 4c4f474e 414d453d 6c6f6d65       csh.LOGNAME=lome |
 * | 00000050       77005553 45523d6c 6f6d6577 00504154       w.USER=lomew.PAT |
 * | 00000060       483d2f61 66732f63 732e7574 61682e65       H=/afs/cs.utah.e |
 * | 00000070       64752f68 6f6d652f 6c6f6d65 772f6269       du/home/lomew/bi |
 * | 00000080       6e2f4073 79733a2f 6166732f 63732e75       n/@sys:/afs/cs.u |
 * | 00000090       7461682e 6564                             tah.ed           |
 * `---------------------------------------------------------------------------'
 */
void
dohexdump(void *base, void *buf, int len, int words)
{
	int i, j;
	char *b = (char *)buf;

	if (words)
		len *= 4;
	
	printf(".---------------------------------------------------------------------------.\n");
	for (i = 0; i < len; i += 16) {
		printf("| %08x      ", (unsigned)(base+i));
		for (j = i; j < i+16; j++) {
			if (j % 4 == 0)
				printf(" ");
			if (j < len) {
				if (words) {
					/* Gag, puke. */
					int t = *((int *)b + j/4);
					printf("%02x",
					       (t >> (8*(3 - j%4))) & 0xff);
				}
				else
					printf("%02x", (unsigned char)b[j]);
			}
			else
				printf("  ");
		}

		printf("       ");
		for (j = i; j < i+16; j++)
			if (j >= len)
				printf(" ");
			else
				printf("%c", isgraph(b[j]) ? b[j] : '.');
		printf(" |\n");
	}
	printf("`---------------------------------------------------------------------------'\n");
}

