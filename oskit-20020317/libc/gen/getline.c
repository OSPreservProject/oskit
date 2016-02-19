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
 * Get a line of input from the console device. Includes 
 * XXX This does cooking, echo, backspace, linekill.
 */
char *
getline(char *str, int size)
{
	int i;
	int c;

	if (size <= 0)
		return NULL;

	i = 0;
	while (i < size) {
		c = getchar();
		if (c == EOF) {
			if (i == 0)
				return NULL;
			break;
		}
		if (c == '\r')
			c = '\n';
		else if (c == '\b') {
			if (i > 0) {
				putchar(c);
				putchar(' ');
				putchar(c);
				i--;
			}
			continue;
		}
		else if (c == '\025') {		/* ^U -- kill line */
			while (i) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				i--;
			}
			str[0] = '\0';
			continue;
		}
		putchar(c);
		str[i++] = c;
		if (c == '\n')
			break;
	}
	if (i < size)
		str[i] = '\0';
	return str;
}

