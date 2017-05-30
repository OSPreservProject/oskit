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
/*
 * Basic default termios settings for a typical cooked-mode console.
 */

#include <termios.h>

struct termios base_cooked_termios =
{{
	ICRNL | IXON,			/* input flags */
	OPOST,				/* output flags */
	CS8,				/* control flags */
	ECHO | ECHOE | ECHOK | ICANON,	/* local flags */
	{	'D'-64,			/* VEOF */
		_POSIX_VDISABLE,	/* VEOL */
		0,
		'H'-64,			/* VERASE */
		0,
		'U'-64,			/* VKILL */
		0,
		0,
		'C'-64,			/* VINTR */
		'\\'-64,		/* VQUIT */
		'Z'-64,			/* VSUSP */
		0,
		'Q'-64,			/* VSTART */
		'S'-64,			/* VSTOP */
	},
	B9600,				/* input speed */
	B9600,				/* output speed */
}};

