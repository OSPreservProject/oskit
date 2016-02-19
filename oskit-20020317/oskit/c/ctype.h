/*
 * Copyright (c) 1994-1996, 1998 University of Utah and the Flux Group.
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
#ifndef _OSKIT_C_CTYPE_H_
#define _OSKIT_C_CTYPE_H_

#include <oskit/compiler.h>

OSKIT_INLINE int isascii(int c);
OSKIT_INLINE int iscntrl(int c);
OSKIT_INLINE int isdigit(int c);
OSKIT_INLINE int isgraph(int c);
OSKIT_INLINE int islower(int c);
OSKIT_INLINE int isprint(int c);
OSKIT_INLINE int isspace(int c);
OSKIT_INLINE int isupper(int c);
OSKIT_INLINE int isxdigit(int c);
OSKIT_INLINE int isalpha(int c);
OSKIT_INLINE int isalnum(int c);
OSKIT_INLINE int ispunct(int c);
OSKIT_INLINE int toascii(int c);
OSKIT_INLINE int toupper(int c);
OSKIT_INLINE int tolower(int c);

OSKIT_INLINE int isascii(int c)
{
	return ((c & ~0x7f) == 0);
}

OSKIT_INLINE int iscntrl(int c)
{
	return ((c) < ' ') || ((c) > 126);
}

OSKIT_INLINE int isdigit(int c)
{
	return ((c) >= '0') && ((c) <= '9');
}

OSKIT_INLINE int isgraph(int c)
{
	return ((c) > ' ') && ((c) <= 126);
}

OSKIT_INLINE int islower(int c)
{
	return (c >= 'a') && (c <= 'z');
}

OSKIT_INLINE int isprint(int c)
{
	return ((c) >= ' ') && ((c) <= 126);
}

OSKIT_INLINE int isspace(int c)
{
	return ((c) == ' ') || ((c) == '\f')
		|| ((c) == '\n') || ((c) == '\r')
		|| ((c) == '\t') || ((c) == '\v');
}

OSKIT_INLINE int isupper(int c)
{
	return (c >= 'A') && (c <= 'Z');
}

OSKIT_INLINE int isxdigit(int c)
{
	return isdigit(c) ||
		((c >= 'A') && (c <= 'F')) ||
		((c >= 'a') && (c <= 'f'));
}

OSKIT_INLINE int isalpha(int c)
{
	return islower(c) || isupper(c);
}

OSKIT_INLINE int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

OSKIT_INLINE int ispunct(int c)
{
	return isgraph(c) && !isalnum(c);
}

OSKIT_INLINE int toascii(int c)
{
	return ((c) & 0x7f);
}

OSKIT_INLINE int toupper(int c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}

OSKIT_INLINE int tolower(int c)
{
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}


#endif /* _OSKIT_C_CTYPE_H_ */
