/*
 * Copyright (c) 1999 University of Utah and the Flux Group.
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
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * This is the console code. It uses the OFW, so it does not matter
 * if its a serial or VGA device. The OFW takes care of it.
 */

#include <oskit/arm32/ofw/ofw.h>
#include <oskit/arm32/ofw/ofw_cons.h>
#include <oskit/base_critical.h>
#include <oskit/tty.h>
#include <oskit/c/stdlib.h>

/*
 * These are handles returned by the OFW.
 */
static int	stdin, stdout;

/*
 * Init the OFW stuff.
 */
void
ofw_cons_init(struct termios *com_params)
{
	int	chosen;
	char	stdinbuf[4], stdoutbuf[4];

	if ((chosen = OF_finddevice("/chosen")) == -1)
		return;
	
	if (OF_getprop(chosen, "stdin", stdinbuf, sizeof(stdinbuf)) !=
	      sizeof(stdinbuf))
		return;
	
	if (OF_getprop(chosen, "stdout", stdoutbuf, sizeof(stdoutbuf)) !=
	    sizeof(stdoutbuf))
		return;

	stdin  = OF_decode_int(stdinbuf);
	stdout = OF_decode_int(stdoutbuf);
}

int
ofw_cons_trygetchar(void)
{
	int rv;
	char c;

	base_critical_enter();
	rv = OF_read(stdin, &c, 1);
	base_critical_leave();

	return (rv == 1) ? c : -1;
}

int
ofw_cons_getchar(void)
{
	int rv;
	char c;

	base_critical_enter();
	do {
		rv = OF_read(stdin, &c, 1);
	} while (rv == 0 || rv == -2);
	base_critical_leave();

	return c;
}

void
ofw_cons_putchar(int ch)
{
	char c = ch;

	base_critical_enter();
	if (c == '\n')
		ofw_cons_putchar('\r');
	OF_write(stdout, &c, 1);
	base_critical_leave();
}

