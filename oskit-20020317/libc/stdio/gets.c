/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
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

#include <stdio.h>

/*
 * Read a line from stdin.
 * XXX Cooking should be done by getchar, not by us.
 * But what about handling backspace?
 * If getchar does the backspacing
 * then we can't keep it from going over our prompt.
 */
char *
gets(char *buf)
{
	int i;
	int c;

	i = 0;
	while (1) {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '\r' || c == '\n') {
			/* The FreeBSD manpage says:
			 * The gets() function is equivalent to fgets() 
			 * with an infinite size and a stream of stdin, 
			 * except that the newline character (if any) 
			 * is not stored in the string.
			 */
			putchar('\n');
			break;
		}
		if (c == '\b' || c == 0x7f) {
			if (i > 0) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				i--;
			}
			continue;
		}
		buf[i++] = c;
		putchar(c);
	}
	buf[i] = '\0';
	return buf;
}

