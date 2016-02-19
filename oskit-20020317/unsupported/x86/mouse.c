/*
 * Copyright (c) 1998 University of Utah and the Flux Group.
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
#include <oskit/c/stdio.h>
#include <oskit/c/malloc.h>
#include <oskit/io/mouse.h>
#include <oskit/io/ttystream.h>
#include <oskit/debug.h>

#include "mouse_priv.h"


#define MOUSEBUFSIZE 64

static oskit_mouse_packet_t packet;
static int found_packet = 0;
static MouseDevPtr mouse = NULL;

int
oskit_mouse_init(oskit_ttystream_t *s, int type, int rate)
{
	if (type != P_PS2)
		return 0;

	packet.dx = packet.dy = packet.buttons = 0;

	mouse = calloc(1, sizeof(*mouse));
	mouse->mseType = type;
	mouse->ttystrm = s;

	return 1;
}


extern xf86MouseProtocol(MouseDevPtr mouse, unsigned char *rBuf, int nBytes);

void
xf86PostMseEvent(device, buttons, dx, dy)
    MouseDevPtr device;
    int buttons, dx, dy;
{
#if 0
	printf("%s: got an event! buttons = %d, dx = %d, dy = %d\n",
	      __FUNCTION__, buttons, dx, dy);
#endif

	packet.dx = dx;
	packet.dy = dy;
	packet.buttons = buttons;

	found_packet = 1;
}


oskit_mouse_packet_t *
oskit_mouse_get_packet(void)
{
	unsigned char rBuf[16];
	int nBytes, rc;

	if (!mouse)
		return NULL;

	found_packet = 0;

	if ((rc = oskit_ttystream_read(mouse->ttystrm, 
				       (char *)rBuf, sizeof(rBuf), 
				       &nBytes)) != -1) 
	{
		if ((rc != OSKIT_EAGAIN) && (nBytes > 0))
			xf86MouseProtocol(mouse, rBuf, nBytes);
	}

	return found_packet ? &packet : NULL;
}





